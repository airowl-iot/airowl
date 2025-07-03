#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFiManager.h>
#include <WiFiManagerTz.h>
#include "Sensor.h"
#include "ui/lv_setup.h"
#include "ui/ui.h"
#include "config.h"
#include "time_func.h"
#include <FS.h>
#include <Matter.h>
#include <MatterEndpoints/MatterAirQualitySensor.h>
#include <MatterEndpoints/MatterTemperatureSensor.h>
#include <MatterEndpoints/MatterHumiditySensor.h>  // Added Humidity Sensor
#include <Preferences.h>

#define WDT_TIMEOUT 60
#define FIRMWARE_VERSION "version - 1.1"

WiFiManager wm;
TaskHandle_t sensorTaskHandle;
TaskHandle_t lvglTaskHandle;
TaskHandle_t wifiTaskHandle;
Preferences preferences;

MatterAirQualitySensor air_quality_sensor;
MatterTemperatureSensor temperature_sensor;  
MatterHumiditySensor humidity_sensor;       

void on_time_available(struct timeval *t) {
  Serial.println("Received time adjustment from NTP");
  struct tm timeInfo;
  getLocalTime(&timeInfo, 1000);
  Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

// LVGL/UI FreeRTOS task (runs on Core 0)
void lvgl_freertos_task(void *param) {
  esp_task_wdt_add(NULL);
  //Serial.println("LVGL task started on core: " + String(xPortGetCoreID()));
  const TickType_t delay = pdMS_TO_TICKS(16); // ~60fps
  while (1) {
    lv_handler();
    esp_task_wdt_reset();
    vTaskDelay(delay);
  }
}

// WiFiManager/WiFi FreeRTOS task (runs on Core 1)
void wifi_management_task(void *param) {
  esp_task_wdt_add(NULL);
  //Serial.println("WiFi task started on core: " + String(xPortGetCoreID()));
  while (1) {
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz
  } 
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting AIROWL device setup...");

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  delay(500); 
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String apName = "AIROWL_" + mac.substring(6);

  // Display/UI init
  lv_begin();
  ui_init();
  lv_label_set_text(ui_devicename, apName.c_str());
  lv_label_set_text(ui_qrcodename, apName.c_str());
  lv_label_set_text(ui_firmwareversion, FIRMWARE_VERSION);

  String qrcodeurl = (WiFi.status() == WL_CONNECTED) ? "https://opendata.oizom.com/device/" + apName : "WIFI:T:WPA;S:" + apName + ";P:12345678;;";
  lv_obj_t* qrcode_obj = lv_qrcode_create(ui_qrcode, 150, lv_color_black(), lv_color_white());
  lv_obj_center(qrcode_obj);
  lv_qrcode_update(qrcode_obj, qrcodeurl.c_str(), qrcodeurl.length());
  
  // LVGL/UI task (Core 0, high priority)
  xTaskCreatePinnedToCore(lvgl_freertos_task, "LVGL", 8192, NULL, 3, &lvglTaskHandle, 0);

  // Sensor task (Core 1, medium priority)
  xTaskCreatePinnedToCore(sensorData, "sensorData", 8192, NULL, 2, &sensorTaskHandle, 1);
  esp_task_wdt_add(sensorTaskHandle);

  // WiFiManager setup
  WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
  WiFiManagerNS::init(&wm, nullptr);
  std::vector<const char *> menu = {"wifi", "info", "custom", "param", "sep", "restart", "exit"};
  wm.setMenu(menu);
  wm.setConfigPortalBlocking(false);
  wm.setTitle("AIROWL Configuration");
  wm.setConfigPortalTimeout(120);
  wm.setConnectTimeout(30);
  wm.setDebugOutput(true);
  wm.autoConnect(apName.c_str(), "12345678");

  xTaskCreatePinnedToCore(wifi_management_task, "WiFiMgr", 6144, NULL, 1, &wifiTaskHandle, 1);
  esp_task_wdt_add(wifiTaskHandle);

  // Time init
  time_init();
  Serial.println("Setup completed successfully");
}

void loop() {
  update_time();
  vTaskDelay(pdMS_TO_TICKS(50));

  static int lastAQI = -1;
  static float lastTemp = 0.0;
  static float lastHum = 0.0;
  if (AQI != lastAQI) {
    Serial.printf("AQI from Sensor: %d\n", AQI);
    lastAQI = AQI;
  }
  if (temperature != lastTemp || humidity != lastHum) {
    Serial.printf("Temp: %.1f°C | Hum: %.1f%%\n", temperature, humidity);
    lastTemp = temperature;
    lastHum = humidity;
  }
  
  static bool matter_initialized = false;
  if (!matter_initialized && WiFi.status() == WL_CONNECTED) {
    Serial.println("Starting Matter Setup...");
    air_quality_sensor.begin(AQI);
    temperature_sensor.begin(temperature);  // Initialize Temperature Sensor
    humidity_sensor.begin(humidity);       // Initialize Humidity Sensor
    ArduinoMatter::begin();
    matter_initialized = true;
    Serial.println("Matter initialized");
  }

  static bool was_commissioned = false;
  if (matter_initialized) {
    if (!ArduinoMatter::isDeviceCommissioned()) {
      Serial.println("Matter Node not commissioned");
      Serial.println("Manual pairing code: " + ArduinoMatter::getManualPairingCode());
      Serial.println("QR code URL: " + ArduinoMatter::getOnboardingQRCodeUrl());
    } else if (!was_commissioned) {
      Serial.println("Matter Node commissioned");
      was_commissioned = true;
    }

    static uint32_t last_read_time = 0;
    if (millis() - last_read_time > 5000) {
      last_read_time = millis();
      air_quality_sensor.setAQI(AQI);
      temperature_sensor.setTemperature(temperature);  // Update Temperature Value
      humidity_sensor.setHumidity(humidity);          // Update Humidity Value
    }
  }
}

