// sensor_manager.h - Sensor Manager for Airowl 3.0
#pragma once

#include <Arduino.h>
#include "config_manager.h"
#include "hal/hal_pms.h"
#include "hal/hal_pm700.h"
#include "hal/hal_aht.h"
#include "event_manager.h"

namespace APP {

class SensorManager {
public:
    struct SensorConfig {
        bool enabled{false};
        unsigned long readInterval{10000};
        unsigned long publishInterval{60000};
    };

    // Lifecycle
    static bool init();
    static bool init(const SensorConfig& pmsCfg,
                     const SensorConfig& pm700Cfg,
                     const SensorConfig& ahtCfg);
    static bool start();
    static void stop();
    static bool isRunning();

    // Config updates
    static void updateConfigFromManager();
    static void updateConfig(const SensorConfig& pmsCfg,
                             const SensorConfig& pm700Cfg,
                             const SensorConfig& ahtCfg);

    // Accessors
    static const SensorConfig& getPMSConfig();
    static const SensorConfig& getPM700Config();
    static const SensorConfig& getAHTConfig();

    // Task control
    static bool startTask();
    static bool restartTask();
    static TaskHandle_t getTaskHandle() { return sensorTaskHandle; }

private:
    // Internal lifecycle
    static bool loadConfigsFromManager();
    static SensorConfig convertConfig(const ::SensorConfig& cfg);

    // Sensor handlers
    static void readPMSSensor();
    static void readPM700Sensor();
    static void readAHTSensor();

    template<typename DataT>
    static void publishData(CORE::Event::Type type,
                          const DataT& data,
                          const float* values,
                          size_t count);
                          
    static void task(void* parameter);

    // Internal state
    static bool initialized;
    static bool running;
    static TaskHandle_t sensorTaskHandle;

    // Configs
    static SensorConfig pmsConfig;
    static SensorConfig pm700Config;
    static SensorConfig ahtConfig;

    // Timestamps
    static unsigned long lastPMSRead, lastPM700Read, lastAHTRead;
    static unsigned long lastPMSPublish, lastPM700Publish, lastAHTPublish;

    // Data + flags
    static HAL::PMS::Data lastPMSData;
    static HAL::PM700::Data lastPM700Data;
    static HAL::AHT::Data lastAHTData;
   
    static bool newPMSData, newPM700Data, newAHTData;

    // Global sensor values
    static float AQI;
    static float temperature;
    static float humidity;

    // Averaging buffers for MQTT (2-minute window)
    static constexpr int SENSOR_BUFFER_SIZE = 60;  
    static float pm25Buffer[SENSOR_BUFFER_SIZE];
    static float pm10Buffer[SENSOR_BUFFER_SIZE];
    static float tempBuffer[SENSOR_BUFFER_SIZE];
    static float humdBuffer[SENSOR_BUFFER_SIZE];
    static float tvocBuffer[SENSOR_BUFFER_SIZE];
    static float eco2Buffer[SENSOR_BUFFER_SIZE];

    static int pmBufferIndex;
    static int pmBufferCount;
    static int ahtBufferIndex;
    static int ahtBufferCount;

    static unsigned long lastMqttPublish;
    static const unsigned long MQTT_PUBLISH_INTERVAL = 120000; 

    // Averaging functions
    static void calculatePMAverages(float& avgPM25, float& avgPM10);
    static void calculateAHTAverages(float& avgTemp, float& avgHumd, float& avgTvoc, float& avgECo2);
    static void publishMqttAverages();
};

} // namespace APP
