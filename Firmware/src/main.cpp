#include <Arduino.h>
#include <esp_task_wdt.h>
#include <Wire.h>
#include "airowl_config.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "manager/sensor_manager.h"
#include "manager/config_manager.h"
#include "manager/service_manager.h"
#include "manager/ui_manager.h"
#include <nvs_flash.h>

#define WDT_TIMEOUT_SECONDS 180

static void scanI2CBus(TwoWire &bus, const char *name, int sda, int scl) {
  Serial.printf("\n[SCAN] %s (SDA=%d, SCL=%d)\n", name, sda, scl);
  int nDevices = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    bus.beginTransmission(address);
    uint8_t error = bus.endTransmission();
    if (error == 0) {
      Serial.printf("  Found device at 0x%02X\n", address);
      ++nDevices;
    } else if (error == 4) {
      Serial.printf("  Unknown error at 0x%02X\n", address);
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  if (nDevices == 0) Serial.println("  No I2C devices found");
  else Serial.printf("  Done, found %d device(s)\n", nDevices);
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n===== AIROWL BOOT =====");

  Serial.println("[BOOT] Initializing NVS flash...");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.println("[BOOT] NVS partition needs to be erased, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  if (ret == ESP_OK) {
    Serial.println("[BOOT] NVS initialized successfully");
  } else {
    Serial.printf("[BOOT] ERROR: NVS initialization failed with code: 0x%x\n", ret);
  }

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_reconfigure(&wdt_config);  
  esp_task_wdt_add(NULL);


  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(400000);

  vTaskDelay(pdMS_TO_TICKS(100));

  scanI2CBus(Wire,  "I2C0 (Display/Touch)", TOUCH_SDA, TOUCH_SCL);

  if (!ConfigManager::getInstance()->initialize()) {
    Serial.println("[BOOT] ConfigManager init failed!");
  } else {
    ConfigManager::getInstance()->printConfig();
  }

  if (!APP::SensorManager::init()) {
    Serial.println("[BOOT] SensorManager init failed!");
    } else {
    APP::SensorManager::start();
    APP::SensorManager::startTask();
    Serial.println("[BOOT] SensorManager started");
    }

  if (!APP::UIController::init()) {
    Serial.println("[BOOT] UIController init failed!");
  } else {
    Serial.println("[BOOT] UIController initialized successfully");
    if (APP::UIController::startTask()) {
      Serial.println("[BOOT] UIController task started");
    } else {
      Serial.println("[BOOT] Failed to start UIController task");
    }
  }

  if (!APP::ServiceManager::getInstance().init()) {
      Serial.println("[BOOT] ServiceManager init failed!");
  } else {
      if (!APP::ServiceManager::getInstance().start()) {
          Serial.println("[BOOT] Failed to start services!");
      }
  }

}

void loop() {
  esp_task_wdt_reset();
  vTaskDelay(pdMS_TO_TICKS(10));
}