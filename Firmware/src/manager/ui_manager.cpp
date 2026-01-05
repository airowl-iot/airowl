// ui_controller.cpp - UI Controller implementation for Airowl 3.0
#include "ui_manager.h"
#include "../hal/hal_display.h"
#include "../hal/hal_wifi.h"
#include "../service/mqtt_service.h"
#include <esp_task_wdt.h>
#include <time.h>
#include <esp_sntp.h>
#include "../service/ota_service.h"
#include "ui/ui_airowl/ui.h"
#include "manager/config_manager.h"

namespace APP{

    struct ScreenMap {
    const char* name;
    lv_obj_t* screen;
    };
    static ScreenMap screens[] = {
        {"dashboard", ui_dashboard},
        {"intro", ui_Intro},
        {"qrcode", ui_qrcode},
        {"matter", ui_matter},
        {"owl", ui_owl},
        {"pm1graph", ui_PM1graph},
        {"pm25graph", ui_PM25graph},
        {"pm10graph", ui_PM10graph},
        {"ota", ui_ota}
    };
   
    bool initialized = false;
    bool running = false;
    bool otaInProgress = false;
    TaskHandle_t uiTaskHandle = nullptr;

    lv_obj_t* qrCodeObj = nullptr;
    String lastQRCodeUrl = "";
    String storedApName = ""; 

    uint32_t sensorReadingSubscriptionId = 0;
    uint32_t commandReceivedSubscriptionId = 0;
    uint32_t wifiStateSubscriptionId = 0;
    uint32_t otaProgressSubscriptionId = 0;

    static float lastPM1 = 0.0, lastPM25 = 0.0, lastPM10 = 0.0;

    static void ui_event_WifiIcon(lv_event_t * e) { 
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (HAL::WiFi::getStatus() != HAL::WiFi::Status::CONNECTED) {
            _ui_screen_change(&ui_qrcode, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_qrcode_screen_init);
        }
    }   
}

    unsigned long bootTime = 0;
    const unsigned long EYE_COLOR_ACTIVATION_DELAY = 60000; 
    
    #define CHART_DATA_LENGTH 12

    lv_chart_series_t *ui_PM1chart_series_1 = nullptr;
    static lv_coord_t ui_PM1chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm1_sum = 0.0;
    static int pm1_count = 0;

    lv_chart_series_t *ui_PM25chart_series_1 = nullptr;
    static lv_coord_t ui_PM25chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm25_sum = 0.0;
    static int pm25_count = 0;

    lv_chart_series_t *ui_PM10chart_series_1 = nullptr;
    static lv_coord_t ui_PM10chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm10_sum = 0.0;
    static int pm10_count = 0;

    struct AQIBreakpoint {
        float low;
        float high;
        int aqiLow;
        int aqiHigh;
    };
    
    const AQIBreakpoint pm1Bps[] = {
    {0.0, 10.0, 0, 50},      // Good
    {10.1, 20.0, 51, 100},  // Moderate
    {20.1, 30.0, 101, 150}, // Unhealthy for Sensitive
    {30.1, 50.0, 151, 200}, // Unhealthy
    {50.1, 75.0, 201, 300}, // Very Unhealthy
    {75.1, 500.0, 301, 500} // Hazardous
    };

    const AQIBreakpoint pm25Bps[] = {
        {0.0, 15.9, 0, 50},      // Good: 0-15.9 µg/m³
        {16.0, 25.9, 51, 100},   // Moderate: 16-25.9 µg/m³
        {26.0, 37.9, 101, 150},  // Unhealthy for Sensitive: 26-37.9 µg/m³
        {38.0, 50.9, 151, 200},  // Unhealthy: 38-50.9 µg/m³
        {51.0, 75.9, 201, 300},  // Very Unhealthy: 51-75.9 µg/m³
        {76.0, 500.0, 301, 500}  // Hazardous: 76+ µg/m³
    };
    
    const AQIBreakpoint pm10Bps[] = {
        {0.0, 54.9, 0, 50},
        {55.0, 154.9, 51, 100},
        {155.0, 254.9, 101, 150},
        {255.0, 354.9, 151, 200},
        {355.0, 424.9, 201, 300},
        {425.0, 604.9, 301, 500}
    };
    
    AQIBreakpoint getBreakpoint(float value, const AQIBreakpoint* breakpoints, int count) {
        for (int i = 0; i < count; i++) {
            if (value >= breakpoints[i].low && value <= breakpoints[i].high) {
                return breakpoints[i];
            }
        }
        return breakpoints[count - 1];
    }
    
    int calculateSubIndex(float value, const AQIBreakpoint& bp) {
    return (int)(((float)(bp.aqiHigh - bp.aqiLow) / (bp.high - bp.low)) * (value - bp.low) + bp.aqiLow);
    }

    void UIController::setupPM1Chart() {
        
        if (ui_pm1Chart != nullptr) {
            lv_chart_set_type(ui_pm1Chart, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_pm1Chart, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_pm1Chart, LV_CHART_AXIS_PRIMARY_Y, 0, 150); 

            ui_PM1chart_series_1 = lv_chart_add_series(
                ui_pm1Chart, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_pm1Chart, ui_PM1chart_series_1,
                                   ui_PM1chart_series_1_array);

           for (int i = 0; i < CHART_DATA_LENGTH; i++) ui_PM1chart_series_1_array[i] = 0;

        Serial.println("[UIController] PM1 Chart initialized");
    }
}

    void UIController::updatePM1Chart(float pm1_value) {

        if (ui_PM1chart_series_1 != nullptr && ui_pm1Chart != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_PM1chart_series_1_array[i] = ui_PM1chart_series_1_array[i + 1];
            }
            ui_PM1chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(pm1_value);

            pm1_sum += pm1_value;
            pm1_count++;
            float pm1_avg = pm1_sum / pm1_count;

            if (ui_pm1maxvalue != nullptr) {
                char avgBuffer[8];
                dtostrf(pm1_avg, 4, 1, avgBuffer);
                lv_label_set_text(ui_pm1maxvalue, avgBuffer);
            }

            lv_chart_refresh(ui_pm1Chart);
        }

}
   void UIController::setupPM25Chart() {
        
        if (ui_Chart3 != nullptr) {
            lv_chart_set_type(ui_Chart3, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_Chart3, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_Chart3, LV_CHART_AXIS_PRIMARY_Y, 0, 100); 

            ui_PM25chart_series_1 = lv_chart_add_series(
                ui_Chart3, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_Chart3, ui_PM25chart_series_1,
                                   ui_PM25chart_series_1_array);

            for (int i = 0; i < CHART_DATA_LENGTH; i++) ui_PM25chart_series_1_array[i] = 0;

            Serial.println("[UIController] PM2.5 Chart initialized");
        }
        
    }
    

     void UIController::updatePM25Chart(float pm25_value) {

        if (ui_PM25chart_series_1 != nullptr && ui_Chart3 != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_PM25chart_series_1_array[i] = ui_PM25chart_series_1_array[i + 1];
            }

            ui_PM25chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(pm25_value);

            pm25_sum += pm25_value;
            pm25_count++;
            float pm25_avg = pm25_sum / pm25_count;

            if (ui_pm25maxvalue != nullptr) {
                char avgBuffer[8];
                dtostrf(pm25_avg, 4, 1, avgBuffer);
                lv_label_set_text(ui_pm25maxvalue, avgBuffer);
            }

            lv_chart_refresh(ui_Chart3);
        }

    }
    
    void UIController::setupPM10Chart() {
        
        if (ui_pmChart != nullptr) {
            lv_chart_set_type(ui_pmChart, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_pmChart, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_pmChart, LV_CHART_AXIS_PRIMARY_Y, 0, 150); 

            ui_PM10chart_series_1 = lv_chart_add_series(
                ui_pmChart, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_pmChart, ui_PM10chart_series_1,
                                   ui_PM10chart_series_1_array);

           for (int i = 0; i < CHART_DATA_LENGTH; i++) ui_PM10chart_series_1_array[i] = 0;

        Serial.println("[UIController] PM10 Chart initialized");
    }
}

    void UIController::updatePM10Chart(float pm10_value) {

        if (ui_PM10chart_series_1 != nullptr && ui_pmChart != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_PM10chart_series_1_array[i] = ui_PM10chart_series_1_array[i + 1];
            }
            ui_PM10chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(pm10_value);

            pm10_sum += pm10_value;
            pm10_count++;
            float pm10_avg = pm10_sum / pm10_count;

            if (ui_pm10maxvalue != nullptr) {
                char avgBuffer[8];
                dtostrf(pm10_avg, 4, 1, avgBuffer);
                lv_label_set_text(ui_pm10maxvalue, avgBuffer);
            }

            lv_chart_refresh(ui_pmChart);
        }

}

void UIController::showOTAScreen() {
    Serial.println("[UIController] OTA mode activated - freezing UI updates");

    otaInProgress = true;

    lv_anim_del_all();
    Serial.println("[UIController] Cleared all LVGL animations");

    if (ui_ota != nullptr) {
        lv_scr_load(ui_ota);
        Serial.println("[UIController] Switched to OTA screen (locked)");
    } else {
        Serial.println("[UIController] ERROR: ui_ota is NULL!");
    }
}

bool UIController::init() {
    if (initialized) {
        Serial.println("[UIController] Already initialized, returning true");
        return true;
    }

    Serial.println("[UIController] Starting initialization...");
    
    bootTime = millis();
    configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); 
    Serial.println("[UIController] NTP time synchronization started");
    
    Serial.println("[UIController] Initializing display...");
    if (!HAL::Display::init()) {
        Serial.println("[UIController] Display/LVGL init failed!");
        return false;
    }
    
    ui_Intro_screen_init();
    ui_qrcode_screen_init();
    ui_matter_screen_init();
    ui_owl_screen_init();
    ui_dashboard_screen_init();
    ui_PM1graph_screen_init();
    ui_PM25graph_screen_init();
    ui_PM10graph_screen_init();
    ui_ota_screen_init();
    Serial.println("[UIController] All UI screens initialized");

    setupPM1Chart();
    setupPM25Chart();
    setupPM10Chart();

    if (ui_Intro != nullptr) {
        lv_disp_load_scr(ui_Intro);
        Serial.println("[UIController] Loaded intro screen");
    } else {
        Serial.println("[UIController] ERROR: ui_Intro is NULL!");
    }

    CORE::EventBus& eventBus = CORE::EventBus::getInstance();
    
    sensorReadingSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::SENSOR_READING,
        handleSensorReadingEvent
    );
    
    commandReceivedSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::COMMAND_RECEIVED,
        handleCommandReceivedEvent
    );

    wifiStateSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::WIFI_STATE_CHANGED,
        [](const std::shared_ptr<const CORE::Event>& event) {
            auto wifiEvent = std::static_pointer_cast<const CORE::WiFiStateChangedEvent>(event);
            Serial.printf("[UIController] WiFi state changed: %d, SSID: %s, RSSI: %d\n",
                          (int)wifiEvent->getState(), wifiEvent->getSSID().c_str(), wifiEvent->getRSSI());
            UIController::updateWiFiStatus(); 
        }
    );

    otaProgressSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::OTA_PROGRESS,
        [](const std::shared_ptr<const CORE::Event>& event) {
            auto otaEvent = std::static_pointer_cast<const CORE::OTAProgressEvent>(event);
            Serial.printf("[UIController] OTA Progress: %zu/%zu bytes\n",
                          otaEvent->getWritten(), otaEvent->getTotal());
        }
    );

    Serial.println("[UIController] Event subscriptions registered");
    Serial.printf("[UIController] Subscriptions - Sensor:%u, Command:%u, WiFi:%u, MQTT:%u, OTA:%u\n",
                  sensorReadingSubscriptionId, commandReceivedSubscriptionId,
                  wifiStateSubscriptionId, otaProgressSubscriptionId);

    SVC::OTA::onProgress([](SVC::OTA::State state, int progress, const char* message) {
    Serial.printf("[UIController][OTA] State=%d, Progress=%d, Msg=%s\n", (int)state, progress, message);

    if (state == SVC::OTA::State::DOWNLOADING || state == SVC::OTA::State::UPDATING) {
        APP::UIController::showOTAScreen();
    }
});

    initialized = true;
    running = true;

    bool taskStarted = startTask();
    Serial.printf("[UIController] Task creation %s\n", taskStarted ? "SUCCESS" : "FAILED");
    return taskStarted;
}

bool UIController::restartTask() {
    if (uiTaskHandle != nullptr) {
        TaskHandle_t tempHandle = uiTaskHandle;
        uiTaskHandle = nullptr;  
        vTaskDelete(tempHandle);
        vTaskDelay(pdMS_TO_TICKS(100)); 
        Serial.println("[UIController] Task deleted, restarting...");
    }
    return startTask();
}

void UIController::stop() {
    running = false;
}

bool UIController::isRunning() {
    return running;
}

bool UIController::switchScreen(const char* screenName) {
    for (auto& s : screens) {
        if (strcmp(s.name, screenName) == 0&& s.screen != nullptr) {
            lv_scr_load(s.screen);

            if (strcmp(screenName, "qrcode") == 0) {
                updateQRCode();
            }
            
            return true;
        }
    }
    return false; 
}

void UIController::updateSensorDisplay(CORE::SensorReadingEvent::SensorType sensorType,
                                       const std::vector<float>& values) {
    if (!running || values.empty()) {
        Serial.printf("[UIController] updateSensorDisplay skipped - running:%d, values:%zu\n",
                      running, values.size());
        return;
    }

    if (otaInProgress) {
        return;
    }

    // Serial.printf("[UIController] updateSensorDisplay called - type:%d, values:%zu\n",
    //               (int)sensorType, values.size());
    
    switch (sensorType) {
        case CORE::SensorReadingEvent::SensorType::PM700:
            if (values.size() >= 4) {
                
                char pm1buffer[16],  pm25buffer[16], pm10buffer[16];
                dtostrf(values[0], 6, 1, pm1buffer);
                dtostrf(values[1], 6, 1, pm25buffer); 
                dtostrf(values[2], 6, 1, pm10buffer);
                
                lv_label_set_text(ui_pm1value, pm1buffer);
                lv_label_set_text(ui_pm25value, pm25buffer);
                lv_label_set_text(ui_pm10value, pm10buffer);

                updatePM1Chart(values[0]);
                updatePM25Chart(values[1]);
                updatePM10Chart(values[2]);
                
                lastPM1 = values[0];
                lastPM25 = values[1];
                lastPM10 = values[2];

                updateSensorColors(values.data());
                
            }
            break;
        default:
            break;
    }
}

void UIController::updateWiFiStatus() {
    if (!running) return;

    bool isConnected = (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED);

    if (ui_nose != nullptr) {
        if (isConnected) {
            lv_img_set_src(ui_nose, &ui_img_airowl_2_png);  
        } else {
            lv_img_set_src(ui_nose, &ui_img_airowl_1_png);  
        }
    }

    if (ui_wifi != nullptr) {
        if (isConnected) {
            lv_img_set_src(ui_wifi, &ui_img_wifi_on_png);
            lv_obj_remove_event_cb(ui_wifi, ui_event_WifiIcon);
            lv_obj_clear_flag(ui_wifi, LV_OBJ_FLAG_CLICKABLE);
        } else {
            lv_img_set_src(ui_wifi, &ui_img_wifi_off_png);
            lv_obj_add_event_cb(ui_wifi, ui_event_WifiIcon, LV_EVENT_CLICKED, nullptr);
            lv_obj_add_flag(ui_wifi, LV_OBJ_FLAG_CLICKABLE);
        }
    }
}

void UIController::updateQRCode() {
    if (!running || !ui_qrcode) return;

    String apName = storedApName.isEmpty() ? HAL::WiFi::generateApName() : storedApName;
    String qrcodeurl = (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) 
        ? "https://opendata.oizom.com/device/" + apName 
        : "WIFI:T:WPA;S:" + apName + ";P:12345678;;";

    // Serial.printf("[UIController] QR Code URL: %s\n", qrcodeurl.c_str());
    
    if (qrcodeurl != lastQRCodeUrl || qrCodeObj == nullptr) {

        if (qrCodeObj == nullptr) {
            qrCodeObj = lv_qrcode_create(ui_qrcode, 150, lv_color_black(), lv_color_white());
            lv_obj_center(qrCodeObj);
        }
        
        lv_qrcode_update(qrCodeObj, qrcodeurl.c_str(), qrcodeurl.length());
        lv_obj_center(qrCodeObj);

        lastQRCodeUrl = qrcodeurl;
        Serial.printf("[UIController] UI labels initialized with AP name: %s\n", apName);
        
    }
        lv_label_set_text(ui_devicename, apName.c_str());
        lv_label_set_text(ui_qrcodename, apName.c_str());
        // lv_label_set_text(ui_firmwareversion, "4.0.0");  
        
}

void UIController::updateSensorColors(const float* pmValues) {
    if (!running) return;
    
    AQIBreakpoint pm1Bp = getBreakpoint(pmValues[0], pm1Bps, sizeof(pm1Bps) / sizeof(pm1Bps[0]));
    AQIBreakpoint pm25Bp = getBreakpoint(pmValues[1], pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
    AQIBreakpoint pm10Bp = getBreakpoint(pmValues[2], pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
 
    int pm1Index = calculateSubIndex(pmValues[0], pm1Bp);
    int pm25Index = calculateSubIndex(pmValues[1], pm25Bp);
    int pm10Index = calculateSubIndex(pmValues[2], pm10Bp);
    uint32_t  pm1_color, pm25_color, pm10_color;

    if (millis() - bootTime < EYE_COLOR_ACTIVATION_DELAY) {
       pm1_color = pm25_color = pm10_color =  0xFFFFFF; 
    } else {
        pm1_color = getAQIColor(pm1Index);
        pm25_color = getAQIColor(pm25Index);
        pm10_color = getAQIColor(pm10Index);
    }
    lv_obj_set_style_text_color(ui_pm1value, lv_color_hex(pm1_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_pm25value, lv_color_hex(pm25_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_pm10value, lv_color_hex(pm10_color), LV_PART_MAIN | LV_STATE_DEFAULT);

    int overallAQI = max(pm25Index, pm10Index);

    uint32_t eyeColor = (millis() - bootTime < EYE_COLOR_ACTIVATION_DELAY) ? 0xFFFFFF : getAQIColor(overallAQI);
    updateEyeColors(eyeColor);
    
}

void UIController::updateEyeColors(uint32_t color) {
    if (ui_lefteye && ui_righteye) {
        lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
}

uint32_t UIController::getAQIColor(int aqi) {
    if (aqi <= 50) return 0x00B050;   
    else if (aqi <= 100) return 0x92D050;   
    else if (aqi <= 200) return 0xFFFF00;   // Moderate (Yellow)
    else if (aqi <= 300) return 0xFF9900;   // Poor (Orange)
    else if (aqi <= 400) return 0xFF0000;   // Very Poor (Red)
    else return 0x7E0023;   // Severe (Maroon)
}

int UIController::calculateAQI(const float* pmValues) {
    AQIBreakpoint pm1Bp  = getBreakpoint(pmValues[0], pm1Bps, sizeof(pm1Bps) / sizeof(pm1Bps[0]));
    AQIBreakpoint pm25Bp = getBreakpoint(pmValues[1], pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
    AQIBreakpoint pm10Bp = getBreakpoint(pmValues[2], pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));

    int pm1Index  = calculateSubIndex(pmValues[0], pm1Bp);
    int pm25Index = calculateSubIndex(pmValues[1], pm25Bp);
    int pm10Index = calculateSubIndex(pmValues[2], pm10Bp);

    return max(pm25Index, pm10Index);
}

void UIController::updateTimeDisplay() {
    if (!running || !ui_time) return;
    
    time_t now = time(nullptr);
    if (now > 0) { 
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char timeStr[9];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        lv_label_set_text(ui_time, timeStr);

        if (ui_date) {
            char dateStr[20];
            strftime(dateStr, sizeof(dateStr), "%d-%m", &timeinfo);  
            lv_label_set_text(ui_date, dateStr);
        }
    } else {
        unsigned long uptime = millis() / 1000;
        unsigned long hours = (uptime / 3600) % 24;
        unsigned long minutes = (uptime % 3600) / 60;
        unsigned long seconds = uptime % 60;
        char uptimeStr[9];
        snprintf(uptimeStr, sizeof(uptimeStr), "%02lu:%02lu:%02lu", hours, minutes, seconds);
        lv_label_set_text(ui_time, uptimeStr);

        if (ui_date) {
            lv_label_set_text(ui_date, "--");
        }
    }
    
}

void UIController::handleSensorReadingEvent(const std::shared_ptr<const CORE::Event>& event) {
    // Serial.println("[UIController] SensorReadingEvent received");
    auto sensorEvent = std::static_pointer_cast<const CORE::SensorReadingEvent>(event);
    auto readings = sensorEvent->getReadings();
    // Serial.printf("[UIController] Sensor type: %d, Reading count: %zu\n",
    //               (int)sensorEvent->getSensor(), readings.size());
    if (!readings.empty()) {
        // Serial.printf("[UIController] First value: %.2f\n", readings[0]);
    }
    updateSensorDisplay(sensorEvent->getSensor(), sensorEvent->getReadings());
}

void UIController::handleCommandReceivedEvent(const std::shared_ptr<const CORE::Event>& event) {
    auto cmdEvent = std::static_pointer_cast<const CORE::CommandReceivedEvent>(event);
    const String& command = cmdEvent->getCommand();
    const String& payload = cmdEvent->getPayload();

    if (command == "screen" && payload.length() > 0) {
        switchScreen(payload.c_str());
    }
}

void UIController::task(void* parameter) {
    esp_task_wdt_add(NULL);

    Serial.println("[UIController] Task started");
    static unsigned long lastDebugLog = 0;

    while (true) {
        esp_task_wdt_reset();

        if (running) {
            esp_task_wdt_reset();
            HAL::Display::lvHandler();  
            esp_task_wdt_reset();

            if (otaInProgress) {
                vTaskDelay(pdMS_TO_TICKS(50));  
                continue;
            }

            if (millis() - lastDebugLog >= 5000) {
                // Serial.printf("[UIController] Task running - PM2.5:%.1f, Temp:%.1f°C\n",
                //              lastPM25, lastTemp);
                lastDebugLog = millis();
            }

            static unsigned long lastWiFiUpdate = 0;
            if (millis() - lastWiFiUpdate >= 1000) {
                updateWiFiStatus();
                updateQRCode();
                updateTimeDisplay();
                lastWiFiUpdate = millis();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool UIController::startTask() {
    if (uiTaskHandle != nullptr) {
        return true;
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        task,
        "UIController",
        8192,
        NULL,
        2,
        &uiTaskHandle,
        1
    );

    return (result == pdPASS);
}

String UIController::FirmwareVersionLabel() {
    if (ui_firmwareversion == nullptr) {
        Serial.println("[UIController] ERROR: ui_firmwareversion label is NULL");
        return "";
    }

    const char* labelText = lv_label_get_text(ui_firmwareversion);
    if (labelText == nullptr) {
        Serial.println("[UIController] ERROR: ui_firmwareversion label text is NULL");
        return "";
    }

    String version = String(labelText);

    // Remove "Version-" or "Version- " prefix if present
    if (version.startsWith("Version-")) {
        version = version.substring(8);
    } else if (version.startsWith("Version- ")) {
        version = version.substring(9);
    }

    version.trim();
    Serial.printf("[UIController] Current firmware version from UI label: '%s'\n", version.c_str());
    return version;
}

TaskHandle_t UIController::getTaskHandle() {
    return uiTaskHandle;
}

} // namespace APP