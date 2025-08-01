

#ifdef CONFIG_ENABLE_ESP_NOW

#include "espnow_module.h"
#include <esp_now.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "configure.h"

#ifdef CONFIG_ENABLE_SENSOR_SEN54
  #include "Sensor.h"
#endif

#include "mqtt_module.h"

extern PubSubClient mqttClient;

// Remove conflicting extern declarations; these should be declared in a header and defined in a single .cpp file
// extern float temperature;
// extern float humidity;
// extern float AQI;
// extern Sensor_t sensor_data;

TaskHandle_t espnowTaskHandle = nullptr;

typedef struct __attribute__((packed)) {
  float temp;
  float hum;
//   float pm1;
//   float pm25;
//   float pm4;
//   float pm10;
//   float tvoc;
  uint32_t timestamp;
  bool valid;
} SlaveData;

SlaveData slaveData[MAX_SLAVES] = {0};
bool isMaster = true;
uint8_t masterMac[6] = {0};
bool newDataReceived = false;
uint32_t lastEspNowCheck = 0;
uint32_t lastMqttPublish = 0;

void espnow_loop_task(void *param);  

// Callback: Data received (Master only)
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int len) {
  if (!isMaster) return;

  if (len == (sizeof(uint8_t) + sizeof(float)*7)) {
    struct __attribute__((packed)) {
      uint8_t slaveId;
      float temp, hum; 
    //   pm1, pm25, pm4, pm10, tvoc;
    } receivedData;

    memcpy(&receivedData, data, sizeof(receivedData));
    uint8_t id = receivedData.slaveId;
    if (id < 1 || id > MAX_SLAVES) return;

    SlaveData s;
    s.temp = receivedData.temp;
    s.hum = receivedData.hum;
    // s.pm1 = receivedData.pm1;
    // s.pm25 = receivedData.pm25;
    // s.pm4 = receivedData.pm4;
    // s.pm10 = receivedData.pm10;
    // s.tvoc = receivedData.tvoc;
    s.timestamp = millis();
    s.valid = !isnan(s.temp) && !isnan(s.hum) && s.temp > -40 && s.temp < 80 && s.hum >= 0 && s.hum <= 100;
    slaveData[id-1] = s;
    newDataReceived = true;
    if(s.valid) {
      Serial.printf("Received from slave %d: %.1fC %.1f%% \n", id, s.temp, s.hum);
    } else {
      Serial.printf("Invalid data from slave %d\n", id);
    }
    } else {
        Serial.printf("Invalid data length: %d bytes\n", len);
    }
}

// Callback: Data sent (Slave only)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (isMaster) return;

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);

  if (status == ESP_NOW_SEND_SUCCESS)
  {
    Serial.printf("Data sent successfully to master (%s)\n", macStr);
  }
  else
  {
    Serial.printf("Failed to send data to master (%s)\n", macStr);
  }
}

void initESPNow() {
  Preferences prefs;
  prefs.begin("MatterPrefs", false);
  isMaster = prefs.getBool("isMaster", true);
  prefs.end();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  Serial.println("ESP-NOW Init Success");

  if (isMaster) {
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Registered as ESP-NOW Master (Receiver)");
  } else {
    esp_now_register_send_cb(OnDataSent);
    Serial.println("Registered as ESP-NOW Slave (Sender)");
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }

  // Reset slave data
  for (int i = 0; i < MAX_SLAVES; i++) {
    slaveData[i].valid = false;
    slaveData[i].timestamp = 0;
  }
}

void espnow_loop_task(void *param) {
  if (!esp_task_wdt_status(NULL)) esp_task_wdt_add(NULL);
  while (true) {
    espnow_loop();
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void restartESPNowTask() {
  // if (espnowTaskHandle) {
  //   esp_task_wdt_delete(espnowTaskHandle);
  //   vTaskDelete(espnowTaskHandle);
  //   espnowTaskHandle = nullptr;
  // }

  BaseType_t result = xTaskCreatePinnedToCore(espnow_loop_task, "ESPNowTask", 6144, nullptr, 2, &espnowTaskHandle, 1);
  if (result == pdPASS && espnowTaskHandle) {
    esp_task_wdt_add(espnowTaskHandle);
    Serial.println("[ESP-NOW] ESPNow task restarted");
  } else {
    Serial.println("[ESP-NOW] Failed to restart ESPNow task!");
    espnowTaskHandle = nullptr;
  }
}


void espnow_loop() {

  mqtt_setup();
  if (!isMaster) return;

  static uint32_t lastCheck = 0;
  uint32_t now = millis();
  if (now - lastCheck >= 5000) {
    for (int i = 0; i < MAX_SLAVES; i++) {
      if (slaveData[i].valid && (now - slaveData[i].timestamp > SLAVE_DATA_TIMEOUT)) {
        slaveData[i].valid = false;
        Serial.printf("[ESP-NOW] Slave %d timeout\n", i);
      }
    }
    lastCheck = now;
  }

  if (now - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
      char tempStr[8], humStr[8], pm1Str[8], pm25Str[8], pm4Str[8], pm10Str[8], tvocStr[8];
      dtostrf(temperature, 1, 1, tempStr);
      dtostrf(humidity, 1, 1, humStr);
    //   dtostrf(pm1_avg, 1, 1, pm1Str);
    //   dtostrf(pm25_avg, 1, 1, pm25Str);
    //   dtostrf(pm4_avg, 1, 1, pm4Str);
    //   dtostrf(pm10_avg, 1, 1, pm10Str);
    //   dtostrf(tvoc_avg, 1, 1, tvocStr);

      mqttClient.publish("sensor/master/temp", tempStr);
      mqttClient.publish("sensor/master/humidity", humStr);
    //   mqttClient.publish("sensor/master/pm1", pm1Str);
    //   mqttClient.publish("sensor/master/pm25", pm25Str);
    //   mqttClient.publish("sensor/master/pm4", pm4Str);
    //   mqttClient.publish("sensor/master/pm10", pm10Str);
    //   mqttClient.publish("sensor/master/tvoc", tvocStr);

      for (int i = 0; i < MAX_SLAVES; i++) {
        if (slaveData[i].valid) {
          char topic[64], value[16];
          sprintf(topic, "sensor/slave%d/temp", i + 1);
          dtostrf(slaveData[i].temp, 1, 1, value);
          mqttClient.publish(topic, value);
          sprintf(topic, "sensor/slave%d/humidity", i + 1);
          dtostrf(slaveData[i].hum, 1, 1, value);
          mqttClient.publish(topic, value);

        //   sprintf(topic, "sensor/slave%d/pm1", i + 1);
        //   dtostrf(slaveData[i].pm1, 1, 1, value);
        //   mqttClient.publish(topic, value);
        //   sprintf(topic, "sensor/slave%d/pm25", i + 1);
        //   dtostrf(slaveData[i].pm25, 1, 1, value);
        //   mqttClient.publish(topic, value);
        //   sprintf(topic, "sensor/slave%d/pm4", i + 1);
        //   dtostrf(slaveData[i].pm4, 1, 1, value);
        //   mqttClient.publish(topic, value);
        //   sprintf(topic, "sensor/slave%d/pm10", i + 1);
        //   dtostrf(slaveData[i].pm10, 1, 1, value);
        //   mqttClient.publish(topic, value);
        //   sprintf(topic, "sensor/slave%d/tvoc", i + 1);
        //   dtostrf(slaveData[i].tvoc, 1, 1, value);
        //   mqttClient.publish(topic, value);
        }
      }
    } 
    else {
      String mac = WiFi.macAddress();
      mac.replace(":", "");
      String ClientId = "AIROWL_" + mac.substring(6);

      Serial.println("[ESP-NOW] MQTT not connected - skipping publish");
      if (WiFi.status() == WL_CONNECTED) {
        if (!mqtt_connected()) {
            mqtt_reconnect(ClientId.c_str());
        }
      }
    }
    lastMqttPublish = now;
  }
}


#endif