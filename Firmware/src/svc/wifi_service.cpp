#include "wifi_service.h"
#include <WiFiManager.h>
#include <esp_task_wdt.h>

namespace {
    bool initialized = false;
    SVC::WiFiService::State currentState = SVC::WiFiService::State::DISCONNECTED;
    SVC::WiFiService::EventCallback eventCallback = nullptr;
    TaskHandle_t wifiTaskHandle = nullptr;

    const uint32_t RECONNECT_INTERVAL_MS = 5000;
    const uint32_t MAX_RECONNECT_ATTEMPTS = 3;
    uint32_t reconnectAttempts = 0;
    unsigned long lastReconnectAttempt = 0;

    String currentSSID;
    String currentPassword;

    void updateState(SVC::WiFiService::State newState, void* data = nullptr) {
        if (currentState != newState) {
            currentState = newState;
            if (eventCallback) eventCallback(newState, data);
        }
    }
}

namespace SVC {

bool WiFiService::init(const char* hostname) {
    if (initialized) return true;
    HAL::WiFi::setHostname(hostname);
    initialized = true;
    updateState(State::DISCONNECTED);
    return true;
}

bool WiFiService::connect(const char* ssid, const char* password) {
    if (!initialized) if (!init()) return false;

    if (ssid && password) {
        currentSSID = ssid;
        currentPassword = password;
        Serial.print("[SVC] Connecting to WiFi SSID: "); Serial.println(currentSSID);
    }

    if (!currentSSID.isEmpty() && !currentPassword.isEmpty()) {
        HAL::WiFi::disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
        updateState(State::CONNECTING);
        return HAL::WiFi::connect(currentSSID.c_str(), currentPassword.c_str());
    }
    return startProvisioning();
}

bool WiFiService::startProvisioning(const char* apName, uint32_t timeout_ms) {
    if (!initialized) if (!init()) return false;

    String baseApName = apName ? apName : "AIROWL";
    Serial.printf("[SVC][WiFiService] Using AP name: %s\n", baseApName);

    if (HAL::WiFi::startConfigPortal(baseApName.c_str(), timeout_ms)) {
        updateState(State::CONNECTED);
        reconnectAttempts = 0;
        return true;
    } else {
        updateState(State::FAILED);
        return false;
    }
}

bool WiFiService::disconnect() {
    if (!initialized) return false;
    if (HAL::WiFi::disconnect()) {
        updateState(State::DISCONNECTED);
        return true;
    }
    return false;
}

WiFiService::State WiFiService::getState() { return currentState; }
HAL::WiFi::ConnectionInfo WiFiService::getConnectionInfo() {
    HAL::WiFi::ConnectionInfo info{};
    HAL::WiFi::getConnectionInfo(&info);
    return info;
}
void WiFiService::onEvent(EventCallback callback) { eventCallback = callback; }

void WiFiService::task(void* parameter) {
    while (true) {
        // esp_task_wdt_reset();
        HAL::WiFi::Status halStatus = HAL::WiFi::getStatus();

        static HAL::WiFi::Status lastLoggedStatus = HAL::WiFi::Status::IDLE;

        if (halStatus == HAL::WiFi::Status::CONNECTED && currentState == State::CONNECTING) {
            updateState(State::CONNECTED);
            reconnectAttempts = 0;
            Serial.printf("[SVC] WiFi connected: SSID=%s, IP=%s\n",
                          ::WiFi.SSID().c_str(),
                          ::WiFi.localIP().toString().c_str());
        } else if (halStatus == HAL::WiFi::Status::FAILED && currentState == State::CONNECTING) {
            updateState(State::FAILED);
            Serial.println("[SVC] WiFi connection failed");
        }

        switch (currentState) {
            case State::CONNECTED:
                if (halStatus != HAL::WiFi::Status::CONNECTED) {
                    updateState(State::DISCONNECTED);
                    reconnectAttempts = 0;
                    lastReconnectAttempt = millis();
                    Serial.println("[SVC] WiFi connection lost, will attempt reconnection");
                }
                break;
            case State::DISCONNECTED:
                if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
                    if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                        reconnectAttempts++;
                        lastReconnectAttempt = millis();
                        Serial.printf("[SVC] Attempting reconnection (attempt %d of %d)\n",
                                      reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
                        connect(currentSSID.c_str(), currentPassword.c_str());
                    } else {
                        Serial.println("[SVC] Max reconnection attempts reached, starting provisioning");
                        startProvisioning();
                        reconnectAttempts = 0;
                    }
                }
                break;
            case State::FAILED:
                if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS * 2) {
                    updateState(State::DISCONNECTED);
                    lastReconnectAttempt = millis();
                    Serial.println("[SVC] Resetting after failure, will try reconnection");
                }
                break;
            default: break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

bool WiFiService::startTask() {
    if (wifiTaskHandle != nullptr) return true;

    BaseType_t result = xTaskCreatePinnedToCore(task, "WiFiService", 4096, NULL, 1, &wifiTaskHandle, 0);
    return (result == pdPASS);
}

bool WiFiService::restartTask() {
    if (wifiTaskHandle != nullptr) {
        vTaskDelete(wifiTaskHandle);
        wifiTaskHandle = nullptr;
    }
    return startTask();
}

} // namespace SVC
