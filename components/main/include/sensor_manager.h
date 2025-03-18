#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "esp_err.h"

// AQI breakpoint structure matching the Arduino code
typedef struct {
    float cp_lo; // Low concentration breakpoint
    float cp_hi; // High concentration breakpoint
    int ip_lo;   // Low index breakpoint
    int ip_hi;   // High index breakpoint
} aqi_breakpoint_t;

// Sensor data structure
typedef struct {
    float pm1;           // PM1.0 concentration
    float pm25;          // PM2.5 concentration
    float pm4;           // PM4.0 concentration
    float pm10;          // PM10 concentration
    float temperature;   // Temperature in Celsius
    float humidity;      // Relative humidity in percent
    float voc_index;     // VOC index
    float nox_index;     // NOx index
    int aqi;             // Air Quality Index
} sensor_data_t;

// Rolling average data
typedef struct {
    float pm1_sum;
    float pm25_sum;
    float pm4_sum;
    float pm10_sum;
    float voc_sum;
    int pm1_max;
    int pm25_max;
    int pm10_max;
    int pm4_max;
    int voc_max;
    int count;
} sensor_avg_data_t;

// Initialize sensor hardware and driver
esp_err_t sensor_manager_init(void);

// Read current sensor data
esp_err_t sensor_manager_read_data(sensor_data_t* data);

// Get the calculated Air Quality Index
int sensor_manager_get_aqi(void);

// Get the AQI category as a string
const char* sensor_manager_get_aqi_category(int aqi);

// Get the color representation for an AQI value
uint32_t sensor_manager_get_aqi_color(int aqi);

// Deinitialize the sensor
esp_err_t sensor_manager_deinit(void);

esp_err_t i2c_master_init(void);
void i2c_master_deinit(void);

#endif // SENSOR_MANAGER_H 