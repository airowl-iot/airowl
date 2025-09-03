// hal_aht.cpp - AHT Sensor HAL implementation for Airowl 3.0
#include "hal_aht.h"
#include "config.h"

#ifdef CONFIG_ENABLE_SENSOR_AHT
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#define AHT_I2C_SDA CONFIG_AHT_SDA
#define AHT_I2C_SCL CONFIG_AHT_SCL

namespace {
    // Private variables
    Adafruit_AHTX0 aht;
    bool initialized = false;
    HAL::AHT::Error lastError = HAL::AHT::Error::NONE;
    HAL::AHT::Data lastReading = {0};
    bool newDataAvailable = false;
    unsigned long lastReadTime = 0;
    const unsigned long READ_INTERVAL = 1000; 
}

namespace HAL {

bool AHT::init() {
    if (initialized) return true;
    if (!aht.begin()) {
        lastError = Error::COMM_ERROR;
        return false;
    }
    initialized = true;
    lastError = Error::NONE;
    return true;
}

AHT::Error AHT::read(Data* data) {
    if (!initialized) {
        lastError = Error::NOT_INITIALIZED;
        return lastError;
    }
    unsigned long currentTime = millis();
    if (currentTime - lastReadTime < READ_INTERVAL) {
        if (data && newDataAvailable) {
            *data = lastReading;
            return Error::NONE;
        }
        return Error::TIMEOUT;
    }

    sensors_event_t humidity, temp;
    if (!aht.getEvent(&humidity, &temp)) {
        lastError = Error::COMM_ERROR;
        return Error::COMM_ERROR;
    }
    if (data) {
        data->temperature = temp.temperature;
        data->humidity = humidity.relative_humidity;
        data->timestamp = currentTime;

        lastReading = *data;
        newDataAvailable = true;
        lastReadTime = currentTime;
    }
    
    lastError = Error::NONE;
    return Error::NONE;
}

bool AHT::isDataAvailable() {
    if (!initialized) return false;
    if (millis() - lastReadTime >= READ_INTERVAL) {
        return true;
    }
    return newDataAvailable;
}

bool AHT::isInitialized() {
    return initialized;
}

AHT::Error AHT::getLastError() {
    return lastError;
}

} // namespace HAL

#else // CONFIG_ENABLE_SENSOR_AHT not defined

namespace HAL {

bool AHT::init() { return false; }
AHT::Error AHT::read(Data*) { return Error::NOT_INITIALIZED; }
bool AHT::isDataAvailable() { return false; }
bool AHT::isInitialized() { return false; }
AHT::Error AHT::getLastError() { return Error::NOT_INITIALIZED; }

} // namespace HAL

#endif // CONFIG_ENABLE_SENSOR_AHT