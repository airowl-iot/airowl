#include <Arduino.h>
#include <esp_task_wdt.h>
#include <Wire.h>
#include "config.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "hal/hal_wifi.h"
#include "hal/hal_pms.h"
#include "hal/hal_aht.h"
#include "hal/hal_ens160.h"

// Core components
#include "core/event_bus.h"

// svc
#include "svc/wifi_service.h"
#include "svc/mqtt_service.h"

#ifdef CONFIG_ENABLE_OTA_ANEDYA
#include "svc/ota_service.h"
#endif

#ifdef CONFIG_ENABLE_ESP_NOW
#include "svc/espnow_service.h"
#endif

// Application modules
#include "app/sensor_manager.h"
#include "app/ui_controller.h"
#include "app/mode_manager.h"

#ifdef CONFIG_ENABLE_LVGL
#include "hal/hal_display.h"
#include "ui/ui.h"
#endif

#ifdef CONFIG_ESP_MATTER_ENABLE
#include "ui/matter_wrapper.h"
#endif

#define FIRMWARE_VERSION "version - 3.1"

void shutdownHandler() {
  Serial.println("[SYSTEM] Shutdown handler called");
  
  #ifdef CONFIG_ESP_MATTER_ENABLE
  Serial.println("[Matter] Cleaning up Matter resources");
  cleanupMatter();
  #endif
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  Serial.println("===== AIROWL BOOT =====");
  Serial.println("Firmware: " + String(FIRMWARE_VERSION));
  
  esp_register_shutdown_handler(shutdownHandler);
  
  #ifdef CONFIG_ENABLE_PMS
  if (HAL::PMS::init()) {
    Serial.println("[HAL] PMS sensor initialized");
  } else {
    Serial.println("[HAL] PMS sensor initialization failed");
  }
  #endif
  
  #ifdef CONFIG_ENABLE_SENSOR_AHT
  if (HAL::AHT::init()) {
    Serial.println("[HAL] AHT sensor initialized");
  } else {
    Serial.println("[HAL] AHT sensor initialization failed");
  }
  #endif
  
  #ifdef CONFIG_ENABLE_SENSOR_ENS160
  if (HAL::ENS160::init()) {
    Serial.println("[HAL] ENS160 sensor initialized");
  } else {
    Serial.println("[HAL] ENS160 sensor initialization failed");
  }
  #endif
  
  Serial.println("[BOOT] Initializing core components...");
  CORE::EventBus::getInstance().clear();

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char deviceId[13];
  sprintf(deviceId, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  String apName = "AIROWL_" + String(deviceId).substring(6);
  
  Serial.println("[BOOT] Initializing svc...");

  if (SVC::WiFiService::init(apName.c_str())) {
    Serial.println("[SVC] WiFi service initialized");
    delay(500);
    SVC::WiFiService::startTask();
    SVC::WiFiService::connect(); 
  } else {
    Serial.println("[SVC] WiFi service initialization failed");
  }

    if (SVC::MQTTService::init(apName.c_str())) {
    Serial.println("[SVC] MQTT service initialized");
    SVC::MQTTService::startTask();
  } else {
    Serial.println("[SVC] MQTT service initialization failed");
  }

 #ifdef CONFIG_ENABLE_LVGL

  Wire.begin(4,5);
  Wire.setClock(100000);
  delay(100);

  int devices = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("[FOUND] I2C device at 0x");
      Serial.println(address, HEX);
      devices++;
    }
    delay(5);
  }

  if (devices == 0) {
      Serial.println("[RESULT] No I2C devices found.");
    } else {
      Serial.printf("[RESULT] Total %d I2C device(s) found.\n", devices);
    }
  Serial.println("[BOOT] Initializing Display and LVGL...");
  
 
  if (HAL::Display::init()) {
    ui_init();
    // lv_label_set_text(ui_devicename, apName.c_str());
    // lv_label_set_text(ui_qrcodename, apName.c_str());
    // lv_label_set_text(ui_firmwareversion, FIRMWARE_VERSION);
    
    // String qrcodeurl = "WIFI:T:WPA;S:" + apName + ";P:12345678;;";
    // lv_obj_t* qrcode_obj = lv_qrcode_create(ui_qrcode, 150, lv_color_black(), lv_color_white());
    // lv_obj_center(qrcode_obj);
    // lv_qrcode_update(qrcode_obj, qrcodeurl.c_str(), qrcodeurl.length());

    if (HAL::Display::restartTask()) {
      Serial.println("[DEBUG][MAIN] LVGL task started");
    } else {
      Serial.println("[DEBUG][MAIN] Failed to start LVGL task");
    }

    // lv_obj_t * test_msg = lv_label_create(ui_Intro);
    // lv_label_set_text(test_msg, "Display Test - Working!");
    // lv_obj_set_style_text_color(test_msg, lv_color_white(), 0);
    // lv_obj_align(test_msg, LV_ALIGN_BOTTOM_MID, 0, -20);
  } else {
    Serial.println("[HAL] Display initialization failed!");
  }
  #endif

    #ifdef CONFIG_ENABLE_ESPNOW
  if (SVC::ESPNowService::init()) {
    Serial.println("[SVC] ESP-NOW service initialized");
    SVC::ESPNowService::startTask();
  } else {
    Serial.println("[SVC] ESP-NOW service initialization failed");
  }
  #endif

  #ifdef CONFIG_ESP_MATTER_ENABLE
  Serial.println("[BOOT] Initializing Matter...");
  try {
    initMatter();
    Serial.println("[Matter] Matter initialized");
  } catch (const std::exception& e) {
    Serial.printf("[Matter] Initialization failed: %s\n", e.what());
    cleanupMatter();
  } catch (...) {
    Serial.println("[Matter] Initialization failed with unknown error");
    cleanupMatter();
  }
  #endif

  Serial.println("[BOOT] Initializing application modules...");
  
  APP::SensorManager::SensorConfig pmsConfig = {
    .enabled = true,
    .readInterval = 1000,
    .publishInterval = 60000
  };
  
  APP::SensorManager::SensorConfig ahtConfig = {
    .enabled = true,
    .readInterval = 1000,
    .publishInterval = 60000
  };
  
  APP::SensorManager::SensorConfig ens160Config = {
    .enabled = true,
    .readInterval = 1000,
    .publishInterval = 60000
  };
  
  if (APP::SensorManager::init(pmsConfig, ahtConfig, ens160Config)) {
    Serial.println("[APP] Sensor manager initialized");
    APP::SensorManager::start();
    APP::SensorManager::startTask();
  } else {
    Serial.println("[APP] Sensor manager initialization failed");
  }
  
  if (APP::UIController::init()) {
    Serial.println("[APP] UI controller initialized");
    APP::UIController::startTask();
  } else {
    Serial.println("[APP] UI controller initialization failed");
  }

  if (APP::ModeManager::init(APP::ModeManager::Mode::NORMAL)) {
    Serial.println("[APP] Mode manager initialized");
    APP::ModeManager::startTask();
  } else {
    Serial.println("[APP] Mode manager initialization failed");
  }
  
  Serial.println("[BOOT] System initialization complete");
}

void loop() {

  #ifdef CONFIG_ENABLE_LVGL
  static unsigned long lastDisplayDebug = 0;
  if (millis() - lastDisplayDebug > 1000) { 
    lastDisplayDebug = millis();
  }

  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 1000) {
    lastTimeUpdate = millis();
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    lv_label_set_text(ui_time, timeStr);
  }

  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 5000) {
    lastDisplayUpdate = millis();
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    
    // Serial.printf("[DEBUG][MAIN] Display update cycle - Current time: %s\n", timeStr);
    // Serial.printf("[DISPLAY] Current time: %s\n", timeStr);
    
    //   Serial.printf("[DEBUG][MAIN] Free heap: %u bytes\n", ESP.getFreeHeap());
    //   Serial.printf("[DISPLAY] Free heap: %u bytes\n", ESP.getFreeHeap());

  }
  #endif
  
  #ifdef CONFIG_ESP_MATTER_ENABLE
  try {
    matter_loop();
  } catch (const std::exception& e) {
    Serial.printf("[Matter] Error in matter_loop: %s\n", e.what());
  } catch (...) {
    Serial.println("[Matter] Unknown error in matter_loop");
  }
  #endif

  static unsigned long lastHealthLog = 0;
  if (millis() - lastHealthLog > 60000) {
    lastHealthLog = millis();
    // Serial.printf("[HEALTH] Free heap: %u bytes\n", ESP.getFreeHeap());
    
    unsigned long uptime = millis() / 1000;
    unsigned long days = uptime / (24 * 3600);
    unsigned long hours = (uptime % (24 * 3600)) / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;
    Serial.printf("[HEALTH] Uptime: %lu days, %lu hours, %lu minutes, %lu seconds\n", 
                  days, hours, minutes, seconds);
  }
  
  // Reset watchdog timer
  // esp_task_wdt_reset();

  delay(10);
}
