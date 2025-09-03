// sensor_manager.cpp - Sensor Manager implementation for Airowl 3.0
#include "sensor_manager.h"
#include <esp_task_wdt.h>

float AQI = 0.0f;
float temperature = 0.0f;
float humidity = 0.0f;
TaskHandle_t sensorTaskHandle = nullptr;

namespace {
    // Private variables
    bool initialized = false;
    bool running = false;
    
    
    // Sensor configurations
    APP::SensorManager::SensorConfig pmsConfig = {
        .enabled = true,
        .readInterval = 10000,     // 10 seconds
        .publishInterval = 60000    // 1 minute
    };
    
    APP::SensorManager::SensorConfig pm700Config = {
        .enabled = true,
        .readInterval = 10000,     // 10 seconds
        .publishInterval = 60000    // 1 minute
    };
    
    APP::SensorManager::SensorConfig ahtConfig = {
        .enabled = true,
        .readInterval = 5000,      // 5 seconds
        .publishInterval = 30000    // 30 seconds
    };
    
    APP::SensorManager::SensorConfig ens160Config = {
        .enabled = true,
        .readInterval = 5000,      // 5 seconds
        .publishInterval = 30000    // 30 seconds
    };
    
    // Sensor reading timestamps
    unsigned long lastPMSRead = 0;
    unsigned long lastPM700Read = 0;
    unsigned long lastAHTRead = 0;
    unsigned long lastENS160Read = 0;
    unsigned long lastPMSPublish = 0;
    unsigned long lastPM700Publish = 0;
    unsigned long lastAHTPublish = 0;
    unsigned long lastENS160Publish = 0;
    
    // Sensor data
    HAL::PMS::Data lastPMSData;
    HAL::PM700::Data lastPM700Data;
    HAL::AHT::Data lastAHTData;
    HAL::ENS160::Data lastENS160Data;
    bool newPMSData = false;
    bool newPM700Data = false;
    bool newAHTData = false;
    bool newENS160Data = false;
}

namespace APP {

bool SensorManager::init(const SensorConfig& pmsConfig, const SensorConfig& pm700Config, const SensorConfig& ahtConfig, const SensorConfig& ens160Config) {
    if (initialized) return true;
    
    // Store configurations
    ::pmsConfig = pmsConfig;
    ::pm700Config = pm700Config;
    ::ahtConfig = ahtConfig;
    ::ens160Config = ens160Config;
    
    // Initialize sensors
    bool pmsInitialized = true;
    bool pm700Initialized = true;
    bool ahtInitialized = true;
    bool ens160Initialized = true;
    
    if (pmsConfig.enabled) {
        pmsInitialized = HAL::PMS::init();
        Serial.println("[Sensor Manager] PMS initialized");
    }
    
    if (pm700Config.enabled) {
        pm700Initialized = HAL::PM700::init();
        Serial.println("[Sensor Manager] PM700 initialized");
    }
    
    if (ahtConfig.enabled) {
        ahtInitialized = HAL::AHT::init();
        Serial.println("[Sensor Manager] AHT initialized");
    }
    
    if (ens160Config.enabled) {
        ens160Initialized = HAL::ENS160::init();
        Serial.println("[Sensor Manager] ENS initialized");
    }
    
    initialized = pmsInitialized || pm700Initialized || ahtInitialized || ens160Initialized;
    return initialized;
}

bool SensorManager::start() {
    if (!initialized) return false;
    running = true;
    Serial.println("[Sensor Manager] PMS, PM700, AHT and ENS160 started");
    return true;
}

void SensorManager::stop() {
    Serial.println("[Sensor Manager] PMS stopped");
    running = false;
}

bool SensorManager::isRunning() {
    Serial.println("[Sensor Manager] PMS running");
    return running;
}

void SensorManager::updateConfig(const SensorConfig& pmsConfig, const SensorConfig& pm700Config, const SensorConfig& ahtConfig, const SensorConfig& ens160Config) {
    bool pmsWasEnabled = ::pmsConfig.enabled;
    bool pm700WasEnabled = ::pm700Config.enabled;
    bool ahtWasEnabled = ::ahtConfig.enabled;
    bool ens160WasEnabled = ::ens160Config.enabled;
    
    // Update configurations
    ::pmsConfig = pmsConfig;
    ::pm700Config = pm700Config;
    ::ahtConfig = ahtConfig;
    ::ens160Config = ens160Config;
    
    if (running) {
        if (pmsConfig.enabled && !pmsWasEnabled) {
            HAL::PMS::init();
            Serial.println("[Sensor Manager] PMS running+initialized");
        } else if (!pmsConfig.enabled && pmsWasEnabled) {
            // HAL::PMS::sleep();
            Serial.println("[Sensor Manager] PMS disabled");
        }
        
        if (pm700Config.enabled && !pm700WasEnabled) {
            HAL::PM700::init();
            Serial.println("[Sensor Manager] PM700 running+initialized");
        } else if (!pm700Config.enabled && pm700WasEnabled) {
            Serial.println("[Sensor Manager] PM700 disabled");
        }
        
        if (ahtConfig.enabled && !ahtWasEnabled) {
            HAL::AHT::init();
            Serial.println("[Sensor Manager] AHT running+initialized");
        }
        
        if (ens160Config.enabled && !ens160WasEnabled) {
            HAL::ENS160::init();
           Serial.println("[Sensor Manager] ENS running+initialized");
        }
    }
}

const SensorManager::SensorConfig& SensorManager::getPMSConfig() {
    return pmsConfig;
}

const SensorManager::SensorConfig& SensorManager::getPM700Config() {
    return pm700Config;
}

const SensorManager::SensorConfig& SensorManager::getAHTConfig() {
    return ahtConfig;
}

const SensorManager::SensorConfig& SensorManager::getENS160Config() {
    return ens160Config;
}

void SensorManager::readPMSSensor() {
    if (!pmsConfig.enabled || !running) return;
    
    unsigned long currentTime = millis();

    if (currentTime - lastPMSRead >= pmsConfig.readInterval) {
        lastPMSRead = currentTime;
        
       if (HAL::PMS::read(&lastPMSData) == HAL::PMS::Error::NONE) {
            if (HAL::PMS::isDataAvailable()) {
                Serial.println("[Sensor Manager] PMS data available - publishing real-time to display");
                newPMSData = true;
                
                // Always publish to EventBus for real-time display updates
                publishPMSData(lastPMSData);
                
                // Track publish timing for logging only
                if (currentTime - lastPMSPublish >= pmsConfig.publishInterval) {
                    lastPMSPublish = currentTime;
                    Serial.printf("[Sensor Manager] PMS publish interval reached (%lu ms)\n", pmsConfig.publishInterval);
                }
            }
        }
    }
}

void SensorManager::readPM700Sensor() {
    if (!pm700Config.enabled || !running) return;
    
    unsigned long currentTime = millis();

    if (currentTime - lastPM700Read >= pm700Config.readInterval) {
        lastPM700Read = currentTime;
        
        if (HAL::PM700::read(&lastPM700Data) == HAL::PM700::Error::NONE) {
            if (HAL::PM700::isDataAvailable()) {
                Serial.println("[Sensor Manager] PM700 data available - publishing real-time to display");
                newPM700Data = true;
                
                // Always publish to EventBus for real-time display updates
                publishPM700Data(lastPM700Data);
                
                // Track publish timing for logging only
                if (currentTime - lastPM700Publish >= pm700Config.publishInterval) {
                    lastPM700Publish = currentTime;
                    Serial.printf("[Sensor Manager] PM700 publish interval reached (%lu ms)\n", pm700Config.publishInterval);
                }
            }
        }
    }
}

void SensorManager::readAHTSensor() {
    if (!ahtConfig.enabled || !running) return;
    
    unsigned long currentTime = millis();
    
    if (currentTime - lastAHTRead >= ahtConfig.readInterval) {
        lastAHTRead = currentTime;
    
         if (HAL::AHT::read(&lastAHTData) == HAL::AHT::Error::NONE) {
            if (HAL::AHT::isDataAvailable()) {
                Serial.println("[Sensor Manager] AHT data available - publishing real-time to display");
                newAHTData = true;
                
                // Always publish to EventBus for real-time display updates
                publishAHTData(lastAHTData);
                
                // Track publish timing for logging only
                if (currentTime - lastAHTPublish >= ahtConfig.publishInterval) {
                    lastAHTPublish = currentTime;
                    Serial.printf("[Sensor Manager] AHT publish interval reached (%lu ms)\n", ahtConfig.publishInterval);
                }
            }
        }
    }
}

void SensorManager::publishPMSData(const HAL::PMS::Data& data) {
    Serial.printf("[PMS][DATA] PM1.0=%u µg/m³, PM2.5=%u µg/m³, PM10=%u µg/m³\n",
                  data.pm1, data.pm25, data.pm10);
        
        float values[4] = {
        static_cast<float>(data.pm1),
        static_cast<float>(data.pm25),
        static_cast<float>(data.pm4),
        static_cast<float>(data.pm10)
    };
    
    CORE::SensorReadingEvent event(
        CORE::SensorReadingEvent::SensorType::PMS,
        0, 
        values,
        4  
    );
    
    CORE::EventBus::getInstance().publish(event);
}

void SensorManager::publishPM700Data(const HAL::PM700::Data& data) {
    Serial.printf("[PM700][DATA] PM1.0=%.1f µg/m³, PM2.5=%.1f µg/m³, PM10=%.1f µg/m³, 0.3µm=%.1f pcs/L\n",
                  data.pm1, data.pm25, data.pm10, data.p03);
        
    float values[5] = {
        data.pm1,
        data.pm25,
        0.0f,        // PM4 not available on PM700
        data.pm10,
        data.p03     // 0.3μm particle count
    };
    
    CORE::SensorReadingEvent event(
        CORE::SensorReadingEvent::SensorType::PM700,
        0, 
        values,
        5  
    );
    
    CORE::EventBus::getInstance().publish(event);
}

void SensorManager::publishAHTData(const HAL::AHT::Data& data) {
    float values[2] = {
        data.temperature,
        data.humidity
    };

    CORE::SensorReadingEvent event(
        CORE::SensorReadingEvent::SensorType::AHT,
        0, // Sensor ID 
        values,
        2  // Number of values
    );
    
    CORE::EventBus::getInstance().publish(event);
}

void SensorManager::readENS160Sensor() {
    if (!ens160Config.enabled || !running) return;
    
    unsigned long currentTime = millis();

    if (currentTime - lastENS160Read >= ens160Config.readInterval) {
        lastENS160Read = currentTime;
  
        if (newAHTData) {
            HAL::ENS160::setEnvironmentalData(lastAHTData.temperature, lastAHTData.humidity);
        }
        
        if (HAL::ENS160::read(&lastENS160Data) == HAL::ENS160::Error::NONE) {
            if (HAL::ENS160::isDataAvailable()) {
                Serial.println("[Sensor Manager] ENS160 data available - publishing real-time to display");
                newENS160Data = true;
                
                AQI = static_cast<float>(lastENS160Data.aqi);

                // Always publish to EventBus for real-time display updates
                publishENS160Data(lastENS160Data);
                
                // Track publish timing for logging only
                if (currentTime - lastENS160Publish >= ens160Config.publishInterval) {
                    lastENS160Publish = currentTime;
                    Serial.printf("[Sensor Manager] ENS160 publish interval reached (%lu ms)\n", ens160Config.publishInterval);
                }
            }
        }
    }
}

void SensorManager::publishENS160Data(const HAL::ENS160::Data& data) {
    float values[3] = {
        static_cast<float>(data.aqi),
        static_cast<float>(data.tvoc),
        static_cast<float>(data.eco2)
    };
    
    CORE::SensorReadingEvent event(
        CORE::SensorReadingEvent::SensorType::OTHER,
        0, // Sensor ID 
        values,
        3  // Number of values
    );
    CORE::EventBus::getInstance().publish(event);
}

void SensorManager::task(void* parameter) {
    esp_task_wdt_add(NULL);
    
    while (true) {
        esp_task_wdt_reset();
        
        if (running) {
            readPMSSensor();
            readPM700Sensor();
            readAHTSensor();
            readENS160Sensor();
        }

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

bool SensorManager::startTask() {
    if (sensorTaskHandle != nullptr) {
        return true; 
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        task,
        "SensorManager",
        4096,
        NULL,
        1,
        &sensorTaskHandle,
        0
    );
    
    return (result == pdPASS);
}

bool SensorManager::restartTask() {
    if (sensorTaskHandle != nullptr) {
        vTaskDelete(sensorTaskHandle);
        sensorTaskHandle = nullptr;
    }
    
    return startTask();
}

} // namespace APP