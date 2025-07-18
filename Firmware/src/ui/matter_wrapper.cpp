
#ifdef CONFIG_ESP_MATTER_ENABLE
#include "matter_wrapper.h"

#include <Matter.h>
#include <MatterEndpoints/MatterAirQualitySensor.h>
#include <MatterEndpoints/MatterTemperatureSensor.h>
#include <MatterEndpoints/MatterHumiditySensor.h>
#include <WiFi.h>

extern float temperature, humidity, AQI;

MatterAirQualitySensor air_quality_sensor;
MatterTemperatureSensor temperature_sensor;
MatterHumiditySensor humidity_sensor;

static bool matter_initialized = false;
static bool was_commissioned = false;
static uint32_t last_read_time = 0;

void initMatter() {
    if (matter_initialized) return;

    air_quality_sensor.begin(AQI);
    temperature_sensor.begin(temperature);
    humidity_sensor.begin(humidity);
    ArduinoMatter::begin();

    matter_initialized = true;
    Serial.println("[Matter] Initialization done");
}

void matter_loop() {
    if (!matter_initialized || WiFi.status() != WL_CONNECTED)
        return;

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
        air_quality_sensor.setAQI(AQI);
        temperature_sensor.setTemperature(temperature);
        humidity_sensor.setHumidity(humidity);
    }
}

bool is_matter_commissioned() {
    return ArduinoMatter::isDeviceCommissioned();
}

#endif
