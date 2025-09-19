#include "wifi_service.h"
#include "hal/hal_wifi.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_task_wdt.h>
#include "../manager/event_manager.h"

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

            CORE::WiFiStateChangedEvent::WiFiState eventState;
            switch(newState) {
                case SVC::WiFiService::State::DISCONNECTED:
                    eventState = CORE::WiFiStateChangedEvent::WiFiState::DISCONNECTED;
                    break;
                case SVC::WiFiService::State::CONNECTING:
                    eventState = CORE::WiFiStateChangedEvent::WiFiState::CONNECTING;
                    break;
                case SVC::WiFiService::State::CONNECTED:
                    eventState = CORE::WiFiStateChangedEvent::WiFiState::CONNECTED;
                    break;
                case SVC::WiFiService::State::FAILED:
                    eventState = CORE::WiFiStateChangedEvent::WiFiState::FAILED;
                    break;
                case SVC::WiFiService::State::PROVISIONING:
                    eventState = CORE::WiFiStateChangedEvent::WiFiState::PROVISIONING;
                    break;
                default:
                    return;
            }

            String ssid = (newState == SVC::WiFiService::State::CONNECTED) ? ::WiFi.SSID() : "";
            int32_t rssi = (newState == SVC::WiFiService::State::CONNECTED) ? ::WiFi.RSSI() : 0;

            auto event = std::make_shared<CORE::WiFiStateChangedEvent>(eventState, ssid, rssi);
            CORE::EventBus::getInstance().publish(event);
            Serial.printf("[WiFiService] Published WiFi state change event: %d\n", (int)eventState);
        }
    }
}

namespace SVC {

bool WiFiService::init(const char* hostname) {
    if (initialized) return true;

    if (!HAL::WiFi::init()) {
        Serial.println("[SVC][WiFiService] Failed to initialize WiFi HAL");
        return false;
    }

    if (hostname && !HAL::WiFi::setHostname(hostname)) {
        Serial.println("[SVC][WiFiService] Warning: Failed to set hostname");
    }

    initialized = true;
    updateState(State::DISCONNECTED);
    Serial.printf("[SVC][WiFiService] Initialized with hostname: %s\n", hostname ? hostname : "default");
    return true;
}

bool WiFiService::connect(const char* ssid, const char* password) {
    if (!initialized) {
        Serial.println("[SVC][WiFiService] Not initialized, initializing now...");
        if (!init()) {
            Serial.println("[SVC][WiFiService] Failed to initialize");
            return false;
        }
    }

    if (ssid && password) {
        currentSSID = ssid;
        currentPassword = password;
        Serial.printf("[SVC][WiFiService] Connecting to WiFi SSID: %s\n", currentSSID.c_str());
    }

    if (!currentSSID.isEmpty() && !currentPassword.isEmpty()) {
        HAL::WiFi::disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
        updateState(State::CONNECTING);

        bool result = HAL::WiFi::connect(currentSSID.c_str(), currentPassword.c_str());
        if (result) {
            updateState(State::CONNECTED);
            Serial.println("[SVC][WiFiService] Successfully connected to WiFi");
        } else {
            updateState(State::FAILED);
            Serial.println("[SVC][WiFiService] Failed to connect to WiFi");
        }
        return result;
    }

    Serial.println("[SVC][WiFiService] No credentials provided, starting provisioning");
    return startProvisioning();
}

bool WiFiService::startProvisioning(const char* apName, uint32_t timeout_ms) {
    if (!initialized) {
        Serial.println("[SVC][WiFiService] Not initialized, initializing now...");
        if (!init()) {
            Serial.println("[SVC][WiFiService] Failed to initialize");
            return false;
        }
    }

    String baseApName = apName ? apName : "AIROWL";
    Serial.printf("[SVC][WiFiService] Starting provisioning with AP name: %s\n", baseApName.c_str());

    updateState(State::PROVISIONING);

    if (HAL::WiFi::startConfigPortal(baseApName.c_str(), timeout_ms)) {
        currentSSID = ::WiFi.SSID();
        currentPassword = ::WiFi.psk();

        updateState(State::CONNECTED);
        reconnectAttempts = 0;
        Serial.println("[SVC][WiFiService] Provisioning successful, WiFi connected");
        return true;
    } else {
        updateState(State::FAILED);
        Serial.println("[SVC][WiFiService] Provisioning failed or timed out");
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
