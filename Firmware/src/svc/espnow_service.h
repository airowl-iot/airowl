// espnow_service.h - ESP-NOW Service interface for Airowl 3.0
#pragma once

#ifdef CONFIG_ENABLE_ESP_NOW

#include <Arduino.h>
#include <esp_now.h>

namespace SVC {

class ESPNowService {
public:
    /**
     * @brief Maximum size of ESP-NOW message payload
     */
    static constexpr size_t MAX_MESSAGE_SIZE = 250;
    
    /**
     * @brief ESP-NOW message structure
     */
    struct Message {
        uint8_t type;                       ///< Message type
        uint8_t id;                         ///< Message ID for tracking
        uint8_t data[MAX_MESSAGE_SIZE];     ///< Message payload
        size_t length;                      ///< Actual payload length
        uint8_t macAddress[6];              ///< Sender/target MAC address
    };
    
    /**
     * @brief Message delivery status
     */
    enum class DeliveryStatus {
        PENDING,        ///< Message delivery pending
        DELIVERED,      ///< Message successfully delivered
        FAILED,         ///< Message delivery failed
        TIMEOUT         ///< Message delivery timed out
    };
    
    /**
     * @brief Message callback type
     */
    using MessageCallback = std::function<void(const Message& message)>;
    
    /**
     * @brief Delivery status callback type
     */
    using DeliveryCallback = std::function<void(uint8_t id, DeliveryStatus status)>;
    
    /**
     * @brief Initialize the ESP-NOW service
     * @param isMaster Whether this device is a master device
     * @return True if initialization was successful
     */
    static bool init(bool isMaster);
    
    /**
     * @brief Send a message to a specific peer
     * @param type Message type
     * @param data Message data
     * @param length Message data length
     * @param macAddress Target MAC address
     * @param retries Number of retries if delivery fails
     * @param timeout Timeout in milliseconds for delivery confirmation
     * @return Message ID if sent successfully, 0 if failed
     */
    static uint8_t sendMessage(uint8_t type, const uint8_t* data, size_t length, 
                               const uint8_t* macAddress, uint8_t retries = 3, 
                               unsigned long timeout = 1000);
    
    /**
     * @brief Send a broadcast message to all peers
     * @param type Message type
     * @param data Message data
     * @param length Message data length
     * @return Message ID if sent successfully, 0 if failed
     */
    static uint8_t broadcastMessage(uint8_t type, const uint8_t* data, size_t length);
    
    /**
     * @brief Add a peer to the ESP-NOW network
     * @param macAddress Peer MAC address
     * @param channel WiFi channel (0 = current channel)
     * @param encrypt Whether to encrypt communication with this peer
     * @param key Encryption key (if encrypt is true)
     * @return True if peer was added successfully
     */
    static bool addPeer(const uint8_t* macAddress, uint8_t channel = 0, 
                        bool encrypt = false, const uint8_t* key = nullptr);
    
    /**
     * @brief Remove a peer from the ESP-NOW network
     * @param macAddress Peer MAC address
     * @return True if peer was removed successfully
     */
    static bool removePeer(const uint8_t* macAddress);
    
    /**
     * @brief Register callback for incoming messages
     * @param callback Function to call when a message is received
     */
    static void onMessage(MessageCallback callback);
    
    /**
     * @brief Register callback for delivery status updates
     * @param callback Function to call when delivery status changes
     */
    static void onDeliveryStatus(DeliveryCallback callback);
    
    /**
     * @brief ESP-NOW service task function
     * @param parameter Task parameters (unused)
     */
    static void task(void* parameter);
    
    /**
     * @brief Start the ESP-NOW service task
     * @return True if task was started successfully
     */
    static bool startTask();
    
    /**
     * @brief Restart the ESP-NOW service task
     * @return True if task was restarted successfully
     */
    static bool restartTask();
    
private:
    // Private implementation details
};

} // namespace SVC

#endif // CONFIG_ENABLE_ESP_NOW