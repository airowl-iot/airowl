// hal_pm700.cpp - Temtop PM700 Sensor HAL implementation for Airowl 3.0
#include "hal_pm700.h"
#include "airowl_config.h"
#include <HardwareSerial.h>
#include <memory>
#include "manager/event_manager.h" 

namespace {
    HardwareSerial pm700Serial(1);
    uint8_t buffer[32];

    bool initialized = false;
    HAL::PM700::Error lastError = HAL::PM700::Error::NONE;
    HAL::PM700::Data lastReading = {};
    bool newDataAvailable = false;

    bool parseFrame(const uint8_t* frame, int length, HAL::PM700::Data* data) {
        if (length < 32 || frame[0] != 0x42 || frame[1] != 0x4D) {
            return false;
        }

        uint16_t frameLen = (frame[2] << 8) | frame[3];
        if (frameLen != 28) return false;
        
        uint16_t checksum = 0;

        for (int i = 0; i < 30; i++) checksum += frame[i];
        uint16_t received = (frame[30] << 8) | frame[31];
        if (checksum != received) return false;
        
    data->pm1  = (frame[4] << 8) | frame[5];
    data->pm25 = (frame[6] << 8) | frame[7];
    data->pm10 = (frame[8] << 8) | frame[9];
    data->p03  = (frame[10] << 8) | frame[11]; 

    return true;
    }
}

namespace HAL {

bool PM700::init() {
    if (initialized) {
        Serial.println("[PM700][HAL] Already initialized");
        return true;
    }

    Serial.println("[PM700][HAL] Initializing PM700 sensor...");
    pm700Serial.begin(PM700_SERIAL_BAUD, SERIAL_8N1, PM700_RX_PIN, -1);
    vTaskDelay(pdMS_TO_TICKS(100));

    Serial.printf("[PM700][HAL] UART1 RX=%d @ %d baud\n", PM700_RX_PIN, PM700_SERIAL_BAUD);
    initialized = true;
    lastError = Error::NONE;
    Serial.println("[PM700][HAL] Init success, sensor ready");
    return true;
}

bool PM700::forceInit() {
    if (initialized) {
        Serial.println("[PM700][HAL] Already initialized");
        return true;
    }

    Serial.println("[PM700][HAL] Initializing PM700 sensor...");
    pm700Serial.begin(PM700_SERIAL_BAUD, SERIAL_8N1, PM700_RX_PIN, -1);
    vTaskDelay(pdMS_TO_TICKS(100));

    Serial.printf("[PM700][HAL] UART1 RX=%d @ %d baud\n", PM700_RX_PIN, PM700_SERIAL_BAUD);
    initialized = true;
    lastError = Error::NONE;
    Serial.println("[PM700][HAL] Init success, sensor ready");
    return true;
}

PM700::Error PM700::read(Data* data) {
    if (!initialized) {
        lastError = Error::NOT_INITIALIZED;
        return lastError;
    }

    while (pm700Serial.available() >= 32) {
        pm700Serial.readBytes(buffer,32);
        if (parseFrame(buffer, 32, data)) {
            lastReading = *data;
            newDataAvailable = true;
            lastError = Error::NONE;
        
            return Error::NONE;
        }
}

    static unsigned long lastNoDataLog = 0;
    unsigned long now = millis();
    if (now - lastNoDataLog >= 5000) {
        lastNoDataLog = now;
        Serial.println("[PM700][HAL] No valid frame yet.");
    }
    return Error::TIMEOUT;
}

bool PM700::isDataAvailable() {
    if (!initialized) return false;
    bool available = newDataAvailable;
    newDataAvailable = false; // reset flag after check
    return available;
}

bool PM700::isInitialized() { return initialized; }
PM700::Error PM700::getLastError() { return lastError; }

} // namespace HAL
