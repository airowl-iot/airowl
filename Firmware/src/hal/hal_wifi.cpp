#include "hal_wifi.h"
#include "airowl_config.h"
#include "esp_system.h"
#include "esp_mac.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiManagerTz.h>

namespace {
    HAL::WiFi::Status currentStatus = HAL::WiFi::Status::IDLE;
    ::WiFiManager wm;
    bool wifiEventsRegistered = false;

    void onWiFiEvent(WiFiEvent_t event) {
        switch(event) {
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                Serial.printf("[HAL][WiFi] Event: Connected! IP: %s\n", ::WiFi.localIP().toString().c_str());
                currentStatus = HAL::WiFi::Status::CONNECTED;
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                Serial.println("[HAL][WiFi] Event: Disconnected");
                if (currentStatus == HAL::WiFi::Status::CONNECTED) {
                    currentStatus = HAL::WiFi::Status::DISCONNECTED;
                }
                break;
            case ARDUINO_EVENT_WIFI_STA_START:
                Serial.println("[HAL][WiFi] Event: Station started");
                break;
            case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                Serial.println("[HAL][WiFi] Event: Connected to AP");
                break;
            case ARDUINO_EVENT_WIFI_STA_LOST_IP:
                Serial.println("[HAL][WiFi] Event: Lost IP");
                break;
            default:
                break;
        }
    }
}

namespace HAL {

bool WiFi::init() {
    if (!wifiEventsRegistered) {
        ::WiFi.onEvent(onWiFiEvent);
        wifiEventsRegistered = true;
        Serial.println("[HAL][WiFi] WiFi event handlers registered");
    }

    ::WiFi.persistent(true);
    ::WiFi.setSleep(false);

    if (!::WiFi.mode(WIFI_STA)) {
        Serial.println("[HAL][WiFi] ERROR: Failed to set WiFi mode to STA");
        return false;
    }
    Serial.println("[HAL][WiFi] WiFi mode set to STA successfully");

    ::WiFi.setAutoReconnect(true);

    currentStatus = Status::IDLE;
    Serial.println("[HAL][WiFi] WiFi HAL initialized");
    return true;
}

bool WiFi::connect(const char* ssid, const char* password) {
    if (!ssid || !ssid[0]) {
        Serial.println("[HAL][WiFi] Missing SSID, cannot connect");
        currentStatus = Status::FAILED;
        return false;
    }

    if (!password) {
        password = "";
    }
    ::WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(200));

    Serial.printf("[HAL][WiFi] Connecting to SSID: %s\n", ssid);
    ::WiFi.begin(ssid, password);
    currentStatus = Status::CONNECTING;

    unsigned long startTime = millis();
    uint8_t retryCount = 0;
    const uint8_t maxRetries = 3;

    while (::WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");

        if (::WiFi.status() == WL_CONNECT_FAILED && retryCount < maxRetries) {
            retryCount++;
            Serial.printf("\n[HAL][WiFi] Connection attempt failed, retry %d/%d\n", retryCount, maxRetries);
            ::WiFi.disconnect(false, false);
            vTaskDelay(pdMS_TO_TICKS(1000));
            ::WiFi.begin(ssid, password);
            startTime = millis(); 
        }
    }
    Serial.println();

    if (::WiFi.status() == WL_CONNECTED) {
        Serial.printf("[HAL][WiFi] Connected! SSID: %s, IP: %s, RSSI: %d dBm\n",
                      ::WiFi.SSID().c_str(),
                      ::WiFi.localIP().toString().c_str(),
                      ::WiFi.RSSI());
        currentStatus = Status::CONNECTED;
        return true;
    } else {
        Serial.printf("[HAL][WiFi] Connection failed after %d retries, status: %d - ", retryCount, ::WiFi.status());
        printWiFiStatus(::WiFi.status());
        currentStatus = Status::FAILED;
        return false;
    }
}

String WiFi::generateApName(const char* baseName) {

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char deviceId[13];
    sprintf(deviceId, "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    String suffix = String(deviceId).substring(6);
    String fullApName = String(baseName ? baseName : "AIROWL") + "_" + suffix;
    return fullApName;
}

bool WiFi::autoConnect(const char* apName, uint32_t timeout_ms) {

    String fullApName = generateApName(apName ? apName : "AIROWL");
    Serial.printf("[HAL][WiFi] Starting autoConnect as: %s (Password: 12345678)\n", fullApName.c_str());

    WiFiManagerNS::init(&wm, nullptr);
    std::vector<const char *> menu = {"wifi", "info", "custom", "param", "sep", "restart", "exit"};
    wm.setMenu(menu);
    wm.setTitle("AIROWL Configuration");
    wm.setClass("invert");
    wm.setConfigPortalBlocking(true);
    wm.setConfigPortalTimeout(timeout_ms / 1000);
    wm.setConnectTimeout(30);
    wm.setConnectRetries(3);
    wm.setDebugOutput(true);

    bool connected = wm.autoConnect(fullApName.c_str(), "12345678");

    if (connected) {
        ::WiFi.persistent(true);
        Serial.printf("[HAL][WiFi] Connected successfully! SSID: %s\n", ::WiFi.SSID().c_str());
        Serial.println("[HAL][WiFi] WiFi credentials saved in NVS");
        currentStatus = Status::CONNECTED;
    } else {
        Serial.println("[HAL][WiFi] autoConnect failed or timed out");
        currentStatus = Status::FAILED;
    }
    return connected;
}

bool WiFi::startConfigPortal(const char* apName, uint32_t timeout_ms) {

    String fullApName = generateApName(apName ? apName : "AIROWL");
    Serial.printf("[HAL][WiFi] Starting Config Portal as: %s (Password: 12345678)\n", fullApName.c_str());

    WiFiManagerNS::init(&wm, nullptr);
    std::vector<const char *> menu = {"wifi", "info", "custom", "param", "sep", "restart", "exit"};
    wm.setMenu(menu);
    wm.setTitle("AIROWL Configuration");
    wm.setClass("invert");
    wm.setConfigPortalBlocking(true);
    wm.setConfigPortalTimeout(timeout_ms / 1000);
    wm.setConnectTimeout(30);
    wm.setConnectRetries(3);
    wm.setDebugOutput(true);

    bool connected = wm.startConfigPortal(fullApName.c_str(), "12345678");

    if (connected) {
        ::WiFi.persistent(true);
        Serial.println("[HAL][WiFi] Re-enabled WiFi persistence after config portal");
        Serial.printf("[HAL][WiFi] User configured WiFi successfully! SSID: %s\n", ::WiFi.SSID().c_str());
        Serial.println("[HAL][WiFi] WiFi credentials saved in NVS by WiFiManager");
        currentStatus = Status::CONNECTED;
    } else {
        Serial.println("[HAL][WiFi] Config portal timeout or failed");
        currentStatus = Status::FAILED;
    }
    return connected;
}

bool WiFi::disconnect() {
    ::WiFi.disconnect(false, false);
    currentStatus = Status::DISCONNECTED;
    return true;
}

WiFi::Status WiFi::getStatus() {
    wl_status_t wifiStatus = ::WiFi.status();

    switch (wifiStatus) {
        case WL_CONNECTED:
            if (currentStatus != Status::CONNECTED) {
                currentStatus = Status::CONNECTED;
            }
            break;
        case WL_DISCONNECTED:
            if (currentStatus == Status::CONNECTED || currentStatus == Status::CONNECTING) {
                currentStatus = Status::DISCONNECTED;
            }
            break;
        case WL_CONNECT_FAILED:
            currentStatus = Status::FAILED;
            break;
        case WL_IDLE_STATUS:
            if (currentStatus != Status::CONNECTING && currentStatus != Status::CONNECTED) {
                currentStatus = Status::IDLE;
            }
            break;
        default:
            break;
    }

    return currentStatus;
}

void WiFi::printWiFiStatus(uint8_t status) {
    switch (status) {
        case WL_IDLE_STATUS: Serial.println("Idle"); break;
        case WL_NO_SSID_AVAIL: Serial.println("No SSID available"); break;
        case WL_CONNECTED: Serial.println("Connected"); break;
        case WL_CONNECT_FAILED: Serial.println("Connect failed"); break;
        case WL_DISCONNECTED: Serial.println("Disconnected"); break;
        default: Serial.println("Unknown"); break;
    }
}

bool WiFi::getConnectionInfo(ConnectionInfo* info) {
    if (!info) {
        Serial.println("[HAL][WiFi] Error: Null pointer provided to getConnectionInfo");
        return false;
    }

    if (::WiFi.status() != WL_CONNECTED) {
        return false;
    }
    
    info->ssid = ::WiFi.SSID();
    info->rssi = ::WiFi.RSSI();
    info->ip = ::WiFi.localIP();
    info->gateway = ::WiFi.gatewayIP();
    info->subnet = ::WiFi.subnetMask();
    info->dns1 = ::WiFi.dnsIP(0);
    info->dns2 = ::WiFi.dnsIP(1);
    ::WiFi.BSSID(info->bssid);
    info->channel = ::WiFi.channel();

    return true;
}

int32_t WiFi::getRSSI() {
    return ::WiFi.RSSI();
}

bool WiFi::setHostname(const char* hostname) {
    return ::WiFi.setHostname(hostname);
}

bool WiFi::getMACAddress(uint8_t* mac) {
    if (!mac) {
        Serial.println("[HAL][WiFi] Error: Null pointer provided to getMACAddress");
        return false;
    }
    return ::WiFi.macAddress(mac);
}


} // namespace HAL