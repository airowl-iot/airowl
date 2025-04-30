#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "ui.h"
#include "sensor_ui.h"
#include <math.h>
#include <time.h>
#include <string.h>
#include "esp_task_wdt.h"

static const char *TAG = "app_main";

// I2C Configuration
#define I2C_MASTER_SDA_IO 2
#define I2C_MASTER_SCL_IO 1
#define I2C_MASTER_FREQ_HZ 400000
#define SEN54_ADDR 0x69
#define DATA_FREQ 2 //Updated to match PlatformIO

// SEN54 Commands
#define START_MEASUREMENT 0x0021
#define READ_MEASUREMENT 0x03C4
#define DEVICE_RESET 0xD304
#define START_FAN_CLEANING 0x5607

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

// Maximum average values
static float pm1_max = 0.0f;
static float pm25_max = 0.0f;
static float pm4_max = 0.0f;
static float pm10_max = 0.0f;
static float tvoc_max = 0.0f;

void i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
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
    ESP_LOGI(TAG, "Starting measurements");
    ESP_ERROR_CHECK(sen54_write_cmd(START_MEASUREMENT));
    vTaskDelay(pdMS_TO_TICKS(2000));
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
    return (readings->pm1 <= 1500 && readings->pm25 <= 1500 && readings->temperature <= 100 &&
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

lv_color_t get_aqi_color(int aqi)
{
    if (aqi <= 50) return lv_color_hex(0x0000ff);
    else if (aqi <= 100) return lv_color_hex(0x8080FF);
    else if (aqi <= 150) return lv_color_hex(0x00FFFF);
    else if (aqi <= 200) return lv_color_hex(0x1aff8c);
    else if (aqi <= 300) return lv_color_hex(0x00ff00);
    else return lv_color_hex(0x808000);
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

void app_main(void)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    // Initialize display & LVGL
    lv_display_t *disp = bsp_display_start();
    bsp_display_backlight_on();
    bsp_display_rotate(disp, LV_DISPLAY_ROTATION_180);

    // Initialize I2C and sensor
    i2c_master_init();
    check_i2c_bus();

    bsp_display_lock(0);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);
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
        // Handle LVGL updates every 5 ticks (~50 ms × 5)
        static int tick_cnt = 0;
        if (++tick_cnt >= 5)
        {
            tick_cnt = 0;
            bsp_display_lock(0);
            lv_timer_handler();
            bsp_display_unlock();
        }

        // Sample sensor data every 2 seconds (matching PlatformIO delay)
        if (xTaskGetTickCount() - last_read >= pdMS_TO_TICKS(2000))
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
                    // Compute averages
                    avg.pm1 = acc.pm1 / acc.count;
                    avg.pm25 = acc.pm25 / acc.count;
                    avg.pm4 = acc.pm4 / acc.count;
                    avg.pm10 = acc.pm10 / acc.count;
                    avg.tvoc = acc.tvoc / acc.count;
                    avg.temperature = acc.temperature / acc.count;
                    avg.humidity = acc.humidity / acc.count;

                    // Compute AQI sub-indices
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

                    // Update maximum values and chart ranges
                    if (avg.pm1 > pm1_max)
                    {
                        pm1_max = avg.pm1;
                        if (pm1_max > 50)
                            lv_chart_set_range(ui_PM1chart, LV_CHART_AXIS_PRIMARY_Y, 0, (int)(pm1_max + 20));
                    }
                    if (avg.pm25 > pm25_max)
                    {
                        pm25_max = avg.pm25;
                        if (pm25_max > 50)
                            lv_chart_set_range(ui_PM25chart, LV_CHART_AXIS_PRIMARY_Y, 0, (int)(pm25_max + 20));
                    }
                    if (avg.pm4 > pm4_max)
                    {
                        pm4_max = avg.pm4;
                        if (pm4_max > 50)
                            lv_chart_set_range(ui_PM4chart, LV_CHART_AXIS_PRIMARY_Y, 0, (int)(pm4_max + 20));
                    }
                    if (avg.pm10 > pm10_max)
                    {
                        pm10_max = avg.pm10;
                        if (pm10_max > 50)
                            lv_chart_set_range(ui_PM10chart, LV_CHART_AXIS_PRIMARY_Y, 0, (int)(pm10_max + 20));
                    }
                    if (avg.tvoc > tvoc_max)
                    {
                        tvoc_max = avg.tvoc;
                        if (tvoc_max > 100)
                            lv_chart_set_range(ui_TVOCchart, LV_CHART_AXIS_PRIMARY_Y, 0, (int)(tvoc_max + 20));
                    }

                    // Batch all LVGL updates
                    bsp_display_lock(0);
                    update_sensor_ui(&avg, i_pm1, i_pm25, i_pm4, i_pm10, i_tvoc);
                    update_all_charts(&avg);

                    // Update average and max labels
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%.1f", avg.pm1);
                    lv_label_set_text(ui_pm1avg, buf);
                    snprintf(buf, sizeof(buf), "%.1f", pm1_max);
                    lv_label_set_text(ui_pm1max, buf);
                    snprintf(buf, sizeof(buf), "%.1f", avg.pm25);
                    lv_label_set_text(ui_pm25avg, buf);
                    snprintf(buf, sizeof(buf), "%.1f", pm25_max);
                    lv_label_set_text(ui_pm25max, buf);
                    snprintf(buf, sizeof(buf), "%.1f", avg.pm4);
                    lv_label_set_text(ui_pm4avg, buf);
                    snprintf(buf, sizeof(buf), "%.1f", pm4_max);
                    lv_label_set_text(ui_pm4max, buf);
                    snprintf(buf, sizeof(buf), "%.1f", avg.pm10);
                    lv_label_set_text(ui_pm10avg, buf);
                    snprintf(buf, sizeof(buf), "%.1f", pm10_max);
                    lv_label_set_text(ui_pm10max, buf);
                    snprintf(buf, sizeof(buf), "%.1f", avg.tvoc);
                    lv_label_set_text(ui_tvocavg, buf);
                    snprintf(buf, sizeof(buf), "%.1f", tvoc_max);
                    lv_label_set_text(ui_tvocmax, buf);

                    // Set eye colors based on overall AQI
                    int overall_aqi = i_pm1;
                    if (i_pm25 > overall_aqi) overall_aqi = i_pm25;
                    if (i_pm4 > overall_aqi) overall_aqi = i_pm4;
                    if (i_pm10 > overall_aqi) overall_aqi = i_pm10;
                    if (i_tvoc > overall_aqi) overall_aqi = i_tvoc;
                    lv_color_t eye_color = get_aqi_color(overall_aqi);
                    lv_obj_set_style_bg_color(ui_lefteye, eye_color, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(ui_righteye, eye_color, LV_PART_MAIN | LV_STATE_DEFAULT);

                    lv_obj_invalidate(lv_scr_act());
                    bsp_display_unlock();

                    // Reset accumulator
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
