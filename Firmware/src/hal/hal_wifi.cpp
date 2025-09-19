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

}

namespace HAL {

bool WiFi::init() {
    ::WiFi.disconnect(true, false);
    vTaskDelay(pdMS_TO_TICKS(100));

    ::WiFi.persistent(true);
    ::WiFi.setSleep(false);
    ::WiFi.mode(WIFI_STA);
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
    ::WiFi.disconnect(true, true);
    vTaskDelay(pdMS_TO_TICKS(200));

    Serial.printf("[HAL][WiFi] Connecting to SSID: %s\n", ssid);
    ::WiFi.begin(ssid, password);
    currentStatus = Status::CONNECTING;

    unsigned long startTime = millis();
    while (::WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println();

    if (::WiFi.status() == WL_CONNECTED) {
        Serial.printf("[HAL][WiFi] Connected! SSID: %s, IP: %s\n",
                      ::WiFi.SSID().c_str(),
                      ::WiFi.localIP().toString().c_str());
        currentStatus = Status::CONNECTED;
        return true;
    } else {
        Serial.printf("[HAL][WiFi] Connection failed, status: %d\n", ::WiFi.status());
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

bool WiFi::startConfigPortal(const char* apName, uint32_t timeout_ms) {

    String fullApName = generateApName(apName ? apName : "AIROWL");
    Serial.printf("[HAL][WiFi] Starting Config Portal as: %s\n", fullApName.c_str());
    
    WiFiManagerNS::init(&wm, nullptr);
    std::vector<const char *> menu = {"wifi", "info", "custom", "param", "sep", "restart", "exit"};
    wm.setMenu(menu);
    wm.setTitle("AIROWL Configuration");
    wm.setConfigPortalBlocking(true);
    wm.setConfigPortalTimeout(timeout_ms / 1000);
    wm.setConnectTimeout(60);
    wm.setDebugOutput(true);

    bool connected = wm.startConfigPortal(fullApName.c_str(), "12345678");
   
    if (connected) {
        Serial.println("[HAL][WiFi] User configured WiFi, saving credentials...");
        currentStatus = Status::CONNECTED;
    } else {
        Serial.println("[HAL][WiFi] Config portal timeout or failed");
        currentStatus = Status::FAILED;
    }
    return connected;
}

bool WiFi::disconnect() {
    ::WiFi.disconnect(true, true);
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