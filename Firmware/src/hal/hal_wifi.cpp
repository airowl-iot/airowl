#include "hal_wifi.h"
#include "airowl_config.h"
#include "esp_system.h"
#include "esp_mac.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiManagerTz.h>
#include <esp_wifi.h>

namespace
{
    HAL::WiFi::Status currentStatus = HAL::WiFi::Status::IDLE;
    ::WiFiManager wm;
    bool wifiEventsRegistered = false;

    // -------------------- Provisioning helpers --------------------

    int selectProvisioningChannel()
    {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);

        // Only non-overlapping 2.4 GHz channels
        const int channels[] = {1, 6, 11};
        return channels[mac[5] % 3];
    }

    void startProvisioningSoftAP(const String &ssid)
    {
        int channel = selectProvisioningChannel();

        // Stagger AP start to avoid beacon storm
        delay(esp_random() % 1500);

        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(ssid.c_str(), "12345678", channel, false, 1);

        // Reduce RF congestion
        esp_wifi_set_max_tx_power(40);

        Serial.printf(
            "[HAL][WiFi] SoftAP started: %s (channel %d)\n",
            ssid.c_str(), channel);
    }

    // -------------------- WiFi events --------------------

    void onWiFiEvent(WiFiEvent_t event)
    {
        switch (event)
        {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf(
                "[HAL][WiFi] Event: Connected! IP: %s\n",
                ::WiFi.localIP().toString().c_str());
            currentStatus = HAL::WiFi::Status::CONNECTED;
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("[HAL][WiFi] Event: Disconnected");
            if (currentStatus == HAL::WiFi::Status::CONNECTED)
            {
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
} // anonymous namespace

// =================================================================

namespace HAL
{

    bool WiFi::init()
    {
        if (!wifiEventsRegistered)
        {
            ::WiFi.onEvent(onWiFiEvent);
            wifiEventsRegistered = true;
            Serial.println("[HAL][WiFi] WiFi event handlers registered");
        }

        ::WiFi.persistent(true);
        ::WiFi.setSleep(false);

        if (!::WiFi.mode(WIFI_STA))
        {
            Serial.println("[HAL][WiFi] ERROR: Failed to set WiFi mode to STA");
            return false;
        }

        ::WiFi.setAutoReconnect(true);
        currentStatus = Status::IDLE;

        Serial.println("[HAL][WiFi] WiFi HAL initialized");
        return true;
    }

    bool WiFi::connect(const char *ssid, const char *password)
    {
        if (!ssid || !ssid[0])
        {
            Serial.println("[HAL][WiFi] Missing SSID, cannot connect");
            currentStatus = Status::FAILED;
            return false;
        }

        if (!password)
            password = "";

        ::WiFi.disconnect(false, false);
        vTaskDelay(pdMS_TO_TICKS(200));

        Serial.printf("[HAL][WiFi] Connecting to SSID: %s\n", ssid);
        ::WiFi.begin(ssid, password);
        currentStatus = Status::CONNECTING;

        unsigned long startTime = millis();
        uint8_t retryCount = 0;
        const uint8_t maxRetries = 3;

        while (::WiFi.status() != WL_CONNECTED && millis() - startTime < 30000)
        {
            vTaskDelay(pdMS_TO_TICKS(500));
            Serial.print(".");

            if (::WiFi.status() == WL_CONNECT_FAILED && retryCount < maxRetries)
            {
                retryCount++;
                Serial.printf(
                    "\n[HAL][WiFi] Connection failed, retry %d/%d\n",
                    retryCount, maxRetries);
                ::WiFi.disconnect(false, false);
                vTaskDelay(pdMS_TO_TICKS(1000));
                ::WiFi.begin(ssid, password);
                startTime = millis();
            }
        }
        Serial.println();

        if (::WiFi.status() == WL_CONNECTED)
        {
            Serial.printf(
                "[HAL][WiFi] Connected! SSID: %s, IP: %s, RSSI: %d dBm\n",
                ::WiFi.SSID().c_str(),
                ::WiFi.localIP().toString().c_str(),
                ::WiFi.RSSI());
            currentStatus = Status::CONNECTED;
            return true;
        }

        Serial.printf(
            "[HAL][WiFi] Connection failed, status: %d - ",
            ::WiFi.status());
        printWiFiStatus(::WiFi.status());
        currentStatus = Status::FAILED;
        return false;
    }

    String WiFi::generateApName(const char *baseName)
    {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);

        char deviceId[13];
        sprintf(
            deviceId,
            "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        String suffix = String(deviceId).substring(6);
        return String(baseName ? baseName : "AIROWL") + "_" + suffix;
    }

    // -------------------- Provisioning --------------------

    bool WiFi::autoConnect(const char *apName, uint32_t timeout_ms)
    {
        String fullApName = generateApName(apName ? apName : "AIROWL");
        Serial.printf(
            "[HAL][WiFi] Starting autoConnect as: %s\n",
            fullApName.c_str());

        startProvisioningSoftAP(fullApName);

        WiFiManagerNS::init(&wm, nullptr);
        wm.setConfigPortalBlocking(true);
        wm.setConfigPortalTimeout(timeout_ms / 1000);
        wm.setConnectTimeout(30);
        wm.setConnectRetries(3);
        wm.setEnableConfigPortal(true);
        wm.setDebugOutput(true);

        bool connected = wm.autoConnect(fullApName.c_str(), "12345678");

        currentStatus = connected ? Status::CONNECTED : Status::FAILED;
        return connected;
    }

    bool WiFi::startConfigPortal(const char *apName, uint32_t timeout_ms)
    {
        String fullApName = generateApName(apName ? apName : "AIROWL");
        Serial.printf(
            "[HAL][WiFi] Starting Config Portal as: %s\n",
            fullApName.c_str());

        startProvisioningSoftAP(fullApName);

        WiFiManagerNS::init(&wm, nullptr);
        wm.setConfigPortalBlocking(true);
        wm.setConfigPortalTimeout(timeout_ms / 1000);
        wm.setConnectTimeout(30);
        wm.setConnectRetries(3);
        wm.setEnableConfigPortal(true);
        wm.setDebugOutput(true);
        wm.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

        bool connected = wm.startConfigPortal(fullApName.c_str(), "12345678");

        currentStatus = connected ? Status::CONNECTED : Status::FAILED;
        return connected;
    }

    // -------------------- Utilities --------------------

    bool WiFi::disconnect()
    {
        ::WiFi.disconnect(false, false);
        currentStatus = Status::DISCONNECTED;
        return true;
    }

    WiFi::Status WiFi::getStatus()
    {
        return currentStatus;
    }

    void WiFi::printWiFiStatus(uint8_t status)
    {
        switch (status)
        {
        case WL_IDLE_STATUS:
            Serial.println("Idle");
            break;
        case WL_NO_SSID_AVAIL:
            Serial.println("No SSID available");
            break;
        case WL_CONNECTED:
            Serial.println("Connected");
            break;
        case WL_CONNECT_FAILED:
            Serial.println("Connect failed");
            break;
        case WL_DISCONNECTED:
            Serial.println("Disconnected");
            break;
        default:
            Serial.println("Unknown");
            break;
        }
    }

    bool WiFi::getConnectionInfo(ConnectionInfo *info)
    {
        if (!info || ::WiFi.status() != WL_CONNECTED)
            return false;

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

    int32_t WiFi::getRSSI() { return ::WiFi.RSSI(); }

    bool WiFi::setHostname(const char *hostname)
    {
        return ::WiFi.setHostname(hostname);
    }

    bool WiFi::getMACAddress(uint8_t *mac)
    {
        if (!mac)
            return false;
        return ::WiFi.macAddress(mac);
    }

} // namespace HAL
