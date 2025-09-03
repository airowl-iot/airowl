// espnow_service.cpp - ESP-NOW Service implementation for Airowl 3.0
#include "espnow_service.h"

#ifdef CONFIG_ENABLE_ESP_NOW

#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <algorithm>
#include "config.h"
#include "../core/event_bus.h"
#include "mqtt_service.h"

extern float temperature;
extern float humidity;
extern float AQI;

TaskHandle_t espnowTaskHandle = nullptr;

namespace {

bool isMasterDevice = true;  
bool initialized = false;

uint8_t masterMac[6] = {0};

SVC::ESPNowService::MessageCallback messageCallback = nullptr;
SVC::ESPNowService::DeliveryCallback deliveryCallback = nullptr;

SVC::ESPNowService::SlaveData slaveData[MAX_SLAVES] = {0};
bool newDataReceived = false;
uint32_t lastMqttPublish = 0;
uint32_t lastEspNowCheck = 0;
uint32_t lastMqttFailTime = 0;  

enum class MessageType : uint8_t {
    SENSOR_DATA = 0x01,
    HEARTBEAT = 0x02,
    COMMAND = 0x03,
    STATUS = 0x04
};

typedef struct __attribute__((packed)) {
    uint8_t slaveId;
    float temp;
    float hum;
    float pm1;
    float pm25;
    float pm4;
    float pm10;
    float tvoc;
} SensorDataPacket;

void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int len) {
    if (!isMasterDevice) return;

    if (len < 2) return; 
    
    MessageType msgType = static_cast<MessageType>(data[0]);
    
    if (msgType == MessageType::SENSOR_DATA) {
        if (len == sizeof(SensorDataPacket) + 1) { 
            SensorDataPacket receivedData;
            memcpy(&receivedData, data + 1, sizeof(SensorDataPacket));
            
            uint8_t id = receivedData.slaveId;
            if (id < 1 || id > MAX_SLAVES) return;
            
            SVC::ESPNowService::SlaveData s;
            s.temp = receivedData.temp;
            s.hum = receivedData.hum;
            s.pm1 = receivedData.pm1;
            s.pm25 = receivedData.pm25;
            s.pm4 = receivedData.pm4;
            s.pm10 = receivedData.pm10;
            s.tvoc = receivedData.tvoc;
            s.timestamp = millis();
            s.valid = !isnan(s.temp) && !isnan(s.hum) &&
                      s.temp > -40 && s.temp < 80 &&
                      s.hum >= 0 && s.hum <= 100;
            
            slaveData[id - 1] = s;
            newDataReceived = true;
            
            if (s.valid) {
                Serial.printf("[ESP-NOW] Slave %d → %.1f°C %.1f%% PM2.5:%.1f\n", 
                             id, s.temp, s.hum, s.pm25);

                float values[7] = {s.temp, s.hum, s.pm1, s.pm25, s.pm4, s.pm10, s.tvoc};
                CORE::SensorReadingEvent ev(
                    CORE::SensorReadingEvent::SensorType::OTHER, id, values, 7
                );
                CORE::EventBus::getInstance().publish(ev);
            } else {
                Serial.printf("[ESP-NOW] Invalid data from slave %d\n", id);
            }
        } else {
            Serial.printf("[ESP-NOW] Invalid data length: %d bytes\n", len);
        }
    }
    
    if (messageCallback) {
        SVC::ESPNowService::Message message;
        message.type = static_cast<SVC::ESPNowService::MessageType>(data[0]);
        message.data = data + 1;
        message.length = len - 1;
        memcpy(message.macAddress, esp_now_info->src_addr, 6);
        messageCallback(message);
    }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (isMasterDevice) return;

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);

    Serial.printf("[ESP-NOW] Data %s → Master (%s)\n",
                  (status == ESP_NOW_SEND_SUCCESS ? "delivered" : "FAILED"),
                  macStr);
                  
    if (deliveryCallback) {
        deliveryCallback(mac_addr, status == ESP_NOW_SEND_SUCCESS);
    }
}

void sendSensorData() {
    if (isMasterDevice) return;
    
    // Check if master MAC is configured
    bool hasMasterMac = (masterMac[0] != 0 || masterMac[1] != 0 || masterMac[2] != 0 || 
                        masterMac[3] != 0 || masterMac[4] != 0 || masterMac[5] != 0);
    
    if (!hasMasterMac) {
        static uint32_t lastWarning = 0;
        if (millis() - lastWarning > 30000) { // Log warning every 30 seconds
            Serial.println("[ESP-NOW] WARNING: Master MAC not set - cannot send data. Use setMasterMac().");
            lastWarning = millis();
        }
        return;
    }
    
    SensorDataPacket packet;
    packet.slaveId = 1; 
    packet.temp = temperature;
    packet.hum = humidity;
    packet.pm1 = 0.0f;   
    packet.pm25 = 0.0f;  
    packet.pm4 = 0.0f;    
    packet.pm10 = 0.0f;   
    packet.tvoc = AQI;   
    
    uint8_t buffer[sizeof(SensorDataPacket) + 1];
    buffer[0] = static_cast<uint8_t>(MessageType::SENSOR_DATA);
    memcpy(buffer + 1, &packet, sizeof(SensorDataPacket));
    
    esp_err_t result = esp_now_send(masterMac, buffer, sizeof(buffer));
    if (result != ESP_OK) {
        Serial.printf("[ESP-NOW] Failed to send sensor data: %d (0x%X)\n", result, result);
        Serial.printf("[ESP-NOW] Target MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                     masterMac[0], masterMac[1], masterMac[2], 
                     masterMac[3], masterMac[4], masterMac[5]);
    } else {
        Serial.println("[ESP-NOW] Sensor data sent to master");
    }
}

void publishMqttData() {
    if (!isMasterDevice) return;
    
    uint32_t now = millis();
    if (now - lastMqttPublish < MQTT_PUBLISH_INTERVAL) return;
    SVC::MQTTService::State mqttState = SVC::MQTTService::getState();

    if (mqttState != SVC::MQTTService::State::CONNECTED) {
        lastMqttFailTime = now;

        static uint32_t lastLogTime = 0;
        if (now - lastLogTime > 30000) {
            const char* stateNames[] = {"DISCONNECTED", "CONNECTING", "CONNECTED", "FAILED"};
            const char* stateName = ((int)mqttState >= 0 && (int)mqttState < 4) ? stateNames[(int)mqttState] : "UNKNOWN";
            Serial.printf("[ESP-NOW] MQTT not ready (%s) - ESP-NOW waiting for stable connection\n", stateName);
            lastLogTime = now;
        }
        return; 
    }

    if (lastMqttFailTime > 0 && (now - lastMqttFailTime) < 5000) {
        return;
    }

    try {
        char tempStr[8], humStr[8], aqiStr[8];
        dtostrf(temperature, 1, 1, tempStr);
        dtostrf(humidity, 1, 1, humStr);
        dtostrf(AQI, 1, 1, aqiStr);
        
        bool publishSuccess = true;
        publishSuccess &= SVC::MQTTService::publish("sensor/master/temp", tempStr);
        publishSuccess &= SVC::MQTTService::publish("sensor/master/humidity", humStr);
        publishSuccess &= SVC::MQTTService::publish("sensor/master/aqi", aqiStr);
        
        if (!publishSuccess) {
            Serial.println("[ESP-NOW] Warning: Some master data publishes failed");
        }

        for (int i = 0; i < MAX_SLAVES; i++) {
            if (slaveData[i].valid) {
                char topic[64], value[16];
                
                sprintf(topic, "sensor/slave%d/temp", i + 1);
                dtostrf(slaveData[i].temp, 1, 1, value);
                SVC::MQTTService::publish(topic, value);
                
                sprintf(topic, "sensor/slave%d/humidity", i + 1);
                dtostrf(slaveData[i].hum, 1, 1, value);
                SVC::MQTTService::publish(topic, value);
                
                sprintf(topic, "sensor/slave%d/pm25", i + 1);
                dtostrf(slaveData[i].pm25, 1, 1, value);
                SVC::MQTTService::publish(topic, value);
            }
        }
        
        lastMqttPublish = now;
        lastMqttFailTime = 0;  
        
        int validSlaves = std::count_if(slaveData, slaveData + MAX_SLAVES, [](const auto& s) { return s.valid; });
        Serial.printf("[ESP-NOW] ✓ Published sensor data: master + %d slaves\n", validSlaves);
    }
    catch (...) {
        Serial.println("[ESP-NOW] Exception during MQTT publish - skipping this cycle");
    }
}

void espnow_loop_task(void *param) {
    esp_task_wdt_add(NULL);
    const uint32_t SENSOR_SEND_INTERVAL = 30000;  
    static uint32_t lastSensorSend = 0;
    
    Serial.printf("[ESP-NOW] Task started, mode: %s (isMasterDevice=%s)\n", 
                 isMasterDevice ? "Master" : "Slave", 
                 isMasterDevice ? "true" : "false");
    
    while (true) {
        try {
            esp_task_wdt_reset();
            
            uint32_t now = millis();
            
            // Debug: Log current state periodically
            static uint32_t lastDebugLog = 0;
            if (now - lastDebugLog > 60000) { // Log every minute
                Serial.printf("[ESP-NOW] Current state - isMasterDevice: %s\n", 
                             isMasterDevice ? "true" : "false");
                lastDebugLog = now;
            }
            
            if (isMasterDevice) {
                for (int i = 0; i < MAX_SLAVES; i++) {
                    if (slaveData[i].valid && (now - slaveData[i].timestamp > SLAVE_DATA_TIMEOUT)) {
                        slaveData[i].valid = false;
                        Serial.printf("[ESP-NOW] Slave %d timed out\n", i + 1);
                    }
                }
                
                publishMqttData();
            } else {
                if (now - lastSensorSend >= SENSOR_SEND_INTERVAL) {
                    sendSensorData();
                    lastSensorSend = now;
                }
            }
        }
        catch (const std::exception& e) {
            Serial.printf("[ESP-NOW] Exception in task loop: %s\n", e.what());
        }
        catch (...) {
            Serial.println("[ESP-NOW] Unknown exception in task loop");
        }

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

} // namespace

namespace SVC {

bool ESPNowService::init(bool master) {
    if (initialized) return true;

    // Use the provided master parameter directly - no preference storage
    isMasterDevice = master;

    Serial.printf("[ESP-NOW] Initialization - Mode set to: %s\n", 
                 isMasterDevice ? "Master" : "Slave");

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Init failed!");
        return false;
    }
    
    Serial.println("[ESP-NOW] Init Success");
    esp_now_register_recv_cb(OnDataRecv);
    esp_now_register_send_cb(OnDataSent);

    if (isMasterDevice) {
        Serial.println("[ESP-NOW] Registered as Master (Receiver)");
    } else {
        Serial.println("[ESP-NOW] Registered as Slave (Sender)");

        if (masterMac[0] != 0 || masterMac[1] != 0 || masterMac[2] != 0) {
            esp_now_peer_info_t peerInfo = {};
            memcpy(peerInfo.peer_addr, masterMac, 6);
            peerInfo.channel = 0;
            peerInfo.encrypt = false;
            esp_now_add_peer(&peerInfo);
        }
    }

    for (int i = 0; i < MAX_SLAVES; i++) {
        slaveData[i].valid = false;
        slaveData[i].timestamp = 0;
    }

    initialized = true;
    return true;
}

bool ESPNowService::send(uint8_t slaveId, float temp, float hum) {
    if (isMasterDevice || !initialized) return false;

    SensorDataPacket payload;
    payload.slaveId = slaveId;
    payload.temp = temp;
    payload.hum = hum;
    payload.pm1 = 0.0f;    
    payload.pm25 = 0.0f;    
    payload.pm4 = 0.0f;     
    payload.pm10 = 0.0f;    
    payload.tvoc = AQI;    

    uint8_t buffer[sizeof(SensorDataPacket) + 1];
    buffer[0] = static_cast<uint8_t>(MessageType::SENSOR_DATA);
    memcpy(buffer + 1, &payload, sizeof(SensorDataPacket));

    esp_err_t res = esp_now_send(masterMac, buffer, sizeof(buffer));
    return (res == ESP_OK);
}

bool ESPNowService::setMasterMode(bool master) {
    if (!initialized) return false;
    
    // Set runtime mode directly - no preference storage
    isMasterDevice = master;
    Serial.printf("[ESP-NOW] Mode changed to %s (runtime only)\n", master ? "Master" : "Slave");
    
    // Note: Mode change requires service restart to take full effect
    Serial.println("[ESP-NOW] Warning: Mode change requires service restart to take full effect");
    
    return true;
}

bool ESPNowService::isMaster() {
    return isMasterDevice;
}

void ESPNowService::setMasterMac(const uint8_t* mac) {
    memcpy(masterMac, mac, 6);
    Serial.printf("[ESP-NOW] Master MAC set to %02X:%02X:%02X:%02X:%02X:%02X\n",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool ESPNowService::startTask() {
    if (espnowTaskHandle != nullptr) {
        return true; 
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        espnow_loop_task, "ESPNowService", 6144, nullptr, 2, &espnowTaskHandle, 1
    );
    if (result == pdPASS && espnowTaskHandle) {
        esp_task_wdt_add(espnowTaskHandle);
        Serial.println("[ESP-NOW] Task started successfully");
        return true;
    } else {
        Serial.println("[ESP-NOW] Failed to start task");
        espnowTaskHandle = nullptr;
        return false;
    }
}

bool ESPNowService::restartTask() {
    if (espnowTaskHandle != nullptr) {
        esp_task_wdt_delete(espnowTaskHandle);
        vTaskDelete(espnowTaskHandle);
        espnowTaskHandle = nullptr;
    }
    
    return startTask();
}

void ESPNowService::debugStatus() {
    Serial.printf("[ESP-NOW] Debug Status:\n");
    Serial.printf("  - Runtime mode: %s\n", isMasterDevice ? "Master" : "Slave");
    Serial.printf("  - Initialized: %s\n", initialized ? "true" : "false");
    Serial.printf("  - Task handle: %s\n", espnowTaskHandle ? "active" : "null");
    
    // Show device MAC
    uint8_t deviceMac[6];
    WiFi.macAddress(deviceMac);
    Serial.printf("  - Device MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 deviceMac[0], deviceMac[1], deviceMac[2], 
                 deviceMac[3], deviceMac[4], deviceMac[5]);
    
    if (!isMasterDevice) {
        Serial.printf("  - Master MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                     masterMac[0], masterMac[1], masterMac[2], 
                     masterMac[3], masterMac[4], masterMac[5]);
        bool hasMasterMac = (masterMac[0] != 0 || masterMac[1] != 0 || masterMac[2] != 0 || 
                            masterMac[3] != 0 || masterMac[4] != 0 || masterMac[5] != 0);
        Serial.printf("  - Master MAC configured: %s\n", hasMasterMac ? "YES" : "NO - Use setMasterMac()");
    }
    
    Serial.printf("  - Note: Mode is set at initialization time only\n");
}

void ESPNowService::getDeviceMAC(uint8_t* mac) {
    WiFi.macAddress(mac);
}

void ESPNowService::setMessageCallback(MessageCallback cb) {
    messageCallback = cb;
}

void ESPNowService::setDeliveryCallback(DeliveryCallback cb) {
    deliveryCallback = cb;
}

} // namespace SVC

#endif // CONFIG_ENABLE_ESP_NOW
