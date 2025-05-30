//Airowl-OTA - raat vdu








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
#include <WiFiClientSecure.h> //library to maintain the secure connection
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
#include <Arduino.h>
#include <PubSubClient.h>     // library to establish mqtt connection
#include "HttpsOTAUpdate.h"
#include <ArduinoJson.h> // Include the Arduino library to make json or abstract the value from the json
#include <TimeLib.h>     // Include the Time library to handle time synchronization with ATS (Anedya Time Services)
#include <esp_heap_caps.h>

#define WDT_TIMEOUT 600000
#define FIRMWARE_VERSION "version - 1.1"
String REGION_CODE = "ap-in-1";                        // Anedya region code (e.g., "ap-in-1" for Asia-Pacific/India) | For other country code, visity [https://docs.anedya.io/device/#region]
//AIROWL_E939F8
const char *CONNECTION_KEY = "a0833765fa6b23753d7c3fb6ba78b970"; 
const char *PHYSICAL_DEVICE_ID = "46ed7de4-e783-4ca7-8f7a-7def3ef51bb3"; 


// Anedya Root CA 3 (ECC - 256)(Pem format)| [https://docs.anedya.io/device/mqtt-endpoints/#tls]
static const char *ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIICDDCCAbOgAwIBAgITQxd3Dqj4u/74GrImxc0M4EbUvDAKBggqhkjOPQQDAjBL
MQswCQYDVQQGEwJJTjEQMA4GA1UECBMHR3VqYXJhdDEPMA0GA1UEChMGQW5lZHlh
MRkwFwYDVQQDExBBbmVkeWEgUm9vdCBDQSAzMB4XDTI0MDEwMTAwMDAwMFoXDTQz
MTIzMTIzNTk1OVowSzELMAkGA1UEBhMCSU4xEDAOBgNVBAgTB0d1amFyYXQxDzAN
BgNVBAoTBkFuZWR5YTEZMBcGA1UEAxMQQW5lZHlhIFJvb3QgQ0EgMzBZMBMGByqG
SM49AgEGCCqGSM49AwEHA0IABKsxf0vpbjShIOIGweak0/meIYS0AmXaujinCjFk
BFShcaf2MdMeYBPPFwz4p5I8KOCopgshSTUFRCXiiKwgYPKjdjB0MA8GA1UdEwEB
/wQFMAMBAf8wHQYDVR0OBBYEFNz1PBRXdRsYQNVsd3eYVNdRDcH4MB8GA1UdIwQY
MBaAFNz1PBRXdRsYQNVsd3eYVNdRDcH4MA4GA1UdDwEB/wQEAwIBhjARBgNVHSAE
CjAIMAYGBFUdIAAwCgYIKoZIzj0EAwIDRwAwRAIgR/rWSG8+L4XtFLces0JYS7bY
5NH1diiFk54/E5xmSaICIEYYbhvjrdR0GVLjoay6gFspiRZ7GtDDr9xF91WbsK0P
-----END CERTIFICATE-----
)EOF";


WiFiManager wm;
TaskHandle_t myTaskHandle;
Preferences preferences;
MatterAirQualitySensor air_quality_sensor;
MatterTemperatureSensor temperature_sensor;  // Temperature Sensor
MatterHumiditySensor humidity_sensor;       // Added Humidity Sensor

//--------------------------- MQTT connection settings --------------------------------
#define MQTT_BUFFER_SIZE 1024
String str_mqtt_broker = "mqtt." + REGION_CODE + ".anedya.io";
const char *mqtt_broker = str_mqtt_broker.c_str();                                   // MQTT broker address
const char *mqtt_username = PHYSICAL_DEVICE_ID;                                      // MQTT username
const char *mqtt_password = CONNECTION_KEY;                                          // MQTT password
const int mqtt_port = 8883;                                                          // MQTT port
String responseTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/response"; // MQTT topic for device responses
String errorTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/errors";      // MQTT topic for device errors

// ----------------------------- Helper Variable ----------------------------------------
long last_check_for_ota_update = 0;
long check_for_ota_update_interval= 1*60*1000;
String timeRes; // variable to handle response

#define TIME_SYNC_BIT 1
#define HEARTBEAT_BIT 2
#define OTA_UPDATE_BIT 9
#define OTA_UPDATE_STATUS_BIT 10

bool otaInProgress = false;
bool suppressSensorPrinting = false;
bool deploymentAvailable = false;
bool statusPublished = false;   

String assetURL = "";
String deploymentID = "";

/*--------------------Object initializing--------------------------------*/
WiFiClientSecure esp_client;          // create a WiFiClientSecure object
PubSubClient mqtt_client(esp_client); // creat a PubSubClient object
static HttpsOTAStatus_t otastatus;

/*------------------------Function declarations--------------------------*/
// void connectToWiFi();
void connectToMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void syncDeviceTime();       // Function to configure the device time with real-time from ATS (Anedya Time Services)
void anedya_sendHeartbeat(); // Function to send heartbeat to the Anedya
bool anedya_check_ota_update();
void anedya_update_ota_status(String deploymentID, String deploymentStatus);
void handleMatter();
void handlesensordata();
void handlesensorAndMatter();

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
        // case HTTP_EVENT_REDIRECT:
        //     Serial.println("Http Event Redirect");
        //     break;
    }
}

void handlesensordata() {
    if (otaInProgress || suppressSensorPrinting) return;
    static int lastAQI = -1;
    static float lastTemp = 0.0;
    static float lastHum = 0.0;
  
    if (!suppressSensorPrinting) {
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
    }
  }

  void handleMatter() {
    static bool matter_initialized = false;
    if (!matter_initialized && WiFi.status() == WL_CONNECTED && !otaInProgress) {
      Serial.println("Starting Matter Setup...");
      air_quality_sensor.begin(AQI);
      temperature_sensor.begin(temperature);
      humidity_sensor.begin(humidity);
      ArduinoMatter::begin();
      matter_initialized = true;
      Serial.println("Matter initialized");
    }
  }

  void handlesensorAndMatter() {
    static bool was_commissioned = false;
    static uint32_t last_read_time = 0;
    static bool matter_initialized = true;
    if (matter_initialized && !otaInProgress ) 
    {
      if (!ArduinoMatter::isDeviceCommissioned()) {
       // Serial.println("Matter Node not commissioned");
      // Serial.println("Manual pairing code: " + ArduinoMatter::getManualPairingCode());
      // Serial.println("QR code URL: " + ArduinoMatter::getOnboardingQRCodeUrl());
      } else if (!was_commissioned) {
        Serial.println("Matter Node commissioned");
        was_commissioned = true;
      }
  
      if (millis() - last_read_time > 5000) {
        last_read_time = millis();
        air_quality_sensor.setAQI(AQI);
        temperature_sensor.setTemperature(temperature);
        humidity_sensor.setHumidity(humidity);
      }
    }
  }

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

  esp_log_level_set("*", ESP_LOG_ERROR);

  if (M5.Rtc.isEnabled()) {
    M5.Log.println("RTC Enabled");
  }

  esp_client.setCACert(ca_cert);                 // Set Root CA certificate
  mqtt_client.setServer(mqtt_broker, mqtt_port); // Set the MQTT server address and port for the MQTT client to connect to anedya broker
  mqtt_client.setKeepAlive(60);                  // Set the keep alive interval (in seconds) 
  mqtt_client.setCallback(mqttCallback);         // Set the callback function to be invoked when MQTT messages are received
  mqtt_client.setBufferSize(MQTT_BUFFER_SIZE);   // Set the MQTT buffer size
  connectToMQTT();                               // Attempt to establish a connection to the anedya broker

  HttpsOTA.onHttpEvent(HttpEvent);
  last_check_for_ota_update = millis();

  syncDeviceTime();

  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = WDT_TIMEOUT * 1000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  WiFi.begin();
  delay(1000);
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String apName = "AIROWL_" + mac.substring(6);

  WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
  WiFiManagerNS::init(&wm, nullptr);
  std::vector<const char *> menu = {"wifi", "info", "custom", "param", "sep", "restart", "exit"};

  wm.setMenu(menu);
  wm.setConfigPortalBlocking(false);
  wm.setTitle("AIROWL Configuration");
  if (WiFi.status() != WL_CONNECTED) {
    wm.autoConnect(apName.c_str(), "12345678");
}

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
  if (millis() - last_check_for_ota_update >= check_for_ota_update_interval)
  {
      anedya_sendHeartbeat(); // sending heartbeat
      bool success = anedya_check_ota_update();
      if (success)
      {
          if (deploymentAvailable)
          {
            otaInProgress = true;
            suppressSensorPrinting = true;
            // Give the sensor task time to notice the flag
           if (myTaskHandle != NULL) {
                Serial.println("Stopping sensor task before OTA...");
                vTaskDelete(myTaskHandle);
                myTaskHandle = NULL;
              }
            
              anedya_update_ota_status(deploymentID, "start");

              Serial.println("Starting firmware update");
              esp_client.setHandshakeTimeout(30000); // 30 seconds timeout
          
              esp_task_wdt_config_t ota_wdt_config = {
                .timeout_ms = 300000,           // 5 minutes for OTA
                .idle_core_mask = 0,
                .trigger_panic = true
            };
            esp_task_wdt_reconfigure(&ota_wdt_config);
            
            HttpsOTA.onHttpEvent(HttpEvent);
          
            Serial.println("Free Internal Heap: " + String(esp_get_free_heap_size()));
            Serial.println("Free PSRAM: " + String(ESP.getFreePsram()));
            Serial.println("Total Heap Size: " + String(esp_get_free_heap_size()));
          
            if (esp_get_free_heap_size() < 80000) {
                Serial.println("Not enough internal heap for OTA, aborting.");
                anedya_update_ota_status(deploymentID, "failure");
                return;
              }

              HttpsOTA.begin(assetURL.c_str(), ca_cert, false);
              Serial.print(" OTA in progress ..");
              const uint32_t ota_timeout = 300000; // 5 minutes timeout
              uint32_t ota_start = millis();
              
              while ((millis() - ota_start) < ota_timeout)
          {
            esp_task_wdt_reset();
                  Serial.print(".");
                  otastatus = HttpsOTA.status();
                  if (otastatus == HTTPS_OTA_SUCCESS)
                  {
                      Serial.println("Firmware written successfully");
                      delay(500);
                          // Properly close MQTT connection before sending status
    mqtt_client.disconnect();
    delay(1000);  // Give time for disconnect to complete
    
    // Reconnect to MQTT
    connectToMQTT();
                      anedya_update_ota_status(deploymentID, "success");
                      statusPublished = false;
                      while(!statusPublished){
                          mqtt_client.loop();
                          delay(500);
                      }                  
                      ESP.restart();
                  }
                  else if (otastatus == HTTPS_OTA_FAIL)
                  {
                      Serial.println("Firmware Upgrade Fail");
                      anedya_update_ota_status(deploymentID, "failure");
                      break;
                  }
                  delay(1000);
              }
              if ((millis() - ota_start) >= ota_timeout) {
                Serial.println("OTA timeout");
                anedya_update_ota_status(deploymentID, "failure");
            }
              deploymentAvailable = false;
        }  
          otaInProgress = false;
          suppressSensorPrinting = false;
    }
      else
      {
          Serial.println("Failed to Publish!");
      }
      last_check_for_ota_update = millis();
  }
  mqtt_client.loop();
  delay(3000);
  
  wm.process();
  lv_handler();
  update_time();
  esp_task_wdt_reset();

  if (!otaInProgress) {
 handlesensordata();
 handleMatter();
 handlesensorAndMatter();
  }
}

// //<---------------------------------------------------------------------------------------------------------------------------->
// void connectToWiFi()
// {
//     if (WiFi.status() == WL_CONNECTED) {
//         Serial.println("WiFi already connected.");
//         return;
//     }
    
//     // Connect to WiFi network
//     WiFi.begin(SSID, PASSWORD);
//     Serial.println();
//     Serial.print("Connecting to WiFi...");
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println();
//     Serial.print("Connected, IP address: ");
//     Serial.println(WiFi.localIP());
// }

void connectToMQTT()
{
    if (WiFi.status() != WL_CONNECTED) {
        wm.process();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Waiting for WiFi connection...");
            return;
        }
    }
    if (mqtt_client.connected()) {
        mqtt_client.disconnect();
        delay(1000);  // Give time for disconnect to complete
    }
    
    while (!mqtt_client.connected())
    {
        const char *client_id = PHYSICAL_DEVICE_ID;
        Serial.print("Connecting to Anedya Broker....... ");
        if (mqtt_client.connect(client_id, mqtt_username, mqtt_password))
        {
            Serial.println("Connected to Anedya broker");
            mqtt_client.subscribe(responseTopic.c_str(), 1); // subscribe to get response
            mqtt_client.subscribe(errorTopic.c_str(), 1);    // subscibe to get error
        }
        else
        {
            Serial.print("Failed to connect to Anedya broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" Retrying in 5 seconds.");
            delay(5000);
        }
    }
}
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    char res[MQTT_BUFFER_SIZE] = "";

    for (unsigned int i = 0; i < length; i++)
    {
        res[i] = payload[i];
    }
    String str_res(res);
    Serial.println("payload:" + str_res);
    Serial.println();

    // Parse the JSON response
    JsonDocument jsonResponse;
    deserializeJson(jsonResponse, str_res); // Deserialize the JSON response from into the JSON document
    int reqID = int(jsonResponse["reqId"]); // Get the server receive time from the JSON document

    switch (reqID)
    {
    case TIME_SYNC_BIT:
        timeRes = str_res;
        break;
    case HEARTBEAT_BIT:
        if (jsonResponse["success"])
        {
            Serial.println("Heatbeat sent successfully!");
        }
        else
        {
            Serial.println("Heatbeat sent failed! :" + str_res);
        }
        break;
    case OTA_UPDATE_BIT:
        deploymentAvailable = bool(jsonResponse["deploymentAvailable"]);
        if (deploymentAvailable)
        {
            suppressSensorPrinting = true;
            Serial.println("Update Available!");
            Serial.println("Asset identifier: " + jsonResponse["data"]["assetIdentifier"].as<String>());
            Serial.println("Asset version: " + jsonResponse["data"]["assetVersion"].as<String>());
            deploymentID = jsonResponse["data"]["deploymentId"].as<String>();
            Serial.println("Deployment ID: " + String(deploymentID));
            assetURL = jsonResponse["data"]["asseturl"].as<String>();
            Serial.println("Asset URL: " + String(assetURL));
        }
        else
        {
            Serial.println("No update available.");
        }
        break;
        case OTA_UPDATE_STATUS_BIT:
        Serial.println("OTA Status: " + str_res);
        statusPublished = true;
        break;
    default:
        Serial.println("Unknown response:" + str_res);
        break;
    }
}
// Function to configure time synchronization with Anedya server
// For more info, visit [https://docs.anedya.io/device/api/http-time-sync/]
void syncDeviceTime()
{
    if (WiFi.status() != WL_CONNECTED) {
        wm.process();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Waiting for WiFi connection...");
            return;
        }
    }
    String timeTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/time/json";
    const char *mqtt_topic = timeTopic.c_str();
    // Attempt to synchronize time with Anedya server

    mqtt_client.connected() ? (void)0 : connectToMQTT(); // check mqtt connection

    Serial.print("Time synchronizing......");

    boolean syncTime = true; // iteration to re-sync to ATS (Anedya Time Services), in case of failed attempt
    // Get the device send time

    long long deviceSendTime;
    long long timeTimer = millis();
    while (syncTime)
    {
        mqtt_client.loop();

        unsigned int iterate = 2000;
        if (millis() - timeTimer >= iterate)
        {
            Serial.print(" .");
            timeTimer = millis();
            deviceSendTime = millis();

            // Prepare the request payload
            JsonDocument requestPayload;                       // Declare a JSON document with a capacity of 200 bytes
            requestPayload["reqId"] = String(TIME_SYNC_BIT);   // Add a key-value pair to the JSON document
            requestPayload["deviceSendTime"] = deviceSendTime; // Add a key-value pair to the JSON document
            String jsonPayload;                                // Declare a string to store the serialized JSON payload
            serializeJson(requestPayload, jsonPayload);        // Serialize the JSON document into a string

            // Convert String object to pointer to a string literal
            const char *jsonPayloadLiteral = jsonPayload.c_str();
            mqtt_client.publish(mqtt_topic, jsonPayloadLiteral);
        } 

        if (timeRes != "")
        {
            String strResTime(timeRes);

            // Parse the JSON response
            JsonDocument jsonResponse;                 // Declare a JSON document with a capacity of 200 bytes
            deserializeJson(jsonResponse, strResTime); // Deserialize the JSON response from the server into the JSON document

            long long serverReceiveTime = jsonResponse["serverReceiveTime"]; // Get the server receive time from the JSON document
            long long serverSendTime = jsonResponse["serverSendTime"];       // Get the server send time from the JSON document

            // Compute the current time
            long long deviceRecTime = millis();                                                                // Get the device receive time
            long long currentTime = (serverReceiveTime + serverSendTime + deviceRecTime - deviceSendTime) / 2; // Compute the current time
            long long currentTimeSeconds = currentTime / 1000;                                                 // Convert current time to seconds

            // Set device time
            setTime(currentTimeSeconds); // Set the device time based on the computed current time
            Serial.println("\n synchronized!");
            syncTime = false;
        } 
    }
} 

// ---------------------- Function to check for OTA update ----------------------
bool anedya_check_ota_update()
{
    if (WiFi.status() != WL_CONNECTED) {
        wm.process();
        if (WiFi.status() != WL_CONNECTED) {
            wm.process();  // or rely on WiFiManager logic
            return false;
        }
    }
    mqtt_client.connected() ? (void)0 : connectToMQTT();

    String getNextDeploymentTopic_str = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/ota/next/json";
    const char *getNextDeploymentTopic = getNextDeploymentTopic_str.c_str();

    String getNextDeploymentPayload_str = "{\"reqId\": \"" + String(OTA_UPDATE_BIT) + "\"}";
    const char *getNextDeploymentPayload = getNextDeploymentPayload_str.c_str();

    Serial.println("Next Deployment Topic: " + String(getNextDeploymentTopic));
    Serial.println("Next Deployment Payload: " + String(getNextDeploymentPayload));

    bool success = mqtt_client.publish(getNextDeploymentTopic, getNextDeploymentPayload);
    mqtt_client.loop();
    return success;
}

// ---------------------- Function to update OTA Status ----------------------
void anedya_update_ota_status(String deploymentID, String deploymentStatus)
{
    if (WiFi.status() != WL_CONNECTED) {
        wm.process();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Waiting for WiFi connection...");
            return;
        }
    }
    mqtt_client.connected() ? (void)0 : connectToMQTT();


    String getNextDeploymentTopic_str = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/ota/updateStatus/json";
    const char *getNextDeploymentTopic = getNextDeploymentTopic_str.c_str();

    String getNextDeploymentPayload_str = "{\"reqId\": \"" + String(OTA_UPDATE_STATUS_BIT) + "\", \"deploymentId\": \"" + deploymentID + "\", \"status\": \"" + deploymentStatus + "\", \"log\": \"OK\" }";
    Serial.println("Status payload : " + String(getNextDeploymentPayload_str));
    const char *getNextDeploymentPayload = getNextDeploymentPayload_str.c_str();

    mqtt_client.publish(getNextDeploymentTopic, getNextDeploymentPayload);
    while(!statusPublished){ 
        mqtt_client.loop();
        delay(100);
    }
}

//---------------------------------- Function to send heartbeat -----------------------------------
void anedya_sendHeartbeat()
{
    if (WiFi.status() != WL_CONNECTED) {
        wm.process();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Waiting for WiFi connection...");
            return;
        }
    }
    mqtt_client.connected() ? (void)0 : connectToMQTT();

    String strSubmitTopic = "$anedya/device/" + String(PHYSICAL_DEVICE_ID) + "/heartbeat/json";
    const char *submitTopic = strSubmitTopic.c_str();

    String strLog = "{\"reqId\": \"" + String(HEARTBEAT_BIT) + "\"}";

    const char *submitLogPayload = strLog.c_str();
    mqtt_client.publish(submitTopic, submitLogPayload);
    mqtt_client.loop();
}