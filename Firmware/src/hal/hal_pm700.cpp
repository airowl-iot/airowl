// hal_pm700.cpp - Temtop PM700 Sensor HAL implementation for Airowl 3.0
#include "hal_pm700.h"
#include "config.h"

#ifdef CONFIG_ENABLE_SENSOR_PM700

#include <HardwareSerial.h>

namespace {
    // Private variables
    HardwareSerial pm700Serial(1);
    uint8_t buffer[32];
    
    bool initialized = false;
    HAL::PM700::Error lastError = HAL::PM700::Error::NONE;
    
    // Last valid reading
    HAL::PM700::Data lastReading = {0};
    bool newDataAvailable = false;
    
    // Helper function to convert bytes to float (little-endian)
    float bytesToFloat(uint8_t *b) {
        float f;
        memcpy(&f, b, 4);
        return f;
    }
    
    // Validate and parse PM700 frame
    bool parseFrame(uint8_t* frame, int length, HAL::PM700::Data* data) {
        // Check frame structure: start bytes, length, end byte
        if (length < 20 || frame[0] != 0x4A || frame[1] != 0x43 || frame[19] != 0x0D) {
            return false;
        }
        
        // Calculate and verify checksum
        uint8_t checksum = frame[18];
        uint8_t calcChecksum = 0;
        for (int i = 2; i < 18; i++) {
            calcChecksum += frame[i];
        }
        
        if (checksum != (calcChecksum & 0xFF)) {
            lastError = HAL::PM700::Error::CHECKSUM_ERROR;
            return false;
        }
        
        // Extract data
        data->pm25 = bytesToFloat(&frame[2]);
        data->pm10 = bytesToFloat(&frame[6]);
        data->pm1 = bytesToFloat(&frame[10]);
        data->p03 = bytesToFloat(&frame[14]);
        data->pm4 = 0;
        data->timestamp = millis();
        
        return true;
    }
}

namespace HAL {

bool PM700::init() {
    if (initialized) return true;

    pm700Serial.begin(PM700_SERIAL_BAUD, SERIAL_8N1, PM700_RX_PIN, -1);
    vTaskDelay(pdMS_TO_TICKS(100));

    Serial.printf("[PM700][HAL] UART1 RX=%d @ %d baud\n", PM700_RX_PIN, PM700_SERIAL_BAUD);
    Serial.println("[PM700][HAL] Temtop PM700 initialized");

    initialized = true;
    lastError = Error::NONE;
    return true;
}

PM700::Error PM700::read(Data* data) {
    if (!initialized) {
        lastError = Error::NOT_INITIALIZED;
        return lastError;
    }
    
    // Check if enough data is available for a complete frame
    if (pm700Serial.available() >= 24) {
        int idx = 0;
        
        // Read data into buffer
        while (pm700Serial.available() && idx < sizeof(buffer)) {
            buffer[idx++] = pm700Serial.read();
            
            // Wait for start byte
            if (buffer[0] != 0x4A) {
                idx = 0;
                continue;
            }
        }
        
        // Try to parse the frame
        if (idx >= 20 && parseFrame(buffer, idx, data)) {
            lastReading = *data;
            newDataAvailable = true;
            lastError = Error::NONE;
            
            Serial.printf("[PM700][HAL][RAW] PM1.0=%.1f µg/m³, PM2.5=%.1f µg/m³, PM10=%.1f µg/m³, 0.3µm=%.1f pcs/L\n",
                          data->pm1, data->pm25, data->pm10, data->p03);
            
            return Error::NONE;
        } else {
            lastError = Error::INVALID_DATA;
            return lastError;
        }
    } else {
        // Not enough data available yet
        lastError = Error::TIMEOUT;
        static unsigned long lastNoDataLog = 0;
        unsigned long now = millis();
        if (now - lastNoDataLog >= 5000) {
            lastNoDataLog = now;
            Serial.println("[PM700][HAL] No data yet (timeout).");
        }
        return Error::TIMEOUT;
    }
}

bool PM700::isDataAvailable() {
    if (!initialized) return false;
    
    if (pm700Serial.available() >= 24) {
        newDataAvailable = true;
        return true;
    }
    
    return newDataAvailable;
}

bool PM700::isInitialized() {
    return initialized;
}

PM700::Error PM700::getLastError() {
    return lastError;
}

const char* PM700::getSensorType() {
    return "Temtop PM700";
}

} // namespace HAL

#else // CONFIG_ENABLE_SENSOR_PM700 not defined

namespace HAL {

bool PM700::init() { return false; }
PM700::Error PM700::read(Data*) { return Error::NOT_INITIALIZED; }
bool PM700::isDataAvailable() { return false; }
bool PM700::isInitialized() { return false; }
PM700::Error PM700::getLastError() { return Error::NOT_INITIALIZED; }
const char* PM700::getSensorType() { return "PM700 (Disabled)"; }

} // namespace HAL

#endif // CONFIG_ENABLE_SENSOR_PM700
