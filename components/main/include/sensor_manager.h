#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <esp_err.h>

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float gas_resistance;  // Legacy field
    float iaq;             // Legacy field - Indoor Air Quality
    float pm1;             // PM1.0 sensor readings in μg/m³
    float pm25;            // PM2.5 sensor readings in μg/m³
    float pm4;             // PM4.0 sensor readings in μg/m³
    float pm10;            // PM10 sensor readings in μg/m³
    float aqi;             // Air Quality Index (0-500 scale)
    float tvoc_index;      // Total Volatile Organic Compounds in ppb
} sensor_data_t;

esp_err_t sensor_manager_init(void);
esp_err_t sensor_manager_read_data(sensor_data_t* data);
esp_err_t sensor_manager_deinit(void);

#endif // SENSOR_MANAGER_H 