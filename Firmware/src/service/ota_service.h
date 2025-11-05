// ota_service.h - OTA Service for Airowl 3.0
#pragma once

#include <functional>
#include "airowl_config.h"

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
     * @brief Initialize OTA service
     * @return true if initialized successfully
     */
    static bool init();

    /**
     * @brief Set OTA URLs from configuration
     * @param versionUrl URL to check version
     * @param firmwareUrl URL to download firmware
     */
    static void setURLs(const char* versionUrl, const char* firmwareUrl);

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
     * @brief Called when WiFi is connected — performs version check and OTA if needed
     */
    static void onWiFiConnected();

    /**
     * @brief Service task that handles OTA operations
     * @param parameter Task parameter (unused)
     */
    static void task(void* parameter);

    /**
     * @brief Main OTA loop - handles OTA status checks
     */
    static void loop();

    /**
     * @brief Start the OTA service task
     * @return true if task started successfully, false otherwise
     */
    static bool startTask();

    /**
     * @brief Restart the OTA service task
     * @return true if restart was successful, false otherwise
     */
    static bool restartTask();

    /**
     * @brief Suspend other tasks during OTA for stability
     */
    static void suspendOtherTasks();

    /**
     * @brief Resume previously suspended tasks after OTA
     */
    static void resumeOtherTasks();

    // Global OTA state flag
    static bool otaInProgress;

private:
};

inline void ota_loop() { SVC::OTA::loop(); }

} // namespace SVC