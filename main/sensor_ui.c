#include "sensor_ui.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h> // Added for PRIx32

#define CHART_COLOR lv_color_hex(0x41b4d1)
#define TAG "sensor_ui"

#define DECLARE_CHART(name) \
    static lv_chart_series_t *name##_series = NULL; \
    static lv_coord_t name##_array[CHART_DATA_LENGTH] = {0}

DECLARE_CHART(ui_PM1chart);
DECLARE_CHART(ui_PM25chart);
DECLARE_CHART(ui_PM4chart);
DECLARE_CHART(ui_PM10chart);
DECLARE_CHART(ui_TVOCchart);

void setup_charts() {
    ui_PM1chart_series = lv_chart_add_series(ui_PM1chart, CHART_COLOR, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_PM1chart, ui_PM1chart_series, ui_PM1chart_array);
    lv_chart_set_range(ui_PM1chart, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM25chart_series = lv_chart_add_series(ui_PM25chart, CHART_COLOR, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_PM25chart, ui_PM25chart_series, ui_PM25chart_array);
    lv_chart_set_range(ui_PM25chart, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM4chart_series = lv_chart_add_series(ui_PM4chart, CHART_COLOR, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_PM4chart, ui_PM4chart_series, ui_PM4chart_array);
    lv_chart_set_range(ui_PM4chart, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM10chart_series = lv_chart_add_series(ui_PM10chart, CHART_COLOR, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_PM10chart, ui_PM10chart_series, ui_PM10chart_array);
    lv_chart_set_range(ui_PM10chart, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_TVOCchart_series = lv_chart_add_series(ui_TVOCchart, CHART_COLOR, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_TVOCchart, ui_TVOCchart_series, ui_TVOCchart_array);
    lv_chart_set_range(ui_TVOCchart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
}

static lv_color_t get_aqi_color(int aqi) {
    lv_color_t color;
    if (aqi <= 50) color = lv_color_hex(0x00E400);      // Good
    else if (aqi <= 100) color = lv_color_hex(0x9CFF9C); // Satisfactory
    else if (aqi <= 150) color = lv_color_hex(0xFFFF00); // Moderate
    else if (aqi <= 200) color = lv_color_hex(0xFF7E00); // Unhealthy
    else if (aqi <= 300) color = lv_color_hex(0xFF0000); // Very Unhealthy
    else color = lv_color_hex(0x8F3F97);                 // Hazardous
    ESP_LOGD(TAG, "AQI=%d, Color=0x%" PRIx32, aqi, lv_color_to32(color)); // Fixed: Use PRIx32
    return color;
}

void update_sensor_ui(const SensorReadings *r,
                      int pm1Index, int pm25Index,
                      int pm4Index, int pm10Index,
                      int tvocIndex)
{
    char buf[16];

    snprintf(buf, sizeof(buf), "%.1f", r->pm1);
    lv_label_set_text(ui_pm1label, buf);
    lv_obj_set_style_text_color(ui_pm1label, get_aqi_color(pm1Index), LV_PART_MAIN | LV_STATE_DEFAULT);

    snprintf(buf, sizeof(buf), "%.1f", r->pm25);
    lv_label_set_text(ui_pm25label, buf);
    lv_obj_set_style_text_color(ui_pm25label, get_aqi_color(pm25Index), LV_PART_MAIN | LV_STATE_DEFAULT);

    snprintf(buf, sizeof(buf), "%.1f", r->pm4);
    lv_label_set_text(ui_pm4label, buf);
    lv_obj_set_style_text_color(ui_pm4label, get_aqi_color(pm4Index), LV_PART_MAIN | LV_STATE_DEFAULT);

    snprintf(buf, sizeof(buf), "%.1f", r->pm10);
    lv_label_set_text(ui_pm10label, buf);
    lv_obj_set_style_text_color(ui_pm10label, get_aqi_color(pm10Index), LV_PART_MAIN | LV_STATE_DEFAULT);

    snprintf(buf, sizeof(buf), "%.1f", r->tvoc);
    lv_label_set_text(ui_tvoclabel, buf);
    lv_obj_set_style_text_color(ui_tvoclabel, get_aqi_color(tvocIndex), LV_PART_MAIN | LV_STATE_DEFAULT);

    snprintf(buf, sizeof(buf), "%.1f", r->temperature);
    lv_label_set_text(ui_templabel, buf);

    snprintf(buf, sizeof(buf), "%.1f", r->humidity);
    lv_label_set_text(ui_RHlabel, buf);
}

static void update_chart(lv_obj_t *chart, lv_chart_series_t *series, lv_coord_t *buf, float new_val) {
    memmove(&buf[0], &buf[1], sizeof(lv_coord_t) * (CHART_DATA_LENGTH - 1));
    buf[CHART_DATA_LENGTH - 1] = (lv_coord_t)new_val;
    lv_chart_set_ext_y_array(chart, series, buf);
    lv_chart_refresh(chart);
}

void update_all_charts(const SensorReadings *r) {
    update_chart(ui_PM1chart, ui_PM1chart_series, ui_PM1chart_array, r->pm1);
    update_chart(ui_PM25chart, ui_PM25chart_series, ui_PM25chart_array, r->pm25);
    update_chart(ui_PM4chart, ui_PM4chart_series, ui_PM4chart_array, r->pm4);
    update_chart(ui_PM10chart, ui_PM10chart_series, ui_PM10chart_array, r->pm10);
    update_chart(ui_TVOCchart, ui_TVOCchart_series, ui_TVOCchart_array, r->tvoc);
}