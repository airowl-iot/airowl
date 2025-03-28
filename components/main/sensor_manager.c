#include <esp_log.h>
#include <stdlib.h>
#include <time.h>
#include "sensor_manager.h"

static const char *TAG = "sensor_manager";

// Define the ranges for sensor values
#define TEMP_MIN 15.0f
#define TEMP_MAX 35.0f
#define HUMIDITY_MIN 30.0f
#define HUMIDITY_MAX 80.0f
#define PRESSURE_MIN 990.0f
#define PRESSURE_MAX 1030.0f
#define GAS_MIN 5000.0f
#define GAS_MAX 20000.0f
#define IAQ_MIN 10.0f
#define IAQ_MAX 200.0f
#define PM1_MIN 0.0f
#define PM1_MAX 50.0f
#define PM25_MIN 0.0f
#define PM25_MAX 100.0f
#define PM4_MIN 0.0f
#define PM4_MAX 150.0f
#define PM10_MIN 0.0f
#define PM10_MAX 200.0f
#define AQI_MIN 0.0f
#define AQI_MAX 300.0f
#define TVOC_MIN 0.0f
#define TVOC_MAX 1000.0f

// Function to generate a random float between min and max
static float random_float(float min, float max) {
    float scale = rand() / (float) RAND_MAX;
    return min + scale * (max - min);
}

esp_err_t sensor_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing sensor manager");
    // Initialize random seed
    srand(time(NULL));
    return ESP_OK;
}

esp_err_t sensor_manager_read_data(sensor_data_t* data)
{
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    // Generate random values within reasonable ranges
    data->temperature = random_float(TEMP_MIN, TEMP_MAX);
    data->humidity = random_float(HUMIDITY_MIN, HUMIDITY_MAX);
    data->pressure = random_float(PRESSURE_MIN, PRESSURE_MAX);
    data->gas_resistance = random_float(GAS_MIN, GAS_MAX);
    data->iaq = random_float(IAQ_MIN, IAQ_MAX);
    
    // Generate new sensor values
    data->pm1 = random_float(PM1_MIN, PM1_MAX);
    data->pm25 = random_float(PM25_MIN, PM25_MAX);
    data->pm4 = random_float(PM4_MIN, PM4_MAX);
    data->pm10 = random_float(PM10_MIN, PM10_MAX);
    data->aqi = random_float(AQI_MIN, AQI_MAX);
    data->tvoc_index = random_float(TVOC_MIN, TVOC_MAX);

    ESP_LOGI(TAG, "Temperature: %.2f°C, Humidity: %.2f%%, Pressure: %.2fhPa", 
             data->temperature, data->humidity, data->pressure);
    ESP_LOGI(TAG, "PM1.0: %.2f μg/m³, PM2.5: %.2f μg/m³, PM4.0: %.2f μg/m³, PM10: %.2f μg/m³",
             data->pm1, data->pm25, data->pm4, data->pm10);
    ESP_LOGI(TAG, "AQI: %.2f, TVOC: %.2f ppb",
             data->aqi, data->tvoc_index);

    return ESP_OK;
}

esp_err_t sensor_manager_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing sensor manager");
    return ESP_OK;
} 