
#ifdef CONFIG_ESP_MATTER_ENABLE
#include "matter_wrapper.h"

#include <Matter.h>
#include <MatterEndpoints/MatterAirQualitySensor.h>
#include <MatterEndpoints/MatterTemperatureSensor.h>
#include <MatterEndpoints/MatterHumiditySensor.h>
#include <WiFi.h>

extern float temperature, humidity, AQI;

// Use pointers to create sensors only when needed
static MatterAirQualitySensor* air_quality_sensor = nullptr;
static MatterTemperatureSensor* temperature_sensor = nullptr;
static MatterHumiditySensor* humidity_sensor = nullptr;

static bool matter_initialized = false;
static bool was_commissioned = false;
static uint32_t last_read_time = 0;

void initMatter() {
    if (matter_initialized) return;

    // Create sensors only if they don't exist
    if (!air_quality_sensor) air_quality_sensor = new MatterAirQualitySensor();
    if (!temperature_sensor) temperature_sensor = new MatterTemperatureSensor();
    if (!humidity_sensor) humidity_sensor = new MatterHumiditySensor();
    
    // Initialize sensors with current values
    air_quality_sensor->begin(AQI);
    temperature_sensor->begin(temperature);
    humidity_sensor->begin(humidity);
    
    // Initialize Matter stack
    ArduinoMatter::begin();

    matter_initialized = true;
    Serial.println("[Matter] Initialization done");
}

void matter_loop() {
    if (!matter_initialized || WiFi.status() != WL_CONNECTED)
        return;

    // Check if sensors exist
    if (!air_quality_sensor || !temperature_sensor || !humidity_sensor) {
        Serial.println("[Matter] Sensors not initialized");
        return;
    }

    if (!ArduinoMatter::isDeviceCommissioned()) {
        // Uncommissioned device – nothing to do
        return;
    }

    if (!was_commissioned) {
        Serial.println("[Matter] Node commissioned");
        was_commissioned = true;
    }

    if (millis() - last_read_time > 5000) {
        last_read_time = millis();
        air_quality_sensor->setAQI(AQI);
        temperature_sensor->setTemperature(temperature);
        humidity_sensor->setHumidity(humidity);
    }
}

bool is_matter_commissioned() {
    return ArduinoMatter::isDeviceCommissioned();
}

// Clean up Matter resources
void cleanupMatter() {
    if (air_quality_sensor) {
        delete air_quality_sensor;
        air_quality_sensor = nullptr;
    }
    
    if (temperature_sensor) {
        delete temperature_sensor;
        temperature_sensor = nullptr;
    }
    
    if (humidity_sensor) {
        delete humidity_sensor;
        humidity_sensor = nullptr;
    }
    
    matter_initialized = false;
    was_commissioned = false;
}

#endif
