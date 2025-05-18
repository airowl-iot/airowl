#ifndef SEN54_H
#define SEN54_H

#include "esp_err.h"
#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHART_DATA_LENGTH 15
#define DATA_FREQ 5

typedef struct {
    float pm1;
    float pm25;
    float pm4;
    float pm10;
    float tvoc;
    float temperature;
    float humidity;
    int pm1_max;
    int pm25_max;
    int pm4_max;
    int pm10_max;
    int tvoc_max;
    int count;
} SensorReadings;

typedef struct {
    float Cp_Lo; // Low concentration breakpoint
    float Cp_Hi; // High concentration breakpoint
    int Ip_Lo;   // Low index breakpoint
    int Ip_Hi;   // High index breakpoint
} AQIBreakpoint;

esp_err_t sen54_init(void);
bool sen54_read(SensorReadings *readings);
int calculate_aqi(SensorReadings *readings);
uint32_t get_aqi_color(int aqi);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // SEN54_H