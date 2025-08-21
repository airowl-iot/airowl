// ota_service.h - OTA Service for Airowl 3.0
#pragma once

#ifdef CONFIG_ENABLE_OTA_ANEDYA

#include <Arduino.h>
#include <functional>

namespace SVC {

class OTAService {
public:
    /**
     * @brief OTA update states
     */
    enum class State {
        IDLE,           // No update in progress
        CHECKING,       // Checking for updates
        DOWNLOADING,    // Downloading update
        VERIFYING,      // Verifying downloaded update
        READY,          // Update ready to apply
        UPDATING,       // Applying update
        COMPLETE,       // Update complete, pending reboot
        FAILED          // Update failed
    };
    
    /**
     * @brief OTA update progress callback type
     */
    using ProgressCallback = std::function<void(State state, int progress, const char* message)>;
    
    /**
     * @brief Initialize OTA service
     * @param currentVersion Current firmware version string
     * @return true if initialization was successful, false otherwise
     */
    static bool init(const char* currentVersion);
    
    /**
     * @brief Check for updates from server
     * @param url Update server URL
     * @return true if check started successfully, false otherwise
     */
    static bool checkForUpdates(const char* url);
    
    /**
     * @brief Start OTA update from URL
     * @param url Update file URL
     * @return true if update started successfully, false otherwise
     */
    static bool beginUpdate(const char* url);
    
    /**
     * @brief Apply downloaded update
     * @return true if update is being applied, false otherwise
     */
    static bool applyUpdate();
    
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
     * @brief Get available update version
     * @return Version string, or empty if no update available
     */
    static const char* getUpdateVersion();
    
    /**
     * @brief Register progress callback
     * @param callback Function to call on progress updates
     */
    static void onProgress(ProgressCallback callback);
    
    /**
     * @brief Service task that handles OTA operations
     * @param parameter Task parameter (unused)
     */
    static void task(void* parameter);
    
    /**
     * @brief Start the OTA service task
     * @return true if task started successfully, false otherwise
     */
    static bool startTask();
    
    /**
     * @brief Restart the OTA service task (after crash, etc.)
     * @return true if restart was successful, false otherwise
     */
    static bool restartTask();
};

} // namespace SVC

#endif // CONFIG_ENABLE_OTA_ANEDYA