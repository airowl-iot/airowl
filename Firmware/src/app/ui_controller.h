// ui_controller.h - UI Controller for Airowl 3.0
#pragma once

#include <Arduino.h>
#include <functional>
#include <map>
#include "../core/event_bus.h"
#include "../hal/hal_display.h"
#include "../hal/hal_wifi.h"

extern "C" {
    #include "../ui/ui.h"
}

namespace APP {

class UIController {
public:
    static bool init();
    static bool start();
    static void stop();
    static bool isRunning();
    static uint8_t getPoint();
    static bool switchScreen(const char* screenName);
    static void updateSensorDisplay(CORE::SensorReadingEvent::SensorType sensorType, 
                                   const float* values, uint8_t valueCount);
    static void updateWiFiStatus();
    static void updateQRCode();
    static void updateSensorColors(const float* pmValues);
    static void updateEyeColors(uint32_t color);
    static uint32_t getAQIColor(int aqi);
    static int calculateAQI(const float* pmValues);
    static void updateTimeDisplay();
    static void setupPM1Chart();
    static void updatePM1Chart(float pm1_value);
    static void setupPM25Chart();
    static void updatePM25Chart(float pm25_value);
    static void setupPM10Chart();
    static void updatePM10Chart(float pm10_value);
    static void task(void* parameter);
    static bool startTask();
    static bool restartTask();

private:
    static void handleSensorReadingEvent(const CORE::Event& event);
    static void handleCommandReceivedEvent(const CORE::Event& event);
};

} // namespace APP