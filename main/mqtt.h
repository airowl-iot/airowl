#ifndef MQTT_H
#define MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h> // Added for bool type

// Initialize MQTT client with device ID
esp_err_t mqtt_init(const char *device_id);

// Publish sensor data to MQTT
esp_err_t mqtt_publish_sensor_data(float pm1, float pm25, float pm4, float pm10, float tvoc);

// Check if MQTT is connected
bool mqtt_is_connected(void);

// Stop MQTT client
void mqtt_stop(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_H