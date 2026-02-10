#include "sensor_manager.h"
#include <esp_task_wdt.h>
#include "event_manager.h"
#include "../service/mqtt_service.h"
#include "../hal/hal_wifi.h"
#include "ui_manager.h"

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

// Historical data buffers (2-hour history)
float SensorManager::pm25History[HISTORY_BUFFER_SIZE] = {0};
float SensorManager::pm10History[HISTORY_BUFFER_SIZE] = {0};
float SensorManager::tempHistory[HISTORY_BUFFER_SIZE] = {0};
float SensorManager::humdHistory[HISTORY_BUFFER_SIZE] = {0};
float SensorManager::tvocHistory[HISTORY_BUFFER_SIZE] = {0};
float SensorManager::eco2History[HISTORY_BUFFER_SIZE] = {0};

int SensorManager::historyIndex = 0;
int SensorManager::historyCount = 0;

// Temporary buffers for 2-minute averaging
float SensorManager::pm25TempBuffer[TEMP_BUFFER_SIZE] = {0};
float SensorManager::pm10TempBuffer[TEMP_BUFFER_SIZE] = {0};
float SensorManager::tempTempBuffer[TEMP_BUFFER_SIZE] = {0};
float SensorManager::humdTempBuffer[TEMP_BUFFER_SIZE] = {0};
float SensorManager::tvocTempBuffer[TEMP_BUFFER_SIZE] = {0};
float SensorManager::eco2TempBuffer[TEMP_BUFFER_SIZE] = {0};

int SensorManager::pmTempIndex = 0;
int SensorManager::pmTempCount = 0;
int SensorManager::ahtTempIndex = 0;
int SensorManager::ahtTempCount = 0;

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

        // Store in temporary buffer for 2-minute averaging
        pm25TempBuffer[pmTempIndex] = lastPM700Data.pm25;
        pm10TempBuffer[pmTempIndex] = lastPM700Data.pm10;
        pmTempIndex = (pmTempIndex + 1) % TEMP_BUFFER_SIZE;
        if (pmTempCount < TEMP_BUFFER_SIZE) {
            pmTempCount++;
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

        // Store in temporary buffer for 2-minute averaging
        tempTempBuffer[ahtTempIndex] = lastAHTData.temperature;
        humdTempBuffer[ahtTempIndex] = lastAHTData.humidity;
        tvocTempBuffer[ahtTempIndex] = (float)lastAHTData.tvoc;
        eco2TempBuffer[ahtTempIndex] = (float)lastAHTData.eco2;
        ahtTempIndex = (ahtTempIndex + 1) % TEMP_BUFFER_SIZE;
        if (ahtTempCount < TEMP_BUFFER_SIZE) {
            ahtTempCount++;
        }

        temperature = lastAHTData.temperature;
        humidity = lastAHTData.humidity;
        AQI = (float)lastAHTData.aqi;

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
    if (pmTempCount == 0) {
        avgPM25 = lastPM700Data.pm25;
        avgPM10 = lastPM700Data.pm10;
        return;
    }

    float sumPM25 = 0, sumPM10 = 0;
    for (int i = 0; i < pmTempCount; i++) {
        sumPM25 += pm25TempBuffer[i];
        sumPM10 += pm10TempBuffer[i];
    }

    avgPM25 = sumPM25 / pmTempCount;
    avgPM10 = sumPM10 / pmTempCount;

    Serial.printf("[SensorManager] Calculated 2-min averages from %d readings - "
                  "PM2.5: %.2f, PM10: %.2f\n", pmTempCount, avgPM25, avgPM10);
}

float SensorManager::getPM25() {
    return lastPM700Data.pm25;
}

float SensorManager::getPM25Avg() {
    if (pmTempCount == 0) {
        return lastPM700Data.pm25;
    }

    float sum = 0;
    for (int i = 0; i < pmTempCount; i++) {
        sum += pm25TempBuffer[i];
    }
    return sum / pmTempCount;
}

float SensorManager::getPM10() {
    return lastPM700Data.pm10;
}

float SensorManager::getPM10Avg() {
    if (pmTempCount == 0) {
        return lastPM700Data.pm10;
    }

    float sum = 0;
    for (int i = 0; i < pmTempCount; i++) {
        sum += pm10TempBuffer[i];
    }
    return sum / pmTempCount;
}

float SensorManager::getTVOC() {
    return (float)lastAHTData.tvoc;
}

float SensorManager::getCO2() {
    return (float)lastAHTData.eco2;
}

void SensorManager::calculateAHTAverages(float& avgTemp, float& avgHumd,
                                        float& avgTvoc, float& avgECo2) {
    if (ahtTempCount == 0) {
        avgTemp = lastAHTData.temperature;
        avgHumd = lastAHTData.humidity;
        avgTvoc = (float)lastAHTData.tvoc;
        avgECo2 = (float)lastAHTData.eco2;
        return;
    }

    float sumTemp = 0, sumHumd = 0, sumTvoc = 0, sumECo2 = 0;
    for (int i = 0; i < ahtTempCount; i++) {
        sumTemp += tempTempBuffer[i];
        sumHumd += humdTempBuffer[i];
        sumTvoc += tvocTempBuffer[i];
        sumECo2 += eco2TempBuffer[i];
    }

    avgTemp = sumTemp / ahtTempCount;
    avgHumd = sumHumd / ahtTempCount;
    avgTvoc = sumTvoc / ahtTempCount;
    avgECo2 = sumECo2 / ahtTempCount;

    Serial.printf("[SensorManager] Calculated 2-min averages from %d readings - "
                  "Temp: %.2f, Humd: %.2f, TVOC: %.2f, eCO2: %.2f\n",
                  ahtTempCount, avgTemp, avgHumd, avgTvoc, avgECo2);
}

void SensorManager::publishMqttAverages() {
    if (millis() - lastMqttPublish < MQTT_PUBLISH_INTERVAL) return;

    Serial.printf("[SensorManager] 2-minute interval reached (%lu ms) - calculating averages\n",
                  MQTT_PUBLISH_INTERVAL);

    esp_task_wdt_reset();

    // Calculate averages from temporary buffers
    float avgPM25, avgPM10, avgTemp, avgHumd, avgTvoc, avgECo2;
    calculatePMAverages(avgPM25, avgPM10);
    calculateAHTAverages(avgTemp, avgHumd, avgTvoc, avgECo2);

    Serial.printf("[SensorManager] Averaged values - PM2.5: %.2f, PM10: %.2f, "
                  "Temp: %.2f, Humd: %.2f, TVOC: %.2f, eCO2: %.2f\n",
                  avgPM25, avgPM10, avgTemp, avgHumd, avgTvoc, avgECo2);

    // Store averaged values in history buffers (circular) - regardless of MQTT status
    pm25History[historyIndex] = avgPM25;
    pm10History[historyIndex] = avgPM10;
    tempHistory[historyIndex] = avgTemp;
    humdHistory[historyIndex] = avgHumd;
    tvocHistory[historyIndex] = avgTvoc;
    eco2History[historyIndex] = avgECo2;

    historyIndex = (historyIndex + 1) % HISTORY_BUFFER_SIZE;
    if (historyCount < HISTORY_BUFFER_SIZE) {
        historyCount++;
    }

    Serial.printf("[SensorManager] Stored 2-min averages in history buffer (index: %d, count: %d)\n",
                  historyIndex, historyCount);

    // Refresh charts to show updated history
    APP::UIController::refreshPM25Chart();
    APP::UIController::refreshPM10Chart();
    APP::UIController::refreshTempChart();
    APP::UIController::refreshHumdChart();
    APP::UIController::refreshTvocChart();
    APP::UIController::refresheCO2Chart();

   // Reset temporary buffers for next 2-minute period
    pmTempCount = 0;
    pmTempIndex = 0;
    ahtTempCount = 0;
    ahtTempIndex = 0;
    Serial.println("[SensorManager] Reset temporary averaging buffers for next 2-minute period");

    // Publish to MQTT if connected
    if (SVC::MQTTService::getState() == SVC::MQTTService::State::CONNECTED) {
        String deviceId = HAL::WiFi::generateApName();

        Serial.printf("[SensorManager] Publishing 2-minute averaged sensor data for device: %s\n",
                      deviceId.c_str());

        if (SVC::MQTTService::publishSensorData(deviceId.c_str(), avgPM25, avgPM10, avgTemp, avgHumd, avgECo2, avgTvoc)) {
            Serial.println("[SensorManager] ✓ 2-minute averaged sensor data published to MQTT successfully");
        } else {
            Serial.println("[SensorManager] ✗ Failed to publish 2-minute averaged sensor data to MQTT");
        }
    } else {
        Serial.println("[SensorManager] MQTT not connected, skipping MQTT publish (history still updated)");
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

            readAHTSensor();

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
        else if (t.startsWith("AHT")) ahtConfig = convertConfig(s);
    }
    return true;
}

SensorManager::SensorConfig SensorManager::convertConfig(const ::SensorConfig& cfg) {
    return SensorConfig{
        .enabled = cfg.enabled,
        .readInterval = cfg.read_interval
    };
}

// Historical data accessors
int SensorManager::getPM25History(float* buffer, int maxCount) {
    int count = min(historyCount, maxCount);

    // Copy from oldest to newest (circular buffer)
    int readIndex = (historyIndex - historyCount + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
    for (int i = 0; i < count; i++) {
        buffer[i] = pm25History[readIndex];
        readIndex = (readIndex + 1) % HISTORY_BUFFER_SIZE;
    }

    return count;
}

int SensorManager::getPM10History(float* buffer, int maxCount) {
    int count = min(historyCount, maxCount);

    int readIndex = (historyIndex - historyCount + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
    for (int i = 0; i < count; i++) {
        buffer[i] = pm10History[readIndex];
        readIndex = (readIndex + 1) % HISTORY_BUFFER_SIZE;
    }

    return count;
}

int SensorManager::getTempHistory(float* buffer, int maxCount) {
    int count = min(historyCount, maxCount);

    int readIndex = (historyIndex - historyCount + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
    for (int i = 0; i < count; i++) {
        buffer[i] = tempHistory[readIndex];
        readIndex = (readIndex + 1) % HISTORY_BUFFER_SIZE;
    }

    return count;
}

int SensorManager::getHumdHistory(float* buffer, int maxCount) {
    int count = min(historyCount, maxCount);

    int readIndex = (historyIndex - historyCount + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
    for (int i = 0; i < count; i++) {
        buffer[i] = humdHistory[readIndex];
        readIndex = (readIndex + 1) % HISTORY_BUFFER_SIZE;
    }

    return count;
}


    int SensorManager::getTvocHistory(float* buffer, int maxCount) {
    int count = min(historyCount, maxCount);

    int readIndex = (historyIndex - historyCount + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
    for (int i = 0; i < count; i++) {
        buffer[i] = tvocHistory[readIndex];
        readIndex = (readIndex + 1) % HISTORY_BUFFER_SIZE;
    }

    return count;
}

int SensorManager::geteCo2History(float* buffer, int maxCount) {
    int count = min(historyCount, maxCount);

    int readIndex = (historyIndex - historyCount + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
    for (int i = 0; i < count; i++) {
        buffer[i] = eco2History[readIndex];
        readIndex = (readIndex + 1) % HISTORY_BUFFER_SIZE;
    }

    return count;
}

} // namespace APP