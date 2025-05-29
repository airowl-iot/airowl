#include "mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "mqtt_manager";

// MQTT configuration
#define MQTT_BROKER_URI "mqtt://mqtt.oizom.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "oizom" // Replace with actual username
#define MQTT_PASSWORD "12345678" // Replace with actual password
#define MQTT_TOPIC "airowl"

// Static variables
static esp_mqtt_client_handle_t mqtt_client = NULL;
static char client_id[32] = {0};
static bool mqtt_connected = false;

// Function prototype for mqtt_event_handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

esp_err_t mqtt_init(const char *device_id) {
    if (!device_id || strlen(device_id) == 0) {
        ESP_LOGE(TAG, "Invalid device ID");
        return ESP_ERR_INVALID_ARG;
    }

    if (mqtt_client) {
        ESP_LOGW(TAG, "MQTT client already initialized");
        return ESP_OK;
    }

    // Use the provided device_id directly as the client_id
    strncpy(client_id, device_id, sizeof(client_id) - 1);
    client_id[sizeof(client_id) - 1] = '\0'; // Ensure null-termination
    ESP_LOGI(TAG, "MQTT client ID set to: %s", client_id);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = MQTT_BROKER_URI,
                .port = MQTT_PORT,
            },
        },
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication = {
                .password = MQTT_PASSWORD,
            },
            .client_id = client_id,
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return err;
    }

    err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return err;
    }

    ESP_LOGI(TAG, "MQTT client initialized");
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected to %s", MQTT_BROKER_URI);
            mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            mqtt_connected = false;
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT published, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error, type: %d", event->error_handle->error_type);
            break;
        default:
            break;
    }
}

esp_err_t mqtt_publish_sensor_data(float pm1, float pm25, float pm4, float pm10, float tvoc) {
    if (!mqtt_connected || !mqtt_client) {
        ESP_LOGE(TAG, "Cannot publish, MQTT not connected or not initialized");
        return ESP_FAIL;
    }

    // Construct JSON payload
    char json_buf[256];
    snprintf(json_buf, sizeof(json_buf),
             "{\"deviceId\":\"%s\",\"p3\":%.2f,\"p1\":%.2f,\"p2\":%.2f,\"p5\":%.2f,\"v2\":%.2f}",
             client_id, pm1, pm25, pm10, pm4, tvoc);

    // Publish to MQTT topic
    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, json_buf, 0, 1, 0);
    if (msg_id >= 0) {
        ESP_LOGI(TAG, "Published to %s: %s", MQTT_TOPIC, json_buf);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to publish to %s", MQTT_TOPIC);
        return ESP_FAIL;
    }
}

bool mqtt_is_connected(void) {
    return mqtt_connected;
}

void mqtt_stop(void) {
    if (mqtt_client) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        mqtt_connected = false;
        ESP_LOGI(TAG, "MQTT client stopped");
    }
}