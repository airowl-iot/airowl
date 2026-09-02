// hal_wifi.h - WiFi HAL for Airowl 3.0
#pragma once

#include <Arduino.h>
#include <WiFi.h>

namespace HAL {

class WiFi {
public:
    /**
     * @brief WiFi connection status
     */
    enum class Status {
        IDLE,           // Not connected or initialized
        CONNECTING,     // Connection in progress
        CONNECTED,      // Connected to network
        DISCONNECTED,   // Disconnected from network
        FAILED          // Connection failed
    };
    
    /**
     * @brief WiFi connection info
     */
    struct ConnectionInfo {
        String ssid;         // Connected network SSID
        int32_t rssi;       // Signal strength in dBm
        IPAddress ip;       // Assigned IP address
        IPAddress gateway;  // Gateway IP address
        IPAddress subnet;   // Subnet mask
        IPAddress dns1;     // Primary DNS server
        IPAddress dns2;     // Secondary DNS server
        uint8_t bssid[6];   // BSSID (MAC address) of the access point
        uint8_t channel;    // WiFi channel
    };
    
    /**
     * @brief Initialize WiFi module
     * @return true if initialization was successful, false otherwise
     */
    static bool init();
    
    /**
     * @brief Connect to WiFi network
     * @param ssid Network SSID
     * @param password Network password
     * @return true if connection was successful, false otherwise
     */
    static bool connect(const char* ssid, const char* password);
    
    /**
     * @brief Auto-connect to saved WiFi or start config portal if no credentials
     * @param apName Name for the configuration access point (if needed)
     * @param timeout_ms Timeout for configuration portal in milliseconds
     * @return true if connected (either via saved credentials or new config), false otherwise
     */
    static bool autoConnect(const char* apName = nullptr, uint32_t timeout_ms = 120000);

    /**
     * @brief Start WiFi Manager for configuration
     * @param apName Name for the configuration access point
     * @param timeout_ms Timeout for configuration portal in milliseconds
     * @return true if connected after configuration, false otherwise
     */
    static bool startConfigPortal(const char* apName = nullptr, uint32_t timeout_ms = 120000);

    /**
     * @brief Service a non-blocking WiFiManager configuration portal
     * @return true when the station has connected to WiFi
     */
    static bool process();

    /**
     * @brief Check whether the configuration portal is currently active
     */
    static bool isConfigPortalActive();

    /**
     * @brief Stop active station retries while preserving saved credentials
     */
    static void enterOfflineMode();

    /**
     * @brief Reconnect only when the saved SSID is visible in a WiFi scan
     * @return true when a connection attempt was started
     */
    static bool reconnectIfNetworkAvailable();

    /**
     * @brief Disconnect from WiFi network
     * @return true if disconnection was successful, false otherwise
     */
    static bool disconnect();
    
    /**
     * @brief Get current WiFi status
     * @return Current status
     */
    static Status getStatus();
    
    /**
     * @brief Get current WiFi status
     * @return Current status
     */
    static void printWiFiStatus(uint8_t status);

    /**
     * @brief Get connection information
     * @param info Pointer to connection info structure to fill
     * @return true if information was retrieved successfully, false otherwise
     */
    static bool getConnectionInfo(ConnectionInfo* info);
    
    /**
     * @brief Get signal strength
     * @return Signal strength in dBm, or 0 if not connected
     */
    static int32_t getRSSI();
    
    /**
     * @brief Set hostname
     * @param hostname Hostname to set
     * @return true if hostname was set successfully, false otherwise
     */
    static bool setHostname(const char* hostname);
    
    /**
     * @brief Get MAC address
     * @param mac Pointer to array to store MAC address (6 bytes)
     * @return true if MAC address was retrieved successfully, false otherwise
     */
    static bool getMACAddress(uint8_t* mac);

    static String generateApName(const char* baseName = "AIROWL");
  
};

} // namespace HAL
