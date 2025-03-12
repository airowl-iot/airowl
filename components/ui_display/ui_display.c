#include "ui_display.h"
#include "sensor_module.h"  // Include sensor data header
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "ui_display";

void ui_init(void)
{
    ESP_LOGI(TAG, "UI display initialized.");
    // Initialize your display hardware here if needed.
}

void ui_task(void *pvParameter)
{
    char display_buffer[256];
    while (1)
    {
        // Format sensor readings into a string.
        snprintf(display_buffer, sizeof(display_buffer),
                 "PM1: %.2f ug/m3\nPM2.5: %.2f ug/m3\nPM4: %.2f ug/m3\nPM10: %.2f ug/m3\nTVOC: %.2f ppb\nTemp: %.2f C\nHumidity: %.2f%%",
                 sensor_data.pm1, sensor_data.pm2_5, sensor_data.pm4, sensor_data.pm10,
                 sensor_data.tvoc, sensor_data.temperature, sensor_data.humidity);

        // For simulation, log the formatted string.
        // Replace this with code to update your physical display.
        ESP_LOGI(TAG, "Display Update:\n%s", display_buffer);

        // Update display every 2 seconds.
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
