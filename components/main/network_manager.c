#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "network_manager.h"
#include "esp_mac.h"
#include "esp_netif.h"

static const char *TAG = "network_manager";

// Event group bits
#define WIFI_AP_STARTED_BIT    BIT0
#define WIFI_STA_CONNECTED_BIT BIT1
#define WIFI_STA_FAIL_BIT      BIT2

// NVS keys
#define NVS_NAMESPACE "network"
#define NVS_KEY_WIFI_CONFIG "wifi_config"

static EventGroupHandle_t wifi_event_group;
static network_manager_config_t current_config = {0};
static esp_netif_t *ap_netif = NULL;
static esp_netif_t *sta_netif = NULL;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "AP started");
                xEventGroupSetBits(wifi_event_group, WIFI_AP_STARTED_BIT);
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "AP stopped");
                xEventGroupClearBits(wifi_event_group, WIFI_AP_STARTED_BIT);
                break;
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                        MAC2STR(event->mac), event->aid);
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                        MAC2STR(event->mac), event->aid);
                break;
            }
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                esp_wifi_connect();
                xEventGroupClearBits(wifi_event_group, WIFI_STA_CONNECTED_BIT);
                ESP_LOGI(TAG, "Retrying connection to AP...");
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(wifi_event_group, WIFI_STA_CONNECTED_BIT);
        }
    }
}

esp_err_t network_manager_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create event group
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    // Initialize TCP/IP adapter
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default network interfaces
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

    // Set WiFi storage to RAM
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Network manager initialized");
    return ESP_OK;
}

esp_err_t network_manager_start_ap(const char* ssid, const char* password, uint8_t max_connections)
{
    if (ssid == NULL || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = max_connections,
            .authmode = password ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN
        },
    };

    strncpy((char*)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    if (password) {
        strncpy((char*)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    // Update current config
    current_config.ap_mode_enabled = true;
    strncpy(current_config.ap_ssid, ssid, sizeof(current_config.ap_ssid));
    if (password) {
        strncpy(current_config.ap_password, password, sizeof(current_config.ap_password));
    }
    current_config.ap_max_connections = max_connections;

    return ESP_OK;
}

esp_err_t network_manager_stop_ap(void)
{
    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret == ESP_OK) {
        current_config.ap_mode_enabled = false;
    }
    return ret;
}

esp_err_t network_manager_connect_sta(const char* ssid, const char* password)
{
    if (ssid == NULL || strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (password) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }

    esp_wifi_disconnect();
    ESP_ERROR_CHECK(esp_wifi_set_mode(current_config.ap_mode_enabled ? WIFI_MODE_APSTA : WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Update current config
    strncpy(current_config.sta_ssid, ssid, sizeof(current_config.sta_ssid));
    if (password) {
        strncpy(current_config.sta_password, password, sizeof(current_config.sta_password));
    }

    return ESP_OK;
}

esp_err_t network_manager_disconnect_sta(void)
{
    esp_err_t ret = esp_wifi_disconnect();
    if (ret == ESP_OK) {
        memset(current_config.sta_ssid, 0, sizeof(current_config.sta_ssid));
        memset(current_config.sta_password, 0, sizeof(current_config.sta_password));
    }
    return ret;
}

esp_err_t network_manager_get_status(bool* is_ap_active, bool* is_sta_connected)
{
    if (is_ap_active == NULL || is_sta_connected == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    *is_ap_active = (bits & WIFI_AP_STARTED_BIT) != 0;
    *is_sta_connected = (bits & WIFI_STA_CONNECTED_BIT) != 0;

    return ESP_OK;
}

esp_err_t network_manager_save_wifi_config(const network_manager_config_t* config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_blob(nvs_handle, NVS_KEY_WIFI_CONFIG, config, sizeof(network_manager_config_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    return ret;
}

esp_err_t network_manager_load_wifi_config(network_manager_config_t* config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t required_size = sizeof(network_manager_config_t);
    ret = nvs_get_blob(nvs_handle, NVS_KEY_WIFI_CONFIG, config, &required_size);
    nvs_close(nvs_handle);

    return ret;
} 