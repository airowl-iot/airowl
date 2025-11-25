// hal_aht.cpp - AHT + ENS160 Sensor HAL implementation for Airowl 3.0
#include "hal_aht.h"
#include "airowl_config.h"
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <ScioSense_ENS160.h>
#include <memory>              
#include "manager/config_manager.h"

#define ENS160_I2C_ADDR 0x53  

namespace {
    Adafruit_AHTX0 aht;
    ScioSense_ENS160 ens160(&Wire1, ENS160_I2C_ADDR);

    bool initialized = false;
    HAL::AHT::Error lastError = HAL::AHT::Error::NONE;
    HAL::AHT::Data lastReading = {};
    bool newDataAvailable = false;
}

namespace HAL {

bool AHT::init() {
    if (initialized) {
        Serial.println("[AHT][HAL] Already initialized, skipping re-init");
        return true;
    }

    Serial.println("[AHT][HAL] Initializing sensors...");

    if (!aht.begin(&Wire1)) {
        lastError = Error::COMM_ERROR;
        Serial.println("[AHT][HAL] Init failed (AHT COMM_ERROR)");
        return false;
    }
    Serial.println("[AHT][HAL] AHT sensor detected");

    if (!ens160.begin()) {
        lastError = Error::COMM_ERROR;
        Serial.println("[ENS160][HAL] Init failed (ENS160 COMM_ERROR)");
        return false;
    }
    Serial.printf("[ENS160][HAL] Available, Rev: %d.%d.%d\n",
                  ens160.getMajorRev(),
                  ens160.getMinorRev(),
                  ens160.getBuild());

    if (!ens160.setMode(ENS160_OPMODE_STD)) {
        lastError = Error::COMM_ERROR;
        Serial.println("[ENS160][HAL] Failed to set STD mode");
        return false;
    }

    initialized = true;
    lastError = Error::NONE;
    Serial.println("[AHT][HAL] Init success, sensors ready");
    return true;
}

AHT::Error AHT::read(Data* data) {
    if (!initialized) {
        lastError = Error::NOT_INITIALIZED;
        Serial.println("[AHT][HAL] Read failed: NOT_INITIALIZED");
        return lastError;
    }

    sensors_event_t humidityEvent, tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);

    if (!ens160.available()) {
        // Serial.println("[ENS160][HAL] Not available, attempting to restore...");
        ens160.setMode(ENS160_OPMODE_STD);
    }

    // Serial.printf("[AHT][HAL] Temp=%.2f °C, Humidity=%.2f %%RH\n",
    //               tempEvent.temperature, humidityEvent.relative_humidity);

    ens160.set_envdata(tempEvent.temperature, humidityEvent.relative_humidity);
    ens160.measure(true);

    if (data) {
        data->temperature = tempEvent.temperature;
        data->humidity    = humidityEvent.relative_humidity;

        data->aqi  = ens160.getAQI();
        data->tvoc = ens160.getTVOC();
        data->eco2 = ens160.geteCO2();

        if (data->aqi == 0 && data->tvoc == 0 && data->eco2 == 0) {
            Serial.println("[ENS160][HAL] Sensor returned zeros, attempting recovery...");
            if (ens160.setMode(ENS160_OPMODE_STD)) {
                vTaskDelay(pdMS_TO_TICKS(50));
                ens160.set_envdata(tempEvent.temperature, humidityEvent.relative_humidity);
                ens160.measure(true);
                vTaskDelay(pdMS_TO_TICKS(50));
                data->aqi  = ens160.getAQI();
                data->tvoc = ens160.getTVOC();
                data->eco2 = ens160.geteCO2();
                Serial.println("[ENS160][HAL] Recovery attempted");
            }
        }

        lastReading = *data;
        newDataAvailable = true;

        // Serial.printf("[ENS160][HAL] AQI=%d, TVOC=%d ppb, eCO2=%d ppm\n",
        //               data->aqi, data->tvoc, data->eco2);
    }

    lastError = Error::NONE;
    return Error::NONE;
}

bool AHT::isDataAvailable() {
    if (newDataAvailable) {
        newDataAvailable = false; 
        // Serial.println("[AHT][HAL] New data available");
        return true;
    }
    return false;
}

bool AHT::isInitialized() {
    return initialized;
}

AHT::Error AHT::getLastError() {
    return lastError;
}

} // namespace HAL
