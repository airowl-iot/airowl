#include "ota_module.h"
#include "ota_config.h"

#ifdef CONFIG_ENABLE_OTA_ANEDYA

#include "HttpsOTAUpdate.h"


#ifdef CONFIG_ENABLE_SENSOR_SEN54
#include "Sensor.h"
#endif

#ifdef CONFIG_ENABLE_SENSOR_SHT
#include "Sensor_sht.h"
#endif

#ifdef CONFIG_ENABLE_LVGL
#include "ui/lv_setup.h"
#else
#include "Flags/led_module.h"
#endif

#ifdef CONFIG_ENABLE_ESP_NOW
#include "Flags/espnow_module.h"
#endif

#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <esp_task_wdt.h>

extern WiFiManager wm;

WiFiClientSecure httpsClient;
static HttpsOTAStatus_t otaStatus;

extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t lvglTaskHandle;
extern TaskHandle_t ledTaskHandle;
extern TaskHandle_t wifiTaskHandle;
extern TaskHandle_t espnowTaskHandle;

bool otaInProgress = false;
bool suppressSensorPrinting = false;
bool deploymentAvailable = false;
bool statusPublished = false;

unsigned long last_check_for_ota_update = 0;
unsigned long last_heartbeat = 0;
const unsigned long check_for_ota_interval = 60000;
const unsigned long heartbeat_interval = 60000;

String assetURL = "";
String deploymentID = "";

// ---------------------------- OTA Event Handler ----------------------------
void HttpEvent(HttpEvent_t *event)
{
    switch (event->event_id)
    {
    case HTTP_EVENT_ERROR:
        Serial.println("Http Event Error");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        Serial.println("Http Event On Connected");
        break;
    case HTTP_EVENT_HEADER_SENT:
        Serial.println("Http Event Header Sent");
        break;
    case HTTP_EVENT_ON_HEADER:
        Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        break;
    case HTTP_EVENT_ON_FINISH:
        Serial.println("Http Event On Finish");
        break;
    case HTTP_EVENT_DISCONNECTED:
        Serial.println("Http Event Disconnected");
        break;
    case HTTP_EVENT_REDIRECT:
        Serial.println("Http Event Redirect");
        break;
    }
}

// ------------- Function to synchronize time -----------
void setDevice_time() {
  String url = anedyaApi("/v1/time");
  Serial.print("ATS time sync ");
  while (true) {
    long long deviceSendTime = millis();
    StaticJsonDocument<128> req;
    req["deviceSendTime"] = deviceSendTime;
    String payload;
    serializeJson(req, payload);

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(payload);
    if (code == 200) {
      StaticJsonDocument<256> res;
      deserializeJson(res, http.getString());
      long long serverReceiveTime = res["serverReceiveTime"];
      long long serverSendTime = res["serverSendTime"];
      long long deviceRecvTime = millis();
      long long cur = (serverReceiveTime + serverSendTime + deviceRecvTime - deviceSendTime) / 2;
      setTime(cur / 1000);
      Serial.println("✓");
      http.end();
      break;
    } else {
      Serial.print(".");
      delay(2000);
      http.end();
    }
  }
}

// ---------------------- Function to check for OTA update ----------------------
bool anedya_check_ota_update(char *assetURLBuf, char *deploymentIDBuf) {
  HTTPClient http;
  String url = anedyaApi("/v1/ota/next");
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Auth-mode", "key");
  http.addHeader("Authorization", CONNECTION_KEY);
  http.setTimeout(5000);

  int code = http.POST("{}");
  if (code <= 0) {
    Serial.println("HTTP error: " + String(code));
    http.end();
    return false;
  }

  StaticJsonDocument<1024> doc;
  deserializeJson(doc, http.getString());
  http.end();

  if (doc["errorcode"] != 0) {
    Serial.println("OTA API error");
    return false;
  }
  if (!(bool)doc["deploymentAvailable"]) return false;

  String urlStr = doc["data"]["asseturl"].as<String>();
  String depStr = doc["data"]["deploymentId"].as<String>();
  urlStr.toCharArray(assetURLBuf, 300);
  depStr.toCharArray(deploymentIDBuf, 50);
  return true;
}

// ---------------------- Function to update OTA Status ----------------------
void anedya_update_ota_status(const char *deploymentID, const char *deploymentStatus) {
  HTTPClient http;
  String url = anedyaApi("/v1/ota/updateStatus");
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Auth-mode", "key");
  http.addHeader("Authorization", CONNECTION_KEY);

  String body = "{\"deploymentId\":\"" + String(deploymentID) +
                "\",\"status\":\"" + String(deploymentStatus) +
                "\",\"log\":\"log\"}";
  int code = http.POST(body);
  if (code > 0) Serial.println("Status -> " + String(deploymentStatus));
  else Serial.println("Status POST error");
  http.end();
}

//---------------------------------- Function to send heartbeat -----------------------------------
void anedya_sendHeartbeat() {
  HTTPClient http;
  String url = anedyaApi("/v1/heartbeat");
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Auth-mode", "key");
  http.addHeader("Authorization", CONNECTION_KEY);
  int code = http.POST("{}");
  if (code > 0) Serial.println("Heartbeat sent");
  else Serial.println("Heartbeat error");
  http.end();
}

void initOTA() {
  httpsClient.setCACert(ca_cert);
  HttpsOTA.onHttpEvent(HttpEvent);
  setDevice_time();
}

/*---------------------LOOP SECTION-------------------------------*/
void ota_loop() {
  unsigned long now = millis();

  if (now - last_heartbeat >= heartbeat_interval) {
    last_heartbeat = now;
    anedya_sendHeartbeat();
  }

  if (!otaInProgress && (now - last_check_for_ota_update) >= check_for_ota_interval) {
    last_check_for_ota_update = now;
    char urlBuf[300] = "";
    char depBuf[50] = "";
    if (anedya_check_ota_update(urlBuf, depBuf)) {
      assetURL = urlBuf;
      deploymentID = depBuf;
      deploymentAvailable = true;
    }
  }

  if (deploymentAvailable && !otaInProgress) {
    otaInProgress = true;
    suppressSensorPrinting = true;

    #ifdef CONFIG_ENABLE_SENSOR_SEN54
      if (sensorTaskHandle) {
        Serial.println("[OTA] Stopping sensor task");
        esp_task_wdt_delete(sensorTaskHandle);
        vTaskDelete(sensorTaskHandle);
        sensorTaskHandle = nullptr;
      }
    #endif

    #ifdef CONFIG_SENSOR_SHT
      if (shtTaskHandle) {
        Serial.println("[OTA] Stopping SHT task");
        esp_task_wdt_delete(shtTaskHandle);
        vTaskDelete(shtTaskHandle);
        shtTaskHandle = nullptr;
      }
    #endif

    #ifdef CONFIG_ENABLE_LVGL
      if (lvglTaskHandle) {
        Serial.println("[OTA] Stopping LVGL task");
        esp_task_wdt_delete(lvglTaskHandle);
        vTaskDelete(lvglTaskHandle);
        lvglTaskHandle = nullptr;
      }
    #endif

    #ifndef CONFIG_ENABLE_LVGL
      if (ledTaskHandle) {
        Serial.println("[OTA] Stopping LED task");
        esp_task_wdt_delete(ledTaskHandle);
        vTaskDelete(ledTaskHandle);
        ledTaskHandle = nullptr;
      }
    #endif

    #ifdef CONFIG_ENABLE_ESP_NOW
      if (espnowTaskHandle) {
        Serial.println("[OTA] Stopping ESP-NOW task");
        esp_task_wdt_delete(espnowTaskHandle);
        vTaskDelete(espnowTaskHandle);
        espnowTaskHandle = nullptr;
      }
    #endif

    anedya_update_ota_status(deploymentID.c_str(), "start");
    Serial.println("[OTA] Starting from: " + assetURL);

    esp_task_wdt_config_t ota_wdt_cfg = {
      .timeout_ms = 300000,
      .idle_core_mask = 0,
      .trigger_panic = true
    };
    esp_task_wdt_reconfigure(&ota_wdt_cfg);

    if (esp_get_free_heap_size() < 80000) {
      Serial.println("[OTA] Heap too low - abort");
      anedya_update_ota_status(deploymentID.c_str(), "failure");
      otaInProgress = false;
    } else {
      HttpsOTA.begin(assetURL.c_str(), ca_cert, false);
      const uint32_t OTA_TIMEOUT = 300000;
      uint32_t start = millis();

      while (otaInProgress && (millis() - start) < OTA_TIMEOUT) {
        esp_task_wdt_reset();
        otaStatus = HttpsOTA.status();

        if (otaStatus == HTTPS_OTA_SUCCESS) {
          Serial.println("[OTA] Success");
          anedya_update_ota_status(deploymentID.c_str(), "success");
          ESP.restart();
        }
        if (otaStatus == HTTPS_OTA_FAIL) {
          Serial.println("[OTA] Failed");
          anedya_update_ota_status(deploymentID.c_str(), "failure");
          otaInProgress = false;
        }
        delay(1000);
      }

      if (otaInProgress) {
        Serial.println("[OTA] Timeout");
        anedya_update_ota_status(deploymentID.c_str(), "failure");
        otaInProgress = false;
      }
    }

    deploymentAvailable = false;
    suppressSensorPrinting = false;

    esp_task_wdt_config_t wdt_normal = {
      .timeout_ms = 20000,
      .idle_core_mask = 0,
      .trigger_panic = true
    };
    esp_task_wdt_reconfigure(&wdt_normal);

    #ifdef CONFIG_ENABLE_SENSOR_SEN54
    restartSensorTask();
    #endif

    #ifdef CONFIG_ENABLE_SENSOR_SHT
      restartSHTTask();
    #endif
    
    #ifdef CONFIG_ENABLE_LVGL
      restartLVGLTask();
    #endif

    #ifndef CONFIG_ENABLE_LVGL
      restartLEDTask();
    #endif

    #ifdef CONFIG_ENABLE_ESP_NOW
      restartESPNowTask();
    #endif

    // Restart sensor and LVGL tasks (assuming initSensor / initLVGL exists)
    // You should safely call the re-init logic here from your other modules.
  }
}

#endif
