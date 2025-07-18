#pragma once

#ifdef CONFIG_ENABLE_SENSOR_SHT

#include <Arduino.h>
#include <esp_task_wdt.h>

void initSHTTask();
void restartSHTTask();

#endif
