// ui_controller.h - UI Controller for Airowl 3.0
#pragma once

#include <Arduino.h>
#include <functional>
#include <map>
#include "event_manager.h"
#include "../hal/hal_display.h"
#include "../hal/hal_wifi.h"

extern "C" {
    #include "ui/ui_airowl/ui.h"
}

namespace APP {

class UIController {
public:
    static bool init();
    static void stop();
    static bool isRunning();
    static bool switchScreen(const char* screenName);
    static void updateSensorDisplay(CORE::SensorReadingEvent::SensorType sensorType, 
                                       const std::vector<float>& values);
    static void updateWiFiStatus();
    static void updateQRCode();
    static void updateSensorColors(const float* pmValues);
    static void updateEyeColors(uint32_t color);
    static uint32_t getAQIColor(int aqi);
    static void updateTimeDisplay();
    static void setupPM25Chart();
    static void refreshPM25Chart();  // Refresh chart from history buffer
    static void setupPM10Chart();
    static void refreshPM10Chart();  // Refresh chart from history buffer
    static void setupTempChart();
    static void refreshTempChart(); 
    static void setupHumdChart();
    static void refreshHumdChart(); 
    static void setupTvocChart();
    static void refreshTvocChart(); 
    static void setupeCo2Chart();
    static void refresheCO2Chart(); 
    static void task(void* parameter);
    static bool startTask();
    static bool restartTask();
    static void showOTAScreen();
    static String FirmwareVersionLabel();
    static TaskHandle_t getTaskHandle();  

private:
    static void handleSensorReadingEvent(const std::shared_ptr<const CORE::Event>& event);
    static void handleCommandReceivedEvent(const std::shared_ptr<const CORE::Event>& event);

};

} // namespace APP