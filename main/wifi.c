// wifi.c
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include "lvgl.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ui.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>
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
#define NTP_SERVER1 "0.pool.ntp.org"
#define NTP_SERVER2 "1.pool.ntp.org"
#define NTP_SERVER3 "2.pool.ntp.org"
char ntp_timezone[32] = "IST-5:30"; // Global variable for timezone

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

// Updated main configuration page
static const char *wifi_config_html = "<html>"
                                      "<head>"
                                      "<title>AIROWL Configuration</title>"
                                      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                                      "<style>"
                                      "body {"
                                      "    background-color: #1a1a1a;"
                                      "    color: white;"
                                      "    font-family: Arial, sans-serif;"
                                      "    display: flex;"
                                      "    flex-direction: column;"
                                      "    align-items: center;"
                                      "    justify-content: center;"
                                      "    height: 100vh;"
                                      "    margin: 0;"
                                      "    text-align: center;"
                                      "}"
                                      "h1 {"
                                      "    font-size: 2em;"
                                      "    margin-bottom: 10px;"
                                      "}"
                                      "h2 {"
                                      "    font-size: 1.2em;"
                                      "    margin-bottom: 20px;"
                                      "    color: #ccc;"
                                      "}"
                                      ".button {"
                                      "    background-color: #007bff;"
                                      "    color: white;"
                                      "    padding: 15px;"
                                      "    margin: 10px 0;"
                                      "    border: none;"
                                      "    border-radius: 5px;"
                                      "    width: -webkit-fill-available;"
                                      "    max-width: 300px;"
                                      "    font-size: 1.2em;"
                                      "    cursor: pointer;"
                                      "    text-decoration: none;"
                                      "    display: block;"
                                      "    text-align: center;"
                                      "}"
                                      ".button:hover {"
                                      "    background-color: #0056b3;"
                                      "}"
                                      ".divider {"
                                      "    width: -webkit-fill-available;"
                                      "    max-width: 300px;"
                                      "    height: 1px;"
                                      "    background-color: white;"
                                      "    margin: 10px 0;"
                                      "}"
                                      "</style>"
                                      "</head>"
                                      "<body>"
                                      "<h1>AIROWL Configuration</h1>"
                                      "<h2>%s</h2>"
                                      "<a href='/wifi_form' class='button'>Configure WiFi</a>"
                                      "<a href='/ota' class='button'>OTA Update</a>"
                                      "<a href='/setup_clock' class='button'>Setup Clock</a>"
                                      "<div class='divider'></div>"
                                      "<a href='/restart' class='button'>Restart</a>"
                                      "<a href='/exit' class='button'>Exit</a>"
                                      "</body>"
                                      "</html>";

// New HTML pages
static const char *ota_get_html = "<html>"
                                  "<head>"
                                  "<title>OTA Update</title>"
                                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                                  "<style>"
                                  "body {"
                                  "    background-color: #1a1a1a;"
                                  "    color: white;"
                                  "    font-family: Arial, sans-serif;"
                                  "    display: flex;"
                                  "    flex-direction: column;"
                                  "    align-items: center;"
                                  "    justify-content: center;"
                                  "    height: 100vh;"
                                  "    margin: 0;"
                                  "}"
                                  "h1 {"
                                  "    font-size: 2em;"
                                  "    margin-bottom: 20px;"
                                  "}"
                                  ".button {"
                                  "    background-color: #007bff;"
                                  "    color: white;"
                                  "    padding: 15px;"
                                  "    margin: 10px 0;"
                                  "    border: none;"
                                  "    border-radius: 5px;"
                                  "    width: 80%;"
                                  "    max-width: 300px;"
                                  "    font-size: 1.2em;"
                                  "    cursor: pointer;"
                                  "    text-decoration: none;"
                                  "    display: block;"
                                  "    text-align: center;"
                                  "}"
                                  ".button:hover {"
                                  "    background-color: #0056b3;"
                                  "}"
                                  "</style>"
                                  "</head>"
                                  "<body>"
                                  "<h1>OTA Update</h1>"
                                  "<p>Firmware update without Wi-Fi connection</p>"
                                  "<a href='/do_ota' class='button'>Start OTA Update</a>"
                                  "</body>"
                                  "</html>";

static const char *restart_html = "<html>"
                                  "<head>"
                                  "<title>Restart Device</title>"
                                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                                  "<style>"
                                  "body {"
                                  "    background-color: #1a1a1a;"
                                  "    color: white;"
                                  "    font-family: Arial, sans-serif;"
                                  "    display: flex;"
                                  "    flex-direction: column;"
                                  "    align-items: center;"
                                  "    justify-content: center;"
                                  "    height: 100vh;"
                                  "    margin: 0;"
                                  "}"
                                  "h1 {"
                                  "    font-size: 2em;"
                                  "    margin-bottom: 20px;"
                                  "}"
                                  ".button {"
                                  "    background-color: #007bff;"
                                  "    color: white;"
                                  "    padding: 15px;"
                                  "    margin: 10px 0;"
                                  "    border: none;"
                                  "    border-radius: 5px;"
                                  "    width: 80%;"
                                  "    max-width: 300px;"
                                  "    font-size: 1.2em;"
                                  "    cursor: pointer;"
                                  "    text-decoration: none;"
                                  "    display: block;"
                                  "    text-align: center;"
                                  "}"
                                  ".button:hover {"
                                  "    background-color: #0056b3;"
                                  "}"
                                  "</style>"
                                  "</head>"
                                  "<body>"
                                  "<h1>Restart Device</h1>"
                                  "<a href='/do_restart' class='button'>Confirm Restart</a>"
                                  "</body>"
                                  "</html>";

static const char *exit_html = "<html>"
                               "<head>"
                               "<title>Exit Configuration</title>"
                               "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                               "<style>"
                               "body {"
                               "    background-color: #1a1a1a;"
                               "    color: white;"
                               "    font-family: Arial, sans-serif;"
                               "    display: flex;"
                               "    flex-direction: column;"
                               "    align-items: center;"
                               "    justify-content: center;"
                               "    height: 100vh;"
                               "    margin: 0;"
                               "}"
                               "h1 {"
                               "    font-size: 2em;"
                               "    margin-bottom: 20px;"
                               "}"
                               ".button {"
                               "    background-color: #007bff;"
                               "    color: white;"
                               "    padding: 15px;"
                               "    margin: 10px 0;"
                               "    border: none;"
                               "    border-radius: 5px;"
                               "    width: 80%;"
                               "    max-width: 300px;"
                               "    font-size: 1.2em;"
                               "    cursor: pointer;"
                               "    text-decoration: none;"
                               "    display: block;"
                               "    text-align: center;"
                               "}"
                               ".button:hover {"
                               "    background-color: #0056b3;"
                               "}"
                               "</style>"
                               "</head>"
                               "<body>"
                               "<h1>Exit Configuration</h1>"
                               "<p>You will be disconnected from this network</p>"
                               "<a href='/do_exit' class='button'>Confirm Exit</a>"
                               "</body>"
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
    struct tm timeinfo;
    localtime_r(&tv->tv_sec, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%A, %B %d %Y %H:%M:%S %Z", &timeinfo);
    ESP_LOGI(TAG, "Current time: %s", strftime_buf);
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

    err = nvs_set_str(nvs_handle, "timezone", timezone);
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
        ESP_LOGI(TAG, "NVS partition not found for timezone, using default");
        return err;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS for timezone: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_str(nvs_handle, "timezone", timezone, timezone_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading timezone: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

// Initialize SNTP
static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER1);
    esp_sntp_setservername(1, NTP_SERVER2);
    esp_sntp_setservername(2, NTP_SERVER3);
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    char saved_timezone[32] = {0};
    size_t timezone_len = sizeof(saved_timezone);
    if (load_timezone(saved_timezone, &timezone_len) == ESP_OK && strlen(saved_timezone) > 0) {
        strncpy(ntp_timezone, saved_timezone, sizeof(ntp_timezone));
        ESP_LOGI(TAG, "Loaded timezone from NVS: %s", ntp_timezone);
    } else {
        ESP_LOGI(TAG, "Using default timezone: %s", ntp_timezone);
    }

    setenv("TZ", ntp_timezone, 1);
    tzset();
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
        nvs_flash_init_partition(NVS_DEFAULT_PART_NAME);
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
    const char *resp_str = "<html>"
                           "<head>"
                           "<title>WiFi Setup</title>"
                           "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                           "<style>"
                           "body {"
                           "    background-color: #1a1a1a;"
                           "    color: white;"
                           "    font-family: Arial, sans-serif;"
                           "    display: flex;"
                           "    flex-direction: column;"
                           "    align-items: center;"
                           "    justify-content: center;"
                           "    height: 100vh;"
                           "    margin: 0;"
                           "    text-align: center;"
                           "}"
                           "h1 {"
                           "    font-size: 2em;"
                           "    margin-bottom: 20px;"
                           "}"
                           "form {"
                           "    display: flex;"
                           "    flex-direction: column;"
                           "    align-items: center;"
                           "}"
                           "input[type='text'], input[type='password'] {"
                           "    padding: 10px;"
                           "    margin: 10px 0;"
                           "    width: 80%;"
                           "    max-width: 300px;"
                           "    border-radius: 5px;"
                           "    border: none;"
                           "    font-size: 1em;"
                           "}"
                           "input[type='submit'] {"
                           "    background-color: #007bff;"
                           "    color: white;"
                           "    padding: 15px;"
                           "    margin: 10px 0;"
                           "    border: none;"
                           "    border-radius: 5px;"
                           "    width: 80%;"
                           "    max-width: 300px;"
                           "    font-size: 1.2em;"
                           "    cursor: pointer;"
                           "}"
                           "input[type='submit']:hover {"
                           "    background-color: #0056b3;"
                           "}"
                           "</style>"
                           "</head>"
                           "<body>"
                           "<h1>WiFi Setup</h1>"
                           "<form method='post' action='/set_wifi'>"
                           "SSID: <input type='text' name='ssid'><br>"
                           "Password: <input type='password' name='pass'><br>"
                           "<input type='submit' value='Connect'>"
                           "</form>"
                           "</body>"
                           "</html>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t setup_clock_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);

    // Get current system time for display
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char sys_time[32];
    strftime(sys_time, sizeof(sys_time), "%m/%d/%Y, %I:%M:%S %p", timeinfo);

    const char *resp_str = "<html>"
                           "<head>"
                           "<title>Time Settings</title>"
                           "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                           "<style>"
                           "body {"
                           "    background-color: #1a1a1a;"
                           "    color: white;"
                           "    font-family: Arial, sans-serif;"
                           "    display: flex;"
                           "    flex-direction: column;"
                           "    align-items: center;"
                           "    justify-content: center;"
                           "    min-height: 100vh;"
                           "    margin: 0;"
                           "    padding: 20px;"
                           "    box-sizing: border-box;"
                           "    overflow: hidden;"
                           "}"
                           "h1 {"
                           "    font-size: 2em;"
                           "    margin-bottom: 20px;"
                           "    text-align: center;"
                           "}"
                           "form {"
                           "    display: flex;"
                           "    flex-direction: column;"
                           "    align-items: center;"
                           "    width: 100%;"
                           "    max-width: 400px;"
                           "}"
                           ".form-group {"
                           "    display: flex;"
                           "    justify-content: space-between;"
                           "    align-items: center;"
                           "    width: 100%;"
                           "    margin-bottom: 20px;"
                           "    gap: 20px;"
                           "}"
                           "label {"
                           "    font-size: 1em;"
                           "    flex: 1;"
                           "    text-align: left;"
                           "    padding-left: 20px;"
                           "}"
                           ".time-display {"
                           "    flex: 2;"
                           "    text-align: right;"
                           "    font-size: 1em;"
                           "}"
                           "select, input[type='number'] {"
                           "    flex: 2;"
                           "    padding: 10px;"
                           "    border-radius: 5px;"
                           "    border: none;"
                           "    font-size: 1em;"
                           "    background-color: #fff;"
                           "    color: #000;"
                           "    width: 100%;"
                           "    box-sizing: border-box;"
                           "}"
                           ".time-inputs {"
                           "    display: flex;"
                           "    flex: 2;"
                           "    gap: 10px;"
                           "    align-items: center;"
                           "}"
                           ".time-inputs input {"
                           "    width: 60px;" // Smaller width for hour/minute/second inputs
                           "    text-align: center;"
                           "}"
                           "input[type='submit'] {"
                           "    background-color: #007bff;"
                           "    color: white;"
                           "    padding: 15px;"
                           "    margin-top: 20px;"
                           "    border: none;"
                           "    border-radius: 5px;"
                           "    width: 100%;"
                           "    max-width: 300px;"
                           "    font-size: 1.2em;"
                           "    cursor: pointer;"
                           "}"
                           "input[type='submit']:hover {"
                           "    background-color: #0056b3;"
                           "}"
                           ".refresh-btn {"
                           "    background-color: #007bff;"
                           "    color: white;"
                           "    padding: 5px 10px;"
                           "    border: none;"
                           "    border-radius: 5px;"
                           "    cursor: pointer;"
                           "    font-size: 0.9em;"
                           "    margin-left: 10px;"
                           "}"
                           ".refresh-btn:hover {"
                           "    background-color: #0056b3;"
                           "}"
                           "</style>"
                           "</head>"
                           "<body>"
                           "<h1>Time Settings</h1>"
                           "<form method='post' action='/set_clock'>"
                           "<div class='form-group'>"
                           "<label>System Time</label>"
                           "<span class='time-display'>%s</span>"
                           "<button type='button' class='refresh-btn' onclick='location.reload()'>Refresh</button>"
                           "</div>"
                           "<div class='form-group'>"
                           "<label>Time Zone</label>"
                           "<select name='timezone' required>"
                           "<option value='%s' selected>%s</option>"
                           "<option value='UTC0'>UTC</option>"
                           "<option value='EST5EDT'>Eastern Time (US)</option>"
                           "<option value='CST6CDT'>Central Time (US)</option>"
                           "<option value='MST7MDT'>Mountain Time (US)</option>"
                           "<option value='PST8PDT'>Pacific Time (US)</option>"
                           "<option value='CET-1CEST'>Central European Time</option>"
                           "<option value='IST-5:30'>India Standard Time</option>"
                           "<option value='JST-9'>Japan Standard Time</option>"
                           "</select>"
                           "</div>"
                           "<div class='form-group'>"
                           "<label>Set Time</label>"
                           "<div class='time-inputs'>"
                           "<input type='number' name='hour' min='0' max='23' placeholder='HH' required>"
                           "<input type='number' name='minute' min='0' max='59' placeholder='MM' required>"
                           "<input type='number' name='second' min='0' max='59' placeholder='SS' required>"
                           "</div>"
                           "</div>"
                           "<input type='submit' value='Submit'>"
                           "</form>"
                           "</body>"
                           "</html>";

    char resp_buf[4096];
    snprintf(resp_buf, sizeof(resp_buf), resp_str, sys_time, ntp_timezone, ntp_timezone);
    httpd_resp_send(req, resp_buf, strlen(resp_buf));
    return ESP_OK;
}

// Clock setup POST handler
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
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Request entity too large");
        return ESP_FAIL;
    }

    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf) - 1));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            ESP_LOGE(TAG, "Error receiving POST data: %d", ret);
            return ESP_FAIL;
        }
        buf[ret] = '\0';
        remaining -= ret;

        ESP_LOGI(TAG, "Received POST data: %s", buf);

        // Parse the expected fields: timezone, hour, minute, second
        if (sscanf(buf, "timezone=%31[^&]&hour=%2[^&]&minute=%2[^&]&second=%2s", timezone, hour_str, minute_str,
                   second_str) != 4) {
            ESP_LOGW(TAG, "Failed to parse POST data");
            valid_input = false;
        }
    }

    int hour = atoi(hour_str);
    int minute = atoi(minute_str);
    int second = atoi(second_str);

    ESP_LOGI(TAG, "Parsed values: hour=%s (%d), minute=%s (%d), second=%s (%d), timezone=%s", hour_str, hour,
             minute_str, minute, second_str, second, timezone);

    // Validate input
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        valid_input = false;
        ESP_LOGW(TAG, "Validation failed: hour=%d, minute=%d, second=%d", hour, minute, second);
    }

    // Validate and save timezone
    if (strlen(timezone) > 0) {
        url_decode(decoded_timezone, timezone, sizeof(decoded_timezone));
        ESP_LOGI(TAG, "Received timezone: %s, Decoded timezone: %s", timezone, decoded_timezone);
        if (strlen(decoded_timezone) > 0) {
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
            ESP_LOGW(TAG, "Invalid decoded timezone");
        }
    } else {
        valid_input = false;
        ESP_LOGW(TAG, "Invalid timezone received");
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

        struct timeval tv = {.tv_sec = rawtime, .tv_usec = 0};
        if (settimeofday(&tv, NULL) != 0) {
            ESP_LOGE(TAG, "Failed to set system time");
            httpd_resp_send(req, "Failed to set system time", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }

        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(TAG, "System time set to: %s", time_str);
        httpd_resp_send(req, "Time and timezone set successfully. Redirecting...", HTTPD_RESP_USE_STRLEN);
    } else {
        ESP_LOGW(TAG, "Invalid input received");
        httpd_resp_send(req, "Invalid input. Check hour (0-23), minute (0-59), second (0-59), and timezone.",
                        HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/set_wifi");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req)
{
    char buf[256];
    char ssid[32] = {0};
    char pass[64] = {0};
    int ret, remaining = req->content_len;

    if (remaining > sizeof(buf) - 1) {
        ESP_LOGE(TAG, "POST data too large: %d bytes", remaining);
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Request entity too large");
        return ESP_FAIL;
    }

    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf) - 1));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            ESP_LOGE(TAG, "Error receiving POST data: %d", ret);
            return ESP_FAIL;
        }
        buf[ret] = '\0';
        remaining -= ret;

        char *ssid_start = strstr(buf, "ssid=");
        char *pass_start = strstr(buf, "&pass=");
        if (ssid_start && pass_start) {
            sscanf(ssid_start, "ssid=%32[^&]", ssid);
            sscanf(pass_start, "&pass=%64s", pass);
        } else {
            ESP_LOGW(TAG, "Invalid form data received");
        }
    }

    ESP_LOGI(TAG, "Received Wi-Fi credentials for SSID: %s", ssid);
    if (strlen(ssid) > 0 && strlen(pass) >= 8) {
        ESP_LOGI(TAG, "Received SSID: %s, Password: %s", ssid, pass);
        save_wifi_credentials(ssid, pass);
        httpd_resp_send(req, "WiFi credentials saved. Rebooting...", HTTPD_RESP_USE_STRLEN);

        esp_wifi_stop();
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    ESP_LOGI(TAG, "Handling GET request for root URI: %s", req->uri);
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/set_wifi");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// New handler implementations
static esp_err_t ota_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, ota_get_html, strlen(ota_get_html));
    return ESP_OK;
}

static esp_err_t restart_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, restart_html, strlen(restart_html));
    return ESP_OK;
}

static esp_err_t exit_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, exit_html, strlen(exit_html));
    return ESP_OK;
}

static esp_err_t do_restart_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, "Restarting device...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

static esp_err_t do_exit_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    httpd_resp_send(req, "Disconnecting...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Stop services and disconnect clients
    stop_webserver();
    esp_wifi_stop();

    // Restart to attempt STA connection
    esp_restart();
    return ESP_OK;
}

static esp_err_t do_ota_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "Failed to find OTA update partition");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA partition not found");
        return ESP_FAIL;
    }

    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        return ESP_FAIL;
    }

    // Receive firmware data
    char buf[1024];
    int received;
    int total_len = req->content_len;
    int remaining = total_len;

    while (remaining > 0) {
        received = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue; // Retry on timeout
            }
            esp_ota_end(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive firmware");
            return ESP_FAIL;
        }

        err = esp_ota_write(ota_handle, buf, received);
        if (err != ESP_OK) {
            esp_ota_end(ota_handle);
            ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(err));
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA end failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set boot partition");
        return ESP_FAIL;
    }

    httpd_resp_send(req, "OTA update successful. Rebooting...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

static void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 16; // Increase from default 8
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t config_get = {
            .uri = "/set_wifi", .method = HTTP_GET, .handler = wifi_config_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &config_get);

        httpd_uri_t wifi_form = {
            .uri = "/wifi_form", .method = HTTP_GET, .handler = wifi_form_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &wifi_form);

        httpd_uri_t config_post = {
            .uri = "/set_wifi", .method = HTTP_POST, .handler = wifi_config_post_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &config_post);

        httpd_uri_t setup_clock_get = {
            .uri = "/setup_clock", .method = HTTP_GET, .handler = setup_clock_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &setup_clock_get);

        httpd_uri_t setup_clock_post = {
            .uri = "/set_clock", .method = HTTP_POST, .handler = setup_clock_post_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &setup_clock_post);

        httpd_uri_t root_get = {.uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &root_get);

        httpd_uri_t favicon_get = {
            .uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &favicon_get);

        // Register new handlers
        httpd_uri_t ota_get = {.uri = "/ota", .method = HTTP_GET, .handler = ota_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &ota_get);

        httpd_uri_t restart_get = {
            .uri = "/restart", .method = HTTP_GET, .handler = restart_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &restart_get);

        httpd_uri_t exit_get = {.uri = "/exit", .method = HTTP_GET, .handler = exit_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &exit_get);

        httpd_uri_t do_restart = {
            .uri = "/do_restart", .method = HTTP_GET, .handler = do_restart_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &do_restart);

        httpd_uri_t do_exit = {.uri = "/do_exit", .method = HTTP_GET, .handler = do_exit_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &do_exit);

        httpd_uri_t do_ota = {.uri = "/do_ota", .method = HTTP_POST, .handler = do_ota_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &do_ota);
    }
    ESP_LOGI(TAG, "Webserver started with new features");
}
static void stop_webserver(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            wifi_connected = false;
            ESP_LOGI(TAG, "Failed to connect, starting AP mode");
            esp_wifi_stop();
            wifi_config_t wifi_config = {
                .ap =
                    {
                        .password = AP_PASS,
                        .max_connection = 4,
                        .authmode = WIFI_AUTH_WPA2_PSK,
                    },
            };
            strncpy((char *)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid));
            wifi_config.ap.ssid_len = strlen(ap_ssid);
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_start());
            start_webserver();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
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
        ESP_LOGI(TAG, "Default event loop already exists, skipping creation");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    } else {
        ESP_LOGI(TAG, "Created default event loop");
    }

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    generate_ap_ssid();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    xTaskCreatePinnedToCore(clock_update_task, "clock_task", 8192, NULL, 2, NULL, 0);

    char ssid[32] = {0};
    char pass[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);

    if (load_wifi_credentials(ssid, &ssid_len, pass, &pass_len) == ESP_OK) {
        ESP_LOGI(TAG, "Loaded saved credentials, SSID: %s", ssid);
        wifi_config_t wifi_config = {
            .sta =
                {
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                },
        };
        strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
    } else {
        ESP_LOGI(TAG, "No saved credentials, starting AP mode");
        wifi_config_t wifi_config = {
            .ap =
                {
                    .password = AP_PASS,
                    .max_connection = 4,
                    .authmode = WIFI_AUTH_WPA2_PSK,
                },
        };
        strncpy((char *)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid));
        wifi_config.ap.ssid_len = strlen(ap_ssid);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        start_webserver();
    }
}

const char *wifi_get_ap_ssid(void)
{
    return ap_ssid;
}

bool wifi_is_connected(void)
{
    return wifi_connected;
}