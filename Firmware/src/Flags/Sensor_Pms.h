#pragma once
#ifdef CONFIG_ENABLE_SENSOR_PMSA003A

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <PMS.h>
#include <HardwareSerial.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "Flags/mqtt_module.h"

#define CHART_DATA_LENGTH 30
#define DATA_FREQ 10

#define PMS_RX_PIN 44
// #define PMS_TX_PIN 44

typedef struct {
    float pm1 = 0;
    float pm25 = 0;
    float pm10 = 0;
    int pm1_max = 0;
    int pm25_max = 0;
    int pm10_max = 0;
    int count = 0;
} PMSensor_t;

typedef struct {
    float Cp_Lo;
    float Cp_Hi;
    int Ip_Lo;
    int Ip_Hi;
} AQIBreakpoint;

extern PMSensor_t pmsensor_data;
extern PMS pms;

extern TaskHandle_t PMSTaskHandle;
extern String deviceName;

extern volatile int AQI;
extern volatile float temperature;
extern volatile float humidity;

extern AQIBreakpoint pm1Bps[];
extern AQIBreakpoint pm25Bps[];
extern AQIBreakpoint pm10Bps[];

int calculateSubIndex(float Cp, AQIBreakpoint bp);
AQIBreakpoint getBreakpoint(float Cp, AQIBreakpoint* bps, int numBps);
String getAQICategory(int aqi);
uint32_t getAQIColor(int aqi);

#ifdef CONFIG_ENABLE_LVGL
#include "ui/ui.h"
extern lv_chart_series_t* ui_PM1chart_series_1;
extern lv_chart_series_t* ui_PM25chart_series_1;
extern lv_chart_series_t* ui_PM10chart_series_1;
extern unsigned long bootTime;
void setupCharts();
#endif

void initSensor();
void restartSensorTask();

#else
inline void initSensor() {}
inline void restartSensorTask() {}
#endif
