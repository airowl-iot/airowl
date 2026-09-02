// wifi_service.h - WiFi Service for Airowl 3.0
#pragma once

#include <Arduino.h>
#include <functional>
#include "hal/hal_wifi.h"

namespace SVC {

class WiFiService {
public:
    /**
     * @brief WiFi connection states
     */
    enum class State {
        DISCONNECTED,    // Not connected to any network
        CONNECTING,      // Connection attempt in progress
        CONNECTED,       // Connected to network
        PROVISIONING,    // WiFi Manager provisioning mode active
        FAILED           // Connection attempt failed
    };
    
    /**
     * @brief WiFi event callback type
     */
    using EventCallback = std::function<void(State state, void* data)>;
    
    /**
     * @brief Initialize WiFi service
     * @param hostname Device hostname
     * @param provisioningTimeoutMs Configuration portal timeout in milliseconds
     * @return true if initialization was successful, false otherwise
     */
    static bool init(const char* hostname = "AIROWL", uint32_t provisioningTimeoutMs = 120000);
    
    /**
     * @brief Start WiFi connection process
     * @param ssid Network SSID (nullptr to use stored credentials)
     * @param password Network password (nullptr to use stored credentials)
     * @return true if connection process started, false otherwise
     */
    static bool connect(const char* ssid = nullptr, const char* password = nullptr);
    
    /**
     * @brief Start WiFi provisioning mode
     * @param apName Name for the configuration access point
     * @param timeout_ms Timeout for configuration portal in milliseconds
     * @return true if provisioning started, false otherwise
     */
    static bool startProvisioning(const char* apName = nullptr, uint32_t timeout_ms = 120000);
    
    /**
     * @brief Disconnect from WiFi network
     * @return true if disconnection was successful, false otherwise
     */
    static bool disconnect();
    
    /**
     * @brief Get current WiFi state
     * @return Current state
     */
    static State getState();
    
    /**
     * @brief Get connection information
     * @return Connection info structure
     */
    static HAL::WiFi::ConnectionInfo getConnectionInfo();
    
    /**
     * @brief Register event callback
     * @param callback Function to call on state changes
     */
    static void onEvent(EventCallback callback);
    
    /**
     * @brief Start the WiFi service task
     * @return true if task started successfully, false otherwise
     */
    static bool startTask();
    
    /**
     * @brief Restart the WiFi service task (after OTA, crash, etc.)
     * @return true if restart was successful, false otherwise
     */
    static bool restartTask();

    private:
    static void task(void* parameter);
};

} // namespace SVC
