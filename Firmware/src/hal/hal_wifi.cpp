#include "hal_wifi.h"
#include "config.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiManagerTz.h>

namespace {
    HAL::WiFi::Status currentStatus = HAL::WiFi::Status::IDLE;
    ::WiFiManager wm;

}

namespace HAL {

bool WiFi::init() {
    ::WiFi.persistent(true);
    ::WiFi.setSleep(false);
    ::WiFi.mode(WIFI_STA);
    ::WiFi.setAutoReconnect(true);
    
    currentStatus = Status::IDLE;
    return true;
}

bool WiFi::connect(const char* ssid, const char* password) {
    if (!ssid || !password || !ssid[0]) {
        Serial.println("[HAL][WiFi] Missing SSID/pass, cannot connect");
        currentStatus = Status::FAILED;
        return false;
    }
    ::WiFi.disconnect(true, true);
    delay(200);

    Serial.printf("[HAL][WiFi] Connecting to SSID: %s\n", ssid);
    ::WiFi.begin(ssid, password);
    currentStatus = Status::CONNECTING;

    unsigned long startTime = millis();
    while (::WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (::WiFi.status() == WL_CONNECTED) {
        Serial.printf("[HAL] Connected! SSID: %s, IP: %s\n",
                      ::WiFi.SSID().c_str(),
                      ::WiFi.localIP().toString().c_str());
        currentStatus = Status::CONNECTED;
        return true;
    } else {
        Serial.printf("[HAL] Connection failed, status: %d\n", ::WiFi.status());
        currentStatus = Status::FAILED;
        return false;
    }
}

String WiFi::generateApName(const char* baseName) {
    
    if (::WiFi.getMode() == WIFI_MODE_NULL) {
        ::WiFi.mode(WIFI_STA);
        delay(100); 
    }

    String mac = ::WiFi.macAddress();
    mac.replace(":", "");   
    String suffix = mac.substring(6); 
    String fullApName = String(baseName) + "_" + suffix;
    // Serial.printf("[HAL][WiFi] Generated AP Name: %s\n", fullApName.c_str());
    return fullApName;
}

bool WiFi::startConfigPortal(const char* apName, uint32_t timeout_ms) {

    String fullApName = generateApName(apName ? apName : "AIROWL");
    Serial.printf("[HAL][WiFi] Starting Config Portal as: %s\n", fullApName.c_str());
    
    WiFiManagerNS::init(&wm, nullptr);
    // Configure WiFiManager
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
   
    switch (::WiFi.status()) {
        case WL_CONNECTED: return Status::CONNECTED;
        case WL_DISCONNECTED: return Status::DISCONNECTED;
        case WL_IDLE_STATUS: return Status::IDLE;
        case WL_CONNECT_FAILED: return Status::FAILED;
        default: return Status::DISCONNECTED;
    }
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
    return ::WiFi.macAddress(mac); 
}


} // namespace HAL