#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <cJSON.h>
#include "web_server.h"
#include "network_manager.h"

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

// HTML for the configuration page
static const char *config_html = "<!DOCTYPE html>"
    "<html>"
    "<head>"
        "<title>AirOwl Configuration</title>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<style>"
            "body { font-family: Arial; margin: 0; padding: 20px; }"
            ".container { max-width: 400px; margin: 0 auto; }"
            "input[type='text'], input[type='password'] { width: 100%; padding: 12px; margin: 8px 0; }"
            "button { background-color: #4CAF50; color: white; padding: 14px 20px; border: none; width: 100%; }"
            ".status { margin-top: 20px; padding: 10px; background-color: #f1f1f1; }"
        "</style>"
    "</head>"
    "<body>"
        "<div class='container'>"
            "<h2>AirOwl WiFi Setup</h2>"
            "<form id='wifiForm'>"
                "<label>WiFi SSID:</label><br>"
                "<input type='text' id='ssid' name='ssid' required><br>"
                "<label>Password:</label><br>"
                "<input type='password' id='password' name='password'><br><br>"
                "<button type='submit'>Connect</button>"
            "</form>"
            "<div id='status' class='status'></div>"
        "</div>"
        "<script>"
            "document.getElementById('wifiForm').onsubmit = function(e) {"
                "e.preventDefault();"
                "var data = {"
                    "ssid: document.getElementById('ssid').value,"
                    "password: document.getElementById('password').value"
                "};"
                "document.getElementById('status').innerHTML = 'Connecting...';"
                "fetch('/connect', {"
                    "method: 'POST',"
                    "headers: {'Content-Type': 'application/json'},"
                    "body: JSON.stringify(data)"
                "})"
                ".then(response => response.json())"
                ".then(data => {"
                    "document.getElementById('status').innerHTML = data.message;"
                    "if(data.success) {"
                        "setTimeout(() => {"
                            "document.getElementById('status').innerHTML += '<br>Device will restart in 5 seconds...';"
                        "}, 2000);"
                    "}"
                "})"
                ".catch(error => {"
                    "document.getElementById('status').innerHTML = 'Error: ' + error;"
                "});"
                "return false;"
            "};"
        "</script>"
    "</body>"
    "</html>";

// Handler for root path
static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, config_html, strlen(config_html));
    return ESP_OK;
}

// Handler for WiFi connection request
static esp_err_t connect_handler(httpd_req_t *req)
{
    char buf[1024];
    int ret, remaining = req->content_len;
    
    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
        return ESP_FAIL;
    }
    
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive content");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *password = cJSON_GetObjectItem(root, "password");

    if (!ssid || !cJSON_IsString(ssid)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID is required");
        return ESP_FAIL;
    }

    esp_err_t err = network_manager_connect_sta(ssid->valuestring, 
                                              password ? password->valuestring : NULL);

    char *response;
    if (err == ESP_OK) {
        response = "{\"success\":true,\"message\":\"Successfully connected to WiFi\"}";
    } else {
        response = "{\"success\":false,\"message\":\"Failed to connect to WiFi\"}";
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));

    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t web_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;

    if (httpd_start(&server, &config) == ESP_OK) {
        // URI handler for root path
        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_uri);

        // URI handler for connect endpoint
        httpd_uri_t connect_uri = {
            .uri = "/connect",
            .method = HTTP_POST,
            .handler = connect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &connect_uri);

        ESP_LOGI(TAG, "Web server started");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to start web server");
    return ESP_FAIL;
}

esp_err_t web_server_start(void)
{
    if (server == NULL) {
        return web_server_init();
    }
    return ESP_OK;
}

esp_err_t web_server_stop(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    return ESP_OK;
} 