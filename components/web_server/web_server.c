#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "web_server";

/* 
 * Note: The wifi_manager_connect() function is declared in wifi_manager.h
 * and implemented in the wifi_manager component.
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char resp[] =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>Network Connection</title></head>"
        "<body>"
        "<h1>Enter Network Credentials</h1>"
        "<form action=\"/save\" method=\"post\">"
        "SSID: <input type=\"text\" name=\"ssid\"><br>"
        "Password: <input type=\"password\" name=\"password\"><br>"
        "<input type=\"submit\" value=\"Connect\">"
        "</form>"
        "</body>"
        "</html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t save_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = malloc(total_len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }
    memset(buf, 0, total_len + 1);

    while (cur_len < total_len) {
        int received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive post data");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    ESP_LOGI(TAG, "POST Data: %s", buf);

    char ssid[33] = {0};
    char password[65] = {0};

    httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(buf, "password", password, sizeof(password));

    ESP_LOGI(TAG, "Received SSID: %s, Password: %s", ssid, password);
    free(buf);

    esp_err_t ret = wifi_manager_connect(ssid, password);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to reconfigure WiFi");
        return ret;
    }

    const char resp[] = "Credentials received. Attempting to connect...";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_uri_t root = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = root_get_handler,
    .user_ctx = NULL
};

static httpd_uri_t save = {
    .uri      = "/save",
    .method   = HTTP_POST,
    .handler  = save_post_handler,
    .user_ctx = NULL
};

esp_err_t start_web_server(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Error starting server!");
        return ESP_FAIL;
    }

    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &save);
    return ESP_OK;
}
