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

    const uint32_t RECONNECT_INTERVAL_MS = 60000;
    const uint32_t RECONNECT_ATTEMPT_TIMEOUT_MS = 15000;
    unsigned long lastReconnectAttempt = 0;
    unsigned long reconnectStartedAt = 0;
    bool reconnectInProgress = false;
    bool connectionRequested = false;
    bool provisioningRequested = false;
    uint32_t provisioningTimeoutMs = 120000;
    String provisioningApName = "AIROWL";

    SemaphoreHandle_t stateMutex = nullptr;

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

bool WiFiService::init(const char* hostname, uint32_t timeoutMs) {
    if (initialized) return true;

    if (stateMutex == nullptr) {
        stateMutex = xSemaphoreCreateMutex();
        if (stateMutex == nullptr) {
            Serial.println("[SVC][WiFiService] ERROR: Failed to create state mutex");
            return false;
        }
    }

    if (!HAL::WiFi::init()) {
        Serial.println("[SVC][WiFiService] Failed to initialize WiFi HAL");
        return false;
    }

    if (hostname && !HAL::WiFi::setHostname(hostname)) {
        Serial.println("[SVC][WiFiService] Warning: Failed to set hostname");
    }

    initialized = true;
    provisioningTimeoutMs = timeoutMs > 0 ? timeoutMs : 120000;
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

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (ssid && password) {
            currentSSID = ssid;
            currentPassword = password;
            Serial.printf("[SVC][WiFiService] Connecting to WiFi SSID: %s\n", currentSSID.c_str());
        }
        connectionRequested = true;
        xSemaphoreGive(stateMutex);
    } else {
        Serial.println("[SVC][WiFiService] Failed to queue WiFi connection request");
        return false;
    }

    updateState(State::CONNECTING);
    Serial.println("[SVC][WiFiService] WiFi connection queued; boot will continue offline");
    return true;
}

bool WiFiService::startProvisioning(const char* apName, uint32_t timeout_ms) {
    if (!initialized) {
        Serial.println("[SVC][WiFiService] Not initialized, initializing now...");
        if (!init()) {
            Serial.println("[SVC][WiFiService] Failed to initialize");
            return false;
        }
    }

    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("[SVC][WiFiService] Failed to queue provisioning request");
        return false;
    }

    provisioningApName = apName ? apName : "AIROWL";
    provisioningTimeoutMs = timeout_ms > 0 ? timeout_ms : 120000;
    provisioningRequested = true;
    xSemaphoreGive(stateMutex);

    updateState(State::PROVISIONING);
    Serial.printf("[SVC][WiFiService] Provisioning queued with AP name: %s\n", provisioningApName.c_str());
    return true;
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
    Serial.println("[WiFiService] Background task started");
    bool portalWasActive = false;

    while (true) {
        bool runConnect = false;
        bool runProvisioning = false;
        String ssid;
        String password;
        String apName;
        uint32_t portalTimeout = 120000;

        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            runConnect = connectionRequested;
            runProvisioning = provisioningRequested;
            connectionRequested = false;
            provisioningRequested = false;
            ssid = currentSSID;
            password = currentPassword;
            apName = provisioningApName;
            portalTimeout = provisioningTimeoutMs;
            xSemaphoreGive(stateMutex);
        }

        if (runProvisioning) {
            updateState(State::PROVISIONING);
            HAL::WiFi::startConfigPortal(apName.c_str(), portalTimeout);
        } else if (runConnect) {
            bool connected = false;
            if (!ssid.isEmpty()) {
                connected = HAL::WiFi::connect(ssid.c_str(), password.c_str());
            } else {
                Serial.println("[SVC][WiFiService] Trying saved WiFi in background; provisioning will remain available");
                connected = HAL::WiFi::autoConnect("AIROWL", portalTimeout);
            }

            if (connected) {
                reconnectInProgress = false;
                updateState(State::CONNECTED);
            } else if (HAL::WiFi::isConfigPortalActive()) {
                updateState(State::PROVISIONING);
            } else {
                updateState(State::DISCONNECTED);
                Serial.println("[SVC][WiFiService] WiFi unavailable; continuing in offline mode");
            }
        }

        if (HAL::WiFi::isConfigPortalActive()) {
            portalWasActive = true;
            if (HAL::WiFi::process()) {
                bool newlyConnected = currentState != State::CONNECTED;
                updateState(State::CONNECTED);
                if (newlyConnected) {
                    Serial.printf("[SVC][WiFiService] Connected to WiFi: %s\n", ::WiFi.SSID().c_str());
                }
            }
        } else if (portalWasActive) {
            portalWasActive = false;
            if (::WiFi.status() != WL_CONNECTED) {
                updateState(State::DISCONNECTED);
                HAL::WiFi::enterOfflineMode();
                lastReconnectAttempt = millis();
                Serial.println("[SVC][WiFiService] Provisioning timed out; continuing in offline mode");
            }
        }

        HAL::WiFi::Status halStatus = HAL::WiFi::getStatus();

        if (halStatus == HAL::WiFi::Status::CONNECTED && currentState != State::CONNECTED) {
            reconnectInProgress = false;
            ::WiFi.setAutoReconnect(true);
            updateState(State::CONNECTED);
            Serial.printf("[SVC] WiFi connected: SSID=%s, IP=%s\n",
                          ::WiFi.SSID().c_str(),
                          ::WiFi.localIP().toString().c_str());
        }

        switch (currentState) {
            case State::CONNECTED:
                if (halStatus != HAL::WiFi::Status::CONNECTED) {
                    HAL::WiFi::enterOfflineMode();
                    updateState(State::DISCONNECTED);
                    lastReconnectAttempt = millis();

                    Serial.println("[SVC] WiFi connection lost; continuing offline and reconnecting in background");
                }
                break;
            case State::DISCONNECTED:
                if (reconnectInProgress &&
                    millis() - reconnectStartedAt >= RECONNECT_ATTEMPT_TIMEOUT_MS) {
                    HAL::WiFi::enterOfflineMode();
                    reconnectInProgress = false;
                    lastReconnectAttempt = millis();
                } else if (!reconnectInProgress &&
                           !HAL::WiFi::isConfigPortalActive() &&
                           millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
                    lastReconnectAttempt = millis();
                    reconnectInProgress = HAL::WiFi::reconnectIfNetworkAvailable();
                    if (reconnectInProgress) {
                        reconnectStartedAt = millis();
                        updateState(State::CONNECTING);
                    }
                }
                break;
            case State::CONNECTING:
                if (reconnectInProgress &&
                    millis() - reconnectStartedAt >= RECONNECT_ATTEMPT_TIMEOUT_MS) {
                    HAL::WiFi::enterOfflineMode();
                    reconnectInProgress = false;
                    lastReconnectAttempt = millis();
                    updateState(State::DISCONNECTED);
                }
                break;
            case State::FAILED:
                updateState(State::DISCONNECTED);
                break;
            default: break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

bool WiFiService::startTask() {
    if (wifiTaskHandle != nullptr) return true;

    BaseType_t result = xTaskCreatePinnedToCore(task, "WiFiService", 8192, NULL, 1, &wifiTaskHandle, 0);
    if (result == pdPASS) {
        return true;
    }
    Serial.println("[WiFiService] ERROR: Failed to create WiFi task");
    return false;  // add this line
}

bool WiFiService::restartTask() {
    if (wifiTaskHandle != nullptr) {
        // esp_task_wdt_delete(wifiTaskHandle);
        Serial.println("[WiFiService] WDT removed for WiFi task");

        TaskHandle_t tempHandle = wifiTaskHandle;
        wifiTaskHandle = nullptr;  
        vTaskDelete(tempHandle);
        vTaskDelay(pdMS_TO_TICKS(100)); 
        Serial.println("[SVC][WiFiService] Task deleted, restarting...");
    }
    return startTask();
}

} // namespace SVC
