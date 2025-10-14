// mqtt_service.cpp - MQTT Service implementation for Airowl 3.0
#include "mqtt_service.h"
#include "wifi_service.h"
#include "../manager/event_manager.h"
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <stdexcept>
#include <memory>

namespace {
    bool initialized = false;
    SVC::MQTTService::State currentState = SVC::MQTTService::State::DISCONNECTED;
    SVC::MQTTService::MessageCallback messageCallback = nullptr;
    SVC::MQTTService::StateCallback stateCallback = nullptr;
    TaskHandle_t mqttTaskHandle = nullptr;

    WiFiClient wifiClient;
    PubSubClient mqttClient(wifiClient);

    String mqttServer;
    uint16_t mqttPort = 1883;
    String mqttUsername;
    String mqttPassword;
    String mqttClientId;

    const uint32_t RECONNECT_INTERVAL_MS = 5000;
    const uint32_t MAX_RECONNECT_ATTEMPTS = 10;
    uint32_t reconnectAttempts = 0;
    unsigned long lastReconnectAttempt = 0;

    struct PendingMessage {
        String topic;
        String payload;
        SVC::MQTTService::QoS qos;
        bool retain;
        unsigned long timestamp;
        uint8_t attempts;
    };

    const size_t MAX_PENDING_MESSAGES = 20;
    const uint8_t MAX_RETRY_ATTEMPTS = 3;
    const unsigned long RETRY_INTERVAL_MS = 3000;

    PendingMessage pendingMessages[MAX_PENDING_MESSAGES];
    size_t pendingMessageCount = 0;

    SemaphoreHandle_t pendingMessageMutex = nullptr;

    void updateState(SVC::MQTTService::State newState) {
        if (currentState != newState) {
            CORE::MQTTStateChangedEvent::MQTTState eventState;
            switch (newState) {
                case SVC::MQTTService::State::DISCONNECTED:
                    eventState = CORE::MQTTStateChangedEvent::MQTTState::DISCONNECTED;
                    break;
                case SVC::MQTTService::State::CONNECTING:
                    eventState = CORE::MQTTStateChangedEvent::MQTTState::CONNECTING;
                    break;
                case SVC::MQTTService::State::CONNECTED:
                    eventState = CORE::MQTTStateChangedEvent::MQTTState::CONNECTED;
                    break;
                case SVC::MQTTService::State::FAILED:
                    eventState = CORE::MQTTStateChangedEvent::MQTTState::FAILED;
                    break;
                case SVC::MQTTService::State::RECONNECTING:
                    eventState = CORE::MQTTStateChangedEvent::MQTTState::RECONNECTING;
                    break;
                default:
                    eventState = CORE::MQTTStateChangedEvent::MQTTState::DISCONNECTED;
            }

            auto event = std::make_shared<CORE::MQTTStateChangedEvent>(
                eventState, mqttServer, mqttPort
            );
            CORE::EventBus::getInstance().publish(event);

            const char* stateNames[] = {"DISCONNECTED", "CONNECTING", "CONNECTED", "FAILED"};
            Serial.printf("[MQTT] State changed: %s -> %s\n", 
                         stateNames[(int)currentState], 
                         stateNames[(int)newState]);
            
            currentState = newState;

            if (stateCallback) {
                stateCallback(newState);
            }
        }
    }

    void mqttCallback(char* topic, byte* payload, unsigned int length) {
        if (messageCallback) {
            messageCallback(topic, payload, length);
        }
    }

    bool addToPendingMessages(const char* topic, const char* payload,
                             SVC::MQTTService::QoS qos, bool retain) {
        if (qos == SVC::MQTTService::QoS::AT_MOST_ONCE) {
            return false;
        }


        if (xSemaphoreTake(pendingMessageMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
            Serial.println("[MQTT] Failed to acquire mutex for adding pending message");
            return false;
        }

        bool added = false;
        if (pendingMessageCount < MAX_PENDING_MESSAGES) {
            PendingMessage& msg = pendingMessages[pendingMessageCount++];
            msg.topic = topic;
            msg.payload = payload;
            msg.qos = qos;
            msg.retain = retain;
            msg.timestamp = millis();
            msg.attempts = 1;
            added = true;
        }

        xSemaphoreGive(pendingMessageMutex);
        return added;
    }

    void processPendingMessages() {
        if (pendingMessageCount == 0 || currentState != SVC::MQTTService::State::CONNECTED) {
            return;
        }

        if (xSemaphoreTake(pendingMessageMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
            return; 
        }

        unsigned long now = millis();

        for (size_t i = 0; i < pendingMessageCount; i++) {
            PendingMessage& msg = pendingMessages[i];

            if (now - msg.timestamp >= RETRY_INTERVAL_MS) {
                bool success = mqttClient.publish(
                    msg.topic.c_str(),
                    msg.payload.c_str(),
                    msg.retain
                );

                if (success || msg.attempts >= MAX_RETRY_ATTEMPTS) {
                    if (i < pendingMessageCount - 1) {
                        pendingMessages[i] = pendingMessages[pendingMessageCount - 1];
                        i--;
                    }
                    pendingMessageCount--;
                } else {
                    msg.timestamp = now;
                    msg.attempts++;
                }
            }
        }

        xSemaphoreGive(pendingMessageMutex);
    }
}

namespace SVC {

bool MQTTService::init(const char* clientId) {
    if (initialized) return true;

    if (pendingMessageMutex == nullptr) {
        pendingMessageMutex = xSemaphoreCreateMutex();
        if (pendingMessageMutex == nullptr) {
            Serial.println("[MQTT] ERROR: Failed to create pending message mutex");
            return false;
        }
    }

    mqttClientId = clientId;
    mqttClient.setCallback(mqttCallback);

    initialized = true;
    updateState(State::DISCONNECTED);
    return true;
}

bool MQTTService::connect(const char* server, uint16_t port, 
                          const char* username, const char* password) {
    if (!initialized) {
        Serial.println("[MQTT] ERROR: Service not initialized, attempting to initialize...");
        if (!init()) {
            Serial.println("[MQTT] ERROR: Failed to initialize service");
            return false;
        }
    }

    if (!server || !server[0]) {
        Serial.println("[MQTT] ERROR: Invalid server parameter");
        updateState(State::FAILED);
        return false;
    }
    
    wl_status_t wifiStatus = WiFi.status();
    if (wifiStatus != WL_CONNECTED) {
        Serial.printf("[MQTT] WiFi not connected, status: %d\n", wifiStatus);
        updateState(State::FAILED);
        return false;
    }
        
    Serial.printf("[MQTT] Attempting to connect to broker...\n");
    Serial.printf("[MQTT] Server: %s:%d\n", server, port);
    Serial.printf("[MQTT] Username: %s\n", username ? username : "(none)");
    Serial.printf("[MQTT] Client ID: %s\n", mqttClientId.c_str());

    mqttServer = server;
    mqttPort = port;
    mqttUsername = username ? username : "";
    mqttPassword = password ? password : "";

    if (wifiClient.connected()) {
        Serial.println("[MQTT] Closing existing WiFiClient connection");
        wifiClient.stop();
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }

    mqttClient.setClient(wifiClient);
    mqttClient.setServer(mqttServer.c_str(), mqttPort);

    updateState(State::CONNECTING);
    Serial.println("[MQTT] State: CONNECTING");

    bool success = false;
    if (mqttUsername.length() > 0) {
        Serial.println("[MQTT] Connecting with authentication...");
        success = mqttClient.connect(
            mqttClientId.c_str(),
            mqttUsername.c_str(),
            mqttPassword.c_str()
        );
    } else {
        Serial.println("[MQTT] Connecting without authentication...");
        success = mqttClient.connect(mqttClientId.c_str());
    }
        
    if (success) {
        updateState(State::CONNECTED);
        reconnectAttempts = 0;
        Serial.println("[MQTT] ✓ Successfully connected to broker!");
    } else {
        updateState(State::FAILED);
        int errorCode = mqttClient.state();
        Serial.printf("[MQTT] ✗ Connection failed! Error code: %d\n", errorCode);
        Serial.println("[MQTT] Error codes: -4=timeout, -3=lost, -2=failed, -1=disconnected, 1=bad protocol, 2=bad client id, 3=unavailable, 4=bad credentials, 5=unauthorized");
    }

    return success;
}

bool MQTTService::disconnect() {
    if (!initialized) return false;
    
    mqttClient.disconnect();
    updateState(State::DISCONNECTED);
    return true;
}

MQTTService::State MQTTService::getState() {
    return currentState;
}

bool MQTTService::subscribe(const char* topic, QoS qos) {
    if (!initialized || currentState != State::CONNECTED) {
        return false;
    }
    
    return mqttClient.subscribe(topic, static_cast<uint8_t>(qos));
}

bool MQTTService::unsubscribe(const char* topic) {
    if (!initialized || currentState != State::CONNECTED) {
        return false;
    }
    
    return mqttClient.unsubscribe(topic);
}

bool MQTTService::publish(const char* topic, const uint8_t* payload,
                          unsigned int length, QoS qos, bool retain) {
    if (!initialized || currentState != State::CONNECTED) {
        return false;
    }

    bool success = mqttClient.publish(topic, payload, length, retain);

    if (!success && qos != QoS::AT_MOST_ONCE) {
        const size_t STACK_BUFFER_SIZE = 256;
        if (length < STACK_BUFFER_SIZE) {
            char stackBuffer[STACK_BUFFER_SIZE];
            memcpy(stackBuffer, payload, length);
            stackBuffer[length] = '\0';
            addToPendingMessages(topic, stackBuffer, qos, retain);
        } else {
            std::unique_ptr<char[]> tempPayload(new char[length + 1]);
            memcpy(tempPayload.get(), payload, length);
            tempPayload[length] = '\0';
            bool added = addToPendingMessages(topic, tempPayload.get(), qos, retain);
            if (!added) {
                Serial.println("[MQTT] Failed to add to pending messages");
                return false;
            }
        }
        return true;
    }

    return success;
}

bool MQTTService::publish(const char* topic, const char* message, 
                          QoS qos, bool retain) {
    if (!initialized) {
        Serial.println("[MQTT] ERROR: Service not initialized for publish");
        return false;
    }
    
    if (!topic || !message) {
        Serial.println("[MQTT] ERROR: NULL topic or message in publish");
        return false;
    }
    
    if (currentState != State::CONNECTED) {
        Serial.printf("[MQTT] WARNING: Not connected for publish (state: %d)\n", (int)currentState);
        return false;
    }
    
    try {
        bool success = mqttClient.publish(topic, message, retain);

        if (!success && qos != QoS::AT_MOST_ONCE) {
            try {
                addToPendingMessages(topic, message, qos, retain);
                return true; 
            }
            catch (...) {
                Serial.println("[MQTT] Exception adding to pending messages");
                return false;
            }
        }
        
        return success;
    }
    catch (const std::exception& e) {
        Serial.printf("[MQTT] Exception in publish: %s\n", e.what());
        return false;
    }
    catch (...) {
        Serial.println("[MQTT] Unknown exception in publish");
        return false;
    }
}

bool MQTTService::connectToOizom() {
    // Check WiFi status first to avoid blocking
    wl_status_t wifiStatus = WiFi.status();
    if (wifiStatus != WL_CONNECTED) {
        Serial.printf("[MQTT] Cannot connect - WiFi not connected (status: %d)\n", wifiStatus);
        return false;
    }

    if (mqttServer.length() == 0) {
        Serial.println("[MQTT] ERROR: No server configured. Please use connect() with proper parameters.");
        return false;
    }

    Serial.println("[MQTT] Connecting to configured MQTT broker...");
    bool result = connect(
        mqttServer.c_str(),
        mqttPort,
        mqttUsername.length() > 0 ? mqttUsername.c_str() : nullptr,
        mqttPassword.length() > 0 ? mqttPassword.c_str() : nullptr
    );
    if (result) {
        Serial.println("[MQTT] ✓ Connection initiated successfully");
    } else {
        Serial.println("[MQTT] ✗ Failed to initiate connection");
    }
    return result;
}

bool MQTTService::publishSensorData(const char* deviceId, float pm1, float pm25, float pm4, float pm10, float temperature, float humidity, float tvoc){
    if (!initialized) {
        Serial.println("[MQTT] ERROR: Service not initialized");
        return false;
    }

    if (currentState != State::CONNECTED) {
        Serial.printf("[MQTT] ERROR: Not connected to broker. Current state: %d\n", (int)currentState);
        return false;
    }

    const size_t BUFFER_SIZE = 512;
    char payload[BUFFER_SIZE];

    int written = snprintf(payload, BUFFER_SIZE,
        "{\"deviceId\":\"%s\","
        "\"p3\":%.2f,"      // PM1.0
        "\"p1\":%.2f,"      // PM2.5
        "\"p2\":%.2f,"      // PM10
        "\"p5\":%.2f,"      // PM4.0
        "\"temp\":%.2f,"    // temperature
        "\"hum\":%.2f,"     // humidity
        "\"v2\":%.2f}",     // TVOC
        deviceId ? deviceId : "unknown",
        pm1, pm25, pm10, pm4, temperature, humidity, tvoc
    );

    if (written < 0 || written >= BUFFER_SIZE) {
        Serial.printf("[MQTT] ERROR: JSON payload too large (%d bytes)\n", written);
        return false;
    }

    const char* topic = "airowl";

    Serial.printf("[MQTT] Publishing to topic: %s\n", topic);
    Serial.printf("[MQTT] Payload: %s\n", payload);
    Serial.printf("[MQTT] Payload size: %d bytes\n", written);

    bool result = publish(topic, payload, QoS::AT_LEAST_ONCE, false);

    if (result) {
        Serial.println("[MQTT] ✓ Sensor data published successfully");
    } else {
        Serial.println("[MQTT] ✗ Failed to publish sensor data");
    }

    return result;
}

void MQTTService::onMessage(MessageCallback callback) {
    messageCallback = callback;
}

void MQTTService::onStateChange(StateCallback callback) {
    stateCallback = callback;
}

void MQTTService::task(void* parameter) {
    esp_task_wdt_add(NULL);
    
    Serial.println("[MQTT] Task started with comprehensive crash protection");
    
    while (true) {
        esp_task_wdt_reset();

        wl_status_t wifiStatus = WiFi.status();

        switch (currentState) {
            case State::CONNECTED:
                if (!mqttClient.loop()) {
                    Serial.println("[MQTT] Connection lost during loop()");
                    updateState(State::DISCONNECTED);
                    reconnectAttempts = 0;
                    lastReconnectAttempt = millis();
                } else {
                    processPendingMessages();
                }
                break;

            case State::DISCONNECTED:
                if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
                    if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS && wifiStatus == WL_CONNECTED) {
                        reconnectAttempts++;
                        lastReconnectAttempt = millis();
                        Serial.printf("[MQTT] Reconnection attempt %d/%d...\n", reconnectAttempts, MAX_RECONNECT_ATTEMPTS);

                        connect(mqttServer.c_str(), mqttPort,
                               mqttUsername.length() > 0 ? mqttUsername.c_str() : nullptr,
                               mqttPassword.length() > 0 ? mqttPassword.c_str() : nullptr);
                    } else if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
                        Serial.println("[MQTT] Max reconnection attempts reached. Giving up.");
                    }
                }
                break;

            case State::FAILED:
                if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS * 2) {
                    Serial.println("[MQTT] Transitioning from FAILED to DISCONNECTED state");
                    updateState(State::DISCONNECTED);
                    lastReconnectAttempt = millis();
                }
                break;

            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool MQTTService::startTask() {
    if (mqttTaskHandle != nullptr) {
        return true; 
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        task,
        "MQTTService",
        4096,
        NULL,
        1,
        &mqttTaskHandle,
        0
    );
    
    return (result == pdPASS);
}

bool MQTTService::restartTask() {
    if (mqttTaskHandle != nullptr) {
        TaskHandle_t tempHandle = mqttTaskHandle;
        mqttTaskHandle = nullptr; 
        vTaskDelete(tempHandle);
        vTaskDelay(pdMS_TO_TICKS(100));  
        Serial.println("[MQTT] Task deleted, restarting...");
    }

    return startTask();
}

} // namespace SVC