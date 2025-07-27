#ifdef CONFIG_ENABLE_SENSOR_SHT
#include <Wire.h>
#include <SHTSensor.h>
#include <WiFi.h>
#include "Flags/mqtt_module.h"
#include "Sensor_sht.h"

SHTSensor sht;
TaskHandle_t shtTaskHandle = nullptr;

extern TwoWire Wire;

void ShtData(void *params) {
    
    if (!sht.init()) {
        Serial.println("[SHT] Init failed!");
    } else {
        sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM);
        Serial.println("[SHT] Sensor initialized.");
    }

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String deviceId = "AIROWL_" + mac.substring(6);

    esp_task_wdt_add(NULL);  //  Register the task with the WDT

    while (1) {
        float temperature = 0, humidity = 0;
        bool readOk = sht.readSample();
        if (readOk) {
            temperature = sht.getTemperature();
            humidity = sht.getHumidity();
            Serial.printf("[SHT] Read OK: Temp=%.2f C, Hum=%.2f %%\n", temperature, humidity);

            String tempTopic = "airowl/" + deviceId + "/sht/temp";
            String humTopic  = "airowl/" + deviceId + "/sht/humd";
            
            char tempStr[8], humStr[8];
            dtostrf(temperature, 1, 2, tempStr);
            dtostrf(humidity, 1, 2, humStr);

            String jsonString = "{";
                    jsonString += "\"deviceId\":\"";
                    jsonString += deviceId;
                    jsonString += "\",";
                    jsonString += "\"temp\":";
                    jsonString += String(tempStr, 2);
                    jsonString += ",";
                    jsonString += "\"hum\":";
                    jsonString += String(humStr, 2);
                    jsonString += "}";
            
            if (WiFi.status() == WL_CONNECTED) {
                if (!mqtt_connected()) {
                    mqtt_reconnect(deviceId.c_str());
                }
                mqtt_publish("airowl",jsonString.c_str());
            }
        } else {
            Serial.println("[SHT] Sensor read failed!");
        }

        esp_task_wdt_reset();  
        vTaskDelay(pdMS_TO_TICKS(1000));          
    }
}

void initSHTTask() {
    xTaskCreatePinnedToCore(ShtData, "SHTTask", 4096, nullptr, 2, &shtTaskHandle, 1);
    if (shtTaskHandle) {
        esp_task_wdt_add(shtTaskHandle);
    } else {
        Serial.println("[SHT] Failed to start SHT Task!");
    }
}

void restartSHTTask() {
    BaseType_t result = xTaskCreatePinnedToCore(ShtData, "SHTTask", 4096, nullptr, 2, &shtTaskHandle, 1);
    if (result == pdPASS && shtTaskHandle) {
        esp_task_wdt_add(shtTaskHandle);
        Serial.println("[SHT] SHT task restarted");
    } else {
        Serial.println("[SHT] Failed to restart SHT task!");
        shtTaskHandle = nullptr;
    }
}
#endif
