#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include "WifiManager.h"
#include <Wire.h>
// #include "time_func.h"
#include "configure.h"
#include <driver/rtc_io.h>
#include "Config.h"
#include "SPIFFS.h"
#include <driver/touch_sensor.h>

AsyncWebServer webServer(80);
WIFIMANAGER WifiManager;
esp_err_t getErr = ESP_OK;

#ifdef CONFIG_ENABLE_LVGL
#include "ui/lv_setup.h"
#include "ui/ui.h"
#endif

#ifdef CONFIG_ENABLE_SENSOR_SEN54
#include "Sensor.h"
#endif

#ifdef CONFIG_ENABLE_SENSOR_SHT
#include "Sensor_sht.h"
#endif

#ifdef CONFIG_ENABLE_OTA_ANEDYA
#include "Flags/ota_module.h"
#endif

#ifdef CONFIG_ENABLE_ESP_NOW
#include "Flags/espnow_module.h"
#endif

#ifdef CONFIG_ENABLE_VOICE_ASSISTANT
#include "Voice_assistant.h"
#endif

#ifdef CONFIG_ESP_MATTER_ENABLE
#include "ui/matter_wrapper.h"
#endif

#define WDT_TIMEOUT_SECONDS 500
#define FIRMWARE_VERSION "version - 3.1"

String apName = "AIROWL_XXXX";

void setupWiFi()
{
    WifiManager.startBackgroundTask(apName);        // Run the background task to take care of our Wifi
    WifiManager.fallbackToSoftAp(true);       // Run a SoftAP if no known AP can be reached
    WifiManager.attachWebServer(&webServer);  // Attach our API to the Webserver 
    WifiManager.attachUI();                   // Attach the UI to the Webserver
  
    // Run the Webserver and add your webpages to it
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->redirect("/wifi");
    });
    webServer.onNotFound([&](AsyncWebServerRequest *request) {
      request->send(404, "text/plain", "Not found");
    });
    webServer.begin();
}

// -------------------- Handle NTP sync --------------------
void on_time_available(struct timeval *t) {
  struct tm timeInfo;
  getLocalTime(&timeInfo, 1000);
  Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
  // Serial.printf("[Time] NTP time set: %ld\n", t->tv_sec);
  // settimeofday(t, nullptr);
}

// -------------------- Setup --------------------
void setup() {
  
  Serial.begin(115200);
  Serial.println("===== AIROWL BOOT =====");

  esp_task_wdt_init(WDT_TIMEOUT_SECONDS * 1000, true);
  esp_task_wdt_add(NULL);

  
  String mac = WiFi.macAddress(); mac.replace(":", "");
  apName = "AIROWL_" + mac.substring(6);
  // ----- LVGL UI -----
  #ifdef CONFIG_ENABLE_LVGL
    Wire.begin(4, 5);
    delay(1000);
    Serial.println("\n[SCAN] Starting I2C Bus Scan...");

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
    Serial.println("[LVGL] Starting LVGL initialization...");
    lv_begin();
    Serial.println("[LVGL] LVGL begin completed");
    ui_init();
 
    Serial.println("[LVGL] LVGL task started");
    lv_label_set_text(ui_devicename, apName.c_str());
    lv_label_set_text(ui_qrcodename, apName.c_str());
    lv_label_set_text(ui_firmwareversion, FIRMWARE_VERSION);

    String qrcodeurl = (WiFi.status() == WL_CONNECTED) ? "https://opendata.oizom.com/device/" + apName : "WIFI:T:WPA;S:" + apName + ";P:12345678;;";
    lv_obj_t* qrcode_obj = lv_qrcode_create(ui_qrcode, 150, lv_color_black(), lv_color_white());
    lv_obj_center(qrcode_obj);
    lv_qrcode_update(qrcode_obj, qrcodeurl.c_str(), qrcodeurl.length());
  #else
  // ----- Fallback LED status if no LVGL -----
    // delay(500);
    // led_status_init(LED_PIN);   // Initializes the NeoPixel
    // restartLEDTask(); 
    // Serial.println("[LED] LED initialized");
  #endif

  // ----- Sensor Task -----
  #ifdef CONFIG_ENABLE_SENSOR_SEN54
    initSensor();
    Serial.println("[Sensor] Sensor task started");
  #endif

  // ----- SHT Sensor Task -----
  #ifdef CONFIG_ENABLE_SENSOR_SHT
    initSHTTask();
    Serial.println("[SHT] SHT Sensor task started");
  #endif

  //  ----- voice assistant -----
  #ifdef CONFIG_ENABLE_VOICE_ASSISTANT
    initVoiceAssistantTask();
  #endif

  // ----- ESP-NOW Comm -----
  #ifdef CONFIG_ENABLE_ESP_NOW
    initESPNow();
    restartESPNowTask();
    Serial.println("[ESP-NOW] ESP-NOW initialized and task started");
  #endif

  // ----- Anedya OTA -----
  #ifdef CONFIG_ENABLE_OTA_ANEDYA
    initOTA();
    Serial.println("[OTA] OTA initialized");
  #endif

  // ----- Matter -----
  #ifdef CONFIG_ESP_MATTER_ENABLE
    initMatter();
    Serial.println("[Matter] Matter Initialized");
  #endif
  setupWiFi();
  // ----- Clock Sync -----
  // time_init();
}

// -------------------- Main Loop --------------------
void loop() {
  #ifdef CONFIG_ENABLE_LVGL
    lv_handler();
  #endif

  #ifndef CONFIG_ENABLE_LVGL
    //led logic if needed
  #endif

  #ifdef CONFIG_ENABLE_OTA_ANEDYA
    ota_loop();
  #endif

  #ifdef CONFIG_ENABLE_SENSOR_SEN54
  #endif

  #ifdef CONFIG_ENABLE_SENSOR_SHT
  #endif

  #ifdef CONFIG_ENABLE_ESP_NOW
    espnow_loop();
  #endif

  #ifdef CONFIG_ESP_MATTER_ENABLE
    matter_loop();
  #endif

  // update_time();
  esp_task_wdt_reset(); 
}