// ota_service.cpp - OTA Service for Airowl 3.0

#include "ota_service.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "airowl_config.h"
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include "hal/hal_wifi.h"
#include <WiFi.h>
#include <string.h>
#include "manager/config_manager.h"
#include "manager/sensor_manager.h"
#include "manager/event_manager.h"

#include "manager/ui_manager.h"

namespace {
    bool initialized = false;
    SVC::OTA::State currentState = SVC::OTA::State::IDLE;
    SVC::OTA::ProgressCallback progressCallback = nullptr;
    TaskHandle_t otaTaskHandle = nullptr;

    static bool tasksAreSuspended = false;
    static const int maxSuspendedTasks = 32;
    static TaskHandle_t suspendedTasks[maxSuspendedTasks];
    static int suspendedTaskCount = 0;

    String versionURL;
    String firmwareURL;

    String fetchLatestVersion() {
        if (versionURL.isEmpty()) {
            auto* cfg = ConfigManager::getInstance();
            if (cfg && cfg->isInitialized()) {
                auto svcConfig = cfg->getServiceConfig("ota");
                if (svcConfig.params.count("versionURL")) {
                    versionURL = svcConfig.params.at("versionURL");
                }
            }

            if (versionURL.isEmpty()) {
                Serial.println("[OTA] ERROR: versionURL not configured in config.json");
                return "";
            }
        }

        Serial.printf("[OTA] Fetching version from: %s\n", versionURL.c_str());

        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        if (!http.begin(versionURL.c_str())) {
            Serial.println("[OTA] http.begin() failed (version)");
            return "";
        }

        int httpCode = http.GET();
        if (httpCode != HTTP_CODE_OK) {
            Serial.printf("[OTA] Failed to fetch version info, code: %d\n", httpCode);
            http.end();
            return "";
        }

        String payload = http.getString();
        http.end();

        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, payload)) {
            Serial.println("[OTA] JSON parse error");
            return "";
        }

        if (!doc.containsKey("version")) return "";

        String v = doc["version"].as<String>();
        v.trim();
        Serial.printf("[OTA] Latest version: %s\n", v.c_str());
        return v;
    }

    bool isUpdateAvailable() {
        String latest = fetchLatestVersion();
        if (latest.isEmpty()) return false;

        String current;
        auto* cfg = ConfigManager::getInstance();
        if (cfg && cfg->isInitialized()) {
            current = cfg->getFirmwareVersion();
        }

        if (current.isEmpty()) {
            Serial.println("[OTA] ERROR: firmwareVersion not configured");
            return false;
        }

        current.trim();
        Serial.printf("[OTA] Current Firmware: %s, Latest Firmware: %s\n", current.c_str(), latest.c_str());
        return (latest != current);
    }

    void updateState(SVC::OTA::State newState, int progress = -1, const char* message = nullptr) {
        const char* stateNames[] = {
            "IDLE", "CHECKING", "DOWNLOADING", "UPDATING", "SUCCESS", "FAILED"
        };

        Serial.printf("[OTA] State change: %s -> %s\n",
                      stateNames[(int)currentState], stateNames[(int)newState]);

        currentState = newState;

        if (progress >= 0) Serial.printf("[OTA] Progress: %d%%\n", progress);
        if (message) Serial.printf("[OTA] Message: %s\n", message);

        if (progressCallback) progressCallback(newState, progress, message ? message : "");

        if (newState == SVC::OTA::State::DOWNLOADING || newState == SVC::OTA::State::UPDATING) {
            if (progress >= 0) {
                auto event = std::make_shared<CORE::OTAProgressEvent>(progress, 100);
                CORE::EventBus::getInstance().publish(event);
            }
        } else if (newState == SVC::OTA::State::SUCCESS || newState == SVC::OTA::State::FAILED) {
            auto event = std::make_shared<CORE::OTACompleteEvent>(newState == SVC::OTA::State::SUCCESS);
            CORE::EventBus::getInstance().publish(event);
        }
    }

    bool downloadAndApplyFirmware(const String& initialUrl) {
        String url = initialUrl;
        const int MAX_REDIRECTS = 3;
        int redirects = 0;

        WiFiClient clientPlain;
        WiFiClientSecure clientSecure;
        clientSecure.setInsecure();

        while (redirects <= MAX_REDIRECTS) {
            HTTPClient http;
            bool secure = url.startsWith("https://");

            if (secure) {
                if (!http.begin(clientSecure, url)) {
                    Serial.printf("[OTA] http.begin() failed for %s\n", url.c_str());
                    http.end();
                    return false;
                }
            } else {
                if (!http.begin(clientPlain, url)) {
                    Serial.printf("[OTA] http.begin() failed for %s\n", url.c_str());
                    http.end();
                    return false;
                }
            }

            updateState(SVC::OTA::State::DOWNLOADING, 0, "Starting HTTP GET");
            int httpCode = http.GET();

            if (httpCode == HTTP_CODE_OK) {
                int contentLength = http.getSize();
                WiFiClient* stream = http.getStreamPtr();
                if (!stream) {
                    Serial.println("[OTA] ERROR: Failed to get stream pointer");
                    http.end();
                    return false;
                }

                if (contentLength == 0) {
                    Serial.println("[OTA] ERROR: Content length is zero");
                    http.end();
                    return false;
                }

                Serial.printf("[OTA] Content length: %d bytes\n", contentLength);

                if (!Update.begin(contentLength > 0 ? contentLength : UPDATE_SIZE_UNKNOWN)) {
                    Serial.printf("[OTA] Update.begin() failed. Error: %d\n", Update.getError());
                    http.end();
                    return false;
                }

                size_t written = 0;
                size_t totalWritten = 0;
                uint8_t buffer[1024];
                unsigned long downloadStartTime = millis();
                unsigned long lastProgressTime = millis();
                const unsigned long DOWNLOAD_TIMEOUT_MS = 300000;  // 5 minutes total timeout
                const unsigned long STALL_TIMEOUT_MS = 30000;      // 30 seconds stall timeout

                while (true) {
                    esp_task_wdt_reset();  

                    if (millis() - downloadStartTime > DOWNLOAD_TIMEOUT_MS) {
                        Serial.println("[OTA] ERROR: Download timeout (5 minutes exceeded)");
                        Update.abort();
                        http.end();
                        return false;
                    }

                    if (millis() - lastProgressTime > STALL_TIMEOUT_MS) {
                        Serial.println("[OTA] ERROR: Download stalled (30 seconds with no data)");
                        Update.abort();
                        http.end();
                        return false;
                    }

                    size_t bytesAvailable = stream->available();
                    if (bytesAvailable > 0) {
                        size_t bytesToRead = min(bytesAvailable, sizeof(buffer));
                        size_t bytesRead = stream->readBytes(buffer, bytesToRead);

                        if (bytesRead > 0) {
                            written = Update.write(buffer, bytesRead);
                            if (written != bytesRead) {
                                Serial.printf("[OTA] Write error: expected %d, wrote %d\n", bytesRead, written);
                                Update.abort();
                                http.end();
                                return false;
                            }
                            totalWritten += written;
                            lastProgressTime = millis();  

                            if (contentLength > 0) {
                                int progress = (totalWritten * 100) / contentLength;
                                updateState(SVC::OTA::State::DOWNLOADING, progress, "Downloading firmware");
                            }
                        }
                    }

                    if (contentLength > 0) {
                        if (totalWritten >= contentLength) break;
                    } else {
                     
                        if (!stream->connected() && stream->available() == 0) break;
                    }

                    vTaskDelay(pdMS_TO_TICKS(1)); 
                }

                Serial.printf("[OTA] Downloaded %u bytes in %lu ms\n",
                             (unsigned)totalWritten,
                             millis() - downloadStartTime);

                if (!Update.end() || !Update.isFinished()) {
                    Serial.printf("[OTA] Update failed. Error: %d\n", Update.getError());
                    http.end();
                    return false;
                }

                Serial.println("[OTA] ✓ Update finished. Rebooting...");
                http.end();
                return true;

            } else if (httpCode == HTTP_CODE_MOVED_PERMANENTLY ||
                       httpCode == HTTP_CODE_FOUND ||
                       httpCode == HTTP_CODE_TEMPORARY_REDIRECT) {
                url = http.getLocation();
                if (url.length() == 0) {
                    Serial.println("[OTA] ERROR: Empty redirect location");
                    http.end();
                    return false;
                }
                Serial.printf("[OTA] Redirecting to: %s\n", url.c_str());
                redirects++;
                http.end();
                continue;
            } else {
                Serial.printf("[OTA] HTTP GET failed, code: %d\n", httpCode);
                http.end();
                return false;
            }
        }
        Serial.println("[OTA] ERROR: Too many redirects, aborting.");
        return false;
    }

} // namespace

namespace SVC {

bool OTA::otaInProgress = false;

bool OTA::init() {
    if (initialized) return true;
    Serial.println("[OTA] Initializing OTA service...");
    initialized = true;
    updateState(State::IDLE, 0, "OTA initialized");


    return true;
}

void OTA::setURLs(const char* versionUrl, const char* firmwareUrl) {
    if (versionUrl && strlen(versionUrl) > 0) {
        versionURL = versionUrl;
        Serial.printf("[OTA] Version URL set: %s\n", versionURL.c_str());
    }
    if (firmwareUrl && strlen(firmwareUrl) > 0) {
        firmwareURL = firmwareUrl;
        Serial.printf("[OTA] Firmware URL set: %s\n", firmwareURL.c_str());
    }
}

bool OTA::beginUpdate() {
    if (!initialized || otaInProgress) return false;

    if (firmwareURL.isEmpty()) {
        auto* cfg = ConfigManager::getInstance();
        if (cfg && cfg->isInitialized()) {
            auto svcConfig = cfg->getServiceConfig("ota");
            if (svcConfig.params.count("firmwareURL")) {
                firmwareURL = svcConfig.params.at("firmwareURL");
            }
        }

        if (firmwareURL.isEmpty()) {
            Serial.println("[OTA] ERROR: firmwareURL not configured in config.json");
            return false;
        }
    }

    Serial.printf("[OTA] Downloading firmware from: %s\n", firmwareURL.c_str());

    otaInProgress = true;
    updateState(State::DOWNLOADING, 0, "Preparing OTA");

    bool ok = downloadAndApplyFirmware(firmwareURL);
    if (!ok) {
        updateState(State::FAILED, 0, "OTA failed");
        otaInProgress = false;
        return false;
    }

    updateState(State::SUCCESS, 100, "Update complete, rebooting");
    otaInProgress = false;
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP.restart();
    return true;
}

OTA::State OTA::getState() { return currentState; }

const char* OTA::getCurrentVersion() {
    
    static char versionBuffer[64] = {0};

    auto* cfg = ConfigManager::getInstance();
    if (cfg && cfg->isInitialized()) {
        String version = cfg->getFirmwareVersion();
        if (!version.isEmpty()) {
            strncpy(versionBuffer, version.c_str(), sizeof(versionBuffer) - 1);
            versionBuffer[sizeof(versionBuffer) - 1] = '\0';
            return versionBuffer;
        }
    }

    strncpy(versionBuffer, "unknown", sizeof(versionBuffer) - 1);
    return versionBuffer;
}

void OTA::onProgress(ProgressCallback callback) { progressCallback = callback; }

void OTA::suspendOtherTasks() {
    if (tasksAreSuspended) return;
    suspendedTaskCount = 0;
    memset(suspendedTasks, 0, sizeof(suspendedTasks));
    
    TaskHandle_t sensorTask = APP::SensorManager::getTaskHandle();
    if (sensorTask && suspendedTaskCount < maxSuspendedTasks) {
        vTaskSuspend(sensorTask);
        suspendedTasks[suspendedTaskCount++] = sensorTask;
    }


    tasksAreSuspended = true;
}

void OTA::resumeOtherTasks() {
    if (!tasksAreSuspended) return;
    for (int i = 0; i < suspendedTaskCount; ++i) {
        if (suspendedTasks[i]) vTaskResume(suspendedTasks[i]);
        suspendedTasks[i] = nullptr;
    }
    suspendedTaskCount = 0;
    tasksAreSuspended = false;
}

void OTA::onWiFiConnected() {
    if (isUpdateAvailable()) {
        APP::UIController::showOTAScreen();
        vTaskDelay(pdMS_TO_TICKS(100));
        suspendOtherTasks();
        bool ok = beginUpdate(); 
        if (!ok) resumeOtherTasks();
    }
}

void OTA::loop() { /* empty */ }

void OTA::task(void* parameter) { while(true) vTaskDelay(pdMS_TO_TICKS(5000)); }

bool OTA::startTask() {
    if (otaTaskHandle) return true;
    return xTaskCreatePinnedToCore(task, "OTA", 8192, nullptr, 1, &otaTaskHandle, 0) == pdPASS;
}

bool OTA::restartTask() {
    if (otaTaskHandle) {
        TaskHandle_t tempHandle = otaTaskHandle;
        otaTaskHandle = nullptr;  
        vTaskDelete(tempHandle);
        vTaskDelay(pdMS_TO_TICKS(100)); 
        Serial.println("[OTA] Task deleted, restarting...");
    }
    return startTask();
}

} // namespace SVC
