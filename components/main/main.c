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

    // Main loop
    while (1) {
        sensor_data_t sensor_data;
        ret = sensor_manager_read_data(&sensor_data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read sensor data");
        } else {
            ESP_LOGI(TAG, "Temperature: %.2f°C, Humidity: %.2f%%, Pressure: %.2f hPa, Gas: %.2f kΩ, IAQ: %.2f",
                    sensor_data.temperature,
                    sensor_data.humidity,
                    sensor_data.pressure,
                    sensor_data.gas_resistance,
                    sensor_data.iaq);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
} 