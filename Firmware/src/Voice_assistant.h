#ifndef VOICE_ASSISTANT_H
#define VOICE_ASSISTANT_H

#include <Arduino.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern TaskHandle_t voiceAssistantTaskHandle;

void initVoiceAssistantTask();
void setupDeviceMetadata();
void getAuthTokenFromNVS();
void enterSleep();
void stopTasksForVA() ;
void restartVoiceAssistantTask();
void VoiceAssistantTask(void* params);

#ifdef __cplusplus
}
#endif

#endif 