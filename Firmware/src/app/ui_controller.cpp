// ui_controller.cpp - UI Controller implementation for Airowl 3.0
#include "ui_controller.h"
#include "ui/ui.h" 
#include "hal/hal_display.h"
#include "hal/hal_wifi.h"
#include "../svc/mqtt_service.h"
#include <esp_task_wdt.h>
#include <time.h>
#include <esp_sntp.h>

namespace APP{
   
    bool initialized = false;
    bool running = false;
    TaskHandle_t uiTaskHandle = nullptr;
    bool wasTouched = false;
    
    // QR Code variables
    lv_obj_t* qrCodeObj = nullptr;
    String lastQRCodeUrl = "";
    
    // Event subscriptions
    uint32_t sensorReadingSubscriptionId = 0;
    uint32_t commandReceivedSubscriptionId = 0;
    
    // MQTT and sensor data storage with 1-minute averaging
    static float lastPM1 = 0.0, lastPM25 = 0.0, lastPM4 = 0.0, lastPM10 = 0.0;
    static float lastTemp = 0.0, lastHumidity = 0.0;
    static unsigned long lastMqttPublish = 0;
    const unsigned long MQTT_PUBLISH_INTERVAL = 60000; // 60 seconds (1 minute)
    
    // 1-minute averaging buffers
    const int SENSOR_BUFFER_SIZE = 60; // Store 60 readings (1 per second for 1 minute)
    static float pm1Buffer[SENSOR_BUFFER_SIZE] = {0};
    static float pm25Buffer[SENSOR_BUFFER_SIZE] = {0};
    static float pm4Buffer[SENSOR_BUFFER_SIZE] = {0};
    static float pm10Buffer[SENSOR_BUFFER_SIZE] = {0};
    static int bufferIndex = 0;
    static int bufferCount = 0;
    static unsigned long lastSensorStore = 0;
    const unsigned long SENSOR_STORE_INTERVAL = 1000; // Store sensor reading every 1 second
    
    // Function to calculate 1-minute averages
    void calculateSensorAverages(float& avgPM1, float& avgPM25, float& avgPM4, float& avgPM10) {
        if (bufferCount == 0) {
            // No data available, use current values
            avgPM1 = lastPM1;
            avgPM25 = lastPM25;
            avgPM4 = lastPM4;
            avgPM10 = lastPM10;
            return;
        }
        
        float sumPM1 = 0, sumPM25 = 0, sumPM4 = 0, sumPM10 = 0;
        
        for (int i = 0; i < bufferCount; i++) {
            sumPM1 += pm1Buffer[i];
            sumPM25 += pm25Buffer[i];
            sumPM4 += pm4Buffer[i];
            sumPM10 += pm10Buffer[i];
        }
        
        avgPM1 = sumPM1 / bufferCount;
        avgPM25 = sumPM25 / bufferCount;
        avgPM4 = sumPM4 / bufferCount;
        avgPM10 = sumPM10 / bufferCount;
        
        Serial.printf("[UIController] Calculated 1-min averages from %d readings - PM1: %.2f, PM2.5: %.2f, PM4: %.2f, PM10: %.2f\n", 
                     bufferCount, avgPM1, avgPM25, avgPM4, avgPM10);
    }
    
    // AQI and sensor data tracking
    unsigned long bootTime = 0;
    const unsigned long EYE_COLOR_ACTIVATION_DELAY = 60000; // 60 seconds
    
    // Chart data configuration
    #define CHART_DATA_LENGTH 12
    
    // PM1 Chart series and data
    lv_chart_series_t *ui_PM1chart_series_1 = nullptr;
    static lv_coord_t ui_PM1chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm1_max_value = 0.0;
    static int pm1_data_index = 0;
    
    // PM25 Chart series and data
    lv_chart_series_t *ui_PM25chart_series_1 = nullptr;
    static lv_coord_t ui_PM25chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm25_max_value = 0.0;
    
    // PM10 Chart series and data
    lv_chart_series_t *ui_PM10chart_series_1 = nullptr;
    static lv_coord_t ui_PM10chart_series_1_array[CHART_DATA_LENGTH] = {0};
    static float pm10_max_value = 0.0;
    
    // AQI breakpoints for PM2.5 (US EPA standard)
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
    
    const AQIBreakpoint pm1Bps[] = {
        {0.0, 8.0, 0, 50},
        {8.1, 25.4, 51, 100},
        {25.5, 35.4, 101, 150},
        {35.5, 50.4, 151, 200},
        {50.5, 75.4, 201, 300},
        {75.5, 500.4, 301, 500}
    };
    
    AQIBreakpoint getBreakpoint(float value, const AQIBreakpoint* breakpoints, int count) {
        for (int i = 0; i < count; i++) {
            if (value >= breakpoints[i].low && value <= breakpoints[i].high) {
                return breakpoints[i];
            }
        }
        // Return highest breakpoint if value exceeds all ranges
        return breakpoints[count - 1];
    }
    
    int calculateSubIndex(float value, const AQIBreakpoint& bp) {
        return ((bp.aqiHigh - bp.aqiLow) / (bp.high - bp.low)) * (value - bp.low) + bp.aqiLow;
    }
    
    void UIController::setupPM1Chart() {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_Chart1 != nullptr) {
            // Configure chart to match the original PM1 screen settings
            lv_chart_set_type(ui_Chart1, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_Chart1, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y, 0, 500); // Adjust range for PM1 values
            
            // Add series for PM1 data
            ui_PM1chart_series_1 = lv_chart_add_series(
                ui_Chart1, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_Chart1, ui_PM1chart_series_1,
                                   ui_PM1chart_series_1_array);
            
            // Initialize data array with zeros
            for (int i = 0; i < CHART_DATA_LENGTH; i++) {
                ui_PM1chart_series_1_array[i] = 0;
            }
            
            Serial.println("[UIController] PM1 Chart initialized");
        }
        #endif
    }
    
    void UIController::updatePM1Chart(float pm1_value) {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_PM1chart_series_1 != nullptr && ui_Chart1 != nullptr) {
            // Shift existing data points to the left
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_PM1chart_series_1_array[i] = ui_PM1chart_series_1_array[i + 1];
            }
            
            // Add new data point at the end
            ui_PM1chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(pm1_value * 10); // Scale for display
            
            // Update max value tracking
            if (pm1_value > pm1_max_value) {
                pm1_max_value = pm1_value;
                // Update max value label
                if (ui_pm1maxvalue != nullptr) {
                    char maxBuffer[8];
                    dtostrf(pm1_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_pm1maxvalue, maxBuffer);
                }
            }
            
            // Refresh chart to show new data
            lv_chart_refresh(ui_Chart1);
        }
        #endif
    }
    
    void UIController::setupPM25Chart() {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_Chart3 != nullptr) {
            // Configure chart to match the original PM25 screen settings
            lv_chart_set_type(ui_Chart3, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_Chart3, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_Chart3, LV_CHART_AXIS_PRIMARY_Y, 0, 500); // Adjust range for PM25 values
            
            // Add series for PM25 data
            ui_PM25chart_series_1 = lv_chart_add_series(
                ui_Chart3, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_Chart3, ui_PM25chart_series_1,
                                   ui_PM25chart_series_1_array);
            
            // Initialize data array with zeros
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
            // Shift existing data points to the left
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_PM25chart_series_1_array[i] = ui_PM25chart_series_1_array[i + 1];
            }
            
            // Add new data point at the end
            ui_PM25chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(pm25_value * 10); // Scale for display
            
            // Update max value tracking
            if (pm25_value > pm25_max_value) {
                pm25_max_value = pm25_value;
                // Update max value label
                if (ui_pm25maxvalue != nullptr) {
                    char maxBuffer[8];
                    dtostrf(pm25_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_pm25maxvalue, maxBuffer);
                }
            }
            
            // Refresh chart to show new data
            lv_chart_refresh(ui_Chart3);
        }
        #endif
    }
    
    void UIController::setupPM10Chart() {
        #ifdef CONFIG_ENABLE_LVGL
        if (ui_Chart4 != nullptr) {
            // Configure chart to match the original PM10 screen settings
            lv_chart_set_type(ui_Chart4, LV_CHART_TYPE_LINE);
            lv_chart_set_point_count(ui_Chart4, CHART_DATA_LENGTH);
            lv_chart_set_range(ui_Chart4, LV_CHART_AXIS_PRIMARY_Y, 0, 600); // Adjust range for PM10 values
            
            // Add series for PM10 data
            ui_PM10chart_series_1 = lv_chart_add_series(
                ui_Chart4, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
            lv_chart_set_ext_y_array(ui_Chart4, ui_PM10chart_series_1,
                                   ui_PM10chart_series_1_array);
            
            // Initialize data array with zeros
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
            // Shift existing data points to the left
            for (int i = 0; i < CHART_DATA_LENGTH - 1; i++) {
                ui_PM10chart_series_1_array[i] = ui_PM10chart_series_1_array[i + 1];
            }
            
            // Add new data point at the end
            ui_PM10chart_series_1_array[CHART_DATA_LENGTH - 1] = (lv_coord_t)(pm10_value * 10); // Scale for display
            
            // Update max value tracking
            if (pm10_value > pm10_max_value) {
                pm10_max_value = pm10_value;
                // Update max value label
                if (ui_pm10maxvalue != nullptr) {
                    char maxBuffer[8];
                    dtostrf(pm10_max_value, 4, 1, maxBuffer);
                    lv_label_set_text(ui_pm10maxvalue, maxBuffer);
                }
            }
            
            // Refresh chart to show new data
            lv_chart_refresh(ui_Chart4);
        }
        #endif
    }

    // Screen mapping
    struct ScreenMap {
        const char* name;
        lv_obj_t* screen;
    };
    static ScreenMap screens[] = {
        {"dashboard", ui_dashboard},
        {"intro", ui_Intro},
        {"qrcode", ui_qrcode},
        {"owl", ui_owl},
        {"pm1graph", ui_PM1graph},
        {"pm25graph", ui_PM25graph},
        {"pm4graph", ui_PM4graph},
        {"pm10graph", ui_PM10graph},
        {"tvocgraph", ui_TVOCgraph},
    };

static void ui_task(void* parameter) {
    // esp_task_wdt_add(NULL);

    while (running) {
        HAL::Display::lvHandler();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms cycle
    }
}

bool UIController::init() {   
    if (initialized) {
        Serial.println("[DEBUG][UIController] Already initialized, returning true");
        return true;
    }
    
    // Record boot time for warmup period
    bootTime = millis();
    
    // Initialize time synchronization
    configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // GMT+5:30 for India
    Serial.println("[UIController] NTP time synchronization started");
    
    if (!HAL::Display::init()) {
        Serial.println("[UIController] Display/LVGL init failed!");
        return false;
    }
    
    ui_Intro_screen_init();
    ui_qrcode_screen_init();
    ui_owl_screen_init();
    ui_dashboard_screen_init();
    ui_PM1graph_screen_init();
    ui_PM4graph_screen_init();
    ui_PM25graph_screen_init();
    ui_PM10graph_screen_init();
    ui_TVOCgraph_screen_init();
    ui_matter_qrcode_screen_init();
    
    // Setup charts after screen initialization
    setupPM1Chart();
    setupPM25Chart();
    setupPM10Chart();
    
    // Initialize and start MQTT service
    if (SVC::MQTTService::init()) {
        SVC::MQTTService::startTask();
        // Connect to Oizom MQTT broker when WiFi is available
        if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) {
            SVC::MQTTService::connectToOizom();
        }
        Serial.println("[UIController] MQTT service initialized");
    }

    // Load default screen
    lv_disp_load_scr(ui_Intro);

    // Subscribe to events
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
            
            // Update QR code when switching to QR code screen
            if (strcmp(screenName, "qrcode") == 0) {
                updateQRCode();
            }
            
            return true;
        }
    }
    return false; // Screen not found
}

void UIController::updateSensorDisplay(CORE::SensorReadingEvent::SensorType sensorType, 
                                      const float* values, uint8_t valueCount) {
    if (!running) return;
    
    switch (sensorType) {
        case CORE::SensorReadingEvent::SensorType::PMS:
            if (valueCount >= 4) {
                #ifdef CONFIG_ENABLE_LVGL
                // Update PM values on UI with proper formatting
                char pm1buffer[8], pm25buffer[8], pm4buffer[8], pm10buffer[8];
                dtostrf(values[0], 6, 1, pm1buffer);
                dtostrf(values[1], 6, 1, pm25buffer);
                dtostrf(values[2], 6, 1, pm4buffer);
                dtostrf(values[3], 6, 1, pm10buffer);
                
                lv_label_set_text(ui_pm1value, pm1buffer);
                lv_label_set_text(ui_pm25value, pm25buffer);
                lv_label_set_text(ui_pm4value, pm4buffer);
                lv_label_set_text(ui_pm10value, pm10buffer);
                
                // Update charts with new data
                updatePM1Chart(values[0]);
                updatePM25Chart(values[1]);
                updatePM10Chart(values[3]);
                
                // Store current sensor values
                lastPM1 = values[0];
                lastPM25 = values[1];
                lastPM4 = values[2];
                lastPM10 = values[3];
                
                // Store sensor readings in buffer for 1-minute averaging
                if (millis() - lastSensorStore >= SENSOR_STORE_INTERVAL) {
                    pm1Buffer[bufferIndex] = values[0];
                    pm25Buffer[bufferIndex] = values[1];
                    pm4Buffer[bufferIndex] = values[2];
                    pm10Buffer[bufferIndex] = values[3];
                    
                    bufferIndex = (bufferIndex + 1) % SENSOR_BUFFER_SIZE;
                    if (bufferCount < SENSOR_BUFFER_SIZE) {
                        bufferCount++;
                    }
                    
                    lastSensorStore = millis();
                    Serial.printf("[UIController] Stored sensor reading #%d (buffer: %d/%d)\n", 
                                 bufferIndex, bufferCount, SENSOR_BUFFER_SIZE);
                }
                
                // Update sensor colors and eye colors based on AQI
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
                
                lv_label_set_text(ui_temp, tempbuffer);
                lv_label_set_text(ui_humd, humbuffer);
                
                // Store temperature and humidity for MQTT publishing
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
    
    // Check WiFi connection status
    if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) {
        #ifdef CONFIG_ENABLE_LVGL
        // Update UI element to show connected status (airowl_2 image)
        if (ui_nose != nullptr) {
            lv_img_set_src(ui_nose, &ui_img_airowl_2_png);
        }
        #endif
    } else {
        #ifdef CONFIG_ENABLE_LVGL
        // Update UI element to show disconnected status (airowl_1 image)
        if (ui_nose != nullptr) {
            lv_img_set_src(ui_nose, &ui_img_airowl_1_png);
        }
        #endif
    }
}

void UIController::updateQRCode() {
    if (!running || !ui_qrcode) return;
    
    // Generate AP name dynamically
    String apName = HAL::WiFi::generateApName();
    
    // Generate QR code content based on WiFi status
    String qrcodeurl = (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) 
        ? "https://opendata.oizom.com/device/" + apName 
        : "WIFI:T:WPA;S:" + apName + ";P:12345678;;";
    
    // Only update if content changed or QR code doesn't exist
    if (qrcodeurl != lastQRCodeUrl || qrCodeObj == nullptr) {
        #ifdef CONFIG_ENABLE_LVGL
        // Delete old QR code if it exists
        if (qrCodeObj) {
            lv_obj_del(qrCodeObj);
            qrCodeObj = nullptr;
        }
        
        // Create new QR code
        qrCodeObj = lv_qrcode_create(ui_qrcode, 150, lv_color_black(), lv_color_white());
        if (qrCodeObj) {
            lv_obj_center(qrCodeObj);
            lv_qrcode_update(qrCodeObj, qrcodeurl.c_str(), qrcodeurl.length());
            lastQRCodeUrl = qrcodeurl;
        }
        #endif
    }
}

void UIController::updateSensorColors(const float* pmValues) {
    if (!running) return;
    
    #ifdef CONFIG_ENABLE_LVGL
    // Calculate individual AQI values
    AQIBreakpoint pm1Bp = getBreakpoint(pmValues[0], pm1Bps, sizeof(pm1Bps) / sizeof(pm1Bps[0]));
    AQIBreakpoint pm25Bp = getBreakpoint(pmValues[1], pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
    AQIBreakpoint pm10Bp = getBreakpoint(pmValues[3], pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
    
    int pm1Index = calculateSubIndex(pmValues[0], pm1Bp);
    int pm25Index = calculateSubIndex(pmValues[1], pm25Bp);
    int pm10Index = calculateSubIndex(pmValues[3], pm10Bp);
    
    // Get colors for each sensor value
    uint32_t pm1_color, pm25_color, pm10_color, pm4_color;
    
    // Check if warmup period has passed
    if (millis() - bootTime < EYE_COLOR_ACTIVATION_DELAY) {
        pm1_color = pm25_color = pm10_color = pm4_color = 0xFFFFFF; // White during warmup
    } else {
        pm1_color = getAQIColor(pm1Index);
        pm25_color = getAQIColor(pm25Index);
        pm10_color = getAQIColor(pm10Index);
        pm4_color = getAQIColor(pm1Index); // Use PM1 color for PM4
    }
    
    // Apply colors to sensor value labels
    lv_obj_set_style_text_color(ui_pm1value, lv_color_hex(pm1_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_pm25value, lv_color_hex(pm25_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_pm4value, lv_color_hex(pm4_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_pm10value, lv_color_hex(pm10_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Calculate overall AQI (maximum of all indices)
    int overallAQI = max(max(pm1Index, pm25Index), pm10Index);
    
    // Update eye colors based on overall AQI
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
    // Calculate individual AQI values
    AQIBreakpoint pm1Bp = getBreakpoint(pmValues[0], pm1Bps, sizeof(pm1Bps) / sizeof(pm1Bps[0]));
    AQIBreakpoint pm25Bp = getBreakpoint(pmValues[1], pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
    AQIBreakpoint pm10Bp = getBreakpoint(pmValues[3], pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
    
    int pm1Index = calculateSubIndex(pmValues[0], pm1Bp);
    int pm25Index = calculateSubIndex(pmValues[1], pm25Bp);
    int pm10Index = calculateSubIndex(pmValues[3], pm10Bp);
    
    // Return maximum AQI (worst case)
    return max(max(pm1Index, pm25Index), pm10Index);
}

void UIController::updateTimeDisplay() {
    if (!running || !ui_time) return;
    
    #ifdef CONFIG_ENABLE_LVGL
    time_t now = time(nullptr);
    if (now > 0) { // Check if time is valid
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char timeStr[9];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        lv_label_set_text(ui_time, timeStr);
    } else {
        // Show system uptime if NTP time not available
        unsigned long uptime = millis() / 1000;
        unsigned long hours = (uptime / 3600) % 24;
        unsigned long minutes = (uptime % 3600) / 60;
        unsigned long seconds = uptime % 60;
        char uptimeStr[9];
        snprintf(uptimeStr, sizeof(uptimeStr), "%02lu:%02lu:%02lu", hours, minutes, seconds);
        lv_label_set_text(ui_time, uptimeStr);
    }
    #endif
}

void UIController::handleSensorReadingEvent(const CORE::Event& event) {
    const CORE::SensorReadingEvent& sensorEvent = static_cast<const CORE::SensorReadingEvent&>(event);
    
    // Update UI with sensor data
    updateSensorDisplay(
        sensorEvent.getSensorType(),
        sensorEvent.getValues(),
        sensorEvent.getValueCount()
    );
}


void UIController::handleCommandReceivedEvent(const CORE::Event& event) {
    const CORE::CommandReceivedEvent& cmdEvent = static_cast<const CORE::CommandReceivedEvent&>(event);
    
    // Process command (could update UI based on command)
    const String& command = cmdEvent.getCommand();
    const String& payload = cmdEvent.getPayload();
    
    // Example: Handle screen switching command
    if (command == "screen" && payload.length() > 0) {
        switchScreen(payload.c_str());
    }
    // Other commands could be handled here
}

void UIController::task(void* parameter) {
    // esp_task_wdt_add(NULL);
    
    while (true) {
        // Reset watchdog
        // esp_task_wdt_reset();
        
        if (running) {
            HAL::Display::lvHandler();
            
            // Update WiFi status and QR code periodically (every ~1 second)
            static unsigned long lastWiFiUpdate = 0;
            if (millis() - lastWiFiUpdate >= 1000) {
                updateWiFiStatus();
                updateQRCode(); // Update QR code when WiFi status might change
                
                // Update time display
                updateTimeDisplay();
                
                // Check if MQTT should connect when WiFi becomes available
                if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED && 
                    SVC::MQTTService::getState() == SVC::MQTTService::State::DISCONNECTED) {
                    SVC::MQTTService::connectToOizom();
                }
                
                lastWiFiUpdate = millis();
            }
            
            // Publish sensor data via MQTT periodically
            if (millis() - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
                Serial.printf("[UIController] MQTT publish interval reached (%lu ms)\n", MQTT_PUBLISH_INTERVAL);
                Serial.printf("[UIController] MQTT State: %d\n", (int)SVC::MQTTService::getState());
                
                if (SVC::MQTTService::getState() == SVC::MQTTService::State::CONNECTED) {
                    // Generate device ID from WiFi MAC address
                    String deviceId = HAL::WiFi::generateApName();
                    
                    // Calculate 1-minute averages
                    float avgPM1, avgPM25, avgPM4, avgPM10;
                    calculateSensorAverages(avgPM1, avgPM25, avgPM4, avgPM10);
                    
                    Serial.printf("[UIController] Publishing 1-min averaged sensor data for device: %s\n", deviceId.c_str());
                    Serial.printf("[UIController] 1-min averaged values - PM1: %.2f, PM2.5: %.2f, PM4: %.2f, PM10: %.2f\n", 
                                 avgPM1, avgPM25, avgPM4, avgPM10);
                    
                    // Publish averaged sensor data
                    if (SVC::MQTTService::publishSensorData(
                        deviceId.c_str(), 
                        avgPM1, avgPM25, avgPM4, avgPM10, 
                        0.0)) { // TVOC not available, using default value
                        Serial.println("[UIController] ✓ 1-min averaged sensor data published to MQTT successfully");
                        
                        // Reset buffer after successful publish to start fresh averaging period
                        bufferCount = 0;
                        bufferIndex = 0;
                        Serial.println("[UIController] Reset averaging buffer for next 1-minute period");
                    } else {
                        Serial.println("[UIController] ✗ Failed to publish 1-min averaged sensor data to MQTT");
                    }
                } else {
                    Serial.println("[UIController] MQTT not connected, skipping publish");
                    // Try to reconnect if not connected
                    if (SVC::MQTTService::getState() == SVC::MQTTService::State::DISCONNECTED) {
                        Serial.println("[UIController] Attempting to reconnect to MQTT...");
                        SVC::MQTTService::connectToOizom();
                    }
                }
                
                lastMqttPublish = millis();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms task cycle for responsive UI
    }
}

bool UIController::startTask() {
    if (uiTaskHandle != nullptr) {
        return true; // Task already running
    }
    
    // Create UI task
    BaseType_t result = xTaskCreatePinnedToCore(
        task,
        "UIController",
        4096,
        NULL,
        2, // Higher priority for UI responsiveness
        &uiTaskHandle,
        1  // Run on core 1 (application core)
    );
    
    return (result == pdPASS);
}

} // namespace APP