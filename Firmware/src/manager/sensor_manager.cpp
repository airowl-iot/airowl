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

float SensorManager::AQI = 0.0f;
float SensorManager::temperature = 0.0f;
float SensorManager::humidity = 0.0f;

SensorManager::SensorConfig SensorManager::pm700Config;
SensorManager::SensorConfig SensorManager::ahtConfig;

unsigned long SensorManager::lastPM700Read = 0;
unsigned long SensorManager::lastAHTRead = 0;
unsigned long SensorManager::lastPM700Publish = 0;
unsigned long SensorManager::lastAHTPublish = 0;

HAL::PM700::Data SensorManager::lastPM700Data;
HAL::AHT::Data SensorManager::lastAHTData;

bool SensorManager::newPM700Data = false;
bool SensorManager::newAHTData = false;

// Averaging buffers initialization
float SensorManager::pm25Buffer[SENSOR_BUFFER_SIZE] = {0};
float SensorManager::pm10Buffer[SENSOR_BUFFER_SIZE] = {0};
float SensorManager::tempBuffer[SENSOR_BUFFER_SIZE] = {0};
float SensorManager::humdBuffer[SENSOR_BUFFER_SIZE] = {0};
float SensorManager::tvocBuffer[SENSOR_BUFFER_SIZE] = {0};
float SensorManager::eco2Buffer[SENSOR_BUFFER_SIZE] = {0};

int SensorManager::pmBufferIndex = 0;
int SensorManager::pmBufferCount = 0;
int SensorManager::ahtBufferIndex = 0;
int SensorManager::ahtBufferCount = 0;

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
    if (ahtConfig.enabled) any |= HAL::AHT::init();

    initialized = any;
    return initialized;
}

bool SensorManager::init(const SensorConfig& pm700Cfg,
                         const SensorConfig& ahtCfg) {
    if (initialized) return true;
    pm700Config = pm700Cfg;
    ahtConfig = ahtCfg;
   

    initialized = (pm700Cfg.enabled && HAL::PM700::init()) ||
                  (ahtCfg.enabled && HAL::AHT::init()) ;
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

void SensorManager::updateConfig(const SensorConfig& pm700Cfg,
                                 const SensorConfig& ahtCfg) {
    pm700Config = pm700Cfg;
    ahtConfig = ahtCfg;
}

const SensorManager::SensorConfig& SensorManager::getPM700Config() { return pm700Config; }
const SensorManager::SensorConfig& SensorManager::getAHTConfig() { return ahtConfig; }

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
        float vals[] = { lastPM700Data.pm25, lastPM700Data.pm10,
                         lastPM700Data.p03, lastPM700Data.pm1 };

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

void SensorManager::readAHTSensor() {
    if (!ahtConfig.enabled || !running) return;
    auto now = millis();
    if (now - lastAHTRead < ahtConfig.readInterval) return;

    lastAHTRead = now;
    HAL::AHT::Error result = HAL::AHT::read(&lastAHTData);
    if (result == HAL::AHT::Error::NONE &&
        HAL::AHT::isDataAvailable()) {
        newAHTData = true;
        
        float vals[] = { lastAHTData.temperature, lastAHTData.humidity,
                        (float)lastAHTData.tvoc, (float)lastAHTData.eco2 };

        tempBuffer[ahtBufferIndex] = lastAHTData.temperature;
        humdBuffer[ahtBufferIndex] = lastAHTData.humidity;
        tvocBuffer[ahtBufferIndex] = (float)lastAHTData.tvoc;
        eco2Buffer[ahtBufferIndex] = (float)lastAHTData.eco2;
        ahtBufferIndex = (ahtBufferIndex + 1) % SENSOR_BUFFER_SIZE;
        if (ahtBufferCount < SENSOR_BUFFER_SIZE) {
            ahtBufferCount++;
        }

        temperature = lastAHTData.temperature;
        humidity = lastAHTData.humidity;

        auto event = std::make_shared<CORE::SensorReadingEvent>(
            CORE::SensorReadingEvent::SensorType::AHT,
            0,  // sensorId
            vals,
            4
        );
        CORE::EventBus::getInstance().publish(event);
        // Serial.printf("[SensorManager] Published AHT data: Temp=%.1f°C, Humidity=%.1f%%\n",
        //               lastAHTData.temperature, lastAHTData.humidity);
    } else if (result != HAL::AHT::Error::NONE) {
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

void SensorManager::calculatePMAverages(float& avgPM25, float& avgPM10) {
    if (pmBufferCount == 0) {
        avgPM25 = lastPM700Data.pm25;
        avgPM10 = lastPM700Data.pm10;
        return;
    }

    float sumPM25 = 0, sumPM10 = 0;
    for (int i = 0; i < pmBufferCount; i++) {
        sumPM25 += pm25Buffer[i];
        sumPM10 += pm10Buffer[i];
    }

    avgPM25 = sumPM25 / pmBufferCount;
    avgPM10 = sumPM10 / pmBufferCount;

    Serial.printf("[SensorManager] Calculated 2-min averages from %d readings - "
                  "PM2.5: %.2f, PM10: %.2f\n", pmBufferCount, avgPM25, avgPM10);
}

void SensorManager::calculateAHTAverages(float& avgTemp, float& avgHumd,
                                        float& avgTvoc, float& avgECo2) {
    if (ahtBufferCount == 0) {
        avgTemp = lastAHTData.temperature;
        avgHumd = lastAHTData.humidity;
        avgTvoc = (float)lastAHTData.tvoc;
        avgECo2 = (float)lastAHTData.eco2;
        return;
    }

    float sumTemp = 0, sumHumd = 0, sumTvoc = 0, sumECo2 = 0;
    for (int i = 0; i < ahtBufferCount; i++) {
        sumTemp += tempBuffer[i];
        sumHumd += humdBuffer[i];
        sumTvoc += tvocBuffer[i];
        sumECo2 += eco2Buffer[i];
    }

    avgTemp = sumTemp / ahtBufferCount;
    avgHumd = sumHumd / ahtBufferCount;
    avgTvoc = sumTvoc / ahtBufferCount;
    avgECo2 = sumECo2 / ahtBufferCount;

    Serial.printf("[SensorManager] Calculated 2-min averages from %d readings - "
                  "Temp: %.2f, Humd: %.2f, TVOC: %.2f, eCO2: %.2f\n",
                  ahtBufferCount, avgTemp, avgHumd, avgTvoc, avgECo2);
}

void SensorManager::publishMqttAverages() {
    if (millis() - lastMqttPublish < MQTT_PUBLISH_INTERVAL) return;

    Serial.printf("[SensorManager] MQTT publish interval reached (%lu ms) - preparing 2-minute averaged data\n",
                  MQTT_PUBLISH_INTERVAL);

    if (SVC::MQTTService::getState() == SVC::MQTTService::State::CONNECTED) {
        String deviceId = HAL::WiFi::generateApName();

        float avgPM25, avgPM10, avgTemp, avgHumd, avgTvoc, avgECo2;
        calculatePMAverages(avgPM25, avgPM10);
        calculateAHTAverages(avgTemp, avgHumd, avgTvoc, avgECo2);

        float avgPM1 = 0;
        float avgPM4 = 0;

        Serial.printf("[SensorManager] Publishing 2-minute averaged sensor data for device: %s\n",
                      deviceId.c_str());
        Serial.printf("[SensorManager] Averaged values - PM2.5: %.2f, PM10: %.2f, "
                      "Temp: %.2f, Humd: %.2f, TVOC: %.2f, eCO2: %.2f\n",
                      avgPM25, avgPM10, avgTemp, avgHumd, avgTvoc, avgECo2);

        if (SVC::MQTTService::publishSensorData(deviceId.c_str(), avgPM1, avgPM25, avgPM4, avgPM10,
                                               avgTemp, avgHumd, avgTvoc)) {
            Serial.println("[SensorManager] ✓ 2-minute averaged sensor data published to MQTT successfully");
            pmBufferCount = 0;
            pmBufferIndex = 0;
            ahtBufferCount = 0;
            ahtBufferIndex = 0;
            Serial.println("[SensorManager] Reset averaging buffers for next 2-minute period");
        } else {
            Serial.println("[SensorManager] ✗ Failed to publish 2-minute averaged sensor data to MQTT");
        }
    } else {
        Serial.println("[SensorManager] MQTT not connected, skipping publish");
        if (SVC::MQTTService::getState() == SVC::MQTTService::State::DISCONNECTED) {
            Serial.println("[SensorManager] Attempting to reconnect to MQTT...");
            SVC::MQTTService::connectToOizom();
        }
    }

    lastMqttPublish = millis();
}

void SensorManager::task(void*) {
    for (;;) {
        if (running) {
           
            readPM700Sensor();
            readAHTSensor();

            publishMqttAverages();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool SensorManager::startTask() {
    if (sensorTaskHandle) return true;
    return xTaskCreatePinnedToCore(task, "SensorManager", 4096,
                                   nullptr, 1, &sensorTaskHandle, 0) == pdPASS;
}

bool SensorManager::restartTask() {
    if (sensorTaskHandle) {
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
        else if (t.startsWith("AHT")) ahtConfig = convertConfig(s);
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
