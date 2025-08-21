// ota_service.cpp - OTA Service implementation for Airowl 3.0
#include "ota_service.h"

#ifdef CONFIG_ENABLE_OTA_ANEDYA

#include <Update.h>
#include <HTTPClient.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include <WiFiClient.h>

namespace {
    // Private variables
    bool initialized = false;
    SVC::OTAService::State currentState = SVC::OTAService::State::IDLE;
    SVC::OTAService::ProgressCallback progressCallback = nullptr;
    TaskHandle_t otaTaskHandle = nullptr;
    
    // Helper function to validate URL
    bool isValidUrl(const char* url) {
        return url && strlen(url) > 0 && 
               (strncmp(url, "http://", 7) == 0 || 
                strncmp(url, "https://", 8) == 0);
    }
    
    // Version information
    String currentVersion;
    String updateVersion;
    
    // Update URL
    String updateUrl;
    
    // Update progress
    int updateProgress = 0;
    String statusMessage;
    
    // Update task control
    bool checkRequested = false;
    bool updateRequested = false;
    bool applyRequested = false;
    
    // Update validation
    const size_t HASH_SIZE = 32; // SHA-256
    uint8_t expectedHash[HASH_SIZE];
    bool hashValidation = false;
    
    // Update state and trigger callback
    void updateState(SVC::OTAService::State newState, int progress = -1, const char* message = nullptr) {
        currentState = newState;
        
        if (progress >= 0) {
            updateProgress = progress;
        }
        
        if (message) {
            statusMessage = message;
        }
        
        // Call progress callback if registered
        if (progressCallback) {
            progressCallback(newState, updateProgress, statusMessage.c_str());
        }
    }
    
    // Check for updates
    bool performUpdateCheck(const char* url) {
        if (!isValidUrl(url)) {
            updateState(SVC::OTAService::State::FAILED, 0, "Invalid or empty update URL");
            return false;
        }
        
        HTTPClient http;
        http.begin(url);
        
        updateState(SVC::OTAService::State::CHECKING, 0, "Checking for updates...");
        
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            // Parse JSON response (simplified example)
            // In a real implementation, use ArduinoJson to parse properly
            int versionStart = payload.indexOf("\"version\":") + 11;
            int versionEnd = payload.indexOf("\"", versionStart);
            
            if (versionStart > 10 && versionEnd > versionStart) {
                updateVersion = payload.substring(versionStart, versionEnd);
                
                // Compare versions
                if (updateVersion != currentVersion) {
                    // Extract update URL
                    int urlStart = payload.indexOf("\"url\":") + 7;
                    int urlEnd = payload.indexOf("\"", urlStart);
                    
                    if (urlStart > 6 && urlEnd > urlStart) {
                        updateUrl = payload.substring(urlStart, urlEnd);
                        
                        // Extract hash if available
                        int hashStart = payload.indexOf("\"hash\":") + 8;
                        int hashEnd = payload.indexOf("\"", hashStart);
                        
                        if (hashStart > 7 && hashEnd > hashStart) {
                            String hashStr = payload.substring(hashStart, hashEnd);
                            hashValidation = true;
                            
                            // Convert hex string to bytes
                            for (size_t i = 0; i < HASH_SIZE; i++) {
                                expectedHash[i] = strtoul(hashStr.substring(i*2, i*2+2).c_str(), NULL, 16);
                            }
                        } else {
                            hashValidation = false;
                        }
                        
                        updateState(SVC::OTAService::State::IDLE, 100, 
                                   ("Update available: " + updateVersion).c_str());
                        return true;
                    }
                } else {
                    updateState(SVC::OTAService::State::IDLE, 100, "Already up to date");
                }
            }
        } else {
            updateState(SVC::OTAService::State::FAILED, 0, 
                       ("Update check failed: " + String(httpCode)).c_str());
        }
        
        http.end();
        return false;
    }
    
    // Download and install update
    bool performUpdate(const char* url) {
        if (!isValidUrl(url)) {
            updateState(SVC::OTAService::State::FAILED, 0, "Invalid or empty update URL");
            return false;
        }
    
        HTTPClient http;
        http.begin(url);
        
        updateState(SVC::OTAService::State::DOWNLOADING, 0, "Downloading update...");
        
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            int contentLength = http.getSize();
            
            if (contentLength <= 0) {
                updateState(SVC::OTAService::State::FAILED, 0, "Invalid update size");
                http.end();
                return false;
            }
            
            // Begin OTA update
            if (!Update.begin(contentLength)) {
                updateState(SVC::OTAService::State::FAILED, 0, 
                           ("Not enough space: " + String(Update.errorString())).c_str());
                http.end();
                return false;
            }
            
            // Set hash validation if available
            if (hashValidation) {
                Update.setMD5((char*)expectedHash);
            }
            
            // Get update data stream
            WiFiClient* client = http.getStreamPtr();
            
            // Read data and write to update
            size_t written = 0;
            size_t bufSize = 128;
            uint8_t buf[bufSize];
            
            while (http.connected() && (written < contentLength)) {
                size_t available = client->available();
                
                if (available) {
                    size_t bytesToRead = available > bufSize ? bufSize : available;
                    size_t bytesRead = client->readBytes(buf, bytesToRead);
                    
                    if (bytesRead > 0) {
                        size_t bytesWritten = Update.write(buf, bytesRead);
                        if (bytesWritten != bytesRead) {
                            updateState(SVC::OTAService::State::FAILED, 0, 
                                       ("Write error: " + String(Update.errorString())).c_str());
                            http.end();
                            return false;
                        }
                        
                        written += bytesWritten;
                        
                        // Update progress
                        int progress = (written * 100) / contentLength;
                        updateState(SVC::OTAService::State::DOWNLOADING, progress);
                    }
                }
                
                // Small delay to prevent watchdog reset
                delay(1);
            }
            
            updateState(SVC::OTAService::State::VERIFYING, 100, "Verifying update...");
            
            if (written == contentLength) {
                if (Update.end()) {
                    updateState(SVC::OTAService::State::READY, 100, "Update ready to apply");
                    http.end();
                    return true;
                } else {
                    updateState(SVC::OTAService::State::FAILED, 0, 
                               ("Verify failed: " + String(Update.errorString())).c_str());
                }
            } else {
                updateState(SVC::OTAService::State::FAILED, 0, "Download incomplete");
            }
        } else {
            updateState(SVC::OTAService::State::FAILED, 0, 
                       ("Download failed: " + String(httpCode)).c_str());
        }
        
        http.end();
        return false;
    }
    
    // Apply update and reboot
    bool performUpdateApply() {
        if (currentState != SVC::OTAService::State::READY) {
            return false;
        }
        
        updateState(SVC::OTAService::State::UPDATING, 100, "Applying update...");
        
        // In ESP32, the update is already flashed, just need to set boot partition and reboot
        if (esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL)) == ESP_OK) {
            updateState(SVC::OTAService::State::COMPLETE, 100, "Update complete, rebooting...");
            
            // Small delay before reboot
            delay(1000);
            ESP.restart();
            return true;
        } else {
            updateState(SVC::OTAService::State::FAILED, 0, "Failed to set boot partition");
            return false;
        }
    }
}

namespace SVC {

bool OTAService::init(const char* version) {
    if (initialized) return true;
    
    // Store current version
    currentVersion = version;
    
    initialized = true;
    updateState(State::IDLE, 0, "OTA service initialized");
    return true;
}

bool OTAService::checkForUpdates(const char* url) {
    if (!initialized) return false;
    
    // Store URL and set flag for task to check
    updateUrl = url;
    checkRequested = true;
    
    return true;
}

bool OTAService::beginUpdate(const char* url) {
    if (!initialized) return false;
    
    // Store URL and set flag for task to update
    updateUrl = url;
    updateRequested = true;
    
    return true;
}

bool OTAService::applyUpdate() {
    if (!initialized || currentState != State::READY) return false;
    
    // Set flag for task to apply update
    applyRequested = true;
    
    return true;
}

OTAService::State OTAService::getState() {
    return currentState;
}

const char* OTAService::getCurrentVersion() {
    return currentVersion.c_str();
}

const char* OTAService::getUpdateVersion() {
    return updateVersion.c_str();
}

void OTAService::onProgress(ProgressCallback callback) {
    progressCallback = callback;
}

void OTAService::task(void* parameter) {
    esp_task_wdt_add(NULL);
    
    while (true) {
        // Reset watchdog
        esp_task_wdt_reset();
        
        // Handle check request
        if (checkRequested) {
            checkRequested = false;
            if (isValidUrl(updateUrl.c_str())) {
                performUpdateCheck(updateUrl.c_str());
            } else {
                updateState(SVC::OTAService::State::FAILED, 0, "Invalid or missing update URL");
            }
        }
        
        // Handle update request
        if (updateRequested) {
            updateRequested = false;
            if (isValidUrl(updateUrl.c_str())) {
                performUpdate(updateUrl.c_str());
            } else {
                updateState(SVC::OTAService::State::FAILED, 0, "Invalid or missing update URL");
            }
        }
        
        // Handle apply request
        if (applyRequested) {
            applyRequested = false;
            performUpdateApply();
        }
        
        // Task delay
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool OTAService::startTask() {
    if (otaTaskHandle != nullptr) {
        return true; // Task already running
    }
    
    // Create OTA task
    BaseType_t result = xTaskCreatePinnedToCore(
        task,
        "OTAService",
        8192, // Larger stack for OTA operations
        NULL,
        1,
        &otaTaskHandle,
        0
    );
    
    return (result == pdPASS);
}

bool OTAService::restartTask() {
    if (otaTaskHandle != nullptr) {
        vTaskDelete(otaTaskHandle);
        otaTaskHandle = nullptr;
    }
    
    return startTask();
}

} // namespace SVC

#endif // CONFIG_ENABLE_OTA_ANEDYA