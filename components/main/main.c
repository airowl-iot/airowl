#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_timer.h>
#include "sensor_manager.h"
#include "network_manager.h"
#include "web_server.h"
#include "esp_log.h"

static const char *TAG = "main";

// Global variables to track sensor data
static int current_aqi = 0;
static float current_temperature = 0.0f;
static float current_humidity = 0.0f;

// Task for reading sensor data
static void sensor_task(void *pvParameters) {
    while (1) {
        sensor_data_t sensor_data;
        esp_err_t ret = sensor_manager_read_data(&sensor_data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read sensor data");
        } else {
            // Update global variables
            current_aqi = sensor_data.aqi;
            current_temperature = sensor_data.temperature;
            current_humidity = sensor_data.humidity;
            
            // Log sensor data
            ESP_LOGI(TAG, "AQI: %d (%s)", 
                current_aqi, 
                sensor_manager_get_aqi_category(current_aqi));
                
            ESP_LOGI(TAG, "PM1: %.2f, PM2.5: %.2f, PM4: %.2f, PM10: %.2f μg/m³", 
                sensor_data.pm1, 
                sensor_data.pm25,
                sensor_data.pm4,
                sensor_data.pm10);
                
            ESP_LOGI(TAG, "Temperature: %.2f°C, Humidity: %.2f%%", 
                current_temperature, 
                current_humidity);
        }
        
        // Wait 2 seconds before next reading
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    // Initialize sensor manager
    esp_err_t ret = sensor_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensor manager");
        return;
    }

    // Initialize network manager
    network_manager_config_t wifi_config;
    ret = network_manager_load_wifi_config(&wifi_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Found saved WiFi credentials for %s", wifi_config.sta_ssid);
    }

    ret = network_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize network manager");
        return;
    }

    // Start AP mode
    ret = network_manager_start_ap("AirOwl", "12345678", 4);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start AP mode");
        return;
    }

    // Start web server
    ret = web_server_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
        return;
    }
    ESP_LOGI(TAG, "Web configuration portal started at http://192.168.4.1");

    // Create a dedicated task for sensor readings
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);

    // Main loop - for monitoring the system and future UI integration
    while (1) {
        // In the future, this can be used for the display/UI update
        // For now, just monitor system status
        ESP_LOGI(TAG, "System running - Current AQI: %d, Temperature: %.1f°C, Humidity: %.1f%%", 
                current_aqi, current_temperature, current_humidity);
        
        // Sleep for 10 seconds
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
} 