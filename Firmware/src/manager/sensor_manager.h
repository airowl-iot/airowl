// sensor_manager.h - Sensor Manager for Airowl 3.0
#pragma once

#include <Arduino.h>
#include "config_manager.h"
#include "hal/hal_pm700.h"
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
    static bool init(const SensorConfig& pm700Cfg);
    static bool start();
    static void stop();
    static bool isRunning();

    // Config updates
    static void updateConfigFromManager();
    static void updateConfig(const SensorConfig& pm700Cfg);

    // Accessors
    static const SensorConfig& getPM700Config();

    // Particulate Matter accessors
    static float getPM1();     // instantaneous PM10
    static float getPM1Avg();  // averaged 2-minute PM10
    static float getPM25();     // instantaneous PM2.5
    static float getPM25Avg();  // averaged 2-minute PM2.5
    static float getPM10();     // instantaneous PM10
    static float getPM10Avg();  // averaged 2-minute PM10

    // Task control
    static bool startTask();
    static bool restartTask();
    static TaskHandle_t getTaskHandle() { return sensorTaskHandle; }

private:
    // Internal lifecycle
    static bool loadConfigsFromManager();
    static SensorConfig convertConfig(const ::SensorConfig& cfg);

    // Sensor handlers
    static void readPM700Sensor();

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
    static SensorConfig pm700Config;

    // Timestamps
    static unsigned long lastPM700Read;
    static unsigned long lastPM700Publish;

    // Data + flags
    static HAL::PM700::Data lastPM700Data;
   
    static bool newPM700Data;


    // Averaging buffers for MQTT (2-minute window)
    static constexpr int SENSOR_BUFFER_SIZE = 60; 
    static float pm1Buffer[SENSOR_BUFFER_SIZE]; 
    static float pm25Buffer[SENSOR_BUFFER_SIZE];
    static float pm10Buffer[SENSOR_BUFFER_SIZE];

    static int pmBufferIndex;
    static int pmBufferCount;

    // Mutex for buffer protection
    static SemaphoreHandle_t bufferMutex;

    static unsigned long lastMqttPublish;
    static const unsigned long MQTT_PUBLISH_INTERVAL = 120000; 

    // Averaging functions
    static void calculatePMAverages(float& avgPM1, float& avgPM25, float& avgPM10);
    static void publishMqttAverages();
};

} // namespace APP
