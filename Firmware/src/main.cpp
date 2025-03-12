#include <Arduino.h>
#if defined(ARDUINO_M5STACK_Core2)
#include <M5Core2.h>
#endif
#if defined(ARDUINO_M5STACK_CORES3)
#include <M5Unified.h>
#endif
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
TaskHandle_t myTaskHandle;
Preferences preferences;
MatterAirQualitySensor air_quality_sensor;
MatterTemperatureSensor temperature_sensor;  // Temperature Sensor
MatterHumiditySensor humidity_sensor;       // Added Humidity Sensor

void on_time_available(struct timeval *t) {
  Serial.println("Received time adjustment from NTP");
  struct tm timeInfo;
  getLocalTime(&timeInfo, 1000);
  Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
  M5.Rtc.setDateTime(&timeInfo);
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Display.setRotation(3);

  if (M5.Rtc.isEnabled()) {
    M5.Log.println("RTC Enabled");
  }

  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMEOUT * 1000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String apName = "AIROWL_" + mac.substring(6);

  WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
  WiFiManagerNS::init(&wm, nullptr);
  std::vector<const char *> menu = {"wifi", "info", "custom", "param", "sep", "restart", "exit"};

  wm.setMenu(menu);
  wm.setConfigPortalBlocking(false);
  wm.setTitle("AIROWL Configuration");
  wm.autoConnect(apName.c_str(), "12345678");

  lv_begin();
  ui_init();
  lv_label_set_text(ui_devicename, apName.c_str());
  lv_label_set_text(ui_qrcodename, apName.c_str());
  lv_label_set_text(ui_firmwareversion, FIRMWARE_VERSION);

  String qrcodeurl = (WiFi.status() == WL_CONNECTED) ? "https://opendata.oizom.com/device/" + apName : "WIFI:T:WPA;S:" + apName + ";P:12345678;;";
  ui_qrcodedata = qrcodeurl.c_str();
  lv_qrcode_update(ui_qrcode_obj, ui_qrcodedata, strlen(ui_qrcodedata));
  lv_obj_center(ui_qrcode_obj);

  time_init();
  xTaskCreatePinnedToCore(sensorData, "sensorData", 10000, NULL, 2, &myTaskHandle, 1);
  esp_task_wdt_add(myTaskHandle);

  preferences.begin("MatterPrefs", false);
}

void loop() {
  wm.process();
  lv_handler();
  update_time();
  esp_task_wdt_reset();

  static int lastAQI = -1;
  static float lastTemp = 0.0;
  static float lastHum = 0.0;
  if (AQI != lastAQI) {
    M5.Log.printf("AQI from Sensor: %d\n", AQI);
    lastAQI = AQI;
  }
  if (temperature != lastTemp || humidity != lastHum) {
    Serial.print("Temperature from Sensor: ");
    Serial.print(temperature);
    Serial.println(" C");
    Serial.print("Humidity from Sensor: ");
    Serial.print(humidity);
    Serial.println(" %");
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