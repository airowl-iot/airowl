#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAX_RETRY     10
#define NVS_NAMESPACE      "wifi_creds"
#define AP_SSID            "ESP32_WiFi_Setup"
#define AP_PASS            "password123"
#define MIN(a, b)          ((a) < (b) ? (a) : (b))

EventGroupHandle_t s_wifi_event_group;
const int WIFI_CONNECTED_BIT_GLOBAL = WIFI_CONNECTED_BIT;
const int WIFI_FAIL_BIT_GLOBAL = WIFI_FAIL_BIT;

static const char *TAG = "wifi_manager";
static int s_retry_num = 0;
static httpd_handle_t server = NULL;

static esp_err_t save_wifi_credentials(const char *ssid, const char *pass) {
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

static esp_err_t load_wifi_credentials(char *ssid, size_t *ssid_len, char *pass, size_t *pass_len) {
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

static esp_err_t wifi_config_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    const char *resp_str = 
        "<html>"
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
        ".status {"
        "    position: fixed;"
        "    bottom: 20px;"
        "    font-size: 0.9em;"
        "    color: #ccc;"
        "}"
        ".divider {"
        "    width: 80%;"
        "    max-width: 300px;"
        "    height: 1px;"
        "    background-color: white;"
        "    margin: 10px 0;"
        "}"
        "</style>"
        "</head>"
        "<body>"
        "<h1>AIROWL Configuration</h1>"
        "<br>"
        "<h2>AirOwl_009D1C</h2>"
        "<a href='/wifi_form' class='button'>Configure WiFi</a>"
        "<a href='/setup_clock' class='button'>Setup Clock</a>"
        "<a href='#' class='button'>Info</a>"
        "<div class='divider'></div>"
        "<a href='#' class='button'>Restart</a>"
        "<a href='#' class='button'>Exit</a>"
        "<div class='status'>No AP set</div>"
        "</body>"
        "</html>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t wifi_form_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    const char *resp_str = 
        "<html>"
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

static esp_err_t setup_clock_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling GET request for URI: %s", req->uri);
    const char *resp_str = 
        "<html>"
        "<head>"
        "<title>Setup Clock</title>"
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
        "    align-items: center;"
        "    gap: 10px;"
        "}"
        "input[type='number'] {"
        "    padding: 10px;"
        "    width: 60px;"
        "    border-radius: 5px;"
        "    border: none;"
        "    font-size: 1em;"
        "    text-align: center;"
        "}"
        "select {"
        "    padding: 10px;"
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
        "label {"
        "    font-size: 1em;"
        "    margin-right: 5px;"
        "}"
        "</style>"
        "</head>"
        "<body>"
        "<h1>Setup Clock</h1>"
        "<form method='post' action='/set_clock'>"
        "<label for='hour'>Hour:</label>"
        "<input type='number' name='hour' min='1' max='12' required>"
        "<label for='minute'>Min:</label>"
        "<input type='number' name='minute' min='0' max='59' required>"
        "<label for='second'>Sec:</label>"
        "<input type='number' name='second' min='0' max='59' required>"
        "<select name='ampm' required>"
        "<option value='AM'>AM</option>"
        "<option value='PM'>PM</option>"
        "</select>"
        "<input type='submit' value='Set Time'>"
        "</form>"
        "</body>"
        "</html>";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static esp_err_t setup_clock_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling POST request for URI: %s, content length: %d", req->uri, req->content_len);
    char buf[256];
    int ret, remaining = req->content_len;
    char hour_str[3] = {0}, minute_str[3] = {0}, second_str[3] = {0}, ampm_str[3] = {0};
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

        sscanf(buf, "hour=%2[^&]&minute=%2[^&]&second=%2[^&]&ampm=%2s",
               hour_str, minute_str, second_str, ampm_str);
    }

    // Parse input and validate
    int hour = atoi(hour_str);
    int minute = atoi(minute_str);
    int second = atoi(second_str);

    // Validate time
    if (hour < 1 || hour > 12 ||
        minute < 0 || minute > 59 ||
        second < 0 || second > 59 ||
        (strcmp(ampm_str, "AM") != 0 && strcmp(ampm_str, "PM") != 0)) {
        valid_input = false;
    }

    if (valid_input) {
        // Convert to 24-hour format
        if (strcmp(ampm_str, "PM") == 0 && hour != 12) {
            hour += 12;
        } else if (strcmp(ampm_str, "AM") == 0 && hour == 12) {
            hour = 0;
        }

        // Get current date to preserve it
        time_t now = time(NULL);
        struct tm *current_time = localtime(&now);
        struct tm timeinfo = *current_time; // Copy current date
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = second;

        // Convert to time_t and set system time
        time_t rawtime = mktime(&timeinfo);
        if (rawtime == -1) {
            ESP_LOGE(TAG, "Failed to convert time to time_t");
            httpd_resp_send(req, "Invalid time format", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }

        struct timeval tv = {
            .tv_sec = rawtime,
            .tv_usec = 0
        };
        if (settimeofday(&tv, NULL) != 0) {
            ESP_LOGE(TAG, "Failed to set system time");
            httpd_resp_send(req, "Failed to set system time", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }

        // Log the set time
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %I:%M:%S %p", &timeinfo);
        ESP_LOGI(TAG, "System time set to: %s", time_str);
        httpd_resp_send(req, "Time set successfully. Redirecting...", HTTPD_RESP_USE_STRLEN);
    } else {
        ESP_LOGW(TAG, "Invalid time input received");
        httpd_resp_send(req, "Invalid time input", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    // Redirect back to the main page
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/set_wifi");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling POST request for URI: %s, content length: %d", req->uri, req->content_len);
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
        char *pass_start = strstr(buf, "pass=");
        if (ssid_start && pass_start) {
            sscanf(ssid_start, "ssid=%32[^&]", ssid);
            sscanf(pass_start, "pass=%64s", pass);
        } else {
            ESP_LOGW(TAG, "Invalid form data received");
        }
    }

    if (strlen(ssid) > 0 && strlen(pass) > 0) {
        ESP_LOGI(TAG, "Received SSID: %s, Password: %s", ssid, pass);
        save_wifi_credentials(ssid, pass);
        httpd_resp_send(req, "WiFi credentials saved. Rebooting...", HTTPD_RESP_USE_STRLEN);

        esp_wifi_stop();
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        ESP_LOGW(TAG, "Invalid credentials received");
        httpd_resp_send(req, "Invalid credentials", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling GET request for root URI: %s", req->uri);
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/set_wifi");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t config_get = {
            .uri = "/set_wifi",
            .method = HTTP_GET,
            .handler = wifi_config_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &config_get);

        httpd_uri_t wifi_form = {
            .uri = "/wifi_form",
            .method = HTTP_GET,
            .handler = wifi_form_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &wifi_form);

        httpd_uri_t config_post = {
            .uri = "/set_wifi",
            .method = HTTP_POST,
            .handler = wifi_config_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &config_post);

        httpd_uri_t setup_clock_get = {
            .uri = "/setup_clock",
            .method = HTTP_GET,
            .handler = setup_clock_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &setup_clock_get);

        httpd_uri_t setup_clock_post = {
            .uri = "/set_clock",
            .method = HTTP_POST,
            .handler = setup_clock_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &setup_clock_post);

        httpd_uri_t root_get = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_get);
    }
    ESP_LOGI(TAG, "Webserver started");
}

static void stop_webserver(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, 
                         int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG, "Failed to connect, starting AP mode");
            esp_wifi_stop();
            wifi_config_t wifi_config = {
                .ap = {
                    .ssid = AP_SSID,
                    .password = AP_PASS,
                    .max_connection = 4,
                    .authmode = WIFI_AUTH_WPA2_PSK,
                },
            };
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_start());
            start_webserver();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        stop_webserver();
    }
}

void wifi_init(void) {
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

    char ssid[32] = {0};
    char pass[64] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);

    if (load_wifi_credentials(ssid, &ssid_len, pass, &pass_len) == ESP_OK) {
        ESP_LOGI(TAG, "Loaded saved credentials, SSID: %s", ssid);
        wifi_config_t wifi_config = {
            .sta = {
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            },
        };
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
    } else {
        ESP_LOGI(TAG, "No saved credentials, starting AP mode");
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = AP_SSID,
                .password = AP_PASS,
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        start_webserver();
    }
}