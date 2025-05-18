#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_heap_caps.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "ui.h"

static const char *TAG = "app_main";

static void lvgl_loop(void *arg) {
    esp_task_wdt_add(NULL); // Register LVGL task
    while (1) {
        bsp_display_lock(0);
        uint32_t delay_ms = lv_timer_handler();
        bsp_display_unlock();
        esp_task_wdt_reset(); // Reset watchdog
        vTaskDelay(pdMS_TO_TICKS(delay_ms > 0 ? delay_ms : 5));
    }
}

void app_main(void) {
    esp_task_wdt_add(NULL); // Register main task
    esp_task_wdt_reset(); // Reset watchdog

    // Increase TWDT timeout
    const esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 30000, // 30 seconds
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_err_t err = esp_task_wdt_init(&wdt_config);
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "TWDT already initialized, skipping init");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "TWDT init failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "TWDT initialized with 30s timeout");
    }

    // Log memory usage
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "Free heap: %u bytes, Free PSRAM: %u bytes", free_heap, free_psram);

    // Initialize display and LVGL
    ESP_LOGI(TAG, "Initializing BSP");
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = {
            .task_priority = 4,
            .task_stack = 4096,
            .task_affinity = -1,
            .task_max_sleep_ms = 500,
            .timer_period_ms = 5,
        },
        .buffer_size = BSP_LCD_H_RES * 100, // 320 * 100 = 32,000 pixels
        .double_buffer = true,
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
        }
    };
    lv_disp_t *disp = bsp_display_start_with_config(&cfg);
    if (disp == NULL) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return;
    }
    bsp_display_backlight_on();
    esp_task_wdt_reset(); // Reset watchdog

    // Rotate screen 180 degrees
    bsp_display_rotate(disp, LV_DISP_ROTATION_180);

    // Initialize touch
#if CONFIG_BSP_TOUCH_ENABLED
    lv_indev_t *indev = bsp_display_get_input_dev();
    if (indev == NULL) {
        ESP_LOGE(TAG, "Failed to get touch input device");
    } else {
        ESP_LOGI(TAG, "Touch input device initialized");
    }
#endif
    esp_task_wdt_reset(); // Reset watchdog

    // Initialize LVGL
    ESP_LOGI(TAG, "Initializing LVGL");
    lv_init();
    esp_task_wdt_reset(); // Reset watchdog

    // Initialize UI
    ESP_LOGI(TAG, "Initializing UI");
    bsp_display_lock(0);
    ui_init();
    esp_task_wdt_reset(); // Reset watchdog
    bsp_display_unlock();

    // Log memory usage after UI init
    free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "After UI init - Free heap: %u bytes, Free PSRAM: %u bytes", free_heap, free_psram);

    // Launch LVGL loop task
    xTaskCreatePinnedToCore(lvgl_loop, "lvgl_loop", 3072, NULL, 1, NULL, 1);
}