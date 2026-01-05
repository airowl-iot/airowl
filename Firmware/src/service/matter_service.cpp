#include "matter_service.h"
#include <Matter.h>
#include <MatterEndpoints/MatterAirQualitySensor.h>
#include <MatterEndpoints/MatterTemperatureSensor.h>
#include <MatterEndpoints/MatterHumiditySensor.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include "manager/sensor_manager.h"
#include "manager/event_manager.h"
#include "manager/ui_manager.h"

namespace {

    bool initialized = false;
    bool commissioned = false;

    TaskHandle_t matterTaskHandle = nullptr;
    
    MatterAirQualitySensor airSensor;       

    static const uint32_t UPDATE_INTERVAL_MS = 30000;
    static uint32_t lastUpdateTime = 0;     

    inline void log(const char* msg) {
        Serial.printf("[Matter] %s\n", msg);
    }

} // namespace

namespace SVC {
namespace Matter {

bool init() {
    if (initialized) return true;

    log("Initializing Matter Service…");

    float pm1 = APP::SensorManager::getPM1();
    float pm25 = APP::SensorManager::getPM25();
    float pm10 = APP::SensorManager::getPM10();

    uint8_t enabledMeasurements =  MatterAirQualitySensor::ENABLE_PM1 |
                                   MatterAirQualitySensor::ENABLE_PM25 |
                                   MatterAirQualitySensor::ENABLE_PM10;


    ArduinoMatter::begin();
    initialized = true;

    log("Matter stack initialized with air quality sensor (PM1.0 PM2.5, PM10)");
    Serial.printf("[Matter] Initial values -PM1.0:%.1fµg/m³ PM2.5:%.1fµg/m³ PM10:%.1fµg/m³\n", 
                  pm1, pm25, pm10);

    CORE::EventBus::getInstance().publish(
        std::make_shared<CORE::MatterStateChangedEvent>("Matter stack initialized")
    );

    return true;
}

bool startTask() {
    if (matterTaskHandle) return true;

    BaseType_t ok = xTaskCreatePinnedToCore(
        task,
        "MatterService",
        6144,
        nullptr,
        1,
        &matterTaskHandle,
        0
    );

    if (ok != pdPASS) {
        log("ERROR: Failed to create Matter task");
        return false;
    }

    log("Matter task started");
    return true;
}

void task(void* parameter) {
    log("Task loop running");

    CORE::EventBus::getInstance().publish(
        std::make_shared<CORE::MatterStateChangedEvent>("Matter task started")
    );

    while (true) {

        if (!initialized || WiFi.status() != WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        bool nowCommissioned = ArduinoMatter::isDeviceCommissioned();
        if (nowCommissioned && !commissioned) {
            commissioned = true;
            log("Node has been commissioned!");

            auto evt = std::make_shared<CORE::MatterCommissionedEvent>();
            CORE::EventBus::getInstance().publish(evt);
        }

        if (!commissioned) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        uint32_t now = millis();
        if (now - lastUpdateTime >= UPDATE_INTERVAL_MS) {
            lastUpdateTime = now;

            float pm1 = APP::SensorManager::getPM1();
            float pm25 = APP::SensorManager::getPM25();
            float pm10 = APP::SensorManager::getPM10();

            airSensor.setPM1(pm1);
            airSensor.setPM25(pm25);
            airSensor.setPM10(pm10);

            Serial.printf("[Matter] Updated: PM1.0=%.1f PM2.5=%.1f PM10=%.1f\n", 
                         pm1, pm25, pm10);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

bool isCommissioned() {
    return commissioned;
}

String getManualPairingCode() {
    return ArduinoMatter::getManualPairingCode();
}

String getQRCodeData() {
    String qrUrl = ArduinoMatter::getOnboardingQRCodeUrl();

    int dataStart = qrUrl.indexOf("?data=");
    if (dataStart != -1) {
        return qrUrl.substring(dataStart + 6); 
    }

    return "MT:Y.K9042C00KA0648G00";
}

bool restartTask() {
    if (matterTaskHandle) {
        vTaskDelete(matterTaskHandle);
        matterTaskHandle = nullptr;
    }
    return startTask();
}

} // namespace Matter
} // namespace SVC

extern "C" {
    bool is_matter_commissioned() {
        return SVC::Matter::isCommissioned();
    }

    const char* get_matter_pairing_code() {
        static String pairingCode;
        pairingCode = SVC::Matter::getManualPairingCode();
        return pairingCode.c_str();
    }

    const char* get_matter_qrcode_data() {
        static String qrCodeData;
        qrCodeData = SVC::Matter::getQRCodeData();
        return qrCodeData.c_str();
    }
}
