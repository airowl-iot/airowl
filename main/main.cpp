#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "sen54.h"
#include "ui.h"
#include "wifi.h"
#include "esp_task_wdt.h"
#include <algorithm>
#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#include <array>
#include <cmath>
#include <common_macros.h>
#include <esp_err.h>
#include <esp_matter.h>
#include <inttypes.h>
#include <nvs_flash.h>
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "screens/ui_dashboard.h"
#include <time.h>

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

// External declarations from wifi.c
extern EventGroupHandle_t s_wifi_event_group;
extern const int WIFI_CONNECTED_BIT_GLOBAL;
extern const int WIFI_FAIL_BIT_GLOBAL;

// External declaration from ui_dashboard.c
extern lv_obj_t *ui_clock2;

// Global variables for Matter status and endpoint IDs
static bool matter_initialized = false;
static uint16_t temp_endpoint_id = 0;
static uint16_t humidity_endpoint_id = 0;
static uint16_t air_quality_endpoint_id = 0;

// Function declarations
void update_chart_data(lv_obj_t *chart, const float *data, lv_obj_t *y_axis, float min_range, float max_range);
void update_avg_max_labels(lv_obj_t *avg_label, lv_obj_t *max_label, const float *data);

// Task to update clock display
static void clock_update_task(void *pvParameters)
{
    esp_task_wdt_add(NULL); // Subscribe to WDT
    char time_str[16];
    while (1) {
        time_t now = time(NULL);
        struct tm *timeinfo = localtime(&now);
        strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);

        bsp_display_lock(0);
        if (ui_clock2) {
            lv_label_set_text(ui_clock2, time_str);
        } else {
            ESP_LOGE(TAG, "ui_clock2 is NULL");
        }
        bsp_display_unlock();

        esp_task_wdt_reset(); // Reset WDT
        vTaskDelay(pdMS_TO_TICKS(1000)); // Update every 1 seconds
    }
}

// Dedicated LVGL task for rendering
static void lvgl_task(void *pvParameters)
{
    esp_task_wdt_add(NULL); // Subscribe to WDT
    while (1) {
        bsp_display_lock(0);
        lv_task_handler();
        bsp_display_unlock();
        esp_task_wdt_reset(); // Reset WDT
        vTaskDelay(pdMS_TO_TICKS(10)); // Run every 10 ms
    }
}

// Task to handle Wi-Fi initialization
static void wifi_init_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting Wi-Fi initialization task");
    wifi_init();
    vTaskDelete(NULL); // Delete the task once Wi-Fi initialization is complete
}

// Simulate temperature sensor notification (value in 0.01°C units)
static void temp_sensor_notification(uint16_t endpoint_id, float temp)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, temp]() {
        attribute_t *attribute = attribute::get(endpoint_id, TemperatureMeasurement::Id,
                                                TemperatureMeasurement::Attributes::MeasuredValue::Id);

        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.i16 = static_cast<int16_t>(temp * 100);

        attribute::update(endpoint_id, TemperatureMeasurement::Id,
                          TemperatureMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

// Simulate humidity sensor notification (value in 0.01% units)
static void humidity_sensor_notification(uint16_t endpoint_id, float humidity)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, humidity]() {
        attribute_t *attribute = attribute::get(endpoint_id, RelativeHumidityMeasurement::Id,
                                                RelativeHumidityMeasurement::Attributes::MeasuredValue::Id);

        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        val.val.u16 = static_cast<uint16_t>(humidity * 100);

        attribute::update(endpoint_id, RelativeHumidityMeasurement::Id,
                          TemperatureMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

// Simulate air quality sensor notification (AQI as enumerated value)
static void air_quality_sensor_notification(uint16_t endpoint_id, int aqi)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, aqi]() {
        attribute_t *attribute = attribute::get(endpoint_id, AirQuality::Id, AirQuality::Attributes::AirQuality::Id);

        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        attribute::get_val(attribute, &val);
        uint8_t aqi_mapped = static_cast<uint8_t>(
            std::min(std::max(aqi / 50, 0), 6));
        val.val.u8 = aqi_mapped;

        attribute::update(endpoint_id, AirQuality::Id, AirQuality::Attributes::AirQuality::Id, &val);
    });
}

// Open commissioning window if no fabrics are present
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

// Handle Matter device events
static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;
    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;
    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
        ESP_LOGI(TAG, "Fabric removed successfully");
        open_commissioning_window_if_necessary();
        break;
    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        break;
    default:
        break;
    }
}

// Identification callback for Matter
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// Attribute update callback for Matter
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    return ESP_OK;
}

static std::array<float, CHART_DATA_LENGTH> pm1_data = {};
static std::array<float, CHART_DATA_LENGTH> pm25_data = {};
static std::array<float, CHART_DATA_LENGTH> pm4_data = {};
static std::array<float, CHART_DATA_LENGTH> pm10_data = {};
static std::array<float, CHART_DATA_LENGTH> tvoc_data = {};
static int data_index = 0;

static void update_sensor_task(void *pvParameters)
{
    SensorReadings readings = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    char buffer[32];
    esp_task_wdt_add(NULL); // Subscribe to WDT

    int init_retries = 0;
    const int max_init_retries = 5;
    while (sen54_read(&readings) != true && init_retries < max_init_retries) {
        ESP_LOGW(TAG, "Initial sensor read failed, reinitializing sensor (attempt %d/%d)", init_retries + 1,
                 max_init_retries);
        sen54_init();
        vTaskDelay(pdMS_TO_TICKS(1000));
        init_retries++;
    }
    if (init_retries >= max_init_retries) {
        ESP_LOGE(TAG, "Failed to initialize sensor after %d attempts, UI updates may not work", max_init_retries);
    }

    while (1) {
        if (sen54_read(&readings)) {
            pm1_data[data_index] = readings.pm1;
            pm25_data[data_index] = readings.pm25;
            pm4_data[data_index] = readings.pm4;
            pm10_data[data_index] = readings.pm10;
            tvoc_data[data_index] = readings.tvoc;
            data_index = (data_index + 1) % CHART_DATA_LENGTH;

            readings.count = 1;
            int overall_aqi = calculate_aqi(&readings);
            uint32_t pm1_color = get_aqi_color(readings.pm1_aqi);
            uint32_t pm25_color = get_aqi_color(readings.pm25_aqi);
            uint32_t pm4_color = get_aqi_color(readings.pm4_aqi);
            uint32_t pm10_color = get_aqi_color(readings.pm10_aqi);
            uint32_t tvoc_color = get_aqi_color(readings.tvoc_aqi);
            uint32_t overall_aqi_color = get_aqi_color(overall_aqi);

            bsp_display_lock(0);
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm1);
            if (ui_pm1label) lv_label_set_text(ui_pm1label, buffer);
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm25);
            if (ui_pm25label) lv_label_set_text(ui_pm25label, buffer);
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm4);
            if (ui_pm4label) lv_label_set_text(ui_pm4label, buffer);
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm10);
            if (ui_pm10label) lv_label_set_text(ui_pm10label, buffer);
            snprintf(buffer, sizeof(buffer), "%.1f", readings.tvoc);
            if (ui_tvoclabel) lv_label_set_text(ui_tvoclabel, buffer);
            snprintf(buffer, sizeof(buffer), "%.1f", readings.temperature);
            if (ui_templabel) lv_label_set_text(ui_templabel, buffer);
            snprintf(buffer, sizeof(buffer), "%.1f", readings.humidity);
            if (ui_RHlabel) lv_label_set_text(ui_RHlabel, buffer);

            if (ui_pm1label) lv_obj_set_style_text_color(ui_pm1label, lv_color_hex(pm1_color), LV_STATE_DEFAULT);
            if (ui_pm25label) lv_obj_set_style_text_color(ui_pm25label, lv_color_hex(pm25_color), LV_STATE_DEFAULT);
            if (ui_pm4label) lv_obj_set_style_text_color(ui_pm4label, lv_color_hex(pm4_color), LV_STATE_DEFAULT);
            if (ui_pm10label) lv_obj_set_style_text_color(ui_pm10label, lv_color_hex(pm10_color), LV_STATE_DEFAULT);
            if (ui_tvoclabel) lv_obj_set_style_text_color(ui_tvoclabel, lv_color_hex(tvoc_color), LV_STATE_DEFAULT);
            if (ui_lefteye) lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(overall_aqi_color), LV_STATE_DEFAULT);
            if (ui_righteye) lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(overall_aqi_color), LV_STATE_DEFAULT);
            bsp_display_unlock();

            if (ui_PM1chart && ui_PM1chart_Yaxis1) update_chart_data(ui_PM1chart, pm1_data.data(), ui_PM1chart_Yaxis1, 0, 100);
            if (ui_PM25chart && ui_PM25chart_Yaxis1) update_chart_data(ui_PM25chart, pm25_data.data(), ui_PM25chart_Yaxis1, 0, 600);
            if (ui_PM4chart && ui_PM4chart_Yaxis1) update_chart_data(ui_PM4chart, pm4_data.data(), ui_PM4chart_Yaxis1, 0, 600);
            if (ui_PM10chart && ui_PM10chart_Yaxis1) update_chart_data(ui_PM10chart, pm10_data.data(), ui_PM10chart_Yaxis1, 0, 700);
            if (ui_TVOCchart && ui_TVOCchart_Yaxis1) update_chart_data(ui_TVOCchart, tvoc_data.data(), ui_TVOCchart_Yaxis1, 0, 5000);

            if (ui_pm1avg && ui_pm1max) update_avg_max_labels(ui_pm1avg, ui_pm1max, pm1_data.data());
            if (ui_pm25avg && ui_pm25max) update_avg_max_labels(ui_pm25avg, ui_pm25max, pm25_data.data());
            if (ui_pm4avg && ui_pm4max) update_avg_max_labels(ui_pm4avg, ui_pm4max, pm4_data.data());
            if (ui_pm10avg && ui_pm10max) update_avg_max_labels(ui_pm10avg, ui_pm10max, pm10_data.data());
            if (ui_tvocavg && ui_tvocmax) update_avg_max_labels(ui_tvocavg, ui_tvocmax, tvoc_data.data());

            if (matter_initialized) {
                temp_sensor_notification(temp_endpoint_id, readings.temperature);
                humidity_sensor_notification(humidity_endpoint_id, readings.humidity);
                air_quality_sensor_notification(air_quality_endpoint_id, overall_aqi);
            }
        } else {
            bsp_display_lock(0);
            if (ui_pm1label) lv_label_set_text(ui_pm1label, "Err");
            if (ui_pm25label) lv_label_set_text(ui_pm25label, "Err");
            if (ui_pm4label) lv_label_set_text(ui_pm4label, "Err");
            if (ui_pm10label) lv_label_set_text(ui_pm10label, "Err");
            if (ui_tvoclabel) lv_label_set_text(ui_tvoclabel, "Err");
            if (ui_templabel) lv_label_set_text(ui_templabel, "Err");
            if (ui_RHlabel) lv_label_set_text(ui_RHlabel, "Err");
            if (ui_pm1label) lv_obj_set_style_text_color(ui_pm1label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            if (ui_pm25label) lv_obj_set_style_text_color(ui_pm25label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            if (ui_pm4label) lv_obj_set_style_text_color(ui_pm4label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            if (ui_pm10label) lv_obj_set_style_text_color(ui_pm10label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            if (ui_tvoclabel) lv_obj_set_style_text_color(ui_tvoclabel, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            if (ui_lefteye) lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            if (ui_righteye) lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            bsp_display_unlock();
            sen54_init();
        }

        esp_task_wdt_reset(); // Reset WDT
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void update_chart_data(lv_obj_t *chart, const float *data, lv_obj_t *y_axis, float min_range, float max_range)
{
    float data_min = min_range;
    float data_max = max_range;
    bool valid_data = false;

    for (int i = 0; i < CHART_DATA_LENGTH; ++i) {
        if (data[i] > 0 || data[i] < 0) {
            if (!valid_data) {
                data_min = data[i];
                data_max = data[i];
                valid_data = true;
            } else {
                data_min = std::min(data_min, data[i]);
                data_max = std::max(data_max, data[i]);
            }
        }
    }

    if (!valid_data) {
        data_min = min_range;
        data_max = max_range;
    }

    float range = data_max - data_min;
    float padding = range * 0.1f;
    if (padding < 1.0f)
        padding = 1.0f;
    float new_min = std::floor(data_min - padding);
    float new_max = std::ceil(data_max + padding);

    new_min = std::max(0.0f, new_min);
    new_max = std::max(new_max, new_min + 10.0f);

    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, static_cast<lv_coord_t>(new_min),
                       static_cast<lv_coord_t>(new_max));
    lv_scale_set_range(y_axis, static_cast<lv_coord_t>(new_min), static_cast<lv_coord_t>(new_max));

    float tick_interval = (new_max - new_min) / 5.0f;
    int major_tick_count = static_cast<int>(std::ceil((new_max - new_min) / tick_interval)) + 1;
    lv_scale_set_total_tick_count(y_axis, major_tick_count);
    lv_scale_set_major_tick_every(y_axis, 1);

    lv_chart_series_t *series = lv_chart_get_series_next(chart, nullptr);
    if (series) {
        for (int i = 0; i < CHART_DATA_LENGTH; ++i) {
            int idx = (data_index + i) % CHART_DATA_LENGTH;
            lv_chart_set_value_by_id(chart, series, i, static_cast<lv_coord_t>(data[idx]));
        }
        lv_chart_refresh(chart);
    }
}

void update_avg_max_labels(lv_obj_t *avg_label, lv_obj_t *max_label, const float *data)
{
    float avg = 0.0f;
    float max_val = 0.0f;
    int valid_count = 0;

    for (int i = 0; i < CHART_DATA_LENGTH; ++i) {
        if (data[i] > 0 || data[i] < 0) {
            avg += data[i];
            max_val = std::max(max_val, data[i]);
            valid_count++;
        }
    }

    if (valid_count > 0) {
        avg /= valid_count;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f", avg);
        lv_label_set_text(avg_label, buffer);
        snprintf(buffer, sizeof(buffer), "%.1f", max_val);
        lv_label_set_text(max_label, buffer);
    } else {
        lv_label_set_text(avg_label, "-");
        lv_label_set_text(max_label, "-");
    }
}

extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Configure Task Watchdog Timer only if not already initialized
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 10000, // 10 seconds
        .idle_core_mask = (1 << 0), // Watchdog for CPU 0
        .trigger_panic = true
    };
    esp_err_t wdt_ret = esp_task_wdt_init(&wdt_config);
    if (wdt_ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Task WDT already initialized, skipping reinitialization");
        // Optionally reconfigure timeout if possible (requires ESP-IDF v5.4.1 or later)
        esp_task_wdt_reconfigure(&wdt_config);
    } else {
        ESP_ERROR_CHECK(wdt_ret);
    }

    // Initialize display and LVGL
    ESP_LOGI(TAG, "Starting display and LVGL");
    bsp_display_start();

    // Set display brightness to 100%
    bsp_display_backlight_on();

    // Rotate display 180 degrees
    bsp_display_lock(0);
    lv_display_t *disp = lv_display_get_default();
    if (disp) {
        lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_180);
        ESP_LOGI(TAG, "Display rotated 180 degrees");
    } else {
        ESP_LOGE(TAG, "Failed to get default display");
    }

    // Initialize UI
    ESP_LOGI(TAG, "Display Custom UI");
    ui_init();

    // Add series to charts with return button color
    lv_chart_add_series(ui_PM1chart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_add_series(ui_PM25chart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_add_series(ui_PM4chart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_add_series(ui_PM10chart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_add_series(ui_TVOCchart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    bsp_display_unlock();

    // Initialize SEN54 sensor
    ESP_LOGI(TAG, "Initializing SEN54");
    if (sen54_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SEN54");
        return;
    }

    // Start tasks
    xTaskCreate(update_sensor_task, "sensor_task", 12123, NULL, 5, NULL);
    xTaskCreatePinnedToCore(clock_update_task, "clock_task", 8192, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 8192, NULL, 3, NULL, 0);
    xTaskCreate(wifi_init_task, "wifi_init_task", 4096, NULL, 5, NULL);

    // Wait for Wi-Fi to connect
    ESP_LOGI(TAG, "Waiting for Wi-Fi to connect...");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT_GLOBAL | WIFI_FAIL_BIT_GLOBAL,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT_GLOBAL) {
        ESP_LOGI(TAG, "Wi-Fi connected successfully");

        // Create Matter node
        node::config_t node_config;
        node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
        if (!node) {
            ESP_LOGE(TAG, "Failed to create Matter node");
            return;
        }

        // Create temperature sensor endpoint
        temperature_sensor::config_t temp_sensor_config;
        endpoint_t *temp_sensor_ep = temperature_sensor::create(node, &temp_sensor_config, ENDPOINT_FLAG_NONE, NULL);
        if (!temp_sensor_ep) {
            ESP_LOGE(TAG, "Failed to create temperature_sensor endpoint");
            return;
        }

        // Create humidity sensor endpoint
        humidity_sensor::config_t humidity_sensor_config;
        endpoint_t *humidity_sensor_ep =
            humidity_sensor::create(node, &humidity_sensor_config, ENDPOINT_FLAG_NONE, NULL);
        if (!humidity_sensor_ep) {
            ESP_LOGE(TAG, "Failed to create humidity_sensor endpoint");
            return;
        }

        // Create air quality sensor endpoint
        air_quality_sensor::config_t air_quality_config;
        endpoint_t *air_quality_ep = air_quality_sensor::create(node, &air_quality_config, ENDPOINT_FLAG_NONE, NULL);
        if (!air_quality_ep) {
            ESP_LOGE(TAG, "Failed to create air_quality_sensor endpoint");
            return;
        }

        // Start Matter
        esp_err_t err = esp_matter::start(app_event_cb);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start Matter, err:%d", err);
            return;
        }

        // Update global variables with Matter status and endpoint IDs
        matter_initialized = true;
        temp_endpoint_id = endpoint::get_id(temp_sensor_ep);
        humidity_endpoint_id = endpoint::get_id(humidity_sensor_ep);
        air_quality_endpoint_id = endpoint::get_id(air_quality_ep);

    } else if (bits & WIFI_FAIL_BIT_GLOBAL) {
        ESP_LOGI(TAG, "Wi-Fi failed to connect, Matter will not start");
    } else {
        ESP_LOGE(TAG, "Unexpected Wi-Fi event");
    }
}