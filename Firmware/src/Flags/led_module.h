#ifndef LED_MODULE_H
#define LED_MODULE_H

#include <Arduino.h>

#define LED_PIN 1

// Device states
typedef enum {
    DEVICE_STATE_DISCONNECTED,   // Not connected to WiFi or reading sensor
    DEVICE_STATE_WIFI_CONNECTED, // Connected to WiFi, not publishing
    DEVICE_STATE_MQTT_PUBLISHING, // Publishing to MQTT
    DEVICE_STATE_UNKNOWN // Unknown state
} device_state_t;


void led_status_init(uint8_t pin = 1);
void led_status_update(device_state_t state);
void led_status_loop();

// FreeRTOS task management
void led_task_start();
void restartLEDTask();

#endif // LED_MODULE_H
