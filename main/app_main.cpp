/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lv_setup.h"
#include "ui.h"
#include "lvgl.h"
#include <string.h>
#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#include <esp_err.h>
#include <esp_matter.h>
#include <app_reset.h>
#include <esp_matter_cluster.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "main";

// Matter endpoint IDs
static uint16_t temp_endpoint_id = 0;
static uint16_t humid_endpoint_id = 0;
static uint16_t air_endpoint_id = 0;

// LVGL loop task
void lvgl_loop(void *arg) {
    while (1) {
        lv_handler();  // from lv_setup.cpp
        vTaskDelay(pdMS_TO_TICKS(5));  // ~5ms tick
    }
}

// Matter sensor notification functions
static void temp_sensor_notification(uint16_t endpoint_id, float temp)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, temp]() {
        esp_matter::attribute_t *attribute = esp_matter::attribute::get(endpoint_id, chip::app::Clusters::TemperatureMeasurement::Id,
                                                                       chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id);
        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        esp_matter::attribute::get_val(attribute, &val);
        val.val.i16 = static_cast<int16_t>(temp * 100); // °C * 100
        esp_matter::attribute::update(endpoint_id, chip::app::Clusters::TemperatureMeasurement::Id,
                                      chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

static void humidity_sensor_notification(uint16_t endpoint_id, float humidity)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, humidity]() {
        esp_matter::attribute_t *attribute = esp_matter::attribute::get(endpoint_id, chip::app::Clusters::RelativeHumidityMeasurement::Id,
                                                                       chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id);
        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        esp_matter::attribute::get_val(attribute, &val);
        val.val.u16 = static_cast<uint16_t>(humidity * 100); // % * 100
        esp_matter::attribute::update(endpoint_id, chip::app::Clusters::RelativeHumidityMeasurement::Id,
                                      chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, &val);
    });
}

static void air_quality_notification(uint16_t endpoint_id, uint8_t quality_enum)
{
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, quality_enum]() {
        esp_matter::attribute_t *attribute = esp_matter::attribute::get(endpoint_id, chip::app::Clusters::AirQuality::Id,
                                                                       chip::app::Clusters::AirQuality::Attributes::AirQuality::Id);
        esp_matter_attr_val_t val = esp_matter_invalid(NULL);
        esp_matter::attribute::get_val(attribute, &val);
        val.val.u8 = quality_enum; // enum 0=Unknown, 1=Excellent, 2=Good, etc.
        esp_matter::attribute::update(endpoint_id, chip::app::Clusters::AirQuality::Id,
                                      chip::app::Clusters::AirQuality::Attributes::AirQuality::Id, &val);
    });
}

static esp_err_t factory_reset_button_register()
{
    return ESP_OK;
}

static void open_commissioning_window_if_necessary()
{
    if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0 &&
        !chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen()) {
        CHIP_ERROR err = chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow(
            chip::System::Clock::Seconds16(300), chip::CommissioningWindowAdvertisement::kDnssdOnly);
        if (err != CHIP_NO_ERROR) {
            ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
        }
    }
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;
    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
        ESP_LOGI(TAG, "Fabric removed successfully");
        open_commissioning_window_if_necessary();
        break;
    default:
        break;
    }
}

static esp_err_t app_identification_cb(esp_matter::identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

static esp_err_t app_attribute_update_cb(esp_matter::attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    return ESP_OK;
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting LVGL + Matter demo with random sensor values for Matter");
    ESP_LOGI(TAG, "Free heap at start: %lu bytes", esp_get_free_heap_size());

    // Initialize NVS
    nvs_flash_init();
    factory_reset_button_register();
    ESP_LOGI(TAG, "Free heap after NVS: %lu bytes", esp_get_free_heap_size());

    // Create Matter node
    esp_matter::node::config_t node_config;
    esp_matter::node_t *node = esp_matter::node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    if (!node) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        return;
    }
    ESP_LOGI(TAG, "Free heap after Matter node: %lu bytes", esp_get_free_heap_size());

    // Create temperature sensor endpoint only
    esp_matter::endpoint::temperature_sensor::config_t temp_sensor_config;
    esp_matter::endpoint_t *temp_ep = esp_matter::endpoint::temperature_sensor::create(node, &temp_sensor_config, esp_matter::ENDPOINT_FLAG_NONE, NULL);
    if (!temp_ep) {
        ESP_LOGE(TAG, "Failed to create temp sensor");
        return;
    }
    temp_endpoint_id = esp_matter::endpoint::get_id(temp_ep);
    ESP_LOGI(TAG, "Free heap after temp endpoint: %lu bytes", esp_get_free_heap_size());

    // Start Matter
    esp_err_t err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Matter, err:%d", err);
        return;
    }
    ESP_LOGI(TAG, "Free heap after Matter start: %lu bytes", esp_get_free_heap_size());

    // Initialize display & LVGL
    ESP_LOGI(TAG, "Free heap before display start: %lu bytes", esp_get_free_heap_size());
    lv_display_t *disp = bsp_display_start();
    if (!disp) {
        ESP_LOGE(TAG, "Failed to start display");
        return;
    }
    bsp_display_backlight_on();
    bsp_display_rotate(disp, LV_DISPLAY_ROTATION_180);
    ESP_LOGI(TAG, "Free heap after display start: %lu bytes", esp_get_free_heap_size());

    bsp_display_lock(0);
    ESP_LOGI(TAG, "Calling ui_init");
    ui_init();
    ESP_LOGI(TAG, "Free heap after ui_init: %lu bytes", esp_get_free_heap_size());
    lv_obj_invalidate(lv_scr_act());
    bsp_display_unlock();
    ESP_LOGI(TAG, "LVGL UI initialized");

    xTaskCreatePinnedToCore(lvgl_loop, "lvgl_loop", 3000, NULL, 1, NULL, 1);
    ESP_LOGI(TAG, "Free heap after LVGL task: %lu bytes", esp_get_free_heap_size());

    srand(time(NULL));
    TickType_t last_update = xTaskGetTickCount();

    while (1) {
        if (xTaskGetTickCount() - last_update >= pdMS_TO_TICKS(2000)) {
            last_update = xTaskGetTickCount();
            float temperature = (float)(rand() % 400) / 10.0f;
            temp_sensor_notification(temp_endpoint_id, temperature);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}