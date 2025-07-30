#pragma once

#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <SensirionI2CSen5x.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <vector>
#include <stdint.h>

#include "configure.h"
#include "Flags/mqtt_module.h"

#ifdef CONFIG_ENABLE_LVGL
#include "ui/ui.h"
#endif

#ifdef CONFIG_ENABLE_SENSOR_SEN54

#define MAXBUF_REQUIREMENT 48
#define CHART_DATA_LENGTH 15
#define DATA_FREQ 5

#if (defined(I2C_BUFFER_LENGTH) &&                 \
     (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || \
    (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#endif

extern volatile int AQI;           // Declare global AQI variable
extern volatile float temperature; // Declare global temperature variable
extern volatile float humidity;    // Declare global humidity variable

typedef struct {
    float pm1 = 0;
    float pm25 = 0;
    float pm10 = 0;
    float pm4 = 0;
    float tvoc = 0;
    int pm1_max = 0;
    int pm25_max = 0;
    int pm10_max = 0;
    int pm4_max = 0;
    int tvoc_max = 0;
    int count = 0;
} Sensor_t;

extern Sensor_t sensor_data;

extern SensirionI2CSen5x sen5x;
extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern TaskHandle_t sensorTaskHandle;

extern String deviceName;

#ifdef CONFIG_ENABLE_LVGL
extern lv_chart_series_t* ui_PM1chart_series_1;
extern lv_chart_series_t* ui_PM25chart_series_1;
extern lv_chart_series_t* ui_PM4chart_series_1;
extern lv_chart_series_t* ui_PM10chart_series_1;
extern lv_chart_series_t* ui_TVOCchart_series_1;
extern unsigned long bootTime;  // Global timestamp for power-on
#endif

typedef struct {
    float Cp_Lo; // Low concentration breakpoint
    float Cp_Hi; // High concentration breakpoint
    int Ip_Lo;   // Low index breakpoint
    int Ip_Hi;   // High index breakpoint
} AQIBreakpoint;

extern AQIBreakpoint pm1Bps[];
extern AQIBreakpoint pm4Bps[];
extern AQIBreakpoint pm25Bps[];
extern AQIBreakpoint pm10Bps[];
extern AQIBreakpoint tvocBps[];

// Function Declarations
int calculateSubIndex(float Cp, AQIBreakpoint bp);
AQIBreakpoint getBreakpoint(float Cp, AQIBreakpoint* bps, int numBps);
String getAQICategory(int aqi);
uint32_t getAQIColor(int aqi);
void callback(char* topic, byte* message, unsigned int length);
void setupMQTT();
bool isSen5xPresent();
void sensorData(void* parameter);

#ifdef CONFIG_ENABLE_LVGL
void setupCharts();
#endif

void initSensor();
void restartSensorTask();

#else
inline void initSensor() {}
inline void sensorData(void *) {}
#endif