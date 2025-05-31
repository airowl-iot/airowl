#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include "lvgl.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ui.h"
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_MAX_RETRY 10
#define NVS_NAMESPACE "wifi_creds"
#define AP_PASS "12345678"
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// NTP server definitions
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.google.com"
#define NTP_TIMEOUT_MS 20000
#define NTP_RETRY_COUNT 3

char ntp_timezone[32] = "UTC0"; // Global variable for timezone
bool ntp_synced = false;

// Timezone structure from wifi-manager.c
typedef struct {
    const char *region;
    const char *posix_tz;
} timezone_t;

// Comprehensive list of timezones from wifi-manager.c
static const timezone_t timezones[] = {
    {"UTC", "UTC0"},
    {"Africa/Abidjan", "GMT0"},
    {"Africa/Algiers", "CET-1"},
    {"Africa/Cairo", "EET-2"},
    {"Africa/Johannesburg", "SAST-2"},
    {"Africa/Lagos", "WAT-1"},
    {"Africa/Nairobi", "EAT-3"},
    {"America/Adak", "HST10HDT,M3.2.0,M11.1.0"},
    {"America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0"},
    {"America/Argentina/Buenos_Aires", "ART3"},
    {"America/Bogota", "COT5"},
    {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Phoenix", "MST7"},
    {"America/Sao_Paulo", "BRT3"},
    {"America/St_Johns", "NST3:30NDT,M3.2.0,M11.1.0"},
    {"Canada/Atlantic", "AST4ADT,M3.2.0,M11.1.0"}, // Added for Canada (Atlantic Time, e.g., Halifax)
    {"America/Mexico_City", "CST6"},
    {"America/Puerto_Rico", "AST4"},
    {"Pacific/Guam", "ChST-10"},
    {"Pacific/Pago_Pago", "SST11"},
    {"Asia/Almaty", "ALMT-6"},
    {"Asia/Baghdad", "AST-3"},
    {"Asia/Baku", "AZT-4"},
    {"Asia/Bangkok", "ICT-7"},
    {"Asia/Colombo", "LKT-5:30"},
    {"Asia/Dhaka", "BDT-6"},
    {"Asia/Dubai", "GST-4"},
    {"Asia/Hong_Kong", "HKT-8"},
    {"Asia/Jakarta", "WIB-7"},
    {"Asia/Jerusalem", "IST-2IDT,M3.4.4/26,M10.5.0"},
    {"Asia/Kabul", "AFT-4:30"},
    {"Asia/Karachi", "PKT-5"},
    {"Asia/Kathmandu", "NPT-5:45"},
    {"Asia/Kolkata", "IST-5:30"},
    {"Asia/Manila", "PHT-8"},
    {"Asia/Riyadh", "AST-3"},
    {"Asia/Seoul", "KST-9"},
    {"Asia/Singapore", "SGT-8"},
    {"Asia/Tehran", "IRST-3:30"},
    {"Asia/Tokyo", "JST-9"},
    {"Asia/Ulaanbaatar", "ULAT-8"},
    {"Asia/Yangon", "MMT-6:30"},
    {"Asia/Yerevan", "AMT-4"},
    {"Atlantic/Azores", "AZOT1AZOST,M3.5.0,M10.5.0"},
    {"Atlantic/Cape_Verde", "CVT1"},
    {"Australia/Adelaide", "ACST-9:30ACDT,M10.1.0,M4.1.0"},
    {"Australia/Brisbane", "AEST-10"},
    {"Australia/Canberra", "AEST-10AEDT,M10.1.0,M4.1.0"},
    {"Australia/Darwin", "ACST-9:30"},
    {"Australia/Eucla", "ACWST-8:45"},
    {"Australia/Hobart", "AEST-10AEDT,M10.1.0,M4.1.0"},
    {"Australia/Lord_Howe", "LHST-10:30LHDT,M10.1.0,M4.1.0"},
    {"Australia/Perth", "AWST-8"},
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0"},
    {"Indian/Christmas", "CXT-7"},
    {"Indian/Cocos", "CCT-6:30"},
    {"Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0"},
    {"Europe/Athens", "EET-2EEST,M3.5.0,M10.5.0"},
    {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0"},
    {"Europe/Helsinki", "EET-2EEST,M3.5.0,M10.5.0"},
    {"Europe/Istanbul", "TRT-3"},
    {"Europe/Lisbon", "WET0WEST,M3.5.0,M10.5.0"},
    {"Europe/London", "GMT0BST,M3.5.0,M10.5.0"},
    {"Europe/Moscow", "MSK-3"},
    {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0"},
    {"Pacific/Apia", "WST-13"},
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0"},
    {"Pacific/Chatham", "CHAST-12:45CHADT,M9.5.0,M4.1.0"},
    {"Pacific/Fiji", "FJT-12"},
    {"Pacific/Honolulu", "HST10"},
    {"Pacific/Kiritimati", "LINT-14"},
    {"Pacific/Majuro", "MHT-12"},
    {"Pacific/Norfolk", "NFT-11"},
    {"Pacific/Tahiti", "TAHT10"},
    {"Pacific/Tongatapu", "TOT-13"}
};
#define NUM_TIMEZONES (sizeof(timezones) / sizeof(timezones[0]))

EventGroupHandle_t s_wifi_event_group;
const int WIFI_CONNECTED_BIT_GLOBAL = WIFI_CONNECTED_BIT;
const int WIFI_FAIL_BIT_GLOBAL = WIFI_FAIL_BIT;

static const char *TAG = "wifi_manager";
static int s_retry_num = 0;
static httpd_handle_t server = NULL;
static char ap_ssid[32] = "AIROWL_XXXXXX"; // Will be updated with MAC
static bool wifi_connected = false; // Track Wi-Fi connection status

// External declaration from ui_dashboard.c
extern lv_obj_t *ui_clock2;
static void stop_webserver(void);
static esp_err_t setup_clock_post_handler(httpd_req_t *req); // Forward declaration
static esp_err_t load_timezone(char *timezone, size_t *timezone_len); // Forward declaration

// Main configuration page with OTA removed
static const char *wifi_config_html = "<!DOCTYPE html>\n"
                                      "<html lang=\"en\">\n"
                                      "<head>\n"
                                      "<meta charset=\"UTF-8\">\n"
                                      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                                      "<title>AIROWL Configuration</title>\n"
                                      "<style>\n"
                                      "body { font-family: Arial, sans-serif; display: flex; justify-content: center; "
                                      "align-items: center; height: 100vh; margin: 0; background-color: #f0f0f0; }\n"
                                      ".container { text-align: center; background-color: white; padding: 30px; "
                                      "border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); min-width: 400px; max-width: 90%; box-sizing: border-box; }\n"
                                      "h1 { color: #333; }\n"
                                      "button { width: 200px; padding: 12px 0; margin: 10px; background-color: #007bff; color: "
                                      "white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }\n"
                                      "button:hover { background-color: #0056b3; }\n"
                                      "</style>\n"
                                      "</head>\n"
                                      "<body>\n"
                                      "<div class=\"container\">\n"
                                      "<h1>AIROWL Configuration</h1>\n"
                                      "<p>Connect to: %s</p>\n"
                                      "<a href=\"/wifi_form\"><button>Configure WiFi</button></a><br>\n"
                                      "<a href=\"/setup_clock\"><button>Setup Clock</button></a><br>\n"
                                      "<a href=\"/restart\"><button>Restart</button></a><br>\n"
                                      "<a href=\"/exit\"><button>Exit</button></a>\n"
                                      "</div>\n"
                                      "</body>\n"
                                      "</html>";

// Original HTML pages (excluding OTA)
static const char *restart_html = "<!DOCTYPE html>\n"
                                  "<html lang=\"en\">\n"
                                  "<head>\n"
                                  "<meta charset=\"UTF-8\">\n"
                                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                                  "<title>Restart Device</title>\n"
                                  "<style>\n"
                                  "body { font-family: Arial, sans-serif; display: flex; justify-content: center; "
                                  "align-items: center; height: 100vh; margin: 0; background-color: #f0f0f0; }\n"
                                  ".container { text-align: center; background-color: white; padding: 20px; "
                                  "border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }\n"
                                  "h1 { color: #333; }\n"
                                  "button { padding: 10px 20px; margin: 10px; background-color: #007bff; color: white; "
                                  "border: none; border-radius: 5px; cursor: pointer; }\n"
                                  "button:hover { background-color: #0056b3; }\n"
                                  "</style>\n"
                                  "</head>\n"
                                  "<body>\n"
                                  "<div class=\"container\">\n"
                                  "<h1>Restart Device</h1>\n"
                                  "<a href=\"/do_restart\"><button>Confirm Restart</button></a>\n"
                                  "</div>\n"
                                  "</body>\n"
                                  "</html>";

static const char *exit_html = "<!DOCTYPE html>\n"
                               "<html lang=\"en\">\n"
                               "<head>\n"
                               "<meta charset=\"UTF-8\">\n"
                               "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                               "<title>Exit Configuration</title>\n"
                               "<style>\n"
                               "body { font-family: Arial, sans-serif; display: flex; justify-content: center; "
                               "align-items: center; height: 100vh; margin: 0; background-color: #f0f0f0; }\n"
                               ".container { text-align: center; background-color: white; padding: 20px; "
                               "border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }\n"
                               "h1 { color: #333; }\n"
                               "button { padding: 10px 20px; margin: 10px; background-color: #007bff; color: white; "
                               "border: none; border-radius: 5px; cursor: pointer; }\n"
                               "button:hover { background-color: #0056b3; }\n"
                               "</style>\n"
                               "</head>\n"
                               "<body>\n"
                               "<div class=\"container\">\n"
                               "<h1>Exit Configuration</h1>\n"
                               "<p>You will be disconnected from this network</p>\n"
                               "<a href=\"/do_exit\"><button>Confirm Exit</button></a>\n"
                               "</div>\n"
                               "</body>\n"
                               "</html>";

// Function to generate AP SSID based on MAC address
static void generate_ap_ssid(void)
{
    uint8_t mac[6];
    char mac_str[13];
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read SoftAP MAC: %s", esp_err_to_name(err));
        snprintf(ap_ssid, sizeof(ap_ssid), "AIROWL_DEFAULT");
        return;
    }
    ESP_LOGI(TAG, "SoftAP MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(ap_ssid, sizeof(ap_ssid), "AIROWL_%s", &mac_str[6]);
    ESP_LOGI(TAG, "Generated AP SSID: %s", ap_ssid);
}

// SNTP callback for time synchronization
static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Received time adjustment from NTP");
    // Ensure timezone is applied correctly after NTP sync
    char saved_timezone[32] = {0};
    size_t timezone_len = sizeof(saved_timezone);
    if (load_timezone(saved_timezone, &timezone_len) == ESP_OK && strlen(saved_timezone) > 0) {
        strncpy(ntp_timezone, saved_timezone, sizeof(ntp_timezone));
        ESP_LOGI(TAG, "Reapplied timezone from NVS after NTP sync: %s", ntp_timezone);
        setenv("TZ", ntp_timezone, 1);
        tzset();
    } else {
        ESP_LOGI(TAG, "Using default timezone after NTP sync: %s", ntp_timezone);
        setenv("TZ", ntp_timezone, 1);
        tzset();
    }
    struct tm timeinfo;
    localtime_r(&tv->tv_sec, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%A, %B %d %Y %H:%M:%S %Z", &timeinfo);
    ESP_LOGI(TAG, "Current time: %s", strftime_buf);
    ESP_LOGI(TAG, "System time set to: %ld", (long)tv->tv_sec);
    ntp_synced = true;
}

// Task to update clock display
static void clock_update_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);
    char time_buf[16];
    while (1) {
        time_t now = time(NULL);
        struct tm *timeinfo = localtime(&now);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", timeinfo);

        bsp_display_lock(0);
        if (ui_clock2) {
            lv_label_set_text(ui_clock2, time_buf);
        } else {
            ESP_LOGE(TAG, "ui_clock2 is NULL");
        }
        bsp_display_unlock();

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// URL decode function
static void url_decode(char *dst, const char *src, size_t dst_size)
{
    char *dst_ptr = dst;
    const char *src_ptr = src;
    char hex[3] = {0};

    while (*src_ptr && (dst_ptr - dst) < (dst_size - 1)) {
        if (*src_ptr == '%' && src_ptr[1] && src_ptr[2]) {
            hex[0] = src_ptr[1];
            hex[1] = src_ptr[2];
            hex[2] = '\0';
            *dst_ptr = (char)strtol(hex, NULL, 16);
            src_ptr += 3;
            dst_ptr++;
        } else {
            *dst_ptr++ = *src_ptr++;
        }
    }
    *dst_ptr = '\0';
}

// Save timezone to NVS
static esp_err_t save_timezone(const char *timezone)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS for timezone: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, "TZ", timezone);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving timezone: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return err;
}

// Load timezone from NVS
static esp_err_t load_timezone(char *timezone, size_t *timezone_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "NVS partition not found for timezone");
        return err;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS for timezone: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str(nvs_handle, "TZ", timezone, timezone_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading timezone: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

// Initialize SNTP
static esp_err_t initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER_1);
    esp_sntp_setservername(1, NTP_SERVER_2);
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    // Apply saved timezone before NTP sync
    char saved_timezone[32] = {0};
    size_t timezone_len = sizeof(saved_timezone);
    if (load_timezone(saved_timezone, &timezone_len) == ESP_OK && strlen(saved_timezone) > 0) {
        strncpy(ntp_timezone, saved_timezone, sizeof(ntp_timezone));
        ESP_LOGI(TAG, "Loaded timezone from NVS before NTP sync: %s", ntp_timezone);
        setenv("TZ", ntp_timezone, 1);
        tzset();
    } else {
        ESP_LOGI(TAG, "Using default timezone before NTP: %s", ntp_timezone);
        setenv("TZ", ntp_timezone, 1);
        tzset();
    }

    for (int retry = 0; retry < NTP_RETRY_COUNT; retry++) {
        ESP_LOGI(TAG, "NTP sync attempt %d/%d", retry + 1, NTP_RETRY_COUNT);
        int retry_count = NTP_TIMEOUT_MS / 1000;
        int i = 0;
        while (i < retry_count) {
            if (ntp_synced) {
                ESP_LOGI(TAG, "NTP sync already completed, exiting early");
                return ESP_OK;
            }
            ESP_LOGI(TAG, "Waiting for NTP time: attempt %d, retry %d/%d", retry + 1, i + 1, retry_count);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            i++;
        }
        ESP_LOGW(TAG, "NTP sync attempt %d failed", retry + 1);
    }

    ESP_LOGE(TAG, "All NTP sync attempts failed");
    ntp_synced = false;
    // Preserve existing time if previously synced
    if (time(NULL) > 946684800) { // Check if time is beyond Jan 1, 2000
        ESP_LOGI(TAG, "Preserving existing time due to NTP failure: %ld", (long)time(NULL));
        return ESP_OK; // Avoid resetting time
    }
    return ESP_FAIL;
}

static esp_err_t save_wifi_credentials(const char *ssid, const char *pass)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_handle, "ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str(nvs_handle, "pass", pass);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    return err;
}

static esp_err_t load_wifi_credentials(char *ssid, size_t *ssid_len, char *pass, size_t *pass_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "NVS partition not found, initializing...");
        nvs_flash_init();
        err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error opening NVS after init: %s", esp_err_to_name(err));
            return err;
        }
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str(nvs_handle, "ssid", ssid, ssid_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_get_str(nvs_handle, "pass", pass, pass_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

static esp_err_t wifi_config_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    char resp_str[2048];
    snprintf(resp_str, sizeof(resp_str), wifi_config_html, ap_ssid);
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, "", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t wifi_form_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    const char *resp_str = "<!DOCTYPE html>\n"
                           "<html lang=\"en\">\n"
                           "<head>\n"
                           "<meta charset=\"UTF-8\">\n"
                           "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                           "<title>WiFi Setup</title>\n"
                           "<style>\n"
                           "body { font-family: Arial, sans-serif; display: flex; justify-content: center; "
                           "align-items: center; height: 100vh; margin: 0; background-color: #f0f0f0; }\n"
                           ".container { text-align: center; background-color: white; padding: 20px; border-radius: "
                           "10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }\n"
                           "h1 { color: #333; }\n"
                           "input[type=text], input[type=password] { padding: 8px; margin: 10px; width: 200px; }\n"
                           "button { padding: 10px 20px; background-color: #007bff; color: white; border: none; "
                           "border-radius: 5px; cursor: pointer; }\n"
                           "button:hover { background-color: #0056b3; }\n"
                           "</style>\n"
                           "</head>\n"
                           "<body>\n"
                           "<div class=\"container\">\n"
                           "<h1>WiFi Setup</h1>\n"
                           "<form action=\"/wifi_form\" method=\"post\">\n"
                           "<label for=\"ssid\">SSID:</label><br>\n"
                           "<input type=\"text\" id=\"ssid\" name=\"ssid\" required><br>\n"
                           "<label for=\"pass\">Password:</label><br>\n"
                           "<input type=\"password\" id=\"pass\" name=\"pass\" required minlength=\"8\"><br><br>\n"
                           "<button type=\"submit\">Connect</button>\n"
                           "</form>\n"
                           "</div>\n"
                           "</body>\n"
                           "</html>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t setup_clock_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);

    // Get current time for display
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char sys_time[32];
    strftime(sys_time, sizeof(sys_time), "%Y-%m-%d, %H:%M:%S", timeinfo);

    // Load saved timezone from NVS for display
    char saved_timezone[32] = {0};
    size_t timezone_len = sizeof(saved_timezone);
    if (load_timezone(saved_timezone, &timezone_len) == ESP_OK && strlen(saved_timezone) > 0) {
        strncpy(ntp_timezone, saved_timezone, sizeof(ntp_timezone));
        ESP_LOGI(TAG, "Loaded timezone from NVS for display: %s", ntp_timezone);
    } else {
        ESP_LOGI(TAG, "Using default timezone for display: %s", ntp_timezone);
    }

    // Send HTML header
    const char *header = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
                         "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                         "<title>Time Settings</title>"
                         "<style>"
                         "body { font-family: Arial, sans-serif; display: flex; justify-content: center; "
                         "align-items: center; height: 100vh; margin: 0; background-color: #f0f0f0; }"
                         ".container { text-align: center; background-color: white; padding: 20px; "
                         "border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }"
                         "h1 { color: #333; }"
                         "select, input[type=text] { padding: 8px; margin: 10px; width: 200px; }"
                         "button { padding: 10px 20px; background-color: #007bff; color: white; "
                         "border: none; border-radius: 5px; cursor: pointer; }"
                         "button:hover { background-color: #0056b3; }"
                         "</style></head><body><div class=\"container\">"
                         "<h1>Time Settings</h1><form action=\"/setup_clock\" method=\"post\">"
                         "<h3>System Time</h3><p>%s</p>"
                         "<a href=\"/setup_clock\"><button type=\"button\">Refresh</button></a><br><br>"
                         "<h3>Time Zone</h3><select name=\"timezone\" id=\"timezone\">\n";
    char header_buf[1024];
    snprintf(header_buf, sizeof(header_buf), header, sys_time);
    httpd_resp_send_chunk(req, header_buf, strlen(header_buf));

    // Send timezone options in chunks
    for (size_t i = 0; i < NUM_TIMEZONES; i++) {
        char option[128];
        snprintf(option, sizeof(option), "<option value=\"%s\" %s>%s</option>\n",
                 timezones[i].posix_tz,
                 strcmp(timezones[i].posix_tz, ntp_timezone) == 0 ? "selected" : "",
                 timezones[i].region);
        httpd_resp_send_chunk(req, option, strlen(option));
    }

    // Send form footer
    const char *footer = "</select><br><br>"
                         "<label for=\"hour\">Hour (0-23):</label><br>"
                         "<input type=\"text\" id=\"hour\" name=\"hour\" required pattern=\"[0-9]{1,2}\"><br>"
                         "<label for=\"minute\">Minute (0-59):</label><br>"
                         "<input type=\"text\" id=\"minute\" name=\"minute\" required pattern=\"[0-9]{1,2}\"><br>"
                         "<label for=\"second\">Second (0-59):</label><br>"
                         "<input type=\"text\" id=\"second\" name=\"second\" required pattern=\"[0-9]{1,2}\"><br><br>"
                         "<button type=\"submit\">Set Time</button></form></div></body></html>";
    httpd_resp_send_chunk(req, footer, strlen(footer));
    httpd_resp_send_chunk(req, NULL, 0); // End chunked response
    return ESP_OK;
}

static esp_err_t setup_clock_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling POST request for URI: %s, content length: %d", req->uri, req->content_len);
    char buf[256];
    int ret, remaining = req->content_len;
    char hour_str[3] = {0}, minute_str[3] = {0}, second_str[3] = {0}, timezone[32] = {0};
    char decoded_timezone[32] = {0};
    bool valid_input = true;

    if (remaining > sizeof(buf) - 1) {
        ESP_LOGE(TAG, "POST data too large: %d bytes", remaining);
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Content too large");
        return ESP_FAIL;
    }

    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
                return ESP_FAIL;
            }
            return ESP_FAIL;
        }
        buf[ret] = '\0';
        remaining -= ret;
    }

    // Parse POST data
    char *hour_start = strstr(buf, "hour=");
    char *minute_start = strstr(buf, "minute=");
    char *second_start = strstr(buf, "second=");
    char *tz_start = strstr(buf, "timezone=");
    if (hour_start && minute_start && second_start && tz_start) {
        hour_start += strlen("hour=");
        minute_start += strlen("minute=");
        second_start += strlen("second=");
        tz_start += strlen("timezone=");

        char *hour_end = strchr(hour_start, '&');
        char *minute_end = strchr(minute_start, '&');
        char *second_end = strchr(second_start, '&');
        char *tz_end = strchr(tz_start, '&');

        if (!hour_end)
            hour_end = hour_start + strlen(hour_start);
        if (!minute_end)
            minute_end = minute_start + strlen(minute_start);
        if (!second_end)
            second_end = second_start + strlen(second_start);
        if (!tz_end)
            tz_end = tz_start + strlen(tz_start);

        strncpy(hour_str, hour_start, MIN(hour_end - hour_start, sizeof(hour_str) - 1));
        strncpy(minute_str, minute_start, MIN(minute_end - minute_start, sizeof(minute_str) - 1));
        strncpy(second_str, second_start, MIN(second_end - second_start, sizeof(second_str) - 1));
        strncpy(timezone, tz_start, MIN(tz_end - tz_start, sizeof(timezone) - 1));
    } else {
        valid_input = false;
        ESP_LOGW(TAG, "Missing required POST parameters");
    }

    // Validate time inputs
    int hour = atoi(hour_str);
    int minute = atoi(minute_str);
    int second = atoi(second_str);
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        valid_input = false;
        ESP_LOGW(TAG, "Validation failed: hour=%d, minute=%d, second=%d", hour, minute, second);
    }

    // Validate and save timezone
    if (strlen(timezone) > 0) {
        url_decode(decoded_timezone, timezone, sizeof(decoded_timezone));
        ESP_LOGI(TAG, "Received timezone: %s, Decoded timezone: %s", timezone, decoded_timezone);
        if (strlen(decoded_timezone) > 0) {
            // Validate timezone against timezones array
            bool valid_timezone = false;
            for (size_t i = 0; i < NUM_TIMEZONES; i++) {
                if (strcmp(decoded_timezone, timezones[i].posix_tz) == 0) {
                    valid_timezone = true;
                    break;
                }
            }

            if (valid_timezone) {
                esp_err_t err = save_timezone(decoded_timezone);
                if (err == ESP_OK) {
                    strncpy(ntp_timezone, decoded_timezone, sizeof(ntp_timezone));
                    setenv("TZ", ntp_timezone, 1);
                    tzset();
                    ESP_LOGI(TAG, "Timezone set to: %s", ntp_timezone);
                } else {
                    ESP_LOGE(TAG, "Failed to save timezone");
                    httpd_resp_send(req, "Failed to save timezone", HTTPD_RESP_USE_STRLEN);
                    return ESP_OK;
                }
            } else {
                valid_input = false;
                ESP_LOGW(TAG, "Invalid timezone: %s", decoded_timezone);
            }
        } else {
            valid_input = false;
            ESP_LOGW(TAG, "Invalid decoded timezone");
        }
    } else {
        valid_input = false;
        ESP_LOGW(TAG, "No valid timezone provided");
    }

    if (valid_input) {
        time_t now = time(NULL);
        struct tm *current_time = localtime(&now);
        struct tm timeinfo = *current_time;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = second;

        time_t rawtime = mktime(&timeinfo);
        if (rawtime == -1) {
            ESP_LOGE(TAG, "Failed to convert time to time_t");
            httpd_resp_send(req, "Invalid time format", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }

        struct timeval tv = { .tv_sec = rawtime, .tv_usec = 0 };
        if (settimeofday(&tv, NULL) != 0) {
            ESP_LOGE(TAG, "Failed to set system time");
            httpd_resp_send(req, "Failed to set system time", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }

        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(TAG, "System time set to: %s", time_str);
        httpd_resp_send(req, "Time and timezone set successfully. Redirecting...", HTTPD_RESP_USE_STRLEN);

        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/set_wifi");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Invalid input received");
        httpd_resp_send(req, "Invalid input. Check hour (0-23), minute (0-59), second (0-59), and timezone.",
                        HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req)
{
    char buf[256];
    char ssid[32] = {0};
    char pass[64] = {0};
    int ret, remaining = req->content_len;

    if (remaining > sizeof(buf) - 1) {
        ESP_LOGE(TAG, "POST data too large: %d bytes", remaining);
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Content too large");
        return ESP_FAIL;
    }

    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        buf[ret] = '\0';
        remaining -= ret;
    }

    char *ssid_start = strstr(buf, "ssid=");
    char *pass_start = strstr(buf, "pass=");
    if (ssid_start && pass_start) {
        ssid_start += strlen("ssid=");
        pass_start += strlen("pass=");
        char *ssid_end = strchr(ssid_start, '&');
        char *pass_end = strchr(pass_start, '&');
        if (!ssid_end)
            ssid_end = ssid_start + strlen(ssid_start);
        if (!pass_end)
            pass_end = pass_start + strlen(pass_start);

        strncpy(ssid, ssid_start, MIN(ssid_end - ssid_start, sizeof(ssid) - 1));
        strncpy(pass, pass_start, MIN(pass_end - pass_start, sizeof(pass) - 1));

        char decoded_ssid[32];
        char decoded_pass[64];
        url_decode(decoded_ssid, ssid, sizeof(decoded_ssid));
        url_decode(decoded_pass, pass, sizeof(decoded_pass));
        strncpy(ssid, decoded_ssid, sizeof(ssid));
        strncpy(pass, decoded_pass, sizeof(pass));
    }

    if (strlen(ssid) > 0 && strlen(pass) >= 8) {
        ESP_LOGI(TAG, "Received SSID: %s, Password: %s", ssid, pass);
        save_wifi_credentials(ssid, pass);
        httpd_resp_send(req, "WiFi credentials saved. Rebooting...", HTTPD_RESP_USE_STRLEN);
        esp_wifi_stop();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Invalid credentials received");
        httpd_resp_send(req, "Invalid credentials", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/set_wifi");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t restart_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, restart_html, strlen(restart_html));
    return ESP_OK;
}

static esp_err_t exit_config_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, exit_html, strlen(exit_html));
    return ESP_OK;
}

static esp_err_t do_restart_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, "Restarting device...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

static esp_err_t do_exit_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, "Disconnecting...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // Stop services
    stop_webserver();
    esp_wifi_stop();
    esp_restart();
    return ESP_OK;
}

static void stop_webserver(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Webserver stopped");
    }
}

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20; // Increase max URI handlers if needed
    config.stack_size = 24576; // Increase stack size for larger requests

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Starting webserver");
        httpd_uri_t root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t wifi_config = {
            .uri       = "/set_wifi",
            .method    = HTTP_GET,
            .handler   = wifi_config_get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t favicon = {
            .uri       = "/favicon.ico",
            .method    = HTTP_GET,
            .handler   = favicon_get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t wifi_form = {
            .uri       = "/wifi_form",
            .method    = HTTP_GET,
            .handler   = wifi_form_get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t wifi_form_post = {
            .uri       = "/wifi_form",
            .method    = HTTP_POST,
            .handler   = wifi_config_post_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t restart = {
            .uri       = "/restart",
            .method    = HTTP_GET,
            .handler   = restart_get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t do_restart = {
            .uri       = "/do_restart",
            .method    = HTTP_GET,
            .handler   = do_restart_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t exit_config = {
            .uri       = "/exit",
            .method    = HTTP_GET,
            .handler   = exit_config_get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t do_exit = {
            .uri       = "/do_exit",
            .method    = HTTP_GET,
            .handler   = do_exit_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t setup_clock = {
            .uri       = "/setup_clock",
            .method    = HTTP_GET,
            .handler   = setup_clock_get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t setup_clock_post = {
            .uri       = "/setup_clock",
            .method    = HTTP_POST,
            .handler   = setup_clock_post_handler,
            .user_ctx  = NULL
        };

        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &wifi_config);
        httpd_register_uri_handler(server, &favicon);
        httpd_register_uri_handler(server, &wifi_form);
        httpd_register_uri_handler(server, &wifi_form_post);
        httpd_register_uri_handler(server, &restart);
        httpd_register_uri_handler(server, &do_restart);
        httpd_register_uri_handler(server, &exit_config);
        httpd_register_uri_handler(server, &do_exit);
        httpd_register_uri_handler(server, &setup_clock);
        httpd_register_uri_handler(server, &setup_clock_post);
    }
    return server;
}

static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP, attempt %d", s_retry_num);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            wifi_connected = false;
            ESP_LOGI(TAG, "Failed to connect to AP after %d attempts", WIFI_MAX_RETRY);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                 IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", ip_str);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_connected = true;
        stop_webserver();
        initialize_sntp();
    }
}

void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    esp_err_t err = esp_event_loop_create_default();
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "Default event loop already created");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    generate_ap_ssid();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    xTaskCreatePinnedToCore(clock_update_task, "clock_task", 12288, NULL, 2, NULL, 0);

    // Load and apply saved timezone
    char saved_timezone[32] = {0};
    size_t timezone_len = sizeof(saved_timezone);
    if (load_timezone(saved_timezone, &timezone_len) == ESP_OK && strlen(saved_timezone) > 0) {
        strncpy(ntp_timezone, saved_timezone, sizeof(ntp_timezone));
        ESP_LOGI(TAG, "Loaded timezone from NVS at init: %s", ntp_timezone);
        setenv("TZ", ntp_timezone, 1);
        tzset();
    } else {
        ESP_LOGI(TAG, "Using default timezone at init: %s", ntp_timezone);
        setenv("TZ", ntp_timezone, 1);
        tzset();
    }

    char ssid[32] = {0};
    char pass[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);

    if (load_wifi_credentials(ssid, &ssid_len, pass, &pass_len) == ESP_OK) {
        ESP_LOGI(TAG, "Loaded saved credentials SSID: %s", ssid);
        wifi_config_t wifi_config = {0};
        strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
    } else {
        ESP_LOGI(TAG, "No saved credentials found, starting AP mode");
        wifi_config_t wifi_config = {0};
        strncpy((char *)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid));
        wifi_config.ap.ssid_len = strlen(ap_ssid);
        strncpy((char *)wifi_config.ap.password, AP_PASS, sizeof(wifi_config.ap.password));
        wifi_config.ap.max_connection = 4;
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        start_webserver();
    }
}

const char* wifi_get_ap_ssid(void)
{
    return ap_ssid;
}

bool wifi_is_connected(void)
{
    return wifi_connected;
}