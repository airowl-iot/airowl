// mqtt_service.cpp - MQTT Service implementation for Airowl 3.0
#include "mqtt_service.h"
#include "wifi_service.h"
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <stdexcept>

namespace {
    // Private variables
    bool initialized = false;
    SVC::MQTTService::State currentState = SVC::MQTTService::State::DISCONNECTED;
    SVC::MQTTService::MessageCallback messageCallback = nullptr;
    SVC::MQTTService::StateCallback stateCallback = nullptr;
    TaskHandle_t mqttTaskHandle = nullptr;

    WiFiClient wifiClient;
    PubSubClient mqttClient(wifiClient);
    
    String mqttServer= "mqtt.oizom.com";
    uint16_t mqttPort = 1883;
    String mqttUsername= "oizom";
    String mqttPassword= "12345678";
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

    void updateState(SVC::MQTTService::State newState) {
        if (currentState != newState) {
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

        if (pendingMessageCount >= MAX_PENDING_MESSAGES) {
            return false;
        }

        PendingMessage& msg = pendingMessages[pendingMessageCount++];
        msg.topic = topic;
        msg.payload = payload;
        msg.qos = qos;
        msg.retain = retain;
        msg.timestamp = millis();
        msg.attempts = 1; 
        
        return true;
    }

    void processPendingMessages() {
        if (pendingMessageCount == 0 || currentState != SVC::MQTTService::State::CONNECTED) {
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
    }
}

namespace SVC {

bool MQTTService::init(const char* clientId) {
    if (initialized) return true;
    
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
    
    try {
        wl_status_t wifiStatus = WL_DISCONNECTED;
        try {
            wifiStatus = WiFi.status();
        }
        catch (...) {
            Serial.println("[MQTT] ERROR: Exception checking WiFi status");
            updateState(State::FAILED);
            return false;
        }
        
        if (wifiStatus != WL_CONNECTED) {
            // Serial.printf("[MQTT] ERROR: WiFi not connected. Status: %d\n", wifiStatus);
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
        
        try {
            mqttClient.setServer(mqttServer.c_str(), mqttPort);
        }
        catch (...) {
            Serial.println("[MQTT] ERROR: Exception setting server configuration");
            updateState(State::FAILED);
            return false;
        }
        
        updateState(State::CONNECTING);
        Serial.println("[MQTT] State: CONNECTING");

        bool success = false;
        try {
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
        }
        catch (...) {
            Serial.println("[MQTT] ERROR: Exception during connection attempt");
            updateState(State::FAILED);
            return false;
        }
        
        if (success) {
            updateState(State::CONNECTED);
            reconnectAttempts = 0;
            Serial.println("[MQTT] ✓ Successfully connected to broker!");
        } else {
            updateState(State::FAILED);
            int errorCode = -99;
            try {
                errorCode = mqttClient.state();
            }
            catch (...) {
                Serial.println("[MQTT] ERROR: Exception getting error code");
            }
            Serial.printf("[MQTT] ✗ Connection failed! Error code: %d\n", errorCode);
            Serial.println("[MQTT] Error codes: -4=timeout, -3=lost, -2=failed, -1=disconnected, 1=bad protocol, 2=bad client id, 3=unavailable, 4=bad credentials, 5=unauthorized");
        }
        
        return success;
    }
    catch (const std::exception& e) {
        Serial.printf("[MQTT] Exception in connect: %s\n", e.what());
        updateState(State::FAILED);
        return false;
    }
    catch (...) {
        Serial.println("[MQTT] Unknown exception in connect");
        updateState(State::FAILED);
        return false;
    }
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
        char* tempPayload = new char[length + 1];
        memcpy(tempPayload, payload, length);
        tempPayload[length] = '\0';
        addToPendingMessages(topic, tempPayload, qos, retain);
        
        delete[] tempPayload;
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
    Serial.println("[MQTT] Connecting to Oizom MQTT broker...");
     bool result = connect(
        mqttServer.c_str(),   
        mqttPort,              
        mqttUsername.length() > 0 ? mqttUsername.c_str() : nullptr,
        mqttPassword.length() > 0 ? mqttPassword.c_str() : nullptr
    );
    if (result) {
        Serial.println("[MQTT] ✓ Oizom connection initiated successfully");
    } else {
        Serial.println("[MQTT] ✗ Failed to initiate Oizom connection");
    }
    return result;
}

bool MQTTService::publishSensorData(const char* deviceId, float pm25, float pm10, float tvoc) {
    if (!initialized) {
        Serial.println("[MQTT] ERROR: Service not initialized");
        return false;
    }
    
    if (currentState != State::CONNECTED) {
        Serial.printf("[MQTT] ERROR: Not connected to broker. Current state: %d\n", (int)currentState);
        return false;
    }
    
    String payload = "{";
    payload += "\"deviceId\":\"" + String(deviceId) + "\",";
    payload += "\"p1\":" + String(pm25, 2) + ",";    
    payload += "\"p2\":" + String(pm10, 2) + ",";    
    payload += "\"v2\":" + String(tvoc, 2);          
    payload += "}";
    
    const char* topic = "airowl";
    
    Serial.printf("[MQTT] Publishing to topic: %s\n", topic);
    Serial.printf("[MQTT] Payload: %s\n", payload.c_str());
    Serial.printf("[MQTT] Payload size: %d bytes\n", payload.length());
    
    bool result = publish(topic, payload.c_str(), QoS::AT_LEAST_ONCE, false);
    
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
        try {
            // Reset watchdog
            esp_task_wdt_reset();

            wl_status_t wifiStatus = WL_DISCONNECTED;
            try {
                wifiStatus = WiFi.status();
            }
            catch (...) {
                Serial.println("[MQTT] Exception checking WiFi status in task");
                wifiStatus = WL_DISCONNECTED;
            }

            switch (currentState) {
                case State::CONNECTED:
                    try {
                        if (!mqttClient.loop()) {
                            Serial.println("[MQTT] Connection lost during loop()");
                            updateState(State::DISCONNECTED);
                            reconnectAttempts = 0;
                            lastReconnectAttempt = millis();
                        } else {
                            try {
                                processPendingMessages();
                            }
                            catch (...) {
                                Serial.println("[MQTT] Exception processing pending messages");
                            }
                        }
                    }
                    catch (...) {
                        Serial.println("[MQTT] Exception in MQTT loop, disconnecting");
                        updateState(State::DISCONNECTED);
                    }
                    break;
                    
                case State::DISCONNECTED:
                    if (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
                        if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS && wifiStatus == WL_CONNECTED) {
                            reconnectAttempts++;
                            lastReconnectAttempt = millis();
                            Serial.printf("[MQTT] Reconnection attempt %d/%d...\n", reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
                            
                            try {
                                connect(mqttServer.c_str(), mqttPort, 
                                       mqttUsername.length() > 0 ? mqttUsername.c_str() : nullptr,
                                       mqttPassword.length() > 0 ? mqttPassword.c_str() : nullptr);
                            }
                            catch (...) {
                                Serial.println("[MQTT] Exception during reconnection attempt");
                                updateState(State::FAILED);
                            }
                        } else if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
                            Serial.println("[MQTT] Max reconnection attempts reached. Giving up.");
                        } else if (wifiStatus != WL_CONNECTED) {
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
        }
        catch (const std::exception& e) {
            Serial.printf("[MQTT] Exception in task loop: %s\n", e.what());
        }
        catch (...) {
            Serial.println("[MQTT] Unknown exception in task loop");
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
        vTaskDelete(mqttTaskHandle);
        mqttTaskHandle = nullptr;
    }
    
    return startTask();
}

} // namespace SVC