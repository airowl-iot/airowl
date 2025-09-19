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
        {"ota", ui_ota}
    };
   
    bool initialized = false;
    bool running = false;
    TaskHandle_t uiTaskHandle = nullptr;
    
    lv_obj_t* qrCodeObj = nullptr;
    String lastQRCodeUrl = "";
    String storedApName = ""; 

    uint32_t sensorReadingSubscriptionId = 0;
    uint32_t commandReceivedSubscriptionId = 0;
    uint32_t wifiStateSubscriptionId = 0;
    uint32_t otaProgressSubscriptionId = 0;

    // Display tracking variables
    static float lastPM25 = 0.0, lastPM10 = 0.0;
    static float lastTemp = 0.0, lastHumidity = 0.0;
    static float lastTVOC = 0.0, lastEco2 = 0.0;

    // Buffering for MQTT averaging (now handled in SensorManager)
    const int SENSOR_BUFFER_SIZE = 60;
    static int pmBufferIndex = 0;
    static int pmBufferCount = 0;
    static int ahtBufferIndex = 0;
    static int ahtBufferCount = 0; 
    static float pm25Buffer[SENSOR_BUFFER_SIZE] = {0};
    static float pm10Buffer[SENSOR_BUFFER_SIZE] = {0};
    static float tempBuffer[SENSOR_BUFFER_SIZE] = {0};
    static float humdBuffer[SENSOR_BUFFER_SIZE] = {0};
    static float tvocBuffer[SENSOR_BUFFER_SIZE] = {0};
    static float eco2Buffer[SENSOR_BUFFER_SIZE] = {0};

    
    static unsigned long lastPMStore = 0;
    static unsigned long lastAHTStore = 0;
    const unsigned long SENSOR_STORE_INTERVAL = 1000; 

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

    lv_chart_series_t *ui_PM25chart_series_1 = nullptr;
    static lv_coord_t ui_PM25chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm25_max_value = 0.0;

    lv_chart_series_t *ui_PM10chart_series_1 = nullptr;
    static lv_coord_t ui_PM10chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm10_max_value = 0.0;

      lv_chart_series_t *ui_Tempchart_series_1 = nullptr;
    static lv_coord_t ui_Tempchart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float temp_max_value = 0.0;

      lv_chart_series_t *ui_Humdchart_series_1 = nullptr;
    static lv_coord_t ui_Humdchart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float humd_max_value = 0.0;

      lv_chart_series_t *ui_Tvocchart_series_1 = nullptr;
    static lv_coord_t ui_Tvocchart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float tvoc_max_value = 0.0;

      lv_chart_series_t *ui_eCo2chart_series_1 = nullptr;
    static lv_coord_t ui_eCo2chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float eCo2_max_value = 0.0;

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
        
    }
    
    void UIController::setupPM10Chart() {
        
        if (ui_pmChart != nullptr) {
            lv_chart_set_type(ui_pmChart, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_pmChart, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_pmChart, LV_CHART_AXIS_PRIMARY_Y, 0, 100); 

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
            
            if (pm10_value > pm10_max_value) {
                pm10_max_value = pm10_value;
                if (ui_pm10maxvalue != nullptr) {
                    char maxBuffer[8];
                    dtostrf(pm10_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_pm10maxvalue, maxBuffer);
                }
            }
            lv_chart_refresh(ui_pmChart);
        }
        
}


void UIController::setupTempChart() {
        
        if (ui_tempchart != nullptr) {
            lv_chart_set_type(ui_tempchart, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_tempchart, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_tempchart, LV_CHART_AXIS_PRIMARY_Y, 0, 50); 

            ui_Tempchart_series_1 = lv_chart_add_series(
                ui_tempchart, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_tempchart, ui_Tempchart_series_1,
                                   ui_Tempchart_series_1_array);

            for (int i = 0; i < CHART_DATA_LENGTH; i++) {
                ui_Tempchart_series_1_array[i] = 0;
            }
            
            Serial.println("[UIController] Temp Chart initialized");
        }
        
    }
    
    void UIController::updateTempChart(float temp_value) {
        
        if (ui_Tempchart_series_1 != nullptr && ui_tempchart != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_Tempchart_series_1_array[i] = ui_Tempchart_series_1_array[i + 1];
            }
            ui_Tempchart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(temp_value); 
            
            if (temp_value > temp_max_value) {
                temp_max_value = temp_value;
                if (ui_tempmax != nullptr) {
                    char maxBuffer[8];
                    dtostrf(temp_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_tempmax, maxBuffer);
                }
            }
            lv_chart_refresh(ui_tempchart);
        }
        
}


void UIController::setupHumdChart() {
        
        if (ui_Chart2  != nullptr) {
            lv_chart_set_type(ui_Chart2 , LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_Chart2 , CHART_DATA_LENGTH);
            lv_chart_set_range(ui_Chart2 , LV_CHART_AXIS_PRIMARY_Y, 0, 100); 

            ui_Humdchart_series_1 = lv_chart_add_series(
                ui_Chart2 , lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_Chart2 , ui_Humdchart_series_1,
                                   ui_Humdchart_series_1_array);

            for (int i = 0; i < CHART_DATA_LENGTH; i++) {
                ui_Humdchart_series_1_array[i] = 0;
            }
            
            Serial.println("[UIController] Humd Chart initialized");
        }
        
    }
    
    void UIController::updateHumdChart(float humd_value) {
        
        if (ui_Humdchart_series_1 != nullptr && ui_Chart2 != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_Humdchart_series_1_array[i] = ui_Humdchart_series_1_array[i + 1];
            }
            ui_Humdchart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(humd_value); 
            
            if (humd_value > humd_max_value) {
                humd_max_value = humd_value;
                if (ui_Humdmaxvalue1 != nullptr) {
                    char maxBuffer[8];
                    dtostrf(humd_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_Humdmaxvalue1, maxBuffer);
                }
            }
            lv_chart_refresh(ui_Chart2 );
        }
        
}


void UIController::setupTvocChart() {
        
        if (ui_TVOCchart  != nullptr) {
            lv_chart_set_type(ui_TVOCchart , LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_TVOCchart , CHART_DATA_LENGTH);
            lv_chart_set_range(ui_TVOCchart , LV_CHART_AXIS_PRIMARY_Y, 0, 200); 

            ui_Tvocchart_series_1 = lv_chart_add_series(
                ui_TVOCchart , lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_TVOCchart , ui_Tvocchart_series_1,
                                   ui_Tvocchart_series_1_array);

            for (int i = 0; i < CHART_DATA_LENGTH; i++) {
                ui_Tvocchart_series_1_array[i] = 0;
            }
            
            Serial.println("[UIController] Tvoc Chart initialized");
        }
        
    }
    
    void UIController::updateTvocChart(float tvoc_value) {
        
        if (ui_Tvocchart_series_1 != nullptr && ui_TVOCchart != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_Tvocchart_series_1_array[i] = ui_Tvocchart_series_1_array[i + 1];
            }
            ui_Tvocchart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(tvoc_value); 
            
            if (tvoc_value > tvoc_max_value) {
                tvoc_max_value = tvoc_value;
                if (ui_tvocmaxvalue != nullptr) {
                    char maxBuffer[8];
                    dtostrf(tvoc_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_tvocmaxvalue, maxBuffer);
                }
            }
            lv_chart_refresh(ui_TVOCchart);
        }
        
}


void UIController::setupeCo2Chart() {
        
        if (ui_eCO2chart  != nullptr) {
            lv_chart_set_type(ui_eCO2chart , LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_eCO2chart , CHART_DATA_LENGTH);
            lv_chart_set_range(ui_eCO2chart , LV_CHART_AXIS_PRIMARY_Y, 0, 4000); 

            ui_eCo2chart_series_1 = lv_chart_add_series(
                ui_eCO2chart , lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_eCO2chart , ui_eCo2chart_series_1,
                                   ui_eCo2chart_series_1_array);

            for (int i = 0; i < CHART_DATA_LENGTH; i++) {
                ui_eCo2chart_series_1_array[i] = 0;
            }
            
            Serial.println("[UIController] Tvoc Chart initialized");
        }
        
    }
    
    void UIController::updateeCo2Chart(float eco2_value) {
        
        if (ui_eCo2chart_series_1 != nullptr && ui_eCO2chart != nullptr) {
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_eCo2chart_series_1_array[i] = ui_eCo2chart_series_1_array[i + 1];
            }
            ui_eCo2chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(eco2_value); 
            
            if (eco2_value > eCo2_max_value) {
                eCo2_max_value = eco2_value;
                if (ui_eco2max2 != nullptr) {
                    char maxBuffer[8];
                    dtostrf(eCo2_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_eco2max2, maxBuffer);
                }
            }
            lv_chart_refresh(ui_eCO2chart);
        }
        
}
 

void UIController::showOTAScreen() {
    
    if (ui_ota != nullptr) {
        lv_scr_load(ui_ota);
        Serial.println("[UIController] Switched to OTA screen");
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
    ui_owl_screen_init();
    ui_dashboard_screen_init();
    ui_Tempgraph_screen_init();
    ui_Humdgraph_screen_init();
    ui_PM25graph_screen_init();
    ui_PM10graph_screen_init();
    ui_TVOCgraph_screen_init();
    ui_eCO2graph_screen_init();
    Serial.println("[UIController] All UI screens initialized");

    setupPM25Chart();
    setupPM10Chart();
    setupTempChart();
    setupHumdChart();
    setupTvocChart();
    setupeCo2Chart();

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
            UIController::updateWiFiStatus();  // Call static method
        }
    );

    otaProgressSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::OTA_PROGRESS,
        [](const std::shared_ptr<const CORE::Event>& event) {
            auto otaEvent = std::static_pointer_cast<const CORE::OTAProgressEvent>(event);
            Serial.printf("[UIController] OTA Progress: %zu/%zu bytes\n",
                          otaEvent->getWritten(), otaEvent->getTotal());
            // Update OTA progress on screen if needed
        }
    );

    Serial.println("[UIController] Event subscriptions registered");
    Serial.printf("[UIController] Subscriptions - Sensor:%u, Command:%u, WiFi:%u, MQTT:%u, OTA:%u\n",
                  sensorReadingSubscriptionId, commandReceivedSubscriptionId,
                  wifiStateSubscriptionId, otaProgressSubscriptionId);

    SVC::OTA::onProgress([](SVC::OTA::State state, int progress, const char* message) {
    Serial.printf("[UIController][OTA] State=%d, Progress=%d, Msg=%s\n", (int)state, progress, message);

    // Switch to OTA screen if downloading or updating
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
                                       const std::vector<float>& values) {
    if (!running || values.empty()) {
        Serial.printf("[UIController] updateSensorDisplay skipped - running:%d, values:%zu\n",
                      running, values.size());
        return;
    }
    Serial.printf("[UIController] updateSensorDisplay called - type:%d, values:%zu\n",
                  (int)sensorType, values.size());
    
    switch (sensorType) {
        case CORE::SensorReadingEvent::SensorType::PM700:
            if (values.size() >= 4) {
                
                char  pm25buffer[16], pm10buffer[16];
                dtostrf(values[0], 6, 1, pm25buffer); 
                dtostrf(values[1], 6, 1, pm10buffer);
                
                lv_label_set_text(ui_pm25value, pm25buffer);
                lv_label_set_text(ui_pm10value, pm10buffer);

                updatePM25Chart(values[0]);
                updatePM10Chart(values[1]);
                lastPM25 = values[0];
                lastPM10 = values[1];

                if (millis() - lastPMStore >= SENSOR_STORE_INTERVAL) {
                    pm25Buffer[pmBufferIndex] = values[0];
                    pm10Buffer[pmBufferIndex] = values[1];
                    
                    pmBufferIndex = (pmBufferIndex + 1) % SENSOR_BUFFER_SIZE;
                    if (pmBufferCount < SENSOR_BUFFER_SIZE) {
                        pmBufferCount++;
                    }
                    
                    lastPMStore = millis();
                    Serial.printf("[UIController] Stored PM700 sensor reading #%d (buffer: %d/%d) for MQTT averaging - 0.3μm: %.1f pcs/L\n", 
                                 pmBufferIndex, pmBufferCount, SENSOR_BUFFER_SIZE, values[2]);
                }

                updateSensorColors(values.data());
                
            }
            break;
            
        case CORE::SensorReadingEvent::SensorType::AHT:
            if (values.size() >= 4) {
                
                char tempbuffer[16];
                char humbuffer[16]; 
                char tvocbuffer[16];
                char eco2buffer[16];   

                dtostrf(values[0], 4, 1, tempbuffer);
                dtostrf(values[1], 4, 1, humbuffer);
                dtostrf(values[2], 4, 1, tvocbuffer);
                dtostrf(values[3], 4, 1, eco2buffer);

                lv_label_set_text(ui_tempvalue, tempbuffer);
                lv_label_set_text(ui_humdvalue, humbuffer);
                lv_label_set_text(ui_tvocvalue, tvocbuffer);
                lv_label_set_text(ui_eCO2value, eco2buffer);

                updateTempChart(values[0]);
                updateHumdChart(values[1]);
                updateTvocChart(values[2]);
                updateeCo2Chart(values[3]);

                lastTemp = values[0];
                lastHumidity = values[1];
                lastTVOC =  values[2];
                lastEco2 =  values[3];

                if (millis() - lastAHTStore >= SENSOR_STORE_INTERVAL) {
                tempBuffer[ahtBufferIndex] = values[0];
                humdBuffer[ahtBufferIndex] = values[1];
                tvocBuffer[ahtBufferIndex] = values[2];
                eco2Buffer[ahtBufferIndex] = values[3];

                 
                ahtBufferIndex = (ahtBufferIndex+ 1) % SENSOR_BUFFER_SIZE;
                if (ahtBufferCount < SENSOR_BUFFER_SIZE) {
                    ahtBufferCount++;
                }
                
                lastAHTStore = millis();
                 Serial.printf("[UIController] Updated display - T:%.1f°C, H:%.1f%%, TVOC:%d ppb, eCO2:%d ppm\n",
                values[0], values[1], (int)values[2], (int)values[3]);
        }
            }
            break;
        default:
            break;
    }
}

void UIController::updateWiFiStatus() {
    if (!running) return;

    if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) {
        if (ui_nose != nullptr) {
            lv_img_set_src(ui_nose, &ui_img_airowl_2_png);
        }
        if (ui_wifi != nullptr) {
            lv_img_set_src(ui_wifi, &ui_img_wifi_on_png);
            lv_obj_remove_event_cb(ui_wifi, ui_event_WifiIcon);
        }
        
    } else {
        if (ui_nose != nullptr) {
            lv_img_set_src(ui_nose, &ui_img_airowl_1_png);
        }
        if (ui_wifi != nullptr) {
            lv_img_set_src(ui_wifi, &ui_img_wifi_off_png);
            lv_obj_add_event_cb(ui_wifi, ui_event_WifiIcon, LV_EVENT_CLICKED, nullptr);
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
        lv_label_set_text(ui_firmwareversion, "4.0.0");  
        
}

void UIController::updateSensorColors(const float* pmValues) {
    if (!running) return;
    
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
    
}

void UIController::updateEyeColors(uint32_t color) {
    if (ui_lefteye && ui_righteye) {
        lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
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
    Serial.println("[UIController] SensorReadingEvent received");
    auto sensorEvent = std::static_pointer_cast<const CORE::SensorReadingEvent>(event);
    auto readings = sensorEvent->getReadings();
    Serial.printf("[UIController] Sensor type: %d, Reading count: %zu\n",
                  (int)sensorEvent->getSensor(), readings.size());
    if (!readings.empty()) {
        Serial.printf("[UIController] First value: %.2f\n", readings[0]);
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
            HAL::Display::lvHandler();

            if (millis() - lastDebugLog >= 5000) {
                Serial.printf("[UIController] Task running - PM2.5:%.1f, Temp:%.1f°C\n",
                             lastPM25, lastTemp);
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
        4096,
        NULL,
        2, 
        &uiTaskHandle,
        1  
    );
    
    return (result == pdPASS);
}

} // namespace APP