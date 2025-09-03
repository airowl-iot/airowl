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

#define FIRMWARE_VERSION "3.0"

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  Serial.println("===== AIROWL BOOT =====");
  Serial.println("Firmware: " + String(FIRMWARE_VERSION));

  #ifdef CONFIG_ENABLE_SENSOR_PMSA003A
  if (HAL::PMS::init()) {
    Serial.println("[HAL] PMS sensor initialized");
  } else {
    Serial.println("[HAL] PMS sensor initialization failed");
  }
  #endif
  
  #ifdef CONFIG_ENABLE_SENSOR_PM700
  if (HAL::PM700::init()) {
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

  CORE::EventBus::getInstance().clear();
  
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char deviceId[13];
  sprintf(deviceId, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  String apName = "AIROWL_" + String(deviceId).substring(6);
  
  Serial.printf("%s\n", apName.c_str());
  
 #ifdef CONFIG_ENABLE_LVGL
  Wire.begin(4,5);
  Wire.setClock(100000);
  vTaskDelay(pdMS_TO_TICKS(100));

  int devices = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("[FOUND] I2C device at 0x");
      Serial.println(address, HEX);
      devices++;
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  if (devices == 0) {
      Serial.println("[RESULT] No I2C devices found.");
    } else {
      Serial.printf("[RESULT] Total %d I2C device(s) found.\n", devices);
    }
  Serial.println("[BOOT] Initializing Display and LVGL...");
  
  if (HAL::Display::init()) {
    ui_init();
  
  if (HAL::Display::restartTask()) {
    Serial.println("[DEBUG][MAIN] LVGL task started");
  } else {
    Serial.println("[DEBUG][MAIN] Failed to start LVGL task");
  }

} else {
  Serial.println("[HAL] Display initialization failed!");
}
#endif

  APP::SensorManager::SensorConfig pmsConfig = {
    .enabled = true,
    .readInterval = 1000,    
    .publishInterval = 60000 
  };
  
  APP::SensorManager::SensorConfig pm700Config = {
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
  
  if (APP::SensorManager::init(pmsConfig, pm700Config, ahtConfig, ens160Config)) {
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
  
  // Use the pre-generated AP name for WiFi service
  if (SVC::WiFiService::init(apName.c_str())) {
    Serial.println("[SVC] WiFi service initialized");
    vTaskDelay(pdMS_TO_TICKS(500));
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

  #ifdef CONFIG_ENABLE_ESP_NOW
  bool deviceIsMaster = true;  
  
  if (SVC::ESPNowService::init(deviceIsMaster)) {
    Serial.println("[SVC] ESP-NOW service initialized");
    
    if (!deviceIsMaster) {
      Serial.println("[CONFIG] Device configured as SLAVE - set master MAC using setMasterMac()");
    } else {
      Serial.println("[CONFIG] Device configured as MASTER");
    }
    
    if (SVC::ESPNowService::startTask()) {
      Serial.println("[SVC] ESP-NOW task started");
    } else {
      Serial.println("[SVC] Failed to start ESP-NOW task");
    }
  } else {
    Serial.println("[SVC] ESP-NOW service initialization failed");
  }
  #endif

  #ifdef CONFIG_ENABLE_OTA_ANEDYA
  Serial.println("[BOOT] Initializing OTA service...");
  if (SVC::OTA::init(REGION_CODE, CONNECTION_KEY, PHYSICAL_DEVICE_ID, ca_cert)) {
    Serial.println("[SVC] OTA service initialized");
    if (SVC::OTA::startTask()) {
      Serial.println("[SVC] OTA task started");
    } else {
      Serial.println("[SVC] Failed to start OTA task");
    }
  } else {
    Serial.println("[SVC] OTA service initialization failed");
  }
  #endif

  if (APP::ModeManager::init(APP::ModeManager::Mode::NORMAL)) {
    Serial.println("[APP] Mode manager initialized");
    APP::ModeManager::startTask();
  } else {
    Serial.println("[APP] Mode manager initialization failed");
  }
  
  Serial.println("[BOOT] System initialization complete");
}

void loop() {


  // Reset watchdog timer
  // esp_task_wdt_reset();

  vTaskDelay(pdMS_TO_TICKS(10));
}
