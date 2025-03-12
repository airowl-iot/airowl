#include "sensor_module.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "sensor_module";

sensor_data_t sensor_data;

void sensor_init(void)
{
    ESP_LOGI(TAG, "Sensor module initialized.");
    // Initialize sensor data with default values.
    sensor_data.pm1 = 0.0;
    sensor_data.pm2_5 = 0.0;
    sensor_data.pm4 = 0.0;
    sensor_data.pm10 = 0.0;
    sensor_data.tvoc = 0.0;
    sensor_data.temperature = 25.0; // room temperature in °C
    sensor_data.humidity = 50.0;    // 50% relative humidity
}

void sensor_task(void *pvParameter)
{
    while (1)
    {
        // Simulate sensor reading.
        // Replace these with your actual sensor read routines.
        sensor_data.pm1 = (float)(rand() % 50) / 10.0;         // 0.0 - 5.0 ug/m3
        sensor_data.pm2_5 = (float)(rand() % 100) / 10.0;        // 0.0 - 10.0 ug/m3
        sensor_data.pm4 = (float)(rand() % 150) / 10.0;          // 0.0 - 15.0 ug/m3
        sensor_data.pm10 = (float)(rand() % 200) / 10.0;         // 0.0 - 20.0 ug/m3
        sensor_data.tvoc = (float)(rand() % 500) / 10.0;         // 0.0 - 50.0 ppb
        sensor_data.temperature = 20.0 + (float)(rand() % 100) / 10.0; // 20.0 - 30.0 °C
        sensor_data.humidity = 30.0 + (float)(rand() % 700) / 10.0;      // 30.0 - 100.0 %

        ESP_LOGI(TAG,
            "Sensor Readings: PM1: %.2f, PM2.5: %.2f, PM4: %.2f, PM10: %.2f, TVOC: %.2f, Temp: %.2f, Humidity: %.2f",
            sensor_data.pm1, sensor_data.pm2_5, sensor_data.pm4, sensor_data.pm10,
            sensor_data.tvoc, sensor_data.temperature, sensor_data.humidity);

        // Delay between readings.
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
