// hal_ens160.cpp - ENS160 Air Quality Sensor HAL implementation for Airowl 3.0
#include "hal_ens160.h"
#include "config.h"

#ifndef CONFIG_ENABLE_SENSOR_ENS160
#include <Wire.h>
#include <ScioSense_ENS160.h>

// ENS160 sensor configuration
#define ENS160_I2C_SDA CONFIG_ENS160_SDA
#define ENS160_I2C_SCL CONFIG_ENS160_SCL
#define ENS160_I2C_ADDR 0x53  

namespace {
    // Private variables
    ScioSense_ENS160 ens160(ENS160_I2C_ADDR);
    bool initialized = false;
    HAL::ENS160::Error lastError = HAL::ENS160::Error::NONE;
    
    // Last valid reading
    HAL::ENS160::Data lastReading = {0};
    bool newDataAvailable = false;
    unsigned long lastReadTime = 0;
    const unsigned long READ_INTERVAL = 1000; 
}

namespace HAL {

bool ENS160::init() {
    if (initialized) return true;
    if (!ens160.begin()) {
        lastError = Error::COMM_ERROR;
        Serial.println("[ENS160][HAL] Init failed (COMM_ERROR)");
        return false;
    }
    if (!ens160.setMode(ENS160_OPMODE_STD)) {
        lastError = Error::COMM_ERROR;
        Serial.println("[ENS160][HAL] Failed to set STD mode (COMM_ERROR)");
        return false;
    }
    initialized = true;
    lastError = Error::NONE;
    Serial.println("[ENS160][HAL] Init success, running in STD mode");
    return true;
}

ENS160::Error ENS160::read(Data* data) {
    if (!initialized) {
        lastError = Error::NOT_INITIALIZED;
        return lastError;
    }
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime < READ_INTERVAL) {
        if (data) *data = lastReading;
        return lastError;
    }
    
    lastReadTime = currentTime;
    if (!ens160.measure(true)) {
        lastError = Error::COMM_ERROR;
        Serial.println("[ENS160][HAL] Measurement failed (COMM_ERROR)");
        return lastError;
    }  
    lastReading.aqi = ens160.getAQI();
    lastReading.tvoc = ens160.getTVOC();
    lastReading.eco2 = ens160.geteCO2();
    lastReading.timestamp = currentTime;    
    newDataAvailable = true;

    if (data) *data = lastReading;
    Serial.printf("[ENS160][HAL][RAW] AQI=%u, TVOC=%u ppb, eCO2=%u ppm\n",
                  lastReading.aqi,
                  lastReading.tvoc,
                  lastReading.eco2);
    lastError = Error::NONE;
    return lastError;
}

ENS160::Error ENS160::setEnvironmentalData(float temperature, float humidity) {
    if (!initialized) {
        lastError = Error::NOT_INITIALIZED;
        return lastError;
    }
    if (!ens160.set_envdata(temperature, humidity)) {
        lastError = Error::COMM_ERROR;
        return lastError;
    }
    lastError = Error::NONE;
    return lastError;
}

bool ENS160::isDataAvailable() {
    bool available = newDataAvailable;
    newDataAvailable = false;
    return available;
}

bool ENS160::isInitialized() {
    return initialized;
}

} // namespace HAL

#else // CONFIG_ENABLE_SENSOR_ENS160

namespace HAL {

bool ENS160::init() { return false; }
ENS160::Error ENS160::read(Data* data) { return Error::NOT_INITIALIZED; }
ENS160::Error ENS160::setEnvironmentalData(float temperature, float humidity) { return Error::NOT_INITIALIZED; }
bool ENS160::isDataAvailable() { return false; }
bool ENS160::isInitialized() { return false; }

} // namespace HAL

#endif // CONFIG_ENABLE_SENSOR_ENS160