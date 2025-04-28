#ifndef SENSOR_UI_H
#define SENSOR_UI_H

#include "ui.h"
#include "lvgl.h"

#define CHART_DATA_LENGTH 15

typedef struct
{
    float pm1;
    float pm25;
    float pm4;
    float pm10;
    float tvoc;
    float temperature;
    float humidity;
} SensorReadings;

// Call this once after ui_init() to initialize charts
void setup_charts();

// Call every time you get new sensor values
void update_sensor_ui(const SensorReadings *readings,
                      int pm1Index, int pm25Index,
                      int pm4Index, int pm10Index,
                      int tvocIndex);

// Call after update_sensor_ui to feed data into graphs
void update_all_charts(const SensorReadings *readings);

#endif
