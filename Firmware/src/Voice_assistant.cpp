#ifdef CONFIG_ENABLE_VOICE_ASSISTANT
#include <Arduino.h>
#include <WiFi.h>
#include "Voice_assistant.h"
#include "Audio.h"
#include "Config.h"
#include "FactoryReset.h"

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

extern TaskHandle_t networkTaskHandle;

void audioStreamTask(void* parameter);
void micTask(void* parameter);
void networkTask(void* parameter);

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
    Serial.println("Going to sleep...");
    
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

    xTaskCreatePinnedToCore(audioStreamTask, "Speaker Task", 4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(micTask, "Microphone Task", 4096, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(networkTask, "Websocket Task", 8192, NULL, configMAX_PRIORITIES - 1, &networkTaskHandle, 0);

    while (1) {
        processSleepRequest();
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void initVoiceAssistantTask() {
    xTaskCreatePinnedToCore(VoiceAssistantTask, "VoiceAssistantTask", 4096, nullptr, 2, &voiceAssistantTaskHandle, 1);
    if (voiceAssistantTaskHandle) {
        esp_task_wdt_add(voiceAssistantTaskHandle);
        Serial.println("[VA] Voice Assistant task started");
    } else {
        Serial.println("[VA] Failed to start Voice Assistant Task!");
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
        Serial.println("[VA] Voice Assistant task restarted");
    } else {
        Serial.println("[VA] Failed to restart Voice Assistant task!");
        voiceAssistantTaskHandle = nullptr;
    }
}

#endif