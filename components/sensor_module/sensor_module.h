#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

typedef struct {
    float pm1;
    float pm2_5;
    float pm4;
    float pm10;
    float tvoc;
    float temperature;
    float humidity;
} sensor_data_t;

extern sensor_data_t sensor_data;

void sensor_init(void);
void sensor_task(void *pvParameter);

#endif // SENSOR_MODULE_H
