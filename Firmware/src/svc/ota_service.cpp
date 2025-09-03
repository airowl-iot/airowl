// anedya_ota_service.cpp - Anedya OTA Service implementation for Airowl 3.0
#include "ota_service.h"

#ifdef CONFIG_ENABLE_OTA_ANEDYA

#include "HttpsOTAUpdate.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "config.h"
#include <esp_task_wdt.h>
#include "hal/hal_wifi.h"
#include <string.h>

// Certificate for HTTPS OTA
const char *ca_cert = R"EOF(
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

#ifdef CONFIG_ENABLE_SENSOR_PM700
#include "app/sensor_manager.h"
extern TaskHandle_t sensorTaskHandle;
#endif

#ifdef CONFIG_ENABLE_LVGL
#include "app/ui_controller.h"
extern TaskHandle_t lvglTaskHandle;
#endif

#ifdef CONFIG_ENABLE_ESP_NOW
#include "svc/espnow_service.h"
extern TaskHandle_t espnowTaskHandle;
#endif

#define FIRMWARE_VERSION "3.0"

const unsigned long check_for_ota_interval = 30000; // 30 seconds (you had comment 5 minutes)
const unsigned long heartbeat_interval = 60000 ;    // 1 minute


namespace {
    extern String regionCode;
}

String anedyaApi(const String& endpoint) {
    return "https://device." + regionCode + ".anedya.io" + endpoint;
}

namespace {
    // Private variables
    bool initialized = false;
    SVC::OTA::State currentState = SVC::OTA::State::IDLE;
    SVC::OTA::ProgressCallback progressCallback = nullptr;
    TaskHandle_t otaTaskHandle = nullptr;
    
    String regionCode;
    String connectionKey;
    String physicalDeviceId;
    String caCertificate;
    
    unsigned long last_check_for_ota_update = 0;
    unsigned long last_heartbeat = 0;
    
    String assetURL;
    String deploymentID;
    NetworkClientSecure nccClient;
    HttpsOTAStatus_t otaStatus;

    unsigned long lastOtaCheck = 0;
    unsigned long lastHeartbeat = 0;
    const unsigned long OTA_CHECK_INTERVAL = 120000; // 2 minutes
    const unsigned long HEARTBEAT_INTERVAL = 60000;  // 1 minute
    
    TaskHandle_t suspendedTasks[10]; 
    int suspendedTaskCount = 0;
    bool tasksAreSuspended = false;
    
    void updateState(SVC::OTA::State newState, int progress = -1, const char* message = nullptr) {
        const char* stateNames[] = {
            "IDLE", "CHECKING", "DOWNLOADING", "UPDATING", "SUCCESS", "FAILED"
        };
        
        Serial.printf("[OTA] State change: %s -> %s\n", 
                     stateNames[(int)currentState], stateNames[(int)newState]);
        
        currentState = newState;
        
        if (progress >= 0) {
            Serial.printf("[OTA] Progress: %d%%\n", progress);
        }
        
        if (message) {
            Serial.printf("[OTA] Message: %s\n", message);
        }

        if (progressCallback) {
            Serial.println("[OTA] Calling progress callback");
            progressCallback(newState, progress, message ? message : "");
        }
    }
    
    void httpEventHandler(HttpEvent_t* event) {
        switch (event->event_id) {
        case HTTP_EVENT_ERROR:
            Serial.println("[OTA] Http Event Error");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            Serial.println("[OTA] Http Event On Connected - SSL handshake successful");
            break;
        case HTTP_EVENT_HEADER_SENT:
            Serial.println("[OTA] Http Event Header Sent");
            break;
        case HTTP_EVENT_ON_HEADER:
            Serial.printf("[OTA] Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            // Don't spam logs with data events
            break;
        case HTTP_EVENT_ON_FINISH:
            Serial.println("[OTA] Http Event On Finish");
            break;
        case HTTP_EVENT_DISCONNECTED:
            Serial.println("[OTA] Http Event Disconnected - checking SSL errors");
            break;
        case HTTP_EVENT_REDIRECT:
            Serial.println("[OTA] Http Event Redirect");
            break;
        }
    }
    
    bool performUpdateCheck() {
        Serial.println("[OTA] Checking for OTA updates...");
        
        if (HAL::WiFi::getStatus() != HAL::WiFi::Status::CONNECTED) {
            Serial.println("[OTA] ERROR: WiFi not connected");
            updateState(SVC::OTA::State::FAILED, 0, "WiFi not connected");
            return false;
        }
        
        updateState(SVC::OTA::State::CHECKING, 0, "Checking for updates...");
        
        HTTPClient http;
        String url = anedyaApi("/v1/ota/next");
        
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Accept", "application/json");
        http.addHeader("Auth-mode", "key");
        http.addHeader("Authorization", connectionKey);
        http.setTimeout(5000);
        
        int code = http.POST("{}");
        if (code <= 0) {
            Serial.println("HTTP error: " + String(code));
            http.end();
            updateState(SVC::OTA::State::FAILED, 0, "HTTP Error");
            return false;
        }
        
        JsonDocument doc;
        deserializeJson(doc, http.getString());
        http.end();
        
        if (doc["errorcode"] != 0) {
            Serial.println("OTA API error");
            updateState(SVC::OTA::State::FAILED, 0, "API Error");
            return false;
        }
        
        if (!(bool)doc["deploymentAvailable"]) {
            Serial.println("[OTA] No update available");
            updateState(SVC::OTA::State::IDLE, 100, "No update available");
            return false;
        }
        
        String urlStr = doc["data"]["asseturl"].as<String>();
        String depStr = doc["data"]["deploymentId"].as<String>();
        assetURL = urlStr;
        deploymentID = depStr;
        
        SVC::OTA::assetURL = urlStr;
        SVC::OTA::deploymentID = depStr;
        SVC::OTA::deploymentAvailable = true;
        
        Serial.printf("[OTA] ✓ Update available!\n");
        Serial.printf("[OTA] Asset URL: %s\n", assetURL.c_str());
        Serial.printf("[OTA] Deployment ID: %s\n", deploymentID.c_str());
        
        updateState(SVC::OTA::State::IDLE, 100, "Update available");
        return true;
    }

    bool performOTAUpdate() {
        if (assetURL.isEmpty() || deploymentID.isEmpty()) {
            Serial.println("[OTA] ERROR: No update data available");
            return false;
        }

        // Initialize OTA state
        SVC::OTA::otaInProgress = true;
        SVC::OTA::suppressSensorPrinting = true;
        
        Serial.println("\n[OTA] ⚠️ FIRMWARE UPDATE AVAILABLE - SUSPENDING NON-CRITICAL TASKS");
        
        // Suspend non-critical tasks to free up resources
        SVC::OTA::suspendOtherTasks();
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Configure watchdog with a longer timeout for OTA
        // First delete from current watchdog if exists
        esp_task_wdt_delete(NULL);
        
        esp_task_wdt_config_t wdt_config = {
            .timeout_ms = 120000,  // 120 second timeout for OTA (doubled for SSL issues)
            .idle_core_mask = 0,
            .trigger_panic = true
        };
        
        // Deinitialize current watchdog and reinitialize
        esp_task_wdt_deinit();
        esp_err_t wdt_result = esp_task_wdt_init(&wdt_config);
        if (wdt_result == ESP_OK) {
            esp_task_wdt_add(NULL);
            Serial.println("[OTA] Watchdog reconfigured for OTA with 120s timeout");
        } else {
            Serial.printf("[OTA] Warning: Watchdog reconfig failed: %d\n", wdt_result);
        }
        
        esp_http_client_config_t config = {};
        config.timeout_ms = 60000;  // Increased timeout for SSL handshake
        config.skip_cert_common_name_check = true;  // Skip cert name check to avoid SSL issues
        config.cert_pem = ca_cert;
        config.keep_alive_enable = true;
        config.keep_alive_idle = 5;
        config.keep_alive_interval = 5;
        config.keep_alive_count = 3;
        
        Serial.println("[OTA] Starting OTA update process...");
        updateState(SVC::OTA::State::DOWNLOADING, 0, "Starting update...");
        SVC::OTA::updateOTAStatus(deploymentID.c_str(), "start");

        size_t freeHeap = esp_get_free_heap_size();
        const size_t MIN_HEAP_REQUIRED = 60000; 
        
        if (freeHeap < MIN_HEAP_REQUIRED) {
            Serial.printf("[OTA] ERROR: Insufficient heap: %d bytes free (need %d)\n", freeHeap, (int)MIN_HEAP_REQUIRED);
            SVC::OTA::updateOTAStatus(deploymentID.c_str(), "failure");
            SVC::OTA::otaInProgress = false;
            esp_task_wdt_delete(NULL);
            SVC::OTA::resumeOtherTasks();
            return false;
        }
        
        Serial.printf("[OTA] Starting download from: %s\n", assetURL.c_str());
        Serial.printf("[OTA] Free heap: %d bytes\n", freeHeap);

        HttpsOTA.onHttpEvent(httpEventHandler);
        HTTPClient http;
        http.setConnectTimeout(60000);  // 60s connection timeout for SSL handshake
        http.setTimeout(60000);         // 60s response timeout (within uint16_t range)
        
        // Begin OTA with relaxed certificate verification
        HttpsOTA.begin(assetURL.c_str(), ca_cert, false);  // Disable strict cert verification
        
        const uint32_t OTA_TIMEOUT = 300000;  // 5 minutes total timeout
        uint32_t startTime = millis();
        uint32_t lastProgressUpdate = 0;
        int lastProgress = -1;
        int retryCount = 0;
        const int MAX_RETRIES = 3;
        
        while (SVC::OTA::otaInProgress && (millis() - startTime) < OTA_TIMEOUT) {
            // Reset watchdog more frequently
            esp_task_wdt_reset();

            // Check connectivity and handle retries
            if (WiFi.status() != WL_CONNECTED) {
                if (retryCount >= MAX_RETRIES) {
                    Serial.println("[OTA] Network disconnected and max retries reached");
                    updateState(SVC::OTA::State::FAILED, 0, "Network disconnected");
                    break;
                }
                
                // Reset WiFi if multiple failures
                if (retryCount > 0 && retryCount % 2 == 0) {
                    Serial.println("[OTA] Resetting WiFi connection...");
                    WiFi.disconnect();
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    esp_task_wdt_reset();  // Reset watchdog during WiFi reconnect
                    WiFi.reconnect();
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    esp_task_wdt_reset();  // Reset watchdog after reconnect
                }
                
                Serial.printf("[OTA] Network disconnected, retry %d/%d...\n", retryCount + 1, MAX_RETRIES);
                uint32_t backoff_time = 2000 * (retryCount + 1);
                uint32_t backoff_start = millis();
                while ((millis() - backoff_start) < backoff_time) {
                    esp_task_wdt_reset();  // Keep feeding watchdog during backoff
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                retryCount++;
                continue;
            }

            otaStatus = HttpsOTA.status();
            
            uint32_t elapsed = millis() - startTime;
            int progress = (elapsed * 100) / OTA_TIMEOUT;
            progress = constrain(progress, 0, 99); 
            
            if (progress != lastProgress && (millis() - lastProgressUpdate) > 1000) {
                lastProgress = progress;
                lastProgressUpdate = millis();
                size_t currentHeap = esp_get_free_heap_size();
                size_t minFreeHeap = esp_get_minimum_free_heap_size();
                Serial.printf("[OTA] Progress: %d%%, Heap: %d (min: %d)\n", progress, currentHeap, (int)minFreeHeap);
                updateState(SVC::OTA::State::DOWNLOADING, progress, "Downloading update...");
            }

            switch (otaStatus) {
                case HTTPS_OTA_SUCCESS:
                    Serial.println("\n[OTA] ✓ Update downloaded successfully");
                    SVC::OTA::updateOTAStatus(deploymentID.c_str(), "success");
                    updateState(SVC::OTA::State::SUCCESS, 100, "Update complete, rebooting...");
                    esp_task_wdt_delete(NULL);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    ESP.restart();
                    return true;

                case HTTPS_OTA_FAIL:
                case HTTPS_OTA_ERR:
                    if (retryCount < MAX_RETRIES) {
                        retryCount++;
                        Serial.printf("[OTA] OTA error/status=%d, retry %d/%d\n", otaStatus, retryCount, MAX_RETRIES);
                        
                        // For SSL/TLS errors, try with different certificate settings
                        if (retryCount == 2) {
                            Serial.println("[OTA] SSL error detected, retrying with insecure connection");
                            HttpsOTA.begin(assetURL.c_str(), "", false);  // Try without certificate
                        }
                        
                        uint32_t retry_delay = 3000 * retryCount;
                        uint32_t retry_start = millis();
                        while ((millis() - retry_start) < retry_delay) {
                            esp_task_wdt_reset();  // Keep feeding watchdog during retry delay
                            vTaskDelay(pdMS_TO_TICKS(500));
                        }
                        continue;
                    }
                    Serial.printf("[OTA] Error during download: %d\n", otaStatus);
                    updateState(SVC::OTA::State::FAILED, 0, "Download failed");
                    SVC::OTA::updateOTAStatus(deploymentID.c_str(), "failure");
                    SVC::OTA::otaInProgress = false;
                    break;

                default:
                    break;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        if (SVC::OTA::otaInProgress) {
            if ((millis() - startTime) >= OTA_TIMEOUT) {
                Serial.println("\n[OTA] ✗ Update timed out");
                updateState(SVC::OTA::State::FAILED, 0, "Update timed out");
                SVC::OTA::updateOTAStatus(deploymentID.c_str(), "timeout");
            }
        }

        // Reset state
        SVC::OTA::otaInProgress = false;
        SVC::OTA::deploymentAvailable = false;
        SVC::OTA::suppressSensorPrinting = false;
        
        // Reset watchdog to normal configuration
        esp_task_wdt_delete(NULL);
        esp_task_wdt_deinit();
        
        esp_task_wdt_config_t wdt_normal = {
            .timeout_ms = 20000,
            .idle_core_mask = 0,
            .trigger_panic = true
        };
        
        esp_err_t wdt_normal_result = esp_task_wdt_init(&wdt_normal);
        if (wdt_normal_result == ESP_OK) {
            Serial.println("[OTA] Watchdog restored to normal 20s timeout");
        } else {
            Serial.printf("[OTA] Warning: Normal watchdog init failed: %d\n", wdt_normal_result);
        }

        // Resume suspended tasks
        SVC::OTA::resumeOtherTasks();
        
        Serial.printf("[OTA] OTA update finished. Free heap: %d\n", esp_get_free_heap_size());
        return false;
    }

    void resumeLegacyTasks() {
        #ifdef CONFIG_ENABLE_LVGL
        // restartLVGLTask();
        #endif
        
        #ifdef CONFIG_ENABLE_ESP_NOW
        // restartESPNowTask();
        #endif
    }
} // end anonymous namespace

namespace SVC {

bool OTA::otaInProgress = false;
bool OTA::suppressSensorPrinting = false;
bool OTA::deploymentAvailable = false;
bool OTA::statusPublished = false;
String OTA::assetURL = "";
String OTA::deploymentID = "";

void OTA::httpEventHandler(HttpEvent_t* event) {
    httpEventHandler(event);
}

bool OTA::init(const char* region, const char* connKey, 
                           const char* deviceId, const char* caCert) {
    if (initialized) return true;
    
    Serial.println("[OTA] Initializing OTA service...");
    
    regionCode = region;
    connectionKey = connKey;
    physicalDeviceId = deviceId;
    caCertificate = caCert;
    
    nccClient.setCACert(caCert);
    HttpsOTA.onHttpEvent(httpEventHandler);
    
    synchronizeTime();
    
    initialized = true;
    updateState(State::IDLE, 0, " OTA service initialized");
    
    Serial.printf("[OTA] ✓ Initialized with region: %s\n", region);
    return true;
}

bool OTA::checkForUpdates(char* assetURLBuf, char* deploymentIDBuf) {
    if (!initialized) return false;
    
    bool updateAvailable = performUpdateCheck();
    
    if (updateAvailable && assetURLBuf && deploymentIDBuf) {
        assetURL.toCharArray(assetURLBuf, 300);
        deploymentID.toCharArray(deploymentIDBuf, 50);
    }
    return updateAvailable;
}

bool OTA::beginUpdate() {
    if (!initialized || currentState != State::IDLE) return false;
    return performOTAUpdate();
}

OTA::State OTA::getState() {
    return currentState;
}

const char* OTA::getCurrentVersion() {
    return FIRMWARE_VERSION;
}

void OTA::onProgress(ProgressCallback callback) {
    progressCallback = callback;
    Serial.println("[OTA] Progress callback registered");
}

bool OTA::sendHeartbeat() {
    if (!initialized || HAL::WiFi::getStatus() != HAL::WiFi::Status::CONNECTED) {
        return false;
    }
    
    HTTPClient http;
    String url = anedyaApi("/v1/heartbeat");
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Auth-mode", "key");
    http.addHeader("Authorization", connectionKey);
    int code = http.POST("{}");
    
    if (code > 0) {
        Serial.println("Heartbeat sent");
        http.end();
        return true;
    } else {
        Serial.println("Heartbeat error");
        http.end();
        return false;
    }
}

bool OTA::synchronizeTime() {
    if (!initialized || HAL::WiFi::getStatus() != HAL::WiFi::Status::CONNECTED) {
        return false;
    }
    
    String url = anedyaApi("/v1/time");
    Serial.print("ATS time sync ");
    
    while (true) {
        long long deviceSendTime = millis();
        JsonDocument req;
        req["deviceSendTime"] = deviceSendTime;
        String payload;
        serializeJson(req, payload);
        
        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        int code = http.POST(payload);
        
        if (code == 200) {
            JsonDocument res;
            deserializeJson(res, http.getString());
            long long serverReceiveTime = res["serverReceiveTime"];
            long long serverSendTime = res["serverSendTime"];
            long long deviceRecvTime = millis();
            long long cur = (serverReceiveTime + serverSendTime + deviceRecvTime - deviceSendTime) / 2;
            setTime(cur / 1000);
            Serial.println("✓");
            http.end();
            return true;
        } else {
            Serial.print(".");
            delay(2000);
            http.end();
        }
    }
}

void OTA::updateOTAStatus(const char* deploymentId, const char* status) {
    if (!initialized || HAL::WiFi::getStatus() != HAL::WiFi::Status::CONNECTED) {
        return;
    }
    
    HTTPClient http;
    String url = anedyaApi("/v1/ota/updateStatus");
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("Auth-mode", "key");
    http.addHeader("Authorization", connectionKey);
    
    String body = "{\"deploymentId\":\"" + String(deploymentId) +
                  "\",\"status\":\"" + String(status) +
                  "\",\"log\":\"log\"}";
    int code = http.POST(body);
    
    if (code > 0) {
        Serial.println("Status -> " + String(status));
    } else {
        Serial.println("Status POST error");
    }
    http.end();
}

void OTA::suspendOtherTasks() {
    if (tasksAreSuspended) return;
    
    Serial.println("[OTA] 🛑 Suspending non-critical tasks for OTA priority...");
    
    suspendedTaskCount = 0;
    memset(suspendedTasks, 0, sizeof(suspendedTasks));

    #ifdef CONFIG_ENABLE_LVGL
    if (lvglTaskHandle) {
        Serial.println("[OTA] Suspending LVGL task (handle)");
        vTaskSuspend(lvglTaskHandle);
        if (suspendedTaskCount < (int)(sizeof(suspendedTasks)/sizeof(suspendedTasks[0]))) {
            suspendedTasks[suspendedTaskCount++] = lvglTaskHandle;
        }
    }
    #endif

    #ifdef CONFIG_ENABLE_SENSOR_PM700
    if (sensorTaskHandle) {
        Serial.println("[OTA] Suspending Sensor task (handle)");
        vTaskSuspend(sensorTaskHandle);
        if (suspendedTaskCount < (int)(sizeof(suspendedTasks)/sizeof(suspendedTasks[0]))) {
            suspendedTasks[suspendedTaskCount++] = sensorTaskHandle;
        }
    }
    #endif

    #ifdef CONFIG_ENABLE_ESP_NOW
    if (espnowTaskHandle) {
        Serial.println("[OTA] Suspending ESP-NOW task (handle)");
        vTaskSuspend(espnowTaskHandle);
        if (suspendedTaskCount < (int)(sizeof(suspendedTasks)/sizeof(suspendedTasks[0]))) {
            suspendedTasks[suspendedTaskCount++] = espnowTaskHandle;
        }
    }
    #endif

    // Common app task names to suspend (these must match your created task names)
    const char* TASKS_TO_SUSPEND[] = {
        "UIController",
        "ModeManager",
        "MQTTService",
        "SensorManager",
        "displayTask",
        "sensorTask",
        "worker",
        "async_udp"
    };
    const size_t TASK_COUNT = sizeof(TASKS_TO_SUSPEND) / sizeof(TASKS_TO_SUSPEND[0]);

    // System tasks to keep running (case-insensitive compare)
    const char* SYSTEM_KEEP[] = {
        "OTA","IDLE","IDLE0","IDLE1","Tmr Svc","ipc0","ipc1",
        "esp_timer","wifi","tcpip_task","WiFiService",
        "async_udp","sys_evt","arduino_events","main","loopTask"
    };

    auto isSystemKeep = [&](const char* name)->bool {
        if (!name) return true;
        for (size_t i = 0; i < sizeof(SYSTEM_KEEP)/sizeof(SYSTEM_KEEP[0]); ++i) {
            if (strcasecmp(name, SYSTEM_KEEP[i]) == 0) return true;
        }
        return false;
    };

    for (size_t i = 0; i < TASK_COUNT && suspendedTaskCount < (int)(sizeof(suspendedTasks)/sizeof(suspendedTasks[0])); ++i) {
        const char* tname = TASKS_TO_SUSPEND[i];
        if (isSystemKeep(tname)) continue;
        TaskHandle_t h = xTaskGetHandle(tname);
        if (h == nullptr) continue;
        if (h == otaTaskHandle) continue;

        bool already = false;
        for (int j = 0; j < suspendedTaskCount; ++j) {
            if (suspendedTasks[j] == h) { already = true; break; }
        }
        if (already) continue;

        eTaskState state = eTaskGetState(h);
        if (state == eSuspended || state == eDeleted) continue;

        Serial.printf("[OTA] Suspending app task by name: %s\n", tname);
        vTaskSuspend(h);

        // Safely remove from watchdog - check if task was registered first
        esp_err_t e = esp_task_wdt_delete(h);
        if (e == ESP_ERR_NOT_FOUND) {
            // Task wasn't registered with WDT, which is fine
            Serial.printf("[OTA] WDT: task %s not registered (ok)\n", tname);
        } else if (e == ESP_ERR_INVALID_ARG) {
            Serial.printf("[OTA] WDT: invalid task handle for %s (ok)\n", tname);
        } else if (e != ESP_OK) {
            Serial.printf("[OTA] WDT: esp_task_wdt_delete(%s) returned %d\n", tname, e);
        } else {
            Serial.printf("[OTA] WDT: deleted subscription for %s\n", tname);
        }

        suspendedTasks[suspendedTaskCount++] = h;
    }

    tasksAreSuspended = true;
    Serial.printf("[OTA] ✓ Suspended %d app tasks for OTA priority\n", suspendedTaskCount);
    Serial.printf("[OTA] Free heap after suspend: %d\n", esp_get_free_heap_size());
}

void OTA::resumeOtherTasks() {
    if (!tasksAreSuspended) return;

    Serial.println("[OTA] ▶️ Resuming suspended tasks...");
    int resumedCount = 0;

    for (int i = 0; i < suspendedTaskCount; ++i) {
        TaskHandle_t h = suspendedTasks[i];
        if (h == NULL) continue;

        eTaskState state = eTaskGetState(h);
        if (state == eSuspended) {
            vTaskResume(h);
            
            // Only re-add to watchdog if it exists and accepts new tasks
            esp_err_t e = esp_task_wdt_add(h);
            if (e == ESP_OK) {
                Serial.printf("[OTA] WDT: added subscription for handle %p\n", h);
            } else if (e == ESP_ERR_NO_MEM) {
                Serial.printf("[OTA] WDT: no memory to add handle %p (continuing anyway)\n", h);
            } else if (e == ESP_ERR_INVALID_STATE) {
                Serial.printf("[OTA] WDT: not initialized, skipping handle %p\n", h);
            } else {
                Serial.printf("[OTA] WDT: esp_task_wdt_add(handle=%p) returned %d\n", h, e);
            }

            resumedCount++;
            Serial.printf("[OTA] Resumed task handle %p\n", h);
            vTaskDelay(pdMS_TO_TICKS(10)); // small pause to let it run
        } else {
            Serial.printf("[OTA] Not resuming handle %p (state=%d)\n", h, state);
        }
        suspendedTasks[i] = NULL;
    }

    suspendedTaskCount = 0;
    tasksAreSuspended = false;

    size_t freeHeap = esp_get_free_heap_size();
    size_t minFreeHeap = esp_get_minimum_free_heap_size();
    Serial.printf("[OTA] Resumed %d tasks. Free heap: %d (min: %d)\n", resumedCount, (int)freeHeap, (int)minFreeHeap);
}

void OTA::loop() {
    unsigned long now = millis();
    
    if (now - last_heartbeat >= heartbeat_interval) {
        last_heartbeat = now;
        sendHeartbeat();
    }
    
    if (!otaInProgress && (now - last_check_for_ota_update) >= check_for_ota_interval) {
        last_check_for_ota_update = now;
        char urlBuf[300] = "";
        char depBuf[50] = "";
        if (checkForUpdates(urlBuf, depBuf)) {
            assetURL = urlBuf;
            deploymentID = depBuf;
            deploymentAvailable = true;
        }
    }
    
    if (deploymentAvailable && !otaInProgress) {
        beginUpdate();
    }
}

void OTA::task(void* parameter) {
    otaTaskHandle = xTaskGetCurrentTaskHandle();
    if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) {
        synchronizeTime();
    }
    
    while (true) {
        if (HAL::WiFi::getStatus() == HAL::WiFi::Status::CONNECTED) {
            loop();
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // 5 second task cycle
    }
}

bool OTA::startTask() {
    if (otaTaskHandle != nullptr) return true;
    
    BaseType_t result = xTaskCreatePinnedToCore(
        task,
        "OTA",
        8192, 
        NULL,
        1,
        &otaTaskHandle,
        0
    );
    
    if (result == pdPASS) {
        Serial.println("[OTA] ✓ Task started successfully");
        return true;
    } else {
        Serial.println("[OTA] ✗ Failed to start task");
        return false;
    }
}

bool OTA::restartTask() {
    if (otaTaskHandle != nullptr) {
        vTaskDelete(otaTaskHandle);
        otaTaskHandle = nullptr;
    }
    return startTask();
}

} // namespace SVC

#endif // CONFIG_ENABLE_OTA_ANEDYA
