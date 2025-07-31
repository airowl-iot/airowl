// ota_module.h — Anedya OTA interface (flag-wrapped)

#pragma once

#ifdef CONFIG_ENABLE_OTA_ANEDYA

#include <HTTPClient.h>
#include "HttpsOTAUpdate.h"

void setDevice_time();
bool anedya_check_ota_update(char *assetURLBuf, char *deploymentIDBuf);
void anedya_update_ota_status(const char *deploymentID, const char *deploymentStatus);
void anedya_sendHeartbeat();
void ota_loop();
void initOTA();
void HttpEvent(HttpEvent_t *event);

extern bool otaInProgress;
extern bool suppressSensorPrinting;
extern bool deploymentAvailable;
extern bool statusPublished;
extern String assetURL;
extern String deploymentID;

#else

// If OTA disabled, provide safe empty stubs
inline void setDevice_time() {}
inline bool anedya_check_ota_update(char *, char *) { return false; }
inline void anedya_update_ota_status(const char *, const char *) {}
inline void anedya_sendHeartbeat() {}
inline void ota_loop() {}
inline void initOTA() {}
inline void HttpEvent(void *) {}

#endif