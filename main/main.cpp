#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "ui.h"
#include "lvgl.h"
#include "sen54.h"
#include "screens/ui_dashboard.h"
#include <array>
#include <algorithm> // For std::min and std::max
#include <cmath>     // For std::ceil and std::floor

static const char *TAG = "app_main";

// Function declarations
void update_chart_data(lv_obj_t *chart, const float *data, lv_obj_t *y_axis, float min_range, float max_range);
void update_avg_max_labels(lv_obj_t *avg_label, lv_obj_t *max_label, const float *data);

static std::array<float, CHART_DATA_LENGTH> pm1_data = {};
static std::array<float, CHART_DATA_LENGTH> pm25_data = {};
static std::array<float, CHART_DATA_LENGTH> pm4_data = {};
static std::array<float, CHART_DATA_LENGTH> pm10_data = {};
static std::array<float, CHART_DATA_LENGTH> tvoc_data = {};
static int data_index = 0;

static void update_sensor_task(void *pvParameters)
{
    SensorReadings readings = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    char buffer[32];

    while (1)
    {
        if (sen54_read(&readings))
        {
            // Store historical data
            pm1_data[data_index] = readings.pm1;
            pm25_data[data_index] = readings.pm25;
            pm4_data[data_index] = readings.pm4;
            pm10_data[data_index] = readings.pm10;
            tvoc_data[data_index] = readings.tvoc;
            data_index = (data_index + 1) % CHART_DATA_LENGTH;

            bsp_display_lock(0);

            // Calculate overall AQI
            readings.count = 1; // Set count to 1 for AQI calculation
            int overall_aqi = calculate_aqi(&readings);
            uint32_t aqi_color = get_aqi_color(overall_aqi);

            // Update PM1 label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm1);
            lv_label_set_text(ui_pm1label, buffer);
            lv_obj_set_style_text_color(ui_pm1label, lv_color_hex(aqi_color), LV_STATE_DEFAULT);

            // Update PM2.5 label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm25);
            lv_label_set_text(ui_pm25label, buffer);
            lv_obj_set_style_text_color(ui_pm25label, lv_color_hex(aqi_color), LV_STATE_DEFAULT);

            // Update PM4 label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm4);
            lv_label_set_text(ui_pm4label, buffer);
            lv_obj_set_style_text_color(ui_pm4label, lv_color_hex(aqi_color), LV_STATE_DEFAULT);

            // Update PM10 label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.pm10);
            lv_label_set_text(ui_pm10label, buffer);
            lv_obj_set_style_text_color(ui_pm10label, lv_color_hex(aqi_color), LV_STATE_DEFAULT);

            // Update TVOC label and color
            snprintf(buffer, sizeof(buffer), "%.1f", readings.tvoc);
            lv_label_set_text(ui_tvoclabel, buffer);
            lv_obj_set_style_text_color(ui_tvoclabel, lv_color_hex(aqi_color), LV_STATE_DEFAULT);

            // Update Temperature label
            snprintf(buffer, sizeof(buffer), "%.1f", readings.temperature);
            lv_label_set_text(ui_templabel, buffer);

            // Update Humidity label
            snprintf(buffer, sizeof(buffer), "%.1f", readings.humidity);
            lv_label_set_text(ui_RHlabel, buffer);

            // Update eye colors based on overall AQI
            lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(aqi_color), LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(aqi_color), LV_STATE_DEFAULT);

            // Update charts with dynamic range
            update_chart_data(ui_PM1chart, pm1_data.data(), ui_PM1chart_Yaxis1, 0, 100);
            update_chart_data(ui_PM25chart, pm25_data.data(), ui_PM25chart_Yaxis1, 0, 600);
            update_chart_data(ui_PM4chart, pm4_data.data(), ui_PM4chart_Yaxis1, 0, 600);
            update_chart_data(ui_PM10chart, pm10_data.data(), ui_PM10chart_Yaxis1, 0, 700);
            update_chart_data(ui_TVOCchart, tvoc_data.data(), ui_TVOCchart_Yaxis1, 0, 5000);

            // Update average and max labels
            update_avg_max_labels(ui_pm1avg, ui_pm1max, pm1_data.data());
            update_avg_max_labels(ui_pm25avg, ui_pm25max, pm25_data.data());
            update_avg_max_labels(ui_pm4avg, ui_pm4max, pm4_data.data());
            update_avg_max_labels(ui_pm10avg, ui_pm10max, pm10_data.data());
            update_avg_max_labels(ui_tvocavg, ui_tvocmax, tvoc_data.data());

            bsp_display_unlock();
        }
        else
        {
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
            lv_obj_set_style_text_color(ui_pm1label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_pm25label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_pm4label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_pm10label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_tvoclabel, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
            bsp_display_unlock();
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void update_chart_data(lv_obj_t *chart, const float *data, lv_obj_t *y_axis, float min_range, float max_range)
{
    // Find min and max values in the data
    float data_min = min_range;
    float data_max = max_range;
    bool valid_data = false;

    for (int i = 0; i < CHART_DATA_LENGTH; ++i)
    {
        if (data[i] > 0 || data[i] < 0) // Check for non-zero to avoid uninitialized data
        {
            if (!valid_data)
            {
                data_min = data[i];
                data_max = data[i];
                valid_data = true;
            }
            else
            {
                data_min = std::min(data_min, data[i]);
                data_max = std::max(data_max, data[i]);
            }
        }
    }

    // If no valid data, use default range
    if (!valid_data)
    {
        data_min = min_range;
        data_max = max_range;
    }

    // Add padding (10% of range) and round to nice numbers
    float range = data_max - data_min;
    float padding = range * 0.1f;
    if (padding < 1.0f)
        padding = 1.0f; // Minimum padding
    float new_min = std::floor(data_min - padding);
    float new_max = std::ceil(data_max + padding);

    // Ensure min is not negative and respect minimum range
    new_min = std::max(0.0f, new_min);
    new_max = std::max(new_max, new_min + 10.0f); // Ensure some range

    // Update chart and Y-axis range
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, static_cast<lv_coord_t>(new_min), static_cast<lv_coord_t>(new_max));
    lv_scale_set_range(y_axis, static_cast<lv_coord_t>(new_min), static_cast<lv_coord_t>(new_max));

    // Adjust tick count based on range (aim for ~6 major ticks)
    float tick_interval = (new_max - new_min) / 5.0f; // 5 intervals for 6 ticks
    int major_tick_count = static_cast<int>(std::ceil((new_max - new_min) / tick_interval)) + 1;
    lv_scale_set_total_tick_count(y_axis, major_tick_count);
    lv_scale_set_major_tick_every(y_axis, 1);

    // Update chart data
    lv_chart_series_t *series = lv_chart_get_series_next(chart, nullptr);
    if (series)
    {
        for (int i = 0; i < CHART_DATA_LENGTH; ++i)
        {
            int idx = (data_index + i) % CHART_DATA_LENGTH;
            lv_chart_set_value_by_id(chart, series, i, static_cast<lv_coord_t>(data[idx]));
        }
        lv_chart_refresh(chart);
    }
}

void update_avg_max_labels(lv_obj_t *avg_label, lv_obj_t *max_label, const float *data)
{
    float avg = 0.0f;
    float max_val = 0.0f;
    int valid_count = 0;

    for (int i = 0; i < CHART_DATA_LENGTH; ++i)
    {
        if (data[i] > 0 || data[i] < 0) // Check for non-zero to avoid uninitialized data
        {
            avg += data[i];
            max_val = std::max(max_val, data[i]);
            valid_count++;
        }
    }

    if (valid_count > 0)
    {
        avg /= valid_count;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f", avg);
        lv_label_set_text(avg_label, buffer);
        snprintf(buffer, sizeof(buffer), "%.1f", max_val);
        lv_label_set_text(max_label, buffer);
    }
    else
    {
        lv_label_set_text(avg_label, "-");
        lv_label_set_text(max_label, "-");
    }
}

extern "C" void app_main(void)
{
    /* Initialize display and LVGL */
    bsp_display_start();

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    /* Rotate display 180 degrees */
    bsp_display_lock(0);
    lv_display_t *disp = lv_display_get_default();
    if (disp)
    {
        lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_180);
        ESP_LOGI(TAG, "Display rotated 180 degrees");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get default display");
    }

    /* Initialize UI */
    ESP_LOGI(TAG, "Display Custom UI");
    ui_init();

    // Add series to charts with return button color
    lv_chart_add_series(ui_PM1chart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_add_series(ui_PM25chart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_add_series(ui_PM4chart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_add_series(ui_PM10chart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_add_series(ui_TVOCchart, lv_color_hex(0x41B4D1), LV_CHART_AXIS_PRIMARY_Y);

    bsp_display_unlock();

    /* Initialize SEN54 sensor */
    ESP_LOGI(TAG, "Initializing SEN54");
    if (sen54_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize SEN54");
        return;
    }

    /* Start sensor update task */
    xTaskCreate(update_sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}