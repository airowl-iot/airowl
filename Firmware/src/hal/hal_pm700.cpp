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

    unsigned long lastInvalidFrameLog = 0;
    uint32_t invalidFrameCount = 0;
    uint32_t checksumErrorCount = 0;

    bool parseFrame(const uint8_t* frame, int length, HAL::PM700::Data* data, bool logErrors = false) {
        if (length < 20 || frame[0] != 0x4A || frame[1] != 0x43 || frame[19] != 0x0D) {
            if (logErrors) {
                invalidFrameCount++;
                Serial.printf("[PM700][HAL] Invalid frame header/footer: [0]=0x%02X [1]=0x%02X [19]=0x%02X\n",
                              frame[0], frame[1], frame[19]);
            }
            return false;
        }

        uint16_t sum = 0;
        for (int i = 2; i <= 17; i++) sum += frame[i];
        uint8_t checksum = sum & 0xFF;
        if (checksum != frame[18]) {
            if (logErrors) {
                checksumErrorCount++;
                Serial.printf("[PM700][HAL] Checksum mismatch: expected=0x%02X, got=0x%02X\n",
                              checksum, frame[18]);
            }
            return false;
        }

        float pm25, pm10, pm1, particles;
        memcpy(&pm25, &frame[2], 4);
        memcpy(&pm10, &frame[6], 4);
        memcpy(&pm1,  &frame[10], 4);
        memcpy(&particles, &frame[14], 4);

        if (!isfinite(pm1) || pm1 < 0.0f || pm1 > 10000.0f) {
            if (logErrors) {
                Serial.printf("[PM700][HAL] Invalid PM1.0 value: %f\n", pm1);
            }
            return false;
        }

        if (!isfinite(pm25) || pm25 < 0.0f || pm25 > 10000.0f) {
            if (logErrors) {
                Serial.printf("[PM700][HAL] Invalid PM2.5 value: %f\n", pm25);
            }
            return false;
        }

        if (!isfinite(pm10) || pm10 < 0.0f || pm10 > 10000.0f) {
            if (logErrors) {
                Serial.printf("[PM700][HAL] Invalid PM10 value: %f\n", pm10);
            }
            return false;
        }

        if (!isfinite(particles) || particles < 0.0f || particles > 1000000.0f) {
            if (logErrors) {
                Serial.printf("[PM700][HAL] Invalid particle count: %f\n", particles);
            }
            return false;
        }

        data->pm1  = pm1;
        data->pm25 = pm25;
        data->pm10 = pm10;
        data->p03  = particles;

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
    initialized = false;
    return init();
}

PM700::Error PM700::read(Data* data) {
    if (!initialized) {
        lastError = Error::NOT_INITIALIZED;
        return lastError;
    }

    if (!data) {
        Serial.println("[PM700][HAL] ERROR: Null data pointer!");
        lastError = Error::INVALID_DATA;
        return lastError;
    }

    uint32_t framesProcessed = 0;
    uint32_t invalidFramesThisRead = 0;

    while (pm700Serial.available() >= 20) {
        pm700Serial.readBytes(buffer, 20);
        framesProcessed++;

        unsigned long now = millis();
        bool shouldLogErrors = (now - lastInvalidFrameLog >= 30000); 

        if (parseFrame(buffer, 20, data, shouldLogErrors)) {
            lastReading = *data;
            newDataAvailable = true;
            lastError = Error::NONE;

            if (shouldLogErrors && (invalidFrameCount > 0 || checksumErrorCount > 0)) {
                Serial.printf("[PM700][HAL] Valid frame received after %u invalid frames and %u checksum errors\n",
                              invalidFrameCount, checksumErrorCount);
                invalidFrameCount = 0;
                checksumErrorCount = 0;
                lastInvalidFrameLog = now;
            }

            return Error::NONE;
        } else {
            invalidFramesThisRead++;
        }
    }

    static unsigned long lastNoDataLog = 0;
    unsigned long now = millis();
    if (now - lastNoDataLog >= 30000) { 
        lastNoDataLog = now;

        if (invalidFrameCount > 0 || checksumErrorCount > 0) {
            Serial.printf("[PM700][HAL] No valid frame - Invalid: %u, Checksum errors: %u (Total processed: %u)\n",
                          invalidFrameCount, checksumErrorCount, framesProcessed);
            lastInvalidFrameLog = now;
        } else {
            // Serial.println("[PM700][HAL] No valid frame yet - no data available");
        }
    }

    return Error::TIMEOUT;
}

bool PM700::isDataAvailable() {
    if (!initialized) return false;
    bool available = newDataAvailable;
    newDataAvailable = false; 
    return available;
}

bool PM700::isInitialized() { return initialized; }
PM700::Error PM700::getLastError() { return lastError; }

} // namespace HAL
