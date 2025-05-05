#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_matter.h>
#include <nvs_flash.h>
#include <app_reset.h>
#include <common_macros.h>
#include <esp_matter_cluster.h>
#include <lvgl.h>
#include <bsp/esp-bsp.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <string.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

// I2C Configuration
#define I2C_MASTER_SDA_IO 2
#define I2C_MASTER_SCL_IO 1
#define I2C_MASTER_FREQ_HZ 100000
#define SEN54_ADDR 0x69
#define DATA_FREQ 2 // Sensor data averaging frequency

// SEN54 Commands
#define START_MEASUREMENT 0x0021
#define READ_MEASUREMENT 0x03C4
#define DEVICE_RESET 0xD304
#define START_FAN_CLEANING 0x5607

static bool sensor_initialized = false;

// AQI Breakpoints (retained for Matter air quality reporting)
typedef struct {
    float Cp_Lo;
    float Cp_Hi;
    int Ip_Lo;
    int Ip_Hi;
} AQIBreakpoint;

AQIBreakpoint pm1Bps[] = {{0.0, 8.0, 0, 50}, {8.1, 25.4, 51, 100}, {25.5, 35.4, 101, 150},
                          {35.5, 50.4, 151, 200}, {50.5, 75.4, 201, 300}, {75.5, 500.4, 301, 500}};
AQIBreakpoint pm25Bps[] = {{0.0, 12.0, 0, 50}, {12.1, 35.4, 51, 100}, {35.5, 55.4, 101, 150},
                           {55.5, 150.4, 151, 200}, {150.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};
AQIBreakpoint pm4Bps[] = {{0.0, 35.0, 0, 50}, {35.1, 75.4, 51, 100}, {75.5, 125.4, 101, 150},
                          {125.5, 175.4, 151, 200}, {175.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};
AQIBreakpoint pm10Bps[] = {{0, 54, 0, 50}, {55, 154, 51, 100}, {155, 254, 101, 150},
                           {255, 354, 151, 200}, {355, 424, 201, 300}, {425, 604, 301, 500}};
AQIBreakpoint tvocBps[] = {{0.0, 300, 0, 50}, {300, 500, 51, 100}, {500, 1000, 101, 150},
                           {1000, 3000, 151, 200}, {4000, 5000, 201, 300}, {5000, 10000, 301, 500}};

typedef struct {
    float pm1, pm25, pm4, pm10, tvoc, temperature, humidity;
    int count;
} SensorAccumulator;

// LVGL Objects
static lv_obj_t *label_pm1;
static lv_obj_t *label_pm25;
static lv_obj_t *label_pm4;
static lv_obj_t *label_pm10;
static lv_obj_t *label_tvoc;
static lv_obj_t *label_temp;
static lv_obj_t *label_humidity;

// Matter endpoint IDs
static uint16_t temp_endpoint_id = 0;
static uint16_t humid_endpoint_id = 0;
static uint16_t air_endpoint_id = 0;

void i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = I2C_MASTER_FREQ_HZ,
        },
        .clk_flags = 0,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
}

esp_err_t sen54_write_cmd(uint16_t command)
{
    uint8_t cmd[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    esp_err_t ret = i2c_master_write_to_device(I2C_NUM_0, SEN54_ADDR, cmd, sizeof(cmd), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write command 0x%04X failed: 0x%x", command, ret);
    }
    return ret;
}

esp_err_t sen54_read_data(uint8_t *data, size_t len)
{
    esp_err_t ret = i2c_master_read_from_device(I2C_NUM_0, SEN54_ADDR, data, len, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: 0x%x", ret);
    }
    return ret;
}

void initialize_sensor()
{
    ESP_LOGI(TAG, "Resetting sensor");
    if (sen54_write_cmd(DEVICE_RESET) != ESP_OK) {
        sensor_initialized = false;
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Starting measurements");
    if (sen54_write_cmd(START_MEASUREMENT) != ESP_OK) {
        sensor_initialized = false;
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    sensor_initialized = true;
    ESP_LOGI(TAG, "Sensor initialization completed");
}

bool read_sensor_values(float *pm1, float *pm25, float *pm4, float *pm10, float *humidity, float *temperature, float *tvoc)
{
    uint8_t data[24] = {0};

    if (sen54_write_cmd(READ_MEASUREMENT) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send READ_MEASUREMENT command");
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    if (sen54_read_data(data, sizeof(data)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data");
        return false;
    }

    *pm1 = ((data[0] << 8) | data[1]) / 10.0f;
    *pm25 = ((data[3] << 8) | data[4]) / 10.0f;
    *pm4 = ((data[6] << 8) | data[7]) / 10.0f;
    *pm10 = ((data[9] << 8) | data[10]) / 10.0f;
    *humidity = ((data[12] << 8) | data[13]) / 100.0f;
    *temperature = ((data[15] << 8) | data[16]) / 200.0f;
    *tvoc = ((data[18] << 8) | data[19]) / 10.0f;

    if (*pm1 > 1000 || *pm25 > 1000 || *temperature > 100 || *temperature < -40 ||
        *humidity > 100 || *humidity < 0) {
        ESP_LOGE(TAG, "Invalid sensor readings");
        return false;
    }

    return true;
}

int calculateSubIndex(float Cp, AQIBreakpoint bp)
{
    float Ip = ((bp.Ip_Hi - bp.Ip_Lo) / (bp.Cp_Hi - bp.Cp_Lo)) * (Cp - bp.Cp_Lo) + bp.Ip_Lo;
    return (int)roundf(Ip);
}

AQIBreakpoint getBreakpoint(float Cp, AQIBreakpoint bps[], int numBps)
{
    for (int i = 0; i < numBps; i++) {
        if (Cp >= bps[i].Cp_Lo && Cp <= bps[i].Cp_Hi)
            return bps[i];
    }
    return bps[numBps - 1];
}

uint8_t aqi_to_air_quality(int aqi)
{
    if (aqi <= 50) return 1; // Excellent
    else if (aqi <= 100) return 2; // Good
    else if (aqi <= 150) return 3; // Fair
    else if (aqi <= 200) return 4; // Poor
    else return 5; // Very Poor
}

void check_i2c_bus()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SEN54_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Sensor found at address 0x%02X", SEN54_ADDR);
    } else {
        ESP_LOGE(TAG, "Sensor not found at address 0x%02X (error: 0x%x)", SEN54_ADDR, ret);
    }
}

// === Sensor Update Functions ===
static void temp_sensor_notification(uint16_t endpoint_id, float temp)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, temp]() {
        attribute_t *attribute = attribute::get(endpoint_id, TemperatureMeasurement::Id,
                                                TemperatureMeasurement::Attributes::MeasuredValue::Id);
        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.i16 = static_cast<int16_t>(temp * 100); // °C * 100
        attribute::update(endpoint_id, TemperatureMeasurement::Id,
                          TemperatureMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

static void humidity_sensor_notification(uint16_t endpoint_id, float humidity)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, humidity]() {
        attribute_t *attribute = attribute::get(endpoint_id, RelativeHumidityMeasurement::Id,
                                                RelativeHumidityMeasurement::Attributes::MeasuredValue::Id);
        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.u16 = static_cast<uint16_t>(humidity * 100); // % * 100
        attribute::update(endpoint_id, RelativeHumidityMeasurement::Id,
                          RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

static void air_quality_notification(uint16_t endpoint_id, uint8_t quality_enum)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, quality_enum]() {
        attribute_t *attribute = attribute::get(endpoint_id, AirQuality::Id, AirQuality::Attributes::AirQuality::Id);
        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.u8 = quality_enum; // enum 0=Unknown, 1=Excellent, 2=Good, etc.
        attribute::update(endpoint_id, AirQuality::Id, AirQuality::Attributes::AirQuality::Id, &val);
    });
}

static esp_err_t factory_reset_button_register()
{
    return ESP_OK;
}

static void open_commissioning_window_if_necessary()
{
    if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0 &&
        !chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen()) {
        CHIP_ERROR err = chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow(
            chip::System::Clock::Seconds16(300), chip::CommissioningWindowAdvertisement::kDnssdOnly);
        if (err != CHIP_NO_ERROR) {
            ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
        }
    }
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;
    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
        ESP_LOGI(TAG, "Fabric removed successfully");
        open_commissioning_window_if_necessary();
        break;
    default:
        break;
    }
}

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    return ESP_OK;
}

static void init_lvgl_ui()
{
    // Create a screen with black background
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Create labels for sensor readings
    label_pm1 = lv_label_create(scr);
    lv_label_set_text(label_pm1, "PM1: 0.0 µg/m³");
    lv_obj_set_pos(label_pm1, 10, 10);
    lv_obj_set_style_text_color(label_pm1, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    label_pm25 = lv_label_create(scr);
    lv_label_set_text(label_pm25, "PM2.5: 0.0 µg/m³");
    lv_obj_set_pos(label_pm25, 10, 40);
    lv_obj_set_style_text_color(label_pm25, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    label_pm4 = lv_label_create(scr);
    lv_label_set_text(label_pm4, "PM4: 0.0 µg/m³");
    lv_obj_set_pos(label_pm4, 10, 70);
    lv_obj_set_style_text_color(label_pm4, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    label_pm10 = lv_label_create(scr);
    lv_label_set_text(label_pm10, "PM10: 0.0 µg/m³");
    lv_obj_set_pos(label_pm10, 10, 100);
    lv_obj_set_style_text_color(label_pm10, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    label_tvoc = lv_label_create(scr);
    lv_label_set_text(label_tvoc, "TVOC: 0.0 ppb");
    lv_obj_set_pos(label_tvoc, 10, 130);
    lv_obj_set_style_text_color(label_tvoc, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    label_temp = lv_label_create(scr);
    lv_label_set_text(label_temp, "Temp: 0.0 °C");
    lv_obj_set_pos(label_temp, 10, 160);
    lv_obj_set_style_text_color(label_temp, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    label_humidity = lv_label_create(scr);
    lv_label_set_text(label_humidity, "Humidity: 0.0 %");
    lv_obj_set_pos(label_humidity, 10, 190);
    lv_obj_set_style_text_color(label_humidity, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}

extern "C" void app_main()
{
    nvs_flash_init();
    factory_reset_button_register();

    // Initialize display & LVGL
    lv_display_t *disp = bsp_display_start();
    bsp_display_backlight_on();
    bsp_display_rotate(disp, LV_DISPLAY_ROTATION_180);

    bsp_display_lock(0);
    init_lvgl_ui();
    lv_obj_invalidate(lv_scr_act());
    bsp_display_unlock();
    ESP_LOGI(TAG, "LVGL UI initialized");

    // Initialize I2C and sensor
    i2c_master_init();
    check_i2c_bus();
    initialize_sensor();

    // Create Matter node
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    if (!node) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        return;
    }

    // Create temperature sensor endpoint
    temperature_sensor::config_t temp_sensor_config;
    endpoint_t *temp_ep = temperature_sensor::create(node, &temp_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    if (!temp_ep) {
        ESP_LOGE(TAG, "Failed to create temp sensor");
        return;
    }
    temp_endpoint_id = endpoint::get_id(temp_ep);

    // Create humidity sensor endpoint
    humidity_sensor::config_t humidity_sensor_config;
    endpoint_t *humid_ep = humidity_sensor::create(node, &humidity_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    if (!humid_ep) {
        ESP_LOGE(TAG, "Failed to create humidity sensor");
        return;
    }
    humid_endpoint_id = endpoint::get_id(humid_ep);

    // Create air quality sensor endpoint
    air_quality_sensor::config_t air_sensor_config;
    endpoint_t *air_ep = air_quality_sensor::create(node, &air_sensor_config, ENDPOINT_FLAG_NONE, NULL);
    if (!air_ep) {
        ESP_LOGE(TAG, "Failed to create air_quality_sensor endpoint");
        return;
    }
    air_endpoint_id = endpoint::get_id(air_ep);

    // Add air quality cluster with server flag
    cluster::air_quality::config_t air_quality_cluster_config;
    cluster_t *air_cluster = cluster::air_quality::create(air_ep, &air_quality_cluster_config, CLUSTER_FLAG_SERVER);
    if (!air_cluster) {
        ESP_LOGE(TAG, "Failed to create air quality cluster");
        return;
    }
    esp_matter::cluster::air_quality::attribute::create_air_quality(air_cluster, 0);

    // Start Matter
    esp_err_t err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Matter, err:%d", err);
        return;
    }

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    SensorAccumulator acc = {0};
    TickType_t last_read = xTaskGetTickCount();

    while (1) {
        // Sample sensor data every 2 seconds
        if (xTaskGetTickCount() - last_read >= pdMS_TO_TICKS(2000)) {
            last_read = xTaskGetTickCount();
            float pm1, pm25, pm4, pm10, humidity, temperature, tvoc;
            if (read_sensor_values(&pm1, &pm25, &pm4, &pm10, &humidity, &temperature, &tvoc)) {
                acc.pm1 += pm1;
                acc.pm25 += pm25;
                acc.pm4 += pm4;
                acc.pm10 += pm10;
                acc.tvoc += tvoc;
                acc.temperature += temperature;
                acc.humidity += humidity;
                acc.count++;

                if (acc.count >= DATA_FREQ) {
                    // Compute averages
                    float avg_pm1 = acc.pm1 / acc.count;
                    float avg_pm25 = acc.pm25 / acc.count;
                    float avg_pm4 = acc.pm4 / acc.count;
                    float avg_pm10 = acc.pm10 / acc.count;
                    float avg_tvoc = acc.tvoc / acc.count;
                    float avg_temperature = acc.temperature / acc.count;
                    float avg_humidity = acc.humidity / acc.count;

                    // Compute AQI sub-indices for Matter reporting
                    AQIBreakpoint bp;
                    bp = getBreakpoint(avg_pm1, pm1Bps, sizeof(pm1Bps) / sizeof(pm1Bps[0]));
                    int i_pm1 = calculateSubIndex(avg_pm1, bp);
                    bp = getBreakpoint(avg_pm25, pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
                    int i_pm25 = calculateSubIndex(avg_pm25, bp);
                    bp = getBreakpoint(avg_pm4, pm4Bps, sizeof(pm4Bps) / sizeof(pm4Bps[0]));
                    int i_pm4 = calculateSubIndex(avg_pm4, bp);
                    bp = getBreakpoint(avg_pm10, pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
                    int i_pm10 = calculateSubIndex(avg_pm10, bp);
                    bp = getBreakpoint(avg_tvoc, tvocBps, sizeof(tvocBps) / sizeof(tvocBps[0]));
                    int i_tvoc = calculateSubIndex(avg_tvoc, bp);

                    // Determine overall AQI for Matter
                    int overall_aqi = i_pm1;
                    if (i_pm25 > overall_aqi) overall_aqi = i_pm25;
                    if (i_pm4 > overall_aqi) overall_aqi = i_pm4;
                    if (i_pm10 > overall_aqi) overall_aqi = i_pm10;
                    if (i_tvoc > overall_aqi) overall_aqi = i_tvoc;

                    // Update Matter attributes
                    temp_sensor_notification(temp_endpoint_id, avg_temperature);
                    humidity_sensor_notification(humid_endpoint_id, avg_humidity);
                    air_quality_notification(air_endpoint_id, aqi_to_air_quality(overall_aqi));

                    // Update LVGL labels
                    bsp_display_lock(0);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "PM1: %.1f µg/m³", avg_pm1);
                    lv_label_set_text(label_pm1, buf);
                    snprintf(buf, sizeof(buf), "PM2.5: %.1f µg/m³", avg_pm25);
                    lv_label_set_text(label_pm25, buf);
                    snprintf(buf, sizeof(buf), "PM4: %.1f µg/m³", avg_pm4);
                    lv_label_set_text(label_pm4, buf);
                    snprintf(buf, sizeof(buf), "PM10: %.1f µg/m³", avg_pm10);
                    lv_label_set_text(label_pm10, buf);
                    snprintf(buf, sizeof(buf), "TVOC: %.1f ppb", avg_tvoc);
                    lv_label_set_text(label_tvoc, buf);
                    snprintf(buf, sizeof(buf), "Temp: %.1f °C", avg_temperature);
                    lv_label_set_text(label_temp, buf);
                    snprintf(buf, sizeof(buf), "Humidity: %.1f %%", avg_humidity);
                    lv_label_set_text(label_humidity, buf);

                    lv_obj_invalidate(lv_scr_act());
                    bsp_display_unlock();

                    // Reset accumulator
                    memset(&acc, 0, sizeof(acc));
                }
            } else {
                ESP_LOGW(TAG, "Sensor read failed; re-init");
                initialize_sensor();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}