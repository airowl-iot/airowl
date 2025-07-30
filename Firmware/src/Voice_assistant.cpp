#ifdef CONFIG_ENABLE_VOICE_ASSISTANT
#include <Arduino.h>
#include <WiFi.h>
#include "Voice_assistant.h"
#include "Audio.h"
#include "Config.h"
#include "FactoryReset.h"

#ifdef CONFIG_ESP_MATTER_ENABLE
#include "ui/matter_wrapper.h"  // or just "matter_wrapper.h" depending on your file structure
#endif

TaskHandle_t voiceAssistantTaskHandle = nullptr;

// Required global externs and prototypes
extern SemaphoreHandle_t wsMutex;
extern WebSocketsClient webSocket;

extern volatile bool scheduleListeningRestart;
extern volatile bool i2sOutputFlushScheduled;
extern volatile bool i2sInputFlushScheduled;
extern volatile bool sleepRequested;

extern volatile DeviceState deviceState;
extern Preferences preferences;
extern String authTokenGlobal;

extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t lvglTaskHandle;
extern TaskHandle_t ledTaskHandle;
extern TaskHandle_t wifiTaskHandle;
extern TaskHandle_t espnowTaskHandle;
extern TaskHandle_t networkTaskHandle;

void audioStreamTask(void* parameter);
void micTask(void* parameter);
void networkTask(void* parameter);

void stopTasksForVA() {
    #ifdef CONFIG_ENABLE_SENSOR_SEN54
    if (sensorTaskHandle) {
        Serial.println(" [VA] Stopping sensor task");
        esp_task_wdt_delete(sensorTaskHandle);
        vTaskDelete(sensorTaskHandle);
        sensorTaskHandle = nullptr;
    }
    #endif

    #ifdef CONFIG_ENABLE_LVGL
    if (lvglTaskHandle) {
        Serial.println(" [VA] Stopping LVGL task");
        esp_task_wdt_delete(lvglTaskHandle);
        vTaskDelete(lvglTaskHandle);
        lvglTaskHandle = nullptr;
    }
    #endif

    #ifndef CONFIG_ENABLE_LVGL
    if (ledTaskHandle) {
        Serial.println(" [VA] Stopping LED task");
        esp_task_wdt_delete(ledTaskHandle);
        vTaskDelete(ledTaskHandle);
        ledTaskHandle = nullptr;
    }
    #endif

    #ifdef CONFIG_ENABLE_ESP_NOW
    if (espnowTaskHandle) {
        Serial.println(" [VA]Stopping ESP-NOW task");
        esp_task_wdt_delete(espnowTaskHandle);
        vTaskDelete(espnowTaskHandle);
        espnowTaskHandle = nullptr;
    }
    #endif

    #ifdef CONFIG_ENABLE_OTA_ANEDYA
    if (lvglTaskHandle) {
        Serial.println("[OTA] Stopping LVGL task");
        esp_task_wdt_delete(lvglTaskHandle);
        vTaskDelete(lvglTaskHandle);
        lvglTaskHandle = nullptr;
      }

    #ifdef CONFIG_ESP_MATTER_ENABLE
    Serial.println("[VA] Stopping Matter task");
    stopMatter();
    #endif
}

void getAuthTokenFromNVS()
{
    preferences.begin("auth", false);
    authTokenGlobal = preferences.getString("auth_token", "");
    preferences.end();
}

void setupDeviceMetadata() {
    deviceState = IDLE;

    getAuthTokenFromNVS();

    if (factory_reset_status) {
        deviceState = FACTORY_RESET;
    }
}

void enterSleep()
{
    Serial.println(" [VA] Going to sleep...");
    
    // First, change device state to prevent any new data processing
    deviceState = SLEEP;
    scheduleListeningRestart = false;
    i2sOutputFlushScheduled = true;
    i2sInputFlushScheduled = true;
    vTaskDelay(10);  //let all tasks accept state

    xSemaphoreTake(wsMutex, portMAX_DELAY);

    i2sInput.end();  // AudioTools input stream
    i2s.end();       // AudioTools output stream

    // Properly disconnect WebSocket and wait for it to complete
    if (webSocket.isConnected()) {
        webSocket.disconnect();
        // Give some time for the disconnect to process
    }
    xSemaphoreGive(wsMutex);
    delay(100);

    // Flush any remaining serial output
    Serial.flush();
    #ifdef TOUCH_MODE
    touchSleepWakeUpEnable(TOUCH_PAD_NUM2, 28000);  // or your threshold
    #endif

    esp_deep_sleep_start();
    delay(1000);
}


void processSleepRequest() {
    if (sleepRequested) {
        sleepRequested = false;
        enterSleep();  
    }
}

void VoiceAssistantTask(void* params) {
    setupDeviceMetadata();
    wsMutex = xSemaphoreCreateMutex();

    stopTasksForVA();
    vTaskDelay(500);
    
    if (esp_get_free_heap_size() < 80000) {
    Serial.printf("[VA] Heap too low (%d bytes) - skipping audio start\n", esp_get_free_heap_size());
    return;
    }
    xTaskCreatePinnedToCore(audioStreamTask, "Speaker Task", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(micTask, "Microphone Task", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(networkTask, "Websocket Task", 8192, NULL, configMAX_PRIORITIES - 1, &networkTaskHandle, 0);
    Serial.println(" [VA] Voice Assistant task started");
    while (1) {
       if (sleepRequested) {
            sleepRequested = false;
            enterSleep();  // Will deep sleep
        }
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void initVoiceAssistantTask() {
    stopTasksForVA();  
    vTaskDelay(50);  
    
    if (voiceAssistantTaskHandle) {
        esp_task_wdt_delete(voiceAssistantTaskHandle);
        vTaskDelete(voiceAssistantTaskHandle);
        voiceAssistantTaskHandle = nullptr;
    }

    xTaskCreatePinnedToCore(VoiceAssistantTask, "VoiceAssistantTask", 4096, nullptr, 2, &voiceAssistantTaskHandle, 1);
    if (voiceAssistantTaskHandle) {
        esp_task_wdt_add(voiceAssistantTaskHandle);
        Serial.println(" [VA] Voice Assistant task started");
    } else {
        Serial.println(" [VA] Failed to start Voice Assistant Task!");
    }
}

void restartVoiceAssistantTask() {
    if (voiceAssistantTaskHandle) {
        esp_task_wdt_delete(voiceAssistantTaskHandle);
        vTaskDelete(voiceAssistantTaskHandle);
        voiceAssistantTaskHandle = nullptr;
    }

    BaseType_t result = xTaskCreatePinnedToCore(VoiceAssistantTask, "VoiceAssistantTask", 4096, nullptr, 2, &voiceAssistantTaskHandle, 1);
    if (result == pdPASS && voiceAssistantTaskHandle) {
        esp_task_wdt_add(voiceAssistantTaskHandle);
        Serial.println(" [VA] Voice Assistant task restarted");
    } else {
        Serial.println(" [VA] Failed to restart Voice Assistant task!");
        voiceAssistantTaskHandle = nullptr;
    }
}

#endif