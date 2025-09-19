// hal_pms.cpp - PMS Sensor HAL implementation for Airowl 3.0
#include "hal_pms.h"
#include "airowl_config.h"
#include <PMS.h>
#include <HardwareSerial.h>
#include "manager/config_manager.h"

namespace {
    // Private variables
    HardwareSerial pmsSerial(1);
    PMS pms(pmsSerial);
    PMS::DATA pmsData;

    bool initialized = false;
    HAL::PMS::Error lastError = HAL::PMS::Error::NONE;
    HAL::PMS::Data lastReading = {0};
    bool newDataAvailable = false;
}

namespace HAL {

bool PMS::init() {
    if (initialized) return true;

    pmsSerial.begin(PMS_SERIAL_BAUD, SERIAL_8N1, PMS_RX_PIN, -1);
    vTaskDelay(pdMS_TO_TICKS(100));

    Serial.printf("[PMS][HAL] UART1 RX=%d @ %d baud\n", PMS_RX_PIN, PMS_SERIAL_BAUD);

    pms.wakeUp();
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.printf("[PMS][HAL] done\n");
    pms.activeMode();

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
        
        newDataAvailable = true;
        lastError = Error::NONE;

         Serial.printf("[PMS][HAL][RAW] PM1.0=%u µg/m³, PM2.5=%u µg/m³, PM10=%u µg/m³\n",
                      pmsData.PM_AE_UG_1_0,
                      pmsData.PM_AE_UG_2_5,
                      pmsData.PM_AE_UG_10_0);

        return Error::NONE;

    } 
        lastError = Error::TIMEOUT;
        static unsigned long lastNoDataLog = 0;
        unsigned long now = millis();
        if (now - lastNoDataLog >= 5000) {
            lastNoDataLog = now;
            Serial.println("[PMS][HAL] No data yet (timeout).\n");
        }
        return Error::TIMEOUT;
}

bool PMS::isDataAvailable() {
    if (newDataAvailable) {
        newDataAvailable = false; 
        return true;
    }
    return false;
}

bool PMS::sleep() {
    if (!initialized) return false;
    pms.sleep();
    return true;
}

bool PMS::wakeup() {
    if (!initialized) return false;
    pms.wakeUp();
    return true;
}

bool PMS::isInitialized() {
    return initialized;
}

PMS::Error PMS::getLastError() {
    return lastError;
}

} // namespace HAL