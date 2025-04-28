#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "ui.h"
#include "sensor_ui.h"
#include <math.h>
#include <time.h>
#include <string.h> // Added for memset
#include "esp_task_wdt.h"

static const char *TAG = "app_main";

// I2C Configuration
#define I2C_MASTER_SDA_IO 2
#define I2C_MASTER_SCL_IO 1
#define I2C_MASTER_FREQ_HZ 100000
#define SEN54_ADDR 0x69
#define DATA_FREQ 2

// SEN54 Commands
#define START_MEASUREMENT 0x0021
#define READ_MEASUREMENT 0x03C4
#define DEVICE_RESET 0xD304

// AQI Breakpoints
typedef struct
{
    float Cp_Lo;
    float Cp_Hi;
    int Ip_Lo;
    int Ip_Hi;
} AQIBreakpoint;

AQIBreakpoint pm1Bps[] = {{0.0, 8.0, 0, 50}, {8.1, 25.4, 51, 100}, {25.5, 35.4, 101, 150}, {35.5, 50.4, 151, 200}, {50.5, 75.4, 201, 300}, {75.5, 500.4, 301, 500}};
AQIBreakpoint pm25Bps[] = {{0.0, 12.0, 0, 50}, {12.1, 35.4, 51, 100}, {35.5, 55.4, 101, 150}, {55.5, 150.4, 151, 200}, {150.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};
AQIBreakpoint pm4Bps[] = {{0.0, 35.0, 0, 50}, {35.1, 75.4, 51, 100}, {75.5, 125.4, 101, 150}, {125.5, 175.4, 151, 200}, {175.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};
AQIBreakpoint pm10Bps[] = {{0, 54, 0, 50}, {55, 154, 51, 100}, {155, 254, 101, 150}, {255, 354, 151, 200}, {355, 424, 201, 300}, {425, 604, 301, 500}};
AQIBreakpoint tvocBps[] = {{0.0, 300, 0, 50}, {300, 500, 51, 100}, {500, 1000, 101, 150}, {1000, 3000, 151, 200}, {4000, 5000, 201, 300}, {5000, 10000, 301, 500}};

typedef struct
{
    float pm1, pm25, pm4, pm10, tvoc, temperature, humidity;
    int count;
} SensorAccumulator;

// I2C Master Handle
static i2c_master_bus_handle_t i2c_bus_handle = NULL;

void i2c_master_init()
{
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));
    ESP_LOGI(TAG, "I2C master initialized on SDA=%d, SCL=%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
}

esp_err_t sen54_write_cmd(uint16_t command)
{
    i2c_master_dev_handle_t dev_handle;
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SEN54_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &dev_handle));

    uint8_t cmd[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    esp_err_t ret = i2c_master_transmit(dev_handle, cmd, sizeof(cmd), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGD(TAG, "I2C write command 0x%04x succeeded", command);
    }

    i2c_master_bus_rm_device(dev_handle);
    return ret;
}

esp_err_t sen54_read_data(uint8_t *data, size_t len)
{
    i2c_master_dev_handle_t dev_handle;
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SEN54_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &dev_handle));

    esp_err_t ret = i2c_master_receive(dev_handle, data, len, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGD(TAG, "I2C read succeeded, %d bytes", len);
    }

    i2c_master_bus_rm_device(dev_handle);
    return ret;
}

void initialize_sensor()
{
    ESP_LOGI(TAG, "Starting measurements");
    ESP_ERROR_CHECK(sen54_write_cmd(START_MEASUREMENT));
    for (int i = 0; i < 10; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_task_wdt_reset();
    }
    ESP_LOGI(TAG, "Sensor initialization completed");
}

bool read_sensor_values(SensorReadings *readings)
{
    uint8_t data[24] = {0};
    ESP_LOGD(TAG, "Attempting to read sensor data");
    if (sen54_write_cmd(READ_MEASUREMENT) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send READ_MEASUREMENT command");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_task_wdt_reset();
    if (sen54_read_data(data, sizeof(data)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read sensor data");
        return false;
    }
    readings->pm1 = ((data[0] << 8) | data[1]) / 10.0f;
    readings->pm25 = ((data[3] << 8) | data[4]) / 10.0f;
    readings->pm4 = ((data[6] << 8) | data[7]) / 10.0f;
    readings->pm10 = ((data[9] << 8) | data[10]) / 10.0f;
    readings->humidity = ((data[12] << 8) | data[13]) / 100.0f;
    readings->temperature = ((data[15] << 8) | data[16]) / 200.0f;
    readings->tvoc = ((data[18] << 8) | data[19]) / 10.0f;
    ESP_LOGD(TAG, "Raw sensor data: PM1=%.1f, PM2.5=%.1f, PM4=%.1f, PM10=%.1f, Hum=%.1f, Temp=%.1f, TVOC=%.1f",
             readings->pm1, readings->pm25, readings->pm4, readings->pm10, readings->humidity, readings->temperature, readings->tvoc);
    return (readings->pm1 <= 1000 && readings->pm25 <= 1000 && readings->temperature <= 100 &&
            readings->temperature >= -40 && readings->humidity <= 100 && readings->humidity >= 0);
}

int calculateSubIndex(float Cp, AQIBreakpoint bp)
{
    float Ip = ((bp.Ip_Hi - bp.Ip_Lo) / (bp.Cp_Hi - bp.Cp_Lo)) * (Cp - bp.Cp_Lo) + bp.Ip_Lo;
    return (int)round(Ip);
}

AQIBreakpoint getBreakpoint(float Cp, AQIBreakpoint bps[], int numBps)
{
    for (int i = 0; i < numBps; i++)
    {
        if (Cp >= bps[i].Cp_Lo && Cp <= bps[i].Cp_Hi)
            return bps[i];
    }
    return bps[numBps - 1];
}

void app_main(void)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    // ── Initialize display & LVGL ──────────────────────────────
    lv_display_t *disp = bsp_display_start();
    bsp_display_backlight_on();
    bsp_display_rotate(disp, LV_DISPLAY_ROTATION_180);

    // Initialize labels to zero
    // bsp_display_lock(0);
    // {
    //     SensorReadings init = {0};
    //     update_sensor_ui(&init, 0, 0, 0, 0, 0);
    //     lv_obj_invalidate(lv_scr_act());
    // }
    // bsp_display_unlock();

    // ── Initialize sensor ───────────────────────────────────────
    i2c_master_init();

    bsp_display_lock(0);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN); // Clear to black
    lv_obj_invalidate(lv_scr_act());
    ui_init();
    _ui_screen_change(&ui_dashboard, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_dashboard_screen_init);
    setup_charts();
    bsp_display_unlock();
    ESP_LOGI(TAG, "UI ready");
    
    initialize_sensor();

    SensorReadings raw = {0};
    SensorReadings avg = {0};
    SensorAccumulator acc = {0};

    TickType_t last_read = xTaskGetTickCount();

    while (1)
    {
        // 1) Let LVGL do its thing every 5 ticks (~50 ms × 5)
        static int tick_cnt = 0;
        if (++tick_cnt >= 5)
        {
            tick_cnt = 0;
            bsp_display_lock(0);
            lv_timer_handler();
            bsp_display_unlock();
        }

        // 2) Once per second, sample & accumulate
        if (xTaskGetTickCount() - last_read >= pdMS_TO_TICKS(1000))
        {
            last_read = xTaskGetTickCount();
            if (read_sensor_values(&raw))
            {
                acc.pm1 += raw.pm1;
                acc.pm25 += raw.pm25;
                acc.pm4 += raw.pm4;
                acc.pm10 += raw.pm10;
                acc.tvoc += raw.tvoc;
                acc.temperature += raw.temperature;
                acc.humidity += raw.humidity;
                acc.count++;

                if (acc.count >= DATA_FREQ)
                {
                    // compute averages
                    avg.pm1 = acc.pm1 / acc.count;
                    avg.pm25 = acc.pm25 / acc.count;
                    avg.pm4 = acc.pm4 / acc.count;
                    avg.pm10 = acc.pm10 / acc.count;
                    avg.tvoc = acc.tvoc / acc.count;
                    avg.temperature = acc.temperature / acc.count;
                    avg.humidity = acc.humidity / acc.count;

                    // compute AQI sub-indices
                    AQIBreakpoint bp;
                    bp = getBreakpoint(avg.pm1, pm1Bps, sizeof(pm1Bps) / sizeof(pm1Bps[0]));
                    int i_pm1 = calculateSubIndex(avg.pm1, bp);
                    bp = getBreakpoint(avg.pm25, pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
                    int i_pm25 = calculateSubIndex(avg.pm25, bp);
                    bp = getBreakpoint(avg.pm4, pm4Bps, sizeof(pm4Bps) / sizeof(pm4Bps[0]));
                    int i_pm4 = calculateSubIndex(avg.pm4, bp);
                    bp = getBreakpoint(avg.pm10, pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
                    int i_pm10 = calculateSubIndex(avg.pm10, bp);
                    bp = getBreakpoint(avg.tvoc, tvocBps, sizeof(tvocBps) / sizeof(tvocBps[0]));
                    int i_tvoc = calculateSubIndex(avg.tvoc, bp);

                    // 3) Batch all LVGL updates in one lock/unlock
                    bsp_display_lock(0);
                    update_sensor_ui(&avg, i_pm1, i_pm25, i_pm4, i_pm10, i_tvoc);
                    update_all_charts(&avg);
                    lv_obj_invalidate(lv_scr_act());
                    bsp_display_unlock();

                    // reset accumulator
                    memset(&acc, 0, sizeof(acc));
                }
            }
            else
            {
                ESP_LOGW(TAG, "Sensor read failed; re-init");
                initialize_sensor();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
        esp_task_wdt_reset();
    }
}