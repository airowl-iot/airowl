// lv_setup.h — LVGL + Display + Touch Interface
#pragma once

#ifdef CONFIG_ENABLE_LVGL

#include <Arduino.h>

// Public API
void lv_begin();            // Call in setup()
void lv_handler();          // Call in loop() or inside task
void lvgl_task(void *param); // The LVGL FreeRTOS task
void restartLVGLTask();     // Restart LVGL task after OTA, crash, etc.

// Task handle for OTA/task restart coordination
extern TaskHandle_t lvglTaskHandle;

#else

// Safe stubs if LVGL is disabled via build flag
inline void lv_begin() {}
inline void lv_handler() {}
inline void lvgl_task(void *) {}
inline void restartLVGLTask() {}
#endif
