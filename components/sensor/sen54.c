#include "sen54.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_FREQ_HZ       100000
#define I2C_MASTER_SDA_IO        2       // G2
#define I2C_MASTER_SCL_IO        1       // G1
#define SEN54_ADDR               0x69    // SEL=GND → 0x69

#define START_MEASUREMENT        0x0021
#define READ_MEASUREMENT         0x03C4
#define START_FAN_CLEANING       0x5607
#define DEVICE_RESET             0xD304

static const char *TAG = "SEN54";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

// AQI Breakpoints
static AQIBreakpoint pm1Bps[] = {{0.0, 8.0, 0, 50},      {8.1, 25.4, 51, 100},
                                 {25.5, 35.4, 101, 150}, {35.5, 50.4, 151, 200},
                                 {50.5, 75.4, 201, 300}, {75.5, 500.4, 301, 500}};

static AQIBreakpoint pm25Bps[] = {{0.0, 12.0, 0, 50},       {12.1, 35.4, 51, 100},
                                  {35.5, 55.4, 101, 150},   {55.5, 150.4, 151, 200},
                                  {150.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};

static AQIBreakpoint pm4Bps[] = {{0.0, 35.0, 0, 50},       {35.1, 75.4, 51, 100},
                                 {75.5, 125.4, 101, 150},  {125.5, 175.4, 151, 200},
                                 {175.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};

static AQIBreakpoint pm10Bps[] = {{0, 54, 0, 50},       {55, 154, 51, 100},
                                  {155, 254, 101, 150}, {255, 354, 151, 200},
                                  {355, 424, 201, 300}, {425, 604, 301, 500}};

static AQIBreakpoint tvocBps[] = {{0.0, 300, 0, 50},      {300, 500, 51, 100},
                                  {500, 1000, 101, 150},  {1000, 3000, 151, 200},
                                  {4000, 5000, 201, 300}, {5000, 10000, 301, 500}};

static esp_err_t i2c_master_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus: 0x%x", ret);
        return ret;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SEN54_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: 0x%x", ret);
        i2c_del_master_bus(bus_handle);
        return ret;
    }

    return ESP_OK;
}

static esp_err_t sen54_write_cmd(uint16_t command) {
    uint8_t cmd[2] = {(uint8_t)(command >> 8), (uint8_t)(command & 0xFF)};
    esp_err_t ret = i2c_master_transmit(dev_handle, cmd, sizeof(cmd), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write command 0x%04X failed: 0x%x", command, ret);
    }
    return ret;
}

static esp_err_t sen54_read_data(uint8_t *data, size_t len) {
    esp_err_t ret = i2c_master_receive(dev_handle, data, len, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: 0x%x", ret);
    }
    return ret;
}

esp_err_t sen54_init(void) {
    ESP_LOGI(TAG, "Initializing I2C");
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "Resetting sensor");
    ret = sen54_write_cmd(DEVICE_RESET);
    if (ret != ESP_OK) {
        i2c_del_master_bus(bus_handle);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Starting fan cleaning");
    ret = sen54_write_cmd(START_FAN_CLEANING);
    if (ret != ESP_OK) {
        i2c_del_master_bus(bus_handle);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10000));

    ESP_LOGI(TAG, "Starting measurements");
    ret = sen54_write_cmd(START_MEASUREMENT);
    if (ret != ESP_OK) {
        i2c_del_master_bus(bus_handle);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    return ESP_OK;
}

bool sen54_read(SensorReadings *readings) {
    uint8_t data[24] = {0};

    if (sen54_write_cmd(READ_MEASUREMENT) != ESP_OK) {
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    if (sen54_read_data(data, sizeof(data)) != ESP_OK) {
        return false;
    }

    readings->pm1 = ((data[0] << 8) | data[1]) / 10.0f;
    readings->pm25 = ((data[3] << 8) | data[4]) / 10.0f;
    readings->pm4 = ((data[6] << 8) | data[7]) / 10.0f;
    readings->pm10 = ((data[9] << 8) | data[10]) / 10.0f;
    readings->humidity = ((data[12] << 8) | data[13]) / 100.0f;
    readings->temperature = ((data[15] << 8) | data[16]) / 200.0f;
    readings->tvoc = ((data[18] << 8) | data[19]) / 10.0f;

    if (readings->pm1 > 1000 || readings->pm25 > 1000 ||
        readings->temperature > 100 || readings->temperature < -40 ||
        readings->humidity > 100 || readings->humidity < 0) {
        ESP_LOGE(TAG, "Invalid sensor readings");
        return false;
    }

    return true;
}

static AQIBreakpoint get_breakpoint(float Cp, AQIBreakpoint bps[], int numBps) {
    for (int i = 0; i < numBps; i++) {
        if (Cp >= bps[i].Cp_Lo && Cp <= bps[i].Cp_Hi) {
            return bps[i];
        }
    }
    return bps[numBps - 1]; // Return highest breakpoint if Cp exceeds range
}

static int calculate_sub_index(float Cp, AQIBreakpoint bp) {
    float Ip = ((bp.Ip_Hi - bp.Ip_Lo) / (bp.Cp_Hi - bp.Cp_Lo)) * (Cp - bp.Cp_Lo) + bp.Ip_Lo;
    return (int)round(Ip);
}

int calculate_aqi(SensorReadings *readings) {
    float avgPM1 = readings->pm1 / (readings->count ? readings->count : 1);
    float avgPM25 = readings->pm25 / (readings->count ? readings->count : 1);
    float avgPM4 = readings->pm4 / (readings->count ? readings->count : 1);
    float avgPM10 = readings->pm10 / (readings->count ? readings->count : 1);
    float avgTVOC = readings->tvoc / (readings->count ? readings->count : 1);

    AQIBreakpoint pm1Bp = get_breakpoint(avgPM1, pm1Bps, sizeof(pm1Bps) / sizeof(pm1Bps[0]));
    AQIBreakpoint pm25Bp = get_breakpoint(avgPM25, pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
    AQIBreakpoint pm4Bp = get_breakpoint(avgPM4, pm4Bps, sizeof(pm4Bps) / sizeof(pm4Bps[0]));
    AQIBreakpoint pm10Bp = get_breakpoint(avgPM10, pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
    AQIBreakpoint tvocBp = get_breakpoint(avgTVOC, tvocBps, sizeof(tvocBps) / sizeof(tvocBps[0]));

    readings->pm1_aqi = calculate_sub_index(avgPM1, pm1Bp);
    readings->pm25_aqi = calculate_sub_index(avgPM25, pm25Bp);
    readings->pm4_aqi = calculate_sub_index(avgPM4, pm4Bp);
    readings->pm10_aqi = calculate_sub_index(avgPM10, pm10Bp);
    readings->tvoc_aqi = calculate_sub_index(avgTVOC, tvocBp);

    // Return the maximum AQI sub-index
    int aqi = readings->pm1_aqi;
    if (readings->pm25_aqi > aqi) aqi = readings->pm25_aqi;
    if (readings->pm4_aqi > aqi) aqi = readings->pm4_aqi;
    if (readings->pm10_aqi > aqi) aqi = readings->pm10_aqi;
    if (readings->tvoc_aqi > aqi) aqi = readings->tvoc_aqi;

    return aqi;
}

uint32_t get_aqi_color(int aqi) {
    if (aqi >= 0 && aqi <= 50)
        return 0x00E400; // Good (Green)
    else if (aqi <= 100)
        return 0x9CFF9C; // Satisfactory (Light Green)
    else if (aqi <= 150)
        return 0xFFFF00; // Moderate (Yellow)
    else if (aqi <= 200)
        return 0xFF7E00; // Unhealthy (Orange)
    else if (aqi <= 300)
        return 0xFF0000; // Very Unhealthy (Red)
    else
        return 0x8F3F97; // Hazardous (Purple)
}