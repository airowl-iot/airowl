// sensor_manager.h - Sensor Manager for Airowl 3.0
#pragma once

#include <Arduino.h>
#include "config_manager.h"
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
    static bool init(const SensorConfig& pm700Cfg,
                     const SensorConfig& ahtCfg);
    static bool start();
    static void stop();
    static bool isRunning();

    // Config updates
    static void updateConfigFromManager();
    static void updateConfig(const SensorConfig& pm700Cfg,
                             const SensorConfig& ahtCfg);

    // Accessors
    static const SensorConfig& getPM700Config();
    static const SensorConfig& getAHTConfig();

    static float getTemperature() { return temperature; }
    static float getHumidity() { return humidity; }
    static float getAQI() { return AQI; }

    // Particulate Matter accessors
    static float getPM25();     // instantaneous PM2.5
    static float getPM25Avg();  // averaged 2-minute PM2.5
    static float getPM10();     // instantaneous PM10
    static float getPM10Avg();  // averaged 2-minute PM10

    // Historical data accessors
    static int getPM25History(float* buffer, int maxCount);  // Returns actual count
    static int getPM10History(float* buffer, int maxCount);  // Returns actual count
    static int getTempHistory(float* buffer, int maxCount);  // Returns actual count
    static int getHumdHistory(float* buffer, int maxCount);  // Returns actual count
    static int getTvocHistory(float* buffer, int maxCount); 
    static int geteCo2History(float* buffer, int maxCount); 

    // Gas concentration accessors
    static float getTVOC();     // instantaneous TVOC in ppb
    static float getCO2();      // instantaneous eCO2 in ppm

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
    static SensorConfig pm700Config;
    static SensorConfig ahtConfig;

    // Timestamps
    static unsigned long lastPM700Read, lastAHTRead;
    static unsigned long lastPM700Publish, lastAHTPublish;

    // Data + flags
    static HAL::PM700::Data lastPM700Data;
    static HAL::AHT::Data lastAHTData;
   
    static bool newPM700Data, newAHTData;

    // Global sensor values
    static float AQI;
    static float temperature;
    static float humidity;

    // Historical data buffers (2-hour history: 60 x 2-minute averages)
    static constexpr int HISTORY_BUFFER_SIZE = 60;
    static float pm25History[HISTORY_BUFFER_SIZE];
    static float pm10History[HISTORY_BUFFER_SIZE];
    static float tempHistory[HISTORY_BUFFER_SIZE];
    static float humdHistory[HISTORY_BUFFER_SIZE];
    static float tvocHistory[HISTORY_BUFFER_SIZE];
    static float eco2History[HISTORY_BUFFER_SIZE];

    static int historyIndex;      // Circular buffer index
    static int historyCount;      // Number of valid entries (0-60)

    // Temporary accumulators for 2-minute averaging
    static constexpr int TEMP_BUFFER_SIZE = 120;  // Store up to 2 minutes of readings
    static float pm25TempBuffer[TEMP_BUFFER_SIZE];
    static float pm10TempBuffer[TEMP_BUFFER_SIZE];
    static float tempTempBuffer[TEMP_BUFFER_SIZE];
    static float humdTempBuffer[TEMP_BUFFER_SIZE];
    static float tvocTempBuffer[TEMP_BUFFER_SIZE];
    static float eco2TempBuffer[TEMP_BUFFER_SIZE];

    static int pmTempIndex;
    static int pmTempCount;
    static int ahtTempIndex;
    static int ahtTempCount;

    // Mutex for buffer protection
    static SemaphoreHandle_t bufferMutex;

    static unsigned long lastMqttPublish;
    static const unsigned long MQTT_PUBLISH_INTERVAL = 120000; 

    // Averaging functions
    static void calculatePMAverages(float& avgPM25, float& avgPM10);
    static void calculateAHTAverages(float& avgTemp, float& avgHumd, float& avgTvoc, float& avgECo2);
    static void publishMqttAverages();
};

} // namespace APP
