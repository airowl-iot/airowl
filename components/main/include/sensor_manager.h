#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <esp_err.h>

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float gas_resistance;
    float iaq;  // Indoor Air Quality
} sensor_data_t;

esp_err_t sensor_manager_init(void);
esp_err_t sensor_manager_read_data(sensor_data_t* data);
esp_err_t sensor_manager_deinit(void);

#endif // SENSOR_MANAGER_H 