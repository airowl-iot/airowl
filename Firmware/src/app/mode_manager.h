// mode_manager.h - Mode Manager for Airowl 3.0
#pragma once

#include <Arduino.h>
#include <functional>
#include "../core/event_bus.h"

namespace APP {
class ModeManager {
public:
    /**
     * @brief Operating modes
     */
    enum class Mode {
        NORMAL,        
        PROVISIONING,  
        #ifndef CONFIG_ENABLE_OTA_ANEDYA
            OTA_UPDATE,     
        #endif
    };
    
    /**
     * @brief Mode change callback type
     */
    using ModeChangeCallback = std::function<void(Mode previousMode, Mode newMode)>;
    
    /**
     * @brief Initialize the mode manager
     * @param initialMode Initial operating mode
     * @return True if initialization was successful
     */
    static bool init(Mode initialMode = Mode::NORMAL);
    
    /**
     * @brief Get current operating mode
     * @return Current mode
     */
    static Mode getCurrentMode();
    
    /**
     * @brief Change operating mode
     * @param newMode New operating mode
     * @return True if mode was changed successfully
     */
    static bool changeMode(Mode newMode);
    
    /**
     * @brief Register callback for mode changes
     * @param callback Function to call when mode changes
     */
    static void onModeChange(ModeChangeCallback callback);
    
    /**
     * @brief Mode manager task function
     * @param parameter Task parameters (unused)
     */
    static void task(void* parameter);
    
    /**
     * @brief Start the mode manager task
     * @return True if task was started successfully
     */
    static bool startTask();
    
    /**
     * @brief Restart the mode manager task
     * @return True if task was restarted successfully
     */
    static bool restartTask();

private:
    /**
     * @brief Handle WiFi state changed events
     * @param event WiFi state changed event
     */
    static void handleWiFiStateChangedEvent(const CORE::Event& event);
    
#ifndef CONFIG_ENABLE_OTA_ANEDYA
    /**
     * @brief Handle OTA progress events
     * @param event OTA progress event
     */
    static void handleOTAProgressEvent(const CORE::Event& event);
#endif
    
    /**
     * @brief Handle command received events
     * @param event Command received event
     */
    static void handleCommandReceivedEvent(const CORE::Event& event);
};

} // namespace APP