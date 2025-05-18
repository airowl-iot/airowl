#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "ui.h"
#include "lvgl.h"
#include "sen54.h"
#include "screens/ui_dashboard.h"

static const char *TAG = "app_main";

static void update_sensor_task(void *pvParameters) {
    SensorReadings readings = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    char buffer[32];

    while (1) {
        if (sen54_read(&readings)) {
            bsp_display_lock(0);

            // Calculate overall AQI
            readings.count = 1; // Set count to 1 for AQI calculation
            int overall_aqi = calculate_aqi(&readings);
            uint32_t aqi_color = get_aqi_color(overall_aqi);

            // Update PM1 label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm1);
            lv_label_set_text(ui_pm1label, buffer);
            lv_obj_set_style_text_color(ui_pm1label, lv_color_hex(aqi_color), LV_PART_MAIN | LV_STATE_DEFAULT);

            // Update PM2.5 label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm25);
            lv_label_set_text(ui_pm25label, buffer);
            lv_obj_set_style_text_color(ui_pm25label, lv_color_hex(aqi_color), LV_PART_MAIN | LV_STATE_DEFAULT);

            // Update PM4 label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm4);
            lv_label_set_text(ui_pm4label, buffer);
            lv_obj_set_style_text_color(ui_pm4label, lv_color_hex(aqi_color), LV_PART_MAIN | LV_STATE_DEFAULT);

            // Update PM10 label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm10);
            lv_label_set_text(ui_pm10label, buffer);
            lv_obj_set_style_text_color(ui_pm10label, lv_color_hex(aqi_color), LV_PART_MAIN | LV_STATE_DEFAULT);

            // Update TVOC label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.tvoc);
            lv_label_set_text(ui_tvoclabel, buffer);
            lv_obj_set_style_text_color(ui_tvoclabel, lv_color_hex(aqi_color), LV_PART_MAIN | LV_STATE_DEFAULT);

            // Update Temperature label
            snprintf(buffer, sizeof(buffer), "%.1f", readings.temperature);
            lv_label_set_text(ui_templabel, buffer);

            // Update Humidity label
            snprintf(buffer, sizeof(buffer), "%.1f", readings.humidity);
            lv_label_set_text(ui_RHlabel, buffer);

            // Update eye colors based on overall AQI
            lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(aqi_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(aqi_color), LV_PART_MAIN | LV_STATE_DEFAULT);

            bsp_display_unlock();
        } else {
            ESP_LOGW(TAG, "Failed to read sensor - retrying...");
            sen54_init(); // Reinitialize sensor on failure
            bsp_display_lock(0);
            lv_label_set_text(ui_pm1label, "Err");
            lv_label_set_text(ui_pm25label, "Err");
            lv_label_set_text(ui_pm4label, "Err");
            lv_label_set_text(ui_pm10label, "Err");
            lv_label_set_text(ui_tvoclabel, "Err");
            lv_label_set_text(ui_templabel, "Err");
            lv_label_set_text(ui_RHlabel, "Err");
            // Reset colors to default (e.g., white) on error
            lv_obj_set_style_text_color(ui_pm1label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_pm25label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_pm4label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_pm10label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_tvoclabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            // Reset eye colors to default (e.g., white) on error
            lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            bsp_display_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

extern "C" void app_main(void) {
    /* Initialize display and LVGL */
    bsp_display_start();

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    /* Rotate display 180 degrees */
    bsp_display_lock(0);
    lv_display_t *disp = lv_display_get_default();
    if (disp) {
        lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_180);
        ESP_LOGI(TAG, "Display rotated 180 degrees");
    } else {
        ESP_LOGE(TAG, "Failed to get default display");
    }

    /* Initialize UI */
    ESP_LOGI(TAG, "Display Custom UI");
    ui_init();
    bsp_display_unlock();

    /* Initialize SEN54 sensor */
    ESP_LOGI(TAG, "Initializing SEN54");
    if (sen54_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SEN54");
        return;
    }

    /* Start sensor update task */
    xTaskCreate(update_sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}