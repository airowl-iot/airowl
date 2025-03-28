#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "display_manager.h"

static const char *TAG = "display_manager";

// LVGL UI Components
static lv_obj_t *screen = NULL;
static lv_obj_t *temp_label = NULL;
static lv_obj_t *humidity_label = NULL;
static lv_obj_t *tvoc_label = NULL;
static lv_obj_t *pm1_label = NULL;
static lv_obj_t *pm25_label = NULL;
static lv_obj_t *pm4_label = NULL;
static lv_obj_t *pm10_label = NULL;
static lv_obj_t *panel = NULL;
static lv_obj_t *chart = NULL;
static lv_obj_t *chart_title = NULL;  // Add title label
static lv_chart_series_t *temp_series = NULL;
static lv_chart_series_t *humidity_series = NULL;
static lv_chart_series_t *tvoc_series = NULL;
static lv_chart_series_t *pm1_series = NULL;
static lv_chart_series_t *pm25_series = NULL;
static lv_chart_series_t *pm4_series = NULL;
static lv_chart_series_t *pm10_series = NULL;

// View mode flags
static bool chart_view_active = false;

// Auto-switch back timer
static esp_timer_handle_t chart_timer = NULL;

// Data storage for chart history
#define CHART_HISTORY_SIZE 30
static float temp_history[CHART_HISTORY_SIZE];
static float humidity_history[CHART_HISTORY_SIZE];
static float pressure_history[CHART_HISTORY_SIZE];
static float tvoc_history[CHART_HISTORY_SIZE];
static float pm1_history[CHART_HISTORY_SIZE];
static float pm25_history[CHART_HISTORY_SIZE];
static float pm4_history[CHART_HISTORY_SIZE];
static float pm10_history[CHART_HISTORY_SIZE];
static float aqi_history[CHART_HISTORY_SIZE];
static int history_index = 0;

// Buffer for formatting data values
static char buffer[128];

// Add new static variables for sensor selection
static lv_obj_t *sensor_buttons[7] = {NULL};  // Array for sensor selection buttons
static int selected_sensor = -1;  // Currently selected sensor (-1 means none)

// Forward declarations
static void create_text_view(void);
static void create_chart_view(void);
static void screen_event_cb(lv_event_t *e);
static void chart_timer_cb(void *arg);
static void sensor_button_event_cb(lv_event_t *e);
static void update_chart_for_selected_sensor(void);

/**
 * @brief Initialize the timer for auto-switching back to text view
 */
static void init_chart_timer(void)
{
    // Create timer configuration
    const esp_timer_create_args_t timer_config = {
        .callback = &chart_timer_cb,
        .name = "chart_timer"
    };
    
    // Create and start timer
    ESP_ERROR_CHECK(esp_timer_create(&timer_config, &chart_timer));
}

/**
 * @brief Timer callback to switch back to text view
 */
static void chart_timer_cb(void *arg)
{
    if (chart_view_active) {
        // Execute on LVGL thread to avoid sync issues
        lv_async_call((lv_async_cb_t)create_text_view, NULL);
    }
}

/**
 * @brief Create the UI layout for the sensor dashboard
 */
static void create_ui(void)
{
    // Get the current screen
    screen = lv_scr_act();
    
    // Set display rotation to 180 degrees (upside down)
    bsp_display_lock(0);
    lv_disp_t *disp = lv_disp_get_default();
    lv_disp_set_rotation(disp, LV_DISP_ROTATION_180);
    bsp_display_unlock();
    
    // Register event handler for touch events
    lv_obj_add_event_cb(screen, screen_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Create text view as default
    create_text_view();
}

/**
 * @brief Create the text view UI
 */
static void create_text_view(void)
{
    // If timer is running, stop it
    if (chart_timer) {
        esp_timer_stop(chart_timer);
    }
    
    // Clear the screen first
    lv_obj_clean(screen);
    
    // Create a panel for the main dashboard
    panel = lv_obj_create(screen);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Add event handler to the panel
    lv_obj_add_event_cb(panel, screen_event_cb, LV_EVENT_CLICKED, NULL);

    // Create grid layout for sensor values
    // Top row - Temperature and Humidity
    temp_label = lv_label_create(panel);
    lv_label_set_text(temp_label, "Temp: --.-°C");
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 10, 10);

    humidity_label = lv_label_create(panel);
    lv_label_set_text(humidity_label, "Humidity: --.-%");
    lv_obj_set_style_text_color(humidity_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(humidity_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(humidity_label, LV_ALIGN_TOP_RIGHT, -10, 10);

    // Middle - PM Sensors 2x2 Grid
    // Top row of PM grid
    pm1_label = lv_label_create(panel);
    lv_label_set_text(pm1_label, "PM1.0: ---");
    lv_obj_set_style_text_color(pm1_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(pm1_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(pm1_label, LV_ALIGN_LEFT_MID, 10, -20);

    pm25_label = lv_label_create(panel);
    lv_label_set_text(pm25_label, "PM2.5: ---");
    lv_obj_set_style_text_color(pm25_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(pm25_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(pm25_label, LV_ALIGN_RIGHT_MID, -10, -20);

    // Bottom row of PM grid
    pm4_label = lv_label_create(panel);
    lv_label_set_text(pm4_label, "PM4.0: ---");
    lv_obj_set_style_text_color(pm4_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(pm4_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(pm4_label, LV_ALIGN_LEFT_MID, 10, 20);

    pm10_label = lv_label_create(panel);
    lv_label_set_text(pm10_label, "PM10: ---");
    lv_obj_set_style_text_color(pm10_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(pm10_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(pm10_label, LV_ALIGN_RIGHT_MID, -10, 20);

    // Bottom - TVOC
    tvoc_label = lv_label_create(panel);
    lv_label_set_text(tvoc_label, "TVOC: --- ppb");
    lv_obj_set_style_text_color(tvoc_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(tvoc_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(tvoc_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    chart_view_active = false;
}

/**
 * @brief Create the chart view UI
 */
static void create_chart_view(void)
{
    // Start the timer to switch back to text view after 5 seconds
    if (chart_timer) {
        esp_timer_stop(chart_timer);
        ESP_ERROR_CHECK(esp_timer_start_once(chart_timer, 10000000)); // 5 seconds in microseconds
    }
    
    // Clear the screen first
    lv_obj_clean(screen);
    
    // Create a panel for the chart dashboard
    panel = lv_obj_create(screen);
    lv_obj_set_size(panel, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Add event handler to the panel
    lv_obj_add_event_cb(panel, screen_event_cb, LV_EVENT_CLICKED, NULL);

    // Create tap instructions and timer info label
    lv_obj_t *tap_instruction = lv_label_create(panel);
    lv_label_set_text(tap_instruction, "Auto-return in 10s (tap to return now)");
    lv_obj_set_style_text_color(tap_instruction, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(tap_instruction, LV_ALIGN_TOP_MID, 0, 5);

    // Create sensor selection buttons
    const char *sensor_names[] = {
        "PM1.0", "PM2.5", "PM4.0",
        "PM10", "TVOC", "Humidity",
        "Temperature" 
    };
    
    const lv_color_t button_colors[] = {
        lv_palette_main(LV_PALETTE_RED),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_INDIGO),
        lv_palette_main(LV_PALETTE_TEAL),
        lv_palette_main(LV_PALETTE_DEEP_PURPLE),
        lv_palette_main(LV_PALETTE_AMBER),
        lv_palette_main(LV_PALETTE_DEEP_ORANGE)
    };

    // Create a grid of buttons (3x3 layout)
    for (int i = 0; i < 6; i++) {
        sensor_buttons[i] = lv_btn_create(panel);
        lv_obj_set_size(sensor_buttons[i], 90, 30);
        lv_obj_align(sensor_buttons[i], LV_ALIGN_TOP_LEFT, 
                    10 + (i % 3) * 100, 30 + (i / 3) * 40);
        
        lv_obj_t *label = lv_label_create(sensor_buttons[i]);
        lv_label_set_text(label, sensor_names[i]);
        lv_obj_center(label);
        
        lv_obj_set_style_bg_color(sensor_buttons[i], button_colors[i], LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        
        lv_obj_add_event_cb(sensor_buttons[i], sensor_button_event_cb, LV_EVENT_CLICKED, (void*)i);
    }

    // Create larger Temperature button in center
    sensor_buttons[6] = lv_btn_create(panel);
    lv_obj_set_size(sensor_buttons[6], 120, 40);
    lv_obj_align(sensor_buttons[6], LV_ALIGN_TOP_MID, 0, 110);
    
    lv_obj_t *temp_label = lv_label_create(sensor_buttons[6]);
    lv_label_set_text(temp_label, "Temperature");
    lv_obj_center(temp_label);
    
    lv_obj_set_style_bg_color(sensor_buttons[6], button_colors[6], LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    lv_obj_add_event_cb(sensor_buttons[6], sensor_button_event_cb, LV_EVENT_CLICKED, (void*)6);

    // Create chart (initially hidden)
    chart = lv_chart_create(panel);
    lv_obj_set_size(chart, 300, 180);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, CHART_HISTORY_SIZE);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);  // Black background
    lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);
    
    // Set Y-axis range 
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(chart, 5, 5);

    // Add series to the chart (initially hidden)
    temp_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    humidity_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    tvoc_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_DEEP_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
    pm1_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_INDIGO), LV_CHART_AXIS_PRIMARY_Y);
    pm25_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_TEAL), LV_CHART_AXIS_PRIMARY_Y);
    pm4_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_DEEP_PURPLE), LV_CHART_AXIS_PRIMARY_Y);
    pm10_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_AMBER), LV_CHART_AXIS_PRIMARY_Y);
    
    chart_view_active = true;
}

/**
 * @brief Event handler for screen touches
 */
static void screen_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        if (chart_view_active) {
            if (selected_sensor >= 0) {
                // Show all buttons and tap instruction again
                for (int i = 0; i < 7; i++) {
                    lv_obj_clear_flag(sensor_buttons[i], LV_OBJ_FLAG_HIDDEN);
                }
                lv_obj_t *tap_instruction = lv_obj_get_child(panel, 0);
                lv_obj_clear_flag(tap_instruction, LV_OBJ_FLAG_HIDDEN);
                
                // Reset chart position and size
                lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -10);
                lv_obj_set_size(chart, 300, 180);
                lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);
                
                // Hide title
                if (chart_title) {
                    lv_obj_add_flag(chart_title, LV_OBJ_FLAG_HIDDEN);
                }
                
                selected_sensor = -1;
            } else {
                create_text_view();
            }
        } else {
            create_chart_view();
        }
    }
}

static void sensor_button_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int sensor_index = (int)lv_event_get_user_data(e);
    
    // Reset all buttons
    for (int i = 0; i < 7; i++) {
        lv_obj_clear_state(sensor_buttons[i], LV_STATE_CHECKED);
    }
    
    // Set selected button state
    lv_obj_add_state(btn, LV_STATE_CHECKED);
    
    // Update selected sensor
    selected_sensor = sensor_index;
    
    // Hide all buttons and tap instruction
    for (int i = 0; i < 7; i++) {
        lv_obj_add_flag(sensor_buttons[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t *tap_instruction = lv_obj_get_child(panel, 0);
    lv_obj_add_flag(tap_instruction, LV_OBJ_FLAG_HIDDEN);
    
    // Show chart and update it
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 20);  // Move chart down by 20 pixels
    lv_obj_set_size(chart, 300, 180);  // Slightly reduce height
    
    // Create or update title label
    if (!chart_title) {
        chart_title = lv_label_create(panel);
        lv_obj_set_style_text_color(chart_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(chart_title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(chart_title, LV_ALIGN_TOP_MID, 0, 5);  // Position title at top
    }
    
    // Update title text based on selected sensor
    const char *sensor_names[] = {
        "PM1.0", "PM2.5", "PM4.0",
        "PM10", "TVOC", "Humidity",
        "Temperature"
    };
    lv_label_set_text(chart_title, sensor_names[sensor_index]);
    lv_obj_clear_flag(chart_title, LV_OBJ_FLAG_HIDDEN);
    
    update_chart_for_selected_sensor();
}

static void update_chart_for_selected_sensor(void)
{
    if (selected_sensor < 0) return;
    
    // Hide all series by setting their values to 0
    for (int i = 0; i < CHART_HISTORY_SIZE; i++) {
        lv_chart_set_next_value(chart, temp_series, 0);
        lv_chart_set_next_value(chart, humidity_series, 0);
        lv_chart_set_next_value(chart, tvoc_series, 0);
        lv_chart_set_next_value(chart, pm1_series, 0);
        lv_chart_set_next_value(chart, pm25_series, 0);
        lv_chart_set_next_value(chart, pm4_series, 0);
        lv_chart_set_next_value(chart, pm10_series, 0);
    }
    
    // Show and update selected series
    lv_chart_series_t *selected_series = NULL;
    float *history_data = NULL;
    float min_val = 0, max_val = 100;
    
    switch (selected_sensor) {
        case 0: // Temperature
            selected_series = temp_series;
            history_data = temp_history;
            min_val = 0;
            max_val = TEMP_MAX;
            break;
        case 1: // Humidity
            selected_series = humidity_series;
            history_data = humidity_history;
            min_val = 0;
            max_val = 100;
            break;
        case 2: // PM1.0
            selected_series = pm1_series;
            history_data = pm1_history;
            min_val = PM_MIN;
            max_val = PM_MAX;
            break;
        case 3: // PM2.5
            selected_series = pm25_series;
            history_data = pm25_history;
            min_val = PM_MIN;
            max_val = PM_MAX;
            break;
        case 4: // PM4.0
            selected_series = pm4_series;
            history_data = pm4_history;
            min_val = PM_MIN;
            max_val = PM_MAX;
            break;
        case 5: // PM10
            selected_series = pm10_series;
            history_data = pm10_history;
            min_val = PM_MIN;
            max_val = PM_MAX;
            break;
        case 6: // TVOC
            selected_series = tvoc_series;
            history_data = tvoc_history;
            min_val = TVOC_MIN;
            max_val = TVOC_MAX;
            break;
    }
    
    if (selected_series && history_data) {
        // Update chart with normalized values
        for (int i = 0; i < CHART_HISTORY_SIZE; i++) {
            int idx = (history_index + i) % CHART_HISTORY_SIZE;
            if (history_data[idx] != 0) {
                lv_chart_set_next_value(chart, selected_series, 
                    ((history_data[idx] - min_val) * 100) / (max_val - min_val));
            }
        }
    }
}

esp_err_t display_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing display manager");
    
    // Initialize history buffer
    memset(temp_history, 0, sizeof(temp_history));
    memset(humidity_history, 0, sizeof(humidity_history));
    memset(pressure_history, 0, sizeof(pressure_history));
    memset(tvoc_history, 0, sizeof(tvoc_history));
    memset(pm1_history, 0, sizeof(pm1_history));
    memset(pm25_history, 0, sizeof(pm25_history));
    memset(pm4_history, 0, sizeof(pm4_history));
    memset(pm10_history, 0, sizeof(pm10_history));
    memset(aqi_history, 0, sizeof(aqi_history));
    history_index = 0;
    
    // Initialize display and LVGL
    bsp_display_start();

    // Set display brightness to 100%
    bsp_display_backlight_on();
    
    // Initialize chart timer
    init_chart_timer();
    
    // Create the UI
    bsp_display_lock(0);
    create_ui();
    bsp_display_unlock();

    return ESP_OK;
}

esp_err_t display_manager_update(sensor_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    bsp_display_lock(0);

    // Store values in history buffer
    temp_history[history_index] = data->temperature;
    humidity_history[history_index] = data->humidity;
    pressure_history[history_index] = data->pressure;
    tvoc_history[history_index] = data->tvoc_index;
    pm1_history[history_index] = data->pm1;
    pm25_history[history_index] = data->pm25;
    pm4_history[history_index] = data->pm4;
    pm10_history[history_index] = data->pm10;
    
    // Update history index (circular buffer)
    history_index = (history_index + 1) % CHART_HISTORY_SIZE;

    if (!chart_view_active) {
        // Update temperature label
        snprintf(buffer, sizeof(buffer), "Temp: %.1f°C", data->temperature);
        lv_label_set_text(temp_label, buffer);

        // Update humidity label
        snprintf(buffer, sizeof(buffer), "Humidity: %.1f%%", data->humidity);
        lv_label_set_text(humidity_label, buffer);

        // Update PM1.0 label
        snprintf(buffer, sizeof(buffer), "PM1.0: %.1f", data->pm1);
        lv_label_set_text(pm1_label, buffer);

        // Update PM2.5 label
        snprintf(buffer, sizeof(buffer), "PM2.5: %.1f", data->pm25);
        lv_label_set_text(pm25_label, buffer);

        // Update PM4.0 label
        snprintf(buffer, sizeof(buffer), "PM4.0: %.1f", data->pm4);
        lv_label_set_text(pm4_label, buffer);

        // Update PM10 label
        snprintf(buffer, sizeof(buffer), "PM10: %.1f", data->pm10);
        lv_label_set_text(pm10_label, buffer);

        // Update TVOC label
        snprintf(buffer, sizeof(buffer), "TVOC: %.1f", data->tvoc_index);
        lv_label_set_text(tvoc_label, buffer);
    } else if (selected_sensor >= 0) {
        // Update chart with new data for selected sensor
        update_chart_for_selected_sensor();
    }

    bsp_display_unlock();

    return ESP_OK;
}

esp_err_t display_manager_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing display manager");
    
    // Clean up the timer
    if (chart_timer) {
        esp_timer_stop(chart_timer);
        esp_timer_delete(chart_timer);
        chart_timer = NULL;
    }
    
    // Here we would clean up any resources, but since LVGL and the display
    // are managed by the BSP, we don't need to do much
    
    return ESP_OK;
} 