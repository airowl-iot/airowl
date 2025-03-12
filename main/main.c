#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "ui_display.h"
#include "sensor_module.h"
#include "web_server.h"

void app_main(void)
{
    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
         nvs_flash_erase();
         nvs_flash_init();
    }

    // Start WiFi/hotspot manager (AP mode)
    wifi_manager_start();

    // Start web server for network configuration
    start_web_server();

    // Initialize UI/display and sensor modules
    ui_init();
    sensor_init();

    // Create tasks for UI update and sensor readings
    xTaskCreate(ui_task, "ui_task", 4096, NULL, 5, NULL);
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}
