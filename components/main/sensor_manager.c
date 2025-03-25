#include <esp_log.h>
#include "sensor_manager.h"

static const char *TAG = "sensor_manager";

esp_err_t sensor_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing sensor manager");
    // TODO: Initialize actual sensors
    return ESP_OK;
}

esp_err_t sensor_manager_read_data(sensor_data_t* data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    // TODO: Read from actual sensors
    // For now, return dummy values
    data->temperature = 25.0f;
    data->humidity = 50.0f;
    data->pressure = 1013.25f;
    data->gas_resistance = 10000.0f;
    data->iaq = 100.0f;

    ESP_LOGI(TAG, "Temperature: %.2f°C, Humidity: %.2f%%, Pressure: %.2fhPa, Gas: %.2fΩ, IAQ: %.2f",
             data->temperature, data->humidity, data->pressure, data->gas_resistance, data->iaq);

    return ESP_OK;
}

esp_err_t sensor_manager_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing sensor manager");
    // TODO: Deinitialize actual sensors
    return ESP_OK;
} 