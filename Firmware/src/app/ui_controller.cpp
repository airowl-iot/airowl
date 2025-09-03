// ui_controller.cpp - UI Controller implementation for Airowl 3.0
#include "ui_controller.h"
#include "ui/ui.h" 
#include "hal/hal_display.h"
#include "hal/hal_wifi.h"
#include "../svc/mqtt_service.h"
#include <esp_task_wdt.h>
#include <time.h>
#include <esp_sntp.h>

#ifndef CONFIG_ENABLE_OTA_ANEDYA
#include "svc/ota_service.h"
#endif

#define FIRMWARE_VERSION "Version-3.0"

namespace APP{

    struct ScreenMap {
    const char* name;
    lv_obj_t* screen;
    };
    static ScreenMap screens[] = {
        {"dashboard", ui_dashboard},
        {"intro", ui_Intro},
        {"qrcode", ui_qrcode},
        {"owl", ui_owl},
        {"tempgraph", ui_Tempgraph},
        {"pm25graph", ui_PM25graph},
        {"humdgraph", ui_Humdgraph},
        {"pm10graph", ui_PM10graph},
        {"tvocgraph", ui_TVOCgraph},
    };
   
    bool initialized = false;
    bool running = false;
    TaskHandle_t uiTaskHandle = nullptr;
    bool wasTouched = false;
    
    lv_obj_t* qrCodeObj = nullptr;
    String lastQRCodeUrl = "";
    String storedApName = ""; 

    uint32_t sensorReadingSubscriptionId = 0;
    uint32_t commandReceivedSubscriptionId = 0;

    static float lastPM25 = 0.0,  lastPM10 = 0.0;
    static float lastTemp = 0.0, lastHumidity = 0.0;
    static unsigned long lastMqttPublish = 0;
    const unsigned long MQTT_PUBLISH_INTERVAL = 60000; 

    const int SENSOR_BUFFER_SIZE = 60; 
    static float pm25Buffer[SENSOR_BUFFER_SIZE] = {0};
    static float pm10Buffer[SENSOR_BUFFER_SIZE] = {0};
    static int bufferIndex = 0;
    static int bufferCount = 0;
    static unsigned long lastSensorStore = 0;
    const unsigned long SENSOR_STORE_INTERVAL = 1000; 

    static void ui_event_WifiIcon(lv_event_t * e) { 
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (HAL::WiFi::getStatus() != HAL::WiFi::Status::CONNECTED) {
            _ui_screen_change(&ui_qrcode, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_qrcode_screen_init);
        }
    }   
}
    void calculateSensorAverages( float& avgPM25, float& avgPM10) {
        if (bufferCount == 0) {
            avgPM25 = lastPM25;
            avgPM10 = lastPM10;
            return;
        }
        
        float sumPM25 = 0, sumPM10 = 0;
        
        for (int i = 0; i < bufferCount; i++) {
            sumPM25 += pm25Buffer[i];
            sumPM10 += pm10Buffer[i];
        }
        
        avgPM25 = sumPM25 / bufferCount;
        avgPM10 = sumPM10 / bufferCount;
        
        Serial.printf("[UIController] Calculated 1-min averages from %d readings - PM2.5: %.2f, PM10: %.2f\n", 
                     bufferCount, avgPM25, avgPM10);
    }
    
    unsigned long bootTime = 0;
    const unsigned long EYE_COLOR_ACTIVATION_DELAY = 60000; 
    
    #define CHART_DATA_LENGTH 12

    lv_chart_series_t *ui_PM25chart_series_1 = nullptr;
    static lv_coord_t ui_PM25chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm25_max_value = 0.0;

    lv_chart_series_t *ui_PM10chart_series_1 = nullptr;
    static lv_coord_t ui_PM10chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm10_max_value = 0.0;

    struct AQIBreakpoint {
        float low;
        float high;
        int aqiLow;
        int aqiHigh;
    };
    
    const AQIBreakpoint pm25Bps[] = {
        {0.0, 12.0, 0, 50},
        {12.1, 35.4, 51, 100},
        {35.5, 55.4, 101, 150},
        {55.5, 150.4, 151, 200},
        {150.5, 250.4, 201, 300},
        {250.5, 500.4, 301, 500}
    };
    
    const AQIBreakpoint pm10Bps[] = {
        {0.0, 54.0, 0, 50},
        {55.0, 154.0, 51, 100},
        {155.0, 254.0, 101, 150},
        {255.0, 354.0, 151, 200},
        {355.0, 424.0, 201, 300},
        {425.0, 604.0, 301, 500}
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
        return ((bp.aqiHigh - bp.aqiLow) / (bp.high - bp.low)) * (value - bp.low) + bp.aqiLow;
    }
    
    void UIController::setupPM25Chart() {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_Chart3 != nullptr) {
            lv_chart_set_type(ui_Chart3, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_Chart3, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_Chart3, LV_CHART_AXIS_PRIMARY_Y, 0, 500); 

            ui_PM25chart_series_1 = lv_chart_add_series(
                ui_Chart3, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_Chart3, ui_PM25chart_series_1,
                                   ui_PM25chart_series_1_array);

            for (int i = 0; i < CHART_DATA_LENGTH; i++) {
                ui_PM25chart_series_1_array[i] = 0;
            }
            
            Serial.println("[UIController] PM25 Chart initialized");
        }
        #endif
    }
    
    void UIController::updatePM25Chart(float pm25_value) {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_PM25chart_series_1 != nullptr && ui_Chart3 != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_PM25chart_series_1_array[i] = ui_PM25chart_series_1_array[i + 1];
            }

            ui_PM25chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(pm25_value * 10); 

            if (pm25_value > pm25_max_value) {
                pm25_max_value = pm25_value;
                if (ui_pm25maxvalue != nullptr) {
                    char maxBuffer[8];
                    dtostrf(pm25_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_pm25maxvalue, maxBuffer);
                }
            }
            lv_chart_refresh(ui_Chart3);
        }
        #endif
    }
    
    void UIController::setupPM10Chart() {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_Chart4 != nullptr) {
            lv_chart_set_type(ui_Chart4, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_Chart4, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_Chart4, LV_CHART_AXIS_PRIMARY_Y, 0, 600); 

            ui_PM10chart_series_1 = lv_chart_add_series(
                ui_Chart4, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_Chart4, ui_PM10chart_series_1,
                                   ui_PM10chart_series_1_array);

            for (int i = 0; i < CHART_DATA_LENGTH; i++) {
                ui_PM10chart_series_1_array[i] = 0;
            }
            
            Serial.println("[UIController] PM10 Chart initialized");
        }
        #endif
    }
    
    void UIController::updatePM10Chart(float pm10_value) {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_PM10chart_series_1 != nullptr && ui_Chart4 != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_PM10chart_series_1_array[i] = ui_PM10chart_series_1_array[i + 1];
            }
            ui_PM10chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(pm10_value * 10); 
            
            if (pm10_value > pm10_max_value) {
                pm10_max_value = pm10_value;
                if (ui_pm10maxvalue != nullptr) {
                    char maxBuffer[8];
                    dtostrf(pm10_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_pm10maxvalue, maxBuffer);
                }
            }
            lv_chart_refresh(ui_Chart4);
        }
        #endif
}


static void ui_task(void* parameter) {
    // esp_task_wdt_add(NULL);

    while (running) {
        HAL::Display::lvHandler();
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

bool UIController::init() {   
    if (initialized) {
        Serial.println("[DEBUG][UIController] Already initialized, returning true");
        return true;
    }
    
    bootTime = millis();
    configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); 
    Serial.println("[UIController] NTP time synchronization started");
    
    if (!HAL::Display::init()) {
        Serial.println("[UIController] Display/LVGL init failed!");
        return false;
    }
    
    ui_Intro_screen_init();
    ui_qrcode_screen_init();
    ui_owl_screen_init();
    ui_dashboard_screen_init();
    ui_Tempgraph_screen_init();
    ui_Humdgraph_screen_init();
    ui_PM25graph_screen_init();
    ui_PM10graph_screen_init();
    ui_TVOCgraph_screen_init();
    ui_eCO2graph_screen_init();

    setupPM25Chart();
    setupPM10Chart();

    if (SVC::MQTTService::init()) {
        SVC::MQTTService::startTask();
        if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) {
            SVC::MQTTService::connectToOizom();
        }
        Serial.println("[UIController] MQTT service initialized");
    }
    lv_disp_load_scr(ui_Intro);

    CORE::EventBus& eventBus = CORE::EventBus::getInstance();
    
    sensorReadingSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::SENSOR_READING,
        handleSensorReadingEvent
    );
    
    commandReceivedSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::COMMAND_RECEIVED,
        handleCommandReceivedEvent
    );
    
    Serial.println("[UIController] Event subscriptions registered");
    
    #ifndef CONFIG_ENABLE_OTA_ANEDYA
    SVC::OTA::onProgress([](SVC::OTA::State state, int progress, const char* message) {
        const char* stateNames[] = {"IDLE", "CHECKING", "DOWNLOADING", "UPDATING", "SUCCESS", "FAILED"};
        Serial.printf("[UIController] Anedya OTA Progress: %s (%d%%) - %s\n", 
                     stateNames[(int)state], progress, message);
        
    });
    Serial.println("[UIController] Anedya OTA progress callback registered");
    #endif
    
    initialized = true;
    running = true;

    return startTask();
}

bool UIController::restartTask() {
    if (uiTaskHandle != nullptr) {
        vTaskDelete(uiTaskHandle);
        uiTaskHandle = nullptr;
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
                                      const float* values, uint8_t valueCount) {
    if (!running) return;
    
    switch (sensorType) {
        case CORE::SensorReadingEvent::SensorType::PMS:
            if (valueCount >= 2) {
                #ifdef CONFIG_ENABLE_LVGL
                char  pm25buffer[8], pm10buffer[8];
                dtostrf(values[0], 6, 1, pm25buffer);
                dtostrf(values[1], 6, 1, pm10buffer);
                
                lv_label_set_text(ui_pm25value, pm25buffer);
                lv_label_set_text(ui_pm10value, pm10buffer);

                updatePM25Chart(values[0]);
                updatePM10Chart(values[1]);

                lastPM25 = values[0];
                lastPM10 = values[1];

                if (millis() - lastSensorStore >= SENSOR_STORE_INTERVAL) {

                    pm25Buffer[bufferIndex] = values[0];
                    pm10Buffer[bufferIndex] = values[1];
                    
                    bufferIndex = (bufferIndex + 1) % SENSOR_BUFFER_SIZE;
                    if (bufferCount < SENSOR_BUFFER_SIZE) {
                        bufferCount++;
                    }
                    
                    lastSensorStore = millis();
                    Serial.printf("[UIController] Stored PMS sensor reading #%d (buffer: %d/%d) for MQTT averaging\n", 
                                 bufferIndex, bufferCount, SENSOR_BUFFER_SIZE);
                }
                updateSensorColors(values);
                #endif
            }
            break;
            
        case CORE::SensorReadingEvent::SensorType::PM700:
            if (valueCount >= 5) {
                #ifdef CONFIG_ENABLE_LVGL
                char  pm25buffer[8], pm10buffer[8];
                dtostrf(values[0], 6, 1, pm25buffer); 
                dtostrf(values[1], 6, 1, pm10buffer);
                
                lv_label_set_text(ui_pm25value, pm25buffer);
                lv_label_set_text(ui_pm10value, pm10buffer);

                updatePM25Chart(values[0]);
                updatePM10Chart(values[1]);
                lastPM25 = values[0];
                lastPM10 = values[1];

                if (millis() - lastSensorStore >= SENSOR_STORE_INTERVAL) {
                    pm25Buffer[bufferIndex] = values[0];
                    pm10Buffer[bufferIndex] = values[1];
                    
                    bufferIndex = (bufferIndex + 1) % SENSOR_BUFFER_SIZE;
                    if (bufferCount < SENSOR_BUFFER_SIZE) {
                        bufferCount++;
                    }
                    
                    lastSensorStore = millis();
                    Serial.printf("[UIController] Stored PM700 sensor reading #%d (buffer: %d/%d) for MQTT averaging - 0.3μm: %.1f pcs/L\n", 
                                 bufferIndex, bufferCount, SENSOR_BUFFER_SIZE, values[2]);
                }

                updateSensorColors(values);
                #endif
            }
            break;
            
        case CORE::SensorReadingEvent::SensorType::AHT:
            if (valueCount >= 2) {
                #ifdef CONFIG_ENABLE_LVGL
                char tempbuffer[8], humbuffer[8];
                dtostrf(values[0], 4, 1, tempbuffer);
                dtostrf(values[1], 4, 1, humbuffer);

                lastTemp = values[0];
                lastHumidity = values[1];
                #endif
            }
            break;
            
        default:
            break;
    }
}

void UIController::updateWiFiStatus() {
    if (!running) return;

    if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_nose != nullptr) {
            lv_img_set_src(ui_nose, &ui_img_airowl_2_png);
        }
        if (ui_wifi != nullptr) {
            lv_img_set_src(ui_wifi, &ui_img_wifi_on_png);
            lv_obj_remove_event_cb(ui_wifi, ui_event_WifiIcon);
        }
        #endif
    } else {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_nose != nullptr) {
            lv_img_set_src(ui_nose, &ui_img_airowl_1_png);
        }
        if (ui_wifi != nullptr) {
            lv_img_set_src(ui_wifi, &ui_img_wifi_off_png);
            lv_obj_add_event_cb(ui_wifi, ui_event_WifiIcon, LV_EVENT_CLICKED, nullptr);
        }
        #endif
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
        #ifdef CONFIG_ENABLE_LVGL
        if (qrCodeObj == nullptr) {
            qrCodeObj = lv_qrcode_create(ui_qrcode, 150, lv_color_black(), lv_color_white());
            lv_obj_center(qrCodeObj);
        }
        
        lv_qrcode_update(qrCodeObj, qrcodeurl.c_str(), qrcodeurl.length());
        lv_obj_center(qrCodeObj);

        lastQRCodeUrl = qrcodeurl;
        Serial.printf("[UIController] UI labels initialized with AP name: %s\n", apName);
        #endif
    }
        #ifdef CONFIG_ENABLE_LVGL
        lv_label_set_text(ui_devicename, apName.c_str());
        lv_label_set_text(ui_qrcodename, apName.c_str());
        lv_label_set_text(ui_firmwareversion, FIRMWARE_VERSION);
        #endif
}

void UIController::updateSensorColors(const float* pmValues) {
    if (!running) return;
    
    #ifdef CONFIG_ENABLE_LVGL
    AQIBreakpoint pm25Bp = getBreakpoint(pmValues[0], pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
    AQIBreakpoint pm10Bp = getBreakpoint(pmValues[1], pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
 
    int pm25Index = calculateSubIndex(pmValues[0], pm25Bp);
    int pm10Index = calculateSubIndex(pmValues[1], pm10Bp);
    uint32_t  pm25_color, pm10_color;

    if (millis() - bootTime < EYE_COLOR_ACTIVATION_DELAY) {
       pm25_color = pm10_color =  0xFFFFFF; 
    } else {
        pm25_color = getAQIColor(pm25Index);
        pm10_color = getAQIColor(pm10Index);
    }
    lv_obj_set_style_text_color(ui_pm25value, lv_color_hex(pm25_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_pm10value, lv_color_hex(pm10_color), LV_PART_MAIN | LV_STATE_DEFAULT);

    int overallAQI = max(pm25Index, pm10Index);
    uint32_t eyeColor = (millis() - bootTime < EYE_COLOR_ACTIVATION_DELAY) ? 0xFFFFFF : getAQIColor(overallAQI);
    updateEyeColors(eyeColor);
    #endif
}

void UIController::updateEyeColors(uint32_t color) {
    #ifdef CONFIG_ENABLE_LVGL
    if (ui_lefteye && ui_righteye) {
        lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    #endif
}

uint32_t UIController::getAQIColor(int aqi) {
    if (aqi <= 50) return 0x00FF00;      // Good - Green
    else if (aqi <= 100) return 0xFFFF00; // Moderate - Yellow
    else if (aqi <= 150) return 0xFF8000; // Unhealthy for Sensitive - Orange
    else if (aqi <= 200) return 0xFFA07A; // Unhealthy - Light Coral
    else if (aqi <= 300) return 0x800080; // Very Unhealthy - Purple
    else return 0x800000;                 // Hazardous - Maroon
}

int UIController::calculateAQI(const float* pmValues) {
    AQIBreakpoint pm25Bp = getBreakpoint(pmValues[0], pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
    AQIBreakpoint pm10Bp = getBreakpoint(pmValues[1], pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
    int pm25Index = calculateSubIndex(pmValues[0], pm25Bp);
    int pm10Index = calculateSubIndex(pmValues[1], pm10Bp);
    return max(pm25Index, pm10Index);
}

void UIController::updateTimeDisplay() {
    if (!running || !ui_time) return;
    
    #ifdef CONFIG_ENABLE_LVGL
    time_t now = time(nullptr);
    if (now > 0) { 
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char timeStr[9];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        lv_label_set_text(ui_time, timeStr);

        if (ui_date) {
            char dateStr[20];
            strftime(dateStr, sizeof(dateStr), "%d-%m-%Y", &timeinfo);  
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
    #endif
}

void UIController::handleSensorReadingEvent(const CORE::Event& event) {
    const CORE::SensorReadingEvent& sensorEvent = static_cast<const CORE::SensorReadingEvent&>(event);
    updateSensorDisplay(
        sensorEvent.getSensorType(),
        sensorEvent.getValues(),
        sensorEvent.getValueCount()
    );
}

void UIController::handleCommandReceivedEvent(const CORE::Event& event) {
    const CORE::CommandReceivedEvent& cmdEvent = static_cast<const CORE::CommandReceivedEvent&>(event);
    const String& command = cmdEvent.getCommand();
    const String& payload = cmdEvent.getPayload();
    if (command == "screen" && payload.length() > 0) {
        switchScreen(payload.c_str());
    }

}

void UIController::task(void* parameter) {
    // esp_task_wdt_add(NULL);
    
    while (true) {
        // Reset watchdog
        // esp_task_wdt_reset();
        
        if (running) {
            HAL::Display::lvHandler();
            static unsigned long lastWiFiUpdate = 0;
            if (millis() - lastWiFiUpdate >= 1000) {
                updateWiFiStatus();
                updateQRCode(); 
                updateTimeDisplay();
                if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED && 
                    SVC::MQTTService::getState() == SVC::MQTTService::State::DISCONNECTED) {
                    SVC::MQTTService::connectToOizom();
                }
                lastWiFiUpdate = millis();
            }
            if (millis() - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
                Serial.printf("[UIController] MQTT publish interval reached (%lu ms) - preparing 1-minute averaged data\n", MQTT_PUBLISH_INTERVAL);
                Serial.printf("[UIController] MQTT State: %d\n", (int)SVC::MQTTService::getState());
                
                if (SVC::MQTTService::getState() == SVC::MQTTService::State::CONNECTED) {
                    String deviceId = storedApName.isEmpty() ? HAL::WiFi::generateApName() : storedApName;
                    float avgPM25, avgPM10;
                    calculateSensorAverages(avgPM25, avgPM10);
                    Serial.printf("[UIController] Publishing 1-minute averaged sensor data for device: %s\n", deviceId.c_str());
                    Serial.printf("[UIController] Averaged values (from %d readings) -  PM2.5: %.2f, PM10: %.2f\n", 
                                 bufferCount, avgPM25,  avgPM10);

                    if (SVC::MQTTService::publishSensorData(
                        deviceId.c_str(), 
                        avgPM25, avgPM10, 
                        0.0)) { 
                        Serial.println("[UIController] ✓ 1-minute averaged sensor data published to MQTT successfully");
                        bufferCount = 0;
                        bufferIndex = 0;
                        Serial.println("[UIController] Reset averaging buffer for next 1-minute period");
                    } else {
                        Serial.println("[UIController] ✗ Failed to publish 1-minute averaged sensor data to MQTT");
                    }
                } else {
                    Serial.println("[UIController] MQTT not connected, skipping publish");
                    if (SVC::MQTTService::getState() == SVC::MQTTService::State::DISCONNECTED) {
                        Serial.println("[UIController] Attempting to reconnect to MQTT...");
                        SVC::MQTTService::connectToOizom();
                    }
                }
                lastMqttPublish = millis();
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
        4096,
        NULL,
        2, 
        &uiTaskHandle,
        1  
    );
    
    return (result == pdPASS);
}

} // namespace APP