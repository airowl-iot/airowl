#ifdef CONFIG_ENABLE_SENSOR_PMSA003A

#include "Flags/Sensor_Pms.h"

PMSensor_t pmsensor_data;
HardwareSerial PMSserial(1);

volatile int AQI = 0;
volatile float temperature = 0.0;
volatile float humidity = 0.0;

TaskHandle_t PMSTaskHandle = nullptr;

String deviceName = "";                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             

PMS pms(PMSserial);
PMS::DATA pmsData;

#ifdef CONFIG_ENABLE_LVGL

unsigned long bootTime = 0;
#define EYE_COLOR_ACTIVATION_DELAY 45000

lv_chart_series_t *ui_PM1chart_series_1 = nullptr;
static lv_coord_t ui_PM1chart_series_1_array[CHART_DATA_LENGTH] = {0};

lv_chart_series_t *ui_PM25chart_series_1 = nullptr;
static lv_coord_t ui_PM25chart_series_1_array[CHART_DATA_LENGTH] = {0};

lv_chart_series_t *ui_PM10chart_series_1 = nullptr;
static lv_coord_t ui_PM10chart_series_1_array[CHART_DATA_LENGTH] = {0};

void setupCharts()
{
    ui_PM1chart_series_1 = lv_chart_add_series(ui_Chart1, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart1, ui_PM1chart_series_1, ui_PM1chart_series_1_array);
    lv_chart_set_range(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM25chart_series_1 = lv_chart_add_series(ui_Chart3, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart3, ui_PM25chart_series_1, ui_PM25chart_series_1_array);
    lv_chart_set_range(ui_Chart3, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM10chart_series_1 = lv_chart_add_series(ui_Chart4, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart4, ui_PM10chart_series_1, ui_PM10chart_series_1_array);
    lv_chart_set_range(ui_Chart4, LV_CHART_AXIS_PRIMARY_Y, 0, 50);
}
#endif

// AQI Breakpoints (could be moved to a common file)
AQIBreakpoint pm1Bps[] = {{0.0, 8.0, 0, 50}, {8.1, 25.4, 51, 100}, {25.5, 35.4, 101, 150},
                          {35.5, 50.4, 151, 200}, {50.5, 75.4, 201, 300}, {75.5, 500.4, 301, 500}};
AQIBreakpoint pm25Bps[] = {{0.0, 12.0, 0, 50}, {12.1, 35.4, 51, 100}, {35.5, 55.4, 101, 150},
                           {55.5, 150.4, 151, 200}, {150.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};
AQIBreakpoint pm10Bps[] = {{0, 54, 0, 50}, {55, 154, 51, 100}, {155, 254, 101, 150},
                           {255, 354, 151, 200}, {355, 424, 201, 300}, {425, 604, 301, 500}};

int calculateSubIndex(float Cp, AQIBreakpoint bp) {
    float Ip = ((bp.Ip_Hi - bp.Ip_Lo) / (bp.Cp_Hi - bp.Cp_Lo)) * (Cp - bp.Cp_Lo) + bp.Ip_Lo;
    return (int)round(Ip);
}

AQIBreakpoint getBreakpoint(float Cp, AQIBreakpoint bps[], int numBps) {
    for (int i = 0; i < numBps; i++) {
        if (Cp >= bps[i].Cp_Lo && Cp <= bps[i].Cp_Hi) {
            return bps[i];
        }
    }
    return bps[numBps - 1];
}

String getAQICategory(int aqi) {
    if (aqi <= 50) return "Good";
    else if (aqi <= 100) return "Satisfactory";
    else if (aqi <= 150) return "Moderate";
    else if (aqi <= 200) return "Unhealthy";
    else if (aqi <= 300) return "Very Unhealthy";
    return "Hazardous";
}

uint32_t getAQIColor(int aqi) {
    if (aqi <= 50) return 0x00E400;
    else if (aqi <= 100) return 0x9CFF9C;
    else if (aqi <= 150) return 0xFFFF00;
    else if (aqi <= 200) return 0xFF7E00;
    else if (aqi <= 300) return 0xFF0000;
    return 0x8F3F97;
}

void sensorData(void *params) {
    PMSserial.begin(9600, SERIAL_8N1, PMS_RX_PIN, -1); 
    delay(100);

    #ifdef CONFIG_ENABLE_SENSOR_UI
    setupCharts();
    #endif

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    deviceName = "AIROWL_" + mac.substring(6);

    while (1) {
          if (pms.read(pmsData)) {
            pmsensor_data.pm1 += pmsData.PM_AE_UG_1_0;
            pmsensor_data.pm25 += pmsData.PM_AE_UG_2_5;
            pmsensor_data.pm10 += pmsData.PM_AE_UG_10_0;
            pmsensor_data.count++;

            Serial.println("---- PMSA003-A Reading ----");
            Serial.print("PM1.0 : "); Serial.print(pmsData.PM_AE_UG_1_0); Serial.println(" μg/m³");
            Serial.print("PM2.5 : "); Serial.print(pmsData.PM_AE_UG_2_5); Serial.println(" μg/m³");
            Serial.print("PM10 : "); Serial.print(pmsData.PM_AE_UG_10_0); Serial.println(" μg/m³");
            Serial.println("-----------------------------");
        }
            #ifdef CONFIG_ENABLE_LVGL
            lv_label_set_text_fmt(ui_pm1value, "%.1f", pmsData.PM_AE_UG_1_0);
            lv_label_set_text_fmt(ui_pm25value, "%.1f", pmsData.PM_AE_UG_2_5);
            lv_label_set_text_fmt(ui_pm10value, "%.1f", pmsData.PM_AE_UG_10_0);
            #endif

            if (pmsensor_data.count >= DATA_FREQ) {
                float avgPM1 = pmsensor_data.pm1 / pmsensor_data.count;
                float avgPM25 = pmsensor_data.pm25 / pmsensor_data.count;
                float avgPM10 = pmsensor_data.pm10 / pmsensor_data.count;
                
                 Serial.printf("[PMS-AVG] PM1.0: %.2f  PM2.5: %.2f  PM10: %.2f µg/m³ (over %d samples)\n",
                      avgPM1, avgPM25, avgPM10, pmsensor_data.count);
                      
                int pm1Index = calculateSubIndex(avgPM1, getBreakpoint(avgPM1, pm1Bps, 6));
                int pm25Index = calculateSubIndex(avgPM25, getBreakpoint(avgPM25, pm25Bps, 6));
                int pm10Index = calculateSubIndex(avgPM10, getBreakpoint(avgPM10, pm10Bps, 6));

                AQI = std::max({pm1Index, pm25Index, pm10Index});

                #ifdef CONFIG_ENABLE_LVGL
                uint32_t pm1_color = getAQIColor(pm1Index);
                uint32_t pm25_color = getAQIColor(pm25Index);
                uint32_t pm10_color = getAQIColor(pm10Index);

                lv_obj_set_style_text_color(ui_pm1value, lv_color_hex(pm1_color), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(ui_pm25value, lv_color_hex(pm25_color), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(ui_pm10value, lv_color_hex(pm10_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                uint32_t eye_color = (millis() - bootTime < EYE_COLOR_ACTIVATION_DELAY) ? 0xFFFFFF : getAQIColor(AQI);
                lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(eye_color), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(eye_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                // Chart updating 
                memcpy(ui_PM1chart_series_1_array, ui_PM1chart_series_1_array + 1,
                       (CHART_DATA_LENGTH - 1) * sizeof(lv_coord_t));
                ui_PM1chart_series_1_array[CHART_DATA_LENGTH - 1] = (uint16_t)avgPM1;
                lv_chart_set_ext_y_array(ui_Chart1, ui_PM1chart_series_1, ui_PM1chart_series_1_array);

                 memcpy(ui_PM25chart_series_1_array, ui_PM25chart_series_1_array + 1, (CHART_DATA_LENGTH - 1) * sizeof(lv_coord_t));
                ui_PM25chart_series_1_array[CHART_DATA_LENGTH - 1] = (uint16_t)avgPM25;
                lv_chart_set_ext_y_array(ui_Chart3, ui_PM25chart_series_1, ui_PM25chart_series_1_array);

                memcpy(ui_PM10chart_series_1_array, ui_PM10chart_series_1_array + 1, (CHART_DATA_LENGTH - 1) * sizeof(lv_coord_t));
                ui_PM10chart_series_1_array[CHART_DATA_LENGTH - 1] = (uint16_t)avgPM10;
                lv_chart_set_ext_y_array(ui_Chart4, ui_PM10chart_series_1, ui_PM10chart_series_1_array);
                #endif

                if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
                    
                    #ifdef CONFIG_ENABLE_LVGL
                    lv_img_set_src(ui_nose, &ui_img_airowl_2_png);
                    #endif

                    String jsonString = "{";
                    jsonString += "\"deviceId\":\"" + deviceName + "\",";
                    jsonString += "\"p3\":" + String(avgPM1, 2) + ",";
                    jsonString += "\"p1\":" + String(avgPM25, 2) + ",";
                    jsonString += "\"p2\":" + String(avgPM10, 2);
                    jsonString += "}";

                    mqttClient.publish("airowl", jsonString.c_str());
                } 
                else {

                    #ifdef CONFIG_ENABLE_LVGL
                    lv_img_set_src(ui_nose, &ui_img_airowl_1_png);
                    #endif
                }

                // Reset accumulators
                pmsensor_data.pm1 = 0;
                pmsensor_data.pm25 = 0;
                pmsensor_data.pm10 = 0;
                pmsensor_data.count = 0;
            }
        }
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));       
    }


void initSensor() {
    BaseType_t result = xTaskCreatePinnedToCore(sensorData, "SensorTask", 8192, nullptr, 2, &PMSTaskHandle, 1);
     if (result == pdPASS && PMSTaskHandle) {
       Serial.println("[Sensor - PMS] PM Srnsor start Task!");
       esp_task_wdt_add(PMSTaskHandle);
    } else {
        Serial.println("[Sensor - PMS] Failed to start Sensor Task!");
         PMSTaskHandle = nullptr;
    }
}

void restartSensorTask() {
//     if (PMSTaskHandle) {
//         vTaskDelete(PMSTaskHandle);
//         // esp_task_wdt_delete(PMSTaskHandle);
//         PMSTaskHandle = nullptr;
//     }
//     initSensor();
// }
BaseType_t result = xTaskCreatePinnedToCore(sensorData, "SensorTask", 8192, nullptr, 2,  &PMSTaskHandle, 1);
    if (result == pdPASS && PMSTaskHandle) {
        esp_task_wdt_add(PMSTaskHandle);
        Serial.println("[Sensor] Sensor task restarted");
    } else {
        Serial.println("[Sensor] Failed to restart Sensor task!");
        PMSTaskHandle = nullptr;
    }
}

#endif // CONFIG_ENABLE_SENSOR_PMSA003A
