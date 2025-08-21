// hal_pms.cpp - PMS Sensor HAL implementation for Airowl 3.0
#include "hal_pms.h"
#include "config.h"

#ifdef CONFIG_ENABLE_SENSOR_PMSA003A
#include <PMS.h>
#include <HardwareSerial.h>

namespace {
    // Private variables
    HardwareSerial pmsSerial(1);
    PMS pms(pmsSerial);
    PMS::DATA pmsData;

    bool initialized = false;
    HAL::PMS::Error lastError = HAL::PMS::Error::NONE;
    
    // Last valid reading
    HAL::PMS::Data lastReading = {0};
    bool newDataAvailable = false;
}

namespace HAL {

bool PMS::init() {
    if (initialized) return true;

    pmsSerial.begin(PMS_SERIAL_BAUD, SERIAL_8N1, PMS_RX_PIN, -1);
    delay(100); 

    Serial.printf("[PMS][HAL] UART1 RX=%d @ %d baud\n", PMS_RX_PIN, PMS_SERIAL_BAUD);

    pms.wakeUp();
    delay(100);
    Serial.printf("[PMS][HAL] done\n");
    pms.activeMode();
    Serial.printf("[PMS][HAL] done2\n");

    initialized = true;
    lastError = Error::NONE;
    return true;
}

PMS::Error PMS::read(Data* data) {
    if (!initialized) {
        lastError = Error::NOT_INITIALIZED;
        return lastError;
    }
    
    if (pms.read(pmsData)) { 
            data->pm1  = pmsData.PM_AE_UG_1_0;
            data->pm25 = pmsData.PM_AE_UG_2_5;
            data->pm10 = pmsData.PM_AE_UG_10_0;
            data->pm4  = 0; 
            data->timestamp = millis();
            lastReading = *data;

        Serial.printf("[PMS][HAL][RAW] PM1.0=%u µg/m³, PM2.5=%u µg/m³, PM10=%u µg/m³\n",
                      pmsData.PM_AE_UG_1_0,
                      pmsData.PM_AE_UG_2_5,
                      pmsData.PM_AE_UG_10_0);
        
        newDataAvailable = true;
        lastError = Error::NONE;
        return Error::NONE;

    } else {
        lastError = Error::TIMEOUT;
        static unsigned long lastNoDataLog = 0;
        unsigned long now = millis();
        if (now - lastNoDataLog >= 5000) {
            lastNoDataLog = now;
            Serial.println("[PMS][HAL] No data yet (timeout).\n");
        }
        return Error::TIMEOUT;
    }
}

bool PMS::isDataAvailable() {
    if (!initialized) return false;
    
    if (pmsSerial.available()) {
        newDataAvailable = true;
        return true;
    }
    
    return newDataAvailable;
}

bool PMS::isInitialized() {
    return initialized;
}

PMS::Error PMS::getLastError() {
    return lastError;
}

} // namespace HAL

#else // CONFIG_ENABLE_SENSOR_PMSA003A not defined

namespace HAL {

bool PMS::init() { return false; }
PMS::Error PMS::read(Data*) { return Error::NOT_INITIALIZED; }
bool PMS::isDataAvailable() { return false; }
bool PMS::isInitialized() { return false; }
PMS::Error PMS::getLastError() { return Error::NOT_INITIALIZED; }

} // namespace HAL

#endif // CONFIG_ENABLE_SENSOR_PMSA003A