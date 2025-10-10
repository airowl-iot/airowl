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

    const uint32_t RECONNECT_INTERVAL_MS = 10000;  
    const uint32_t MAX_RECONNECT_ATTEMPTS = 5;    
    const uint32_t FAST_RECONNECT_INTERVAL_MS = 3000;  
    uint32_t reconnectAttempts = 0;
    unsigned long lastReconnectAttempt = 0;
    bool hasStoredCredentials = false;

    SemaphoreHandle_t stateMutex = nullptr;

    String currentSSID;
    String currentPassword;

    void getCredentialsCopy(String& ssid, String& password) {
        if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            ssid = currentSSID;
            password = currentPassword;
            xSemaphoreGive(stateMutex);
        }
    }

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
        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            currentSSID = ssid;
            currentPassword = password;
            hasStoredCredentials = true;
            xSemaphoreGive(stateMutex);
            Serial.printf("[SVC][WiFiService] Connecting to WiFi SSID: %s\n", currentSSID.c_str());
        } else {
            Serial.println("[SVC][WiFiService] Failed to acquire mutex for credentials");
            return false;
        }
    }

    if (!currentSSID.isEmpty() && !currentPassword.isEmpty()) {
        HAL::WiFi::disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
        updateState(State::CONNECTING);

        bool result = HAL::WiFi::connect(currentSSID.c_str(), currentPassword.c_str());
        if (result) {
            updateState(State::CONNECTED);
            reconnectAttempts = 0;  
            Serial.printf("[SVC][WiFiService] Successfully connected to WiFi: %s\n", ::WiFi.SSID().c_str());
        } else {
            updateState(State::FAILED);
            Serial.println("[SVC][WiFiService] Failed to connect to WiFi");
        }
        return result;
    }

    Serial.println("[SVC][WiFiService] No credentials provided, checking for saved credentials...");

    ::WiFi.begin();
    vTaskDelay(pdMS_TO_TICKS(100));

    if (::WiFi.status() == WL_CONNECTED || ::WiFi.SSID().length() > 0) {
        Serial.println("[SVC][WiFiService] Found saved credentials, attempting auto-connect...");
        updateState(State::CONNECTING);

        unsigned long startTime = millis();
        while (::WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (::WiFi.status() == WL_CONNECTED) {

            if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                currentSSID = ::WiFi.SSID();
                String psk = ::WiFi.psk();

                if (psk.length() > 0) {
                    currentPassword = psk;
                } else {
                    Serial.println("[SVC][WiFiService] Warning: WiFi.psk() returned empty, keeping existing password");
                }

                hasStoredCredentials = true;
                xSemaphoreGive(stateMutex);

                updateState(State::CONNECTED);
                Serial.printf("[SVC][WiFiService] Auto-connected to saved network: %s\n", currentSSID.c_str());
                return true;
            } else {
                Serial.println("[SVC][WiFiService] Failed to acquire mutex for credential update");
                return false;
            }
        }
    }

    Serial.println("[SVC][WiFiService] No saved credentials found, starting provisioning");
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
        if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            currentSSID = ::WiFi.SSID();
            String psk = ::WiFi.psk();

            if (psk.length() > 0) {
                currentPassword = psk;
            } else {
                Serial.println("[SVC][WiFiService] Warning: WiFi.psk() returned empty after provisioning");
            }

            hasStoredCredentials = true;
            xSemaphoreGive(stateMutex);

            updateState(State::CONNECTED);
            reconnectAttempts = 0;
            Serial.printf("[SVC][WiFiService] Provisioning successful! Connected to: %s\n", currentSSID.c_str());
            return true;
        } else {
            Serial.println("[SVC][WiFiService] Failed to acquire mutex after provisioning");
            updateState(State::FAILED);
            return false;
        }
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

                    String ssid, password;
                    getCredentialsCopy(ssid, password);
                    Serial.printf("[SVC] WiFi connection lost (SSID: %s), will attempt reconnection\n",
                                  ssid.c_str());
                }
                break;
            case State::DISCONNECTED:
                if (hasStoredCredentials) {
                    uint32_t interval = (reconnectAttempts < 2) ? FAST_RECONNECT_INTERVAL_MS : RECONNECT_INTERVAL_MS;

                    if (millis() - lastReconnectAttempt >= interval) {
                        if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                            reconnectAttempts++;
                            lastReconnectAttempt = millis();

                            String ssid, password;
                            getCredentialsCopy(ssid, password);

                            Serial.printf("[SVC] Attempting reconnection (attempt %d of %d) to: %s\n",
                                          reconnectAttempts, MAX_RECONNECT_ATTEMPTS, ssid.c_str());

                            HAL::WiFi::disconnect();
                            vTaskDelay(pdMS_TO_TICKS(500));
                            updateState(State::CONNECTING);

                            HAL::WiFi::connect(ssid.c_str(), password.c_str());
                        } else {
                            Serial.println("[SVC] Max reconnection attempts reached, entering failed state");
                            updateState(State::FAILED);
                        }
                    }
                } else {

                    if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS * 3) {
                        Serial.println("[SVC] No stored credentials available");
                        lastReconnectAttempt = millis(); 
                    }
                }
                break;
            case State::FAILED:
                if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS * 2) {
                    if (hasStoredCredentials) {
                        updateState(State::DISCONNECTED);
                        lastReconnectAttempt = millis();
                        reconnectAttempts = 0;
                        Serial.println("[SVC] Resetting after failure, will retry connection");
                    } else {
                        Serial.println("[SVC] No credentials available - user must call startProvisioning()");
                        lastReconnectAttempt = millis(); 
                    }
                }
                break;
            default: break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

bool WiFiService::startTask() {
    if (wifiTaskHandle != nullptr) return true;

    // Increased stack from 4KB to 8KB to prevent stack overflow during WiFiManager operations
    BaseType_t result = xTaskCreatePinnedToCore(task, "WiFiService", 8192, NULL, 1, &wifiTaskHandle, 0);
    return (result == pdPASS);
}

bool WiFiService::restartTask() {
    if (wifiTaskHandle != nullptr) {
        TaskHandle_t tempHandle = wifiTaskHandle;
        wifiTaskHandle = nullptr;  
        vTaskDelete(tempHandle);
        vTaskDelay(pdMS_TO_TICKS(100)); 
        Serial.println("[SVC][WiFiService] Task deleted, restarting...");
    }
    return startTask();
}

} // namespace SVC
