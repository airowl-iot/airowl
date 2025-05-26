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
    int pm1_max;   // Not used in this context, can be kept for other purposes
    int pm25_max;  // Not used
    int pm4_max;   // Not used
    int pm10_max;  // Not used
    int tvoc_max;  // Not used
    int count;
    int pm1_aqi;   // New: AQI sub-index for PM1
    int pm25_aqi;  // New: AQI sub-index for PM2.5
    int pm4_aqi;   // New: AQI sub-index for PM4
    int pm10_aqi;  // New: AQI sub-index for PM10
    int tvoc_aqi;  // New: AQI sub-index for TVOC
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