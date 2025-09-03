// ota_service.h - Anedya OTA Service for Airowl 3.0
#pragma once

#ifdef CONFIG_ENABLE_OTA_ANEDYA

#include <HTTPClient.h>
#include "HttpsOTAUpdate.h"
#include <functional>

#ifdef __cplusplus
extern "C" {
#endif

// Expose certificate so main.cpp can use it
extern const char *ca_cert;

#ifdef __cplusplus
}
#endif

namespace SVC {

class OTA {
public:
    /**
     * @brief OTA update states
     */
    enum class State {
        IDLE,           // No update in progress
        CHECKING,       // Checking for updates
        DOWNLOADING,    // Downloading update
        UPDATING,       // Applying update
        SUCCESS,        // Update successful
        FAILED          // Update failed
    };
    
    /**
     * @brief OTA update progress callback type
     */
    using ProgressCallback = std::function<void(State state, int progress, const char* message)>;
    
    /**
     * @brief Initialize Anedya OTA service
     * @param regionCode Anedya region code (e.g., "ap-in-1")
     * @param connectionKey Anedya connection key
     * @param physicalDeviceId Physical device ID
     * @param caCert CA certificate for HTTPS
     * @return true if initialization was successful, false otherwise
     */
    static bool init(const char* regionCode, const char* connectionKey, 
                     const char* physicalDeviceId, const char* caCert);
    
    /**
     * @brief Check for updates from Anedya server
     * @param assetURLBuf Buffer to store asset URL (legacy compatibility)
     * @param deploymentIDBuf Buffer to store deployment ID (legacy compatibility)
     * @return true if update is available, false otherwise
     */
    static bool checkForUpdates(char* assetURLBuf = nullptr, char* deploymentIDBuf = nullptr);
    
    /**
     * @brief Start OTA update process
     * @return true if update started successfully, false otherwise
     */
    static bool beginUpdate();
    
    /**
     * @brief Get current OTA state
     * @return Current state
     */
    static State getState();
    
    /**
     * @brief Get current firmware version
     * @return Version string
     */
    static const char* getCurrentVersion();
    
    /**
     * @brief Register progress callback
     * @param callback Function to call on progress updates
     */
    static void onProgress(ProgressCallback callback);
    
    /**
     * @brief Send heartbeat to Anedya server
     * @return true if heartbeat sent successfully, false otherwise
     */
    static bool sendHeartbeat();
    
    /**
     * @brief Synchronize device time with Anedya Time Services
     * @return true if time sync successful, false otherwise
     */
    static bool synchronizeTime();
    
    /**
     * @brief Service task that handles OTA operations
     * @param parameter Task parameter (unused)
     */
    static void task(void* parameter);
    
    /**
     * @brief Main OTA loop - handles periodic checks and updates
     */
    static void loop();
    
    /**
     * @brief Start the Anedya OTA service task
     * @return true if task started successfully, false otherwise
     */
    static bool startTask();
    
    /**
     * @brief Restart the Anedya OTA service task
     * @return true if restart was successful, false otherwise
     */
    static bool restartTask();

    /**
     * @brief Update OTA status on Anedya server
     * @param deploymentId Deployment ID
     * @param status Status string ("start", "success", "failure")
     */
    static void updateOTAStatus(const char* deploymentId, const char* status);
    
    /**
     * @brief Suspend all non-critical tasks during OTA update
     */
    static void suspendOtherTasks();
    
    /**
     * @brief Resume all suspended tasks after OTA completion
     */
    static void resumeOtherTasks();

    /**
     * @brief HTTP event handler for OTA updates
     * @param event HTTP event
     */
    static void httpEventHandler(HttpEvent_t* event);

    // Legacy compatibility - global state variables
    static bool otaInProgress;
    static bool suppressSensorPrinting;
    static bool deploymentAvailable;
    static bool statusPublished;
    static String assetURL;
    static String deploymentID;

private:
};

// Legacy function wrappers for backward compatibility
inline void setDevice_time() { SVC::OTA::synchronizeTime(); }
inline bool anedya_check_ota_update(char* assetURLBuf, char* deploymentIDBuf) { 
    return SVC::OTA::checkForUpdates(assetURLBuf, deploymentIDBuf); 
}
inline void anedya_update_ota_status(const char* deploymentID, const char* deploymentStatus) { 
    SVC::OTA::updateOTAStatus(deploymentID, deploymentStatus); 
}
inline void anedya_sendHeartbeat() { SVC::OTA::sendHeartbeat(); }
inline void ota_loop() { SVC::OTA::loop(); }
inline void initOTA() { /* Use SVC::OTA::init() instead */ }
inline void HttpEvent(HttpEvent_t* event) { SVC::OTA::httpEventHandler(event); }

} // namespace SVC

#else
// If OTA disabled, provide safe empty stubs
namespace SVC {
class OTA {
public:
    enum class State { IDLE };
    using ProgressCallback = std::function<void(State, int, const char*)>;
    
    static bool init(const char*, const char*, const char*, const char*) { return false; }
    static bool checkForUpdates(char* = nullptr, char* = nullptr) { return false; }
    static bool beginUpdate() { return false; }
    static State getState() { return State::IDLE; }
    static const char* getCurrentVersion() { return "0.0.0"; }
    static void onProgress(ProgressCallback) {}
    static bool sendHeartbeat() { return false; }
    static bool synchronizeTime() { return false; }
    static void task(void*) {}
    static void loop() {}
    static bool startTask() { return false; }
    static bool restartTask() { return false; }
    static void updateOTAStatus(const char*, const char*) {}
    static void suspendOtherTasks() {}
    static void resumeOtherTasks() {}
    static void httpEventHandler(HttpEvent_t*) {}
    
    static bool otaInProgress;
    static bool suppressSensorPrinting;
    static bool deploymentAvailable;
    static bool statusPublished;
    static String assetURL;
    static String deploymentID;
};

inline void setDevice_time() {}
inline bool anedya_check_ota_update(char*, char*) { return false; }
inline void anedya_update_ota_status(const char*, const char*) {}
inline void anedya_sendHeartbeat() {}
inline void ota_loop() {}
inline void initOTA() {}
inline void HttpEvent(HttpEvent_t*) {}

} // namespace SVC

#endif // CONFIG_ENABLE_OTA_ANEDYA
