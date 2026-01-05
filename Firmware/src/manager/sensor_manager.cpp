#include "sensor_manager.h"
#include <esp_task_wdt.h>
#include "event_manager.h"
#include "../service/mqtt_service.h"
#include "../hal/hal_wifi.h"

namespace APP {

// -------- Static member definitions --------
bool SensorManager::initialized = false;
bool SensorManager::running = false;
TaskHandle_t SensorManager::sensorTaskHandle = nullptr;

SensorManager::SensorConfig SensorManager::pm700Config;

unsigned long SensorManager::lastPM700Read = 0;
unsigned long SensorManager::lastPM700Publish = 0;

HAL::PM700::Data SensorManager::lastPM700Data;

bool SensorManager::newPM700Data = false;

// Averaging buffers initialization
float SensorManager::pm1Buffer[SENSOR_BUFFER_SIZE] = {0};
float SensorManager::pm25Buffer[SENSOR_BUFFER_SIZE] = {0};
float SensorManager::pm10Buffer[SENSOR_BUFFER_SIZE] = {0};

int SensorManager::pmBufferIndex = 0;
int SensorManager::pmBufferCount = 0;

unsigned long SensorManager::lastMqttPublish = 0;

// -------- Lifecycle --------
bool SensorManager::init() {
    if (initialized) return true;

    auto* cfg = ConfigManager::getInstance();
    if (!cfg || !cfg->isInitialized()) {
        Serial.println("[SensorManager] ConfigManager not ready");
        return false;
    }

    if (!loadConfigsFromManager()) return false;

    bool any = false;
    if (pm700Config.enabled) any |= HAL::PM700::init();

    initialized = any;
    return initialized;
}

bool SensorManager::init(const SensorConfig& pm700Cfg) {
    if (initialized) return true;
    pm700Config = pm700Cfg;
   

    initialized = (pm700Cfg.enabled && HAL::PM700::init()) ;
    return initialized;
}

bool SensorManager::start() {
    if (!initialized) return false;
    running = true;
    return true;
}

void SensorManager::stop() { running = false; }
bool SensorManager::isRunning() { return running; }

// -------- Config handling --------
void SensorManager::updateConfigFromManager() {
    auto* cfg = ConfigManager::getInstance();
    if (!cfg || !cfg->isInitialized()) return;
    loadConfigsFromManager();
}

void SensorManager::updateConfig(const SensorConfig& pm700Cfg) {
    pm700Config = pm700Cfg;
}

const SensorManager::SensorConfig& SensorManager::getPM700Config() { return pm700Config; }

// -------- Sensor read handlers --------

void SensorManager::readPM700Sensor() {
    if (!pm700Config.enabled || !running) return;
    auto now = millis();
    if (now - lastPM700Read < pm700Config.readInterval) return;

    lastPM700Read = now;
    HAL::PM700::Error result = HAL::PM700::read(&lastPM700Data);
    if (result == HAL::PM700::Error::NONE &&
        HAL::PM700::isDataAvailable()) {
        newPM700Data = true;
        float vals[] = {lastPM700Data.pm1, lastPM700Data.pm25, lastPM700Data.pm10,
                         lastPM700Data.p03};

        pm1Buffer[pmBufferIndex] = lastPM700Data.pm1;
        pm25Buffer[pmBufferIndex] = lastPM700Data.pm25;
        pm10Buffer[pmBufferIndex] = lastPM700Data.pm10;
        pmBufferIndex = (pmBufferIndex + 1) % SENSOR_BUFFER_SIZE;
        if (pmBufferCount < SENSOR_BUFFER_SIZE) {
            pmBufferCount++;
        }

        auto event = std::make_shared<CORE::SensorReadingEvent>(
            CORE::SensorReadingEvent::SensorType::PM700,
            0,  // sensorId
            vals,
            4
        );
        CORE::EventBus::getInstance().publish(event);
        // Serial.printf("[SensorManager] Published PM700 data: PM2.5=%.1f, PM10=%.1f\n",
        //               lastPM700Data.pm25, lastPM700Data.pm10);
    } else if (result != HAL::PM700::Error::NONE) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

template<typename DataT>
void SensorManager::publishData(CORE::Event::Type type,
                              const DataT& data,
                              const float* values,
                              size_t count) {
    auto event = std::make_shared<CORE::SensorReadingEvent>(
        static_cast<CORE::SensorReadingEvent::SensorType>(type),
        0,  // sensorId
        values,
        count
    );
    CORE::EventBus::getInstance().publish(event);
}

void SensorManager::calculatePMAverages(float& avgPM1, float& avgPM25, float& avgPM10) {
    if (pmBufferCount == 0) {
        avgPM1 = lastPM700Data.pm1;
        avgPM25 = lastPM700Data.pm25;
        avgPM10 = lastPM700Data.pm10;
        return;
    }

    float sumPM1 = 0, sumPM25 = 0, sumPM10 = 0;
    for (int i = 0; i < pmBufferCount; i++) {
        sumPM1 += pm1Buffer[i];
        sumPM25 += pm25Buffer[i];
        sumPM10 += pm10Buffer[i];
    }

    avgPM1 = sumPM1 / pmBufferCount;
    avgPM25 = sumPM25 / pmBufferCount;
    avgPM10 = sumPM10 / pmBufferCount;

    Serial.printf("[SensorManager] Calculated 2-min averages from %d readings - "
                  "PM1.0: %.2f, PM2.5: %.2f, PM10: %.2f\n", pmBufferCount, avgPM1, avgPM25, avgPM10);
}

float SensorManager::getPM1() {
    return lastPM700Data.pm1;
} 

float SensorManager::getPM1Avg() {
    if (pmBufferCount == 0) {
        return lastPM700Data.pm1;
    }

    float sum = 0;
    for (int i = 0; i < pmBufferCount; i++) {
        sum += pm1Buffer[i];
    }
    return sum / pmBufferCount;
}

float SensorManager::getPM25() {
    return lastPM700Data.pm25;
}

float SensorManager::getPM25Avg() {
    if (pmBufferCount == 0) {
        return lastPM700Data.pm25;
    }

    float sum = 0;
    for (int i = 0; i < pmBufferCount; i++) {
        sum += pm25Buffer[i];
    }
    return sum / pmBufferCount;
}

float SensorManager::getPM10() {
    return lastPM700Data.pm10;
}

float SensorManager::getPM10Avg() {
    if (pmBufferCount == 0) {
        return lastPM700Data.pm10;
    }

    float sum = 0;
    for (int i = 0; i < pmBufferCount; i++) {
        sum += pm10Buffer[i];
    }
    return sum / pmBufferCount;
}


void SensorManager::publishMqttAverages() {
    if (millis() - lastMqttPublish < MQTT_PUBLISH_INTERVAL) return;

    Serial.printf("[SensorManager] MQTT publish interval reached (%lu ms) - preparing 2-minute averaged data\n",
                  MQTT_PUBLISH_INTERVAL);

    if (SVC::MQTTService::getState() == SVC::MQTTService::State::CONNECTED) {
        esp_task_wdt_reset();
        
        String deviceId = HAL::WiFi::generateApName();

        float avgPM1, avgPM25, avgPM10;
        calculatePMAverages(avgPM1, avgPM25, avgPM10);

        float avgPM4 = 0;

        Serial.printf("[SensorManager] Publishing 2-minute averaged sensor data for device: %s\n", deviceId.c_str());
        Serial.printf("[SensorManager] Averaged values - PM1.0: %.2f, PM2.5: %.2f, PM10: %.2f\n", avgPM1, avgPM25, avgPM10);

        if (SVC::MQTTService::publishSensorData(deviceId.c_str(), avgPM1, avgPM25, avgPM10)) {
            Serial.println("[SensorManager] ✓ 2-minute averaged sensor data published to MQTT successfully");
            pmBufferCount = 0;
            pmBufferIndex = 0;
            Serial.println("[SensorManager] Reset averaging buffers for next 2-minute period");
        } else {
            Serial.println("[SensorManager] ✗ Failed to publish 2-minute averaged sensor data to MQTT");
        }
    } else {
        Serial.println("[SensorManager] MQTT not connected, skipping publish");
    }

    lastMqttPublish = millis();
}

void SensorManager::task(void*) {
    vTaskDelay(pdMS_TO_TICKS(1000));

    for (;;) {
        if (running) {
            esp_task_wdt_reset();

            readPM700Sensor();
            esp_task_wdt_reset();
            publishMqttAverages();
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool SensorManager::startTask() {
    if (sensorTaskHandle) return true;
    bool created =  xTaskCreatePinnedToCore(task, "SensorManager", 4096,
                                   nullptr, 1, &sensorTaskHandle, 0) == pdPASS;
    if (created) {
       esp_err_t err = esp_task_wdt_add(sensorTaskHandle);
        if (err == ESP_OK) {
            Serial.println("[SensorManager] WDT added for sensor task");
        } else if (err == ESP_ERR_INVALID_STATE) {
            Serial.println("[SensorManager] WDT not initialized globally yet");
        } else {
            Serial.printf("[SensorManager] Failed to add WDT: 0x%x\n", err);
        }
    }

    return created;
}

bool SensorManager::restartTask() {
    if (sensorTaskHandle) {
        esp_task_wdt_delete(sensorTaskHandle);
        Serial.println("[SensorManager] WDT removed for sensor task");

        TaskHandle_t tempHandle = sensorTaskHandle;
        sensorTaskHandle = nullptr; 
        vTaskDelete(tempHandle);
        vTaskDelay(pdMS_TO_TICKS(100));  
        Serial.println("[SensorManager] Task deleted, restarting...");
    }
    return startTask();
}

bool SensorManager::loadConfigsFromManager() {
    auto* cfg = ConfigManager::getInstance();
    if (!cfg) return false;

    for (const auto& s : cfg->getAllSensors()) {
        String t = s.type; t.toUpperCase();
        if (t.startsWith("PM700")) pm700Config = convertConfig(s);
    }
    return true;
}

SensorManager::SensorConfig SensorManager::convertConfig(const ::SensorConfig& cfg) {
    return SensorConfig{
        .enabled = cfg.enabled,
        .readInterval = cfg.read_interval,
        .publishInterval = cfg.publish_interval
    };
}

} // namespace APP