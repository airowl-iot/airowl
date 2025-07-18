// espnow_module.h — ESP-NOW interface with build flag

#pragma once

#ifdef CONFIG_ENABLE_ESP_NOW

#include <Arduino.h>

void MQTTSetup();
void initESPNow();              // Initialize ESP-NOW (setup + task)
void espnow_loop();            // Loop logic (only for master)
void espnow_loop_task(void *param); // Task wrapper
void restartESPNowTask();      // OTA-compatible task restart

extern bool isMaster;
extern TaskHandle_t espnowTaskHandle;

#else

inline void MQTTSetup();
inline void initESPNow() {}
inline void espnow_loop() {}
inline void espnow_loop_task(void*) {}
inline void restartESPNowTask() {}

#endif
