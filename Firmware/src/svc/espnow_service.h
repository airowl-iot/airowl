#pragma once

#ifdef CONFIG_ENABLE_ESP_NOW

#include <Arduino.h>
#include <esp_now.h>
#include <functional>

namespace SVC {

class ESPNowService {
public:
    /**
     * @brief Slave data structure for master devices
     */
    struct SlaveData {
        float temp;
        float hum;
        float pm1;
        float pm25;
        float pm4;
        float pm10;
        float tvoc;
        uint32_t timestamp;
        bool valid;
    };

    /**
     * @brief Supported ESP-NOW message types
     */
    enum class MessageType : uint8_t {
        SENSOR_DATA = 0x01,
        COMMAND     = 0x02,
        HEARTBEAT   = 0x03,
        STATUS      = 0x04
    };

    /**
     * @brief Structure for received messages
     */
    struct Message {
        MessageType type;
        const uint8_t* data;
        size_t length;
        uint8_t macAddress[6];
    };

    /**
     * @brief Callback types
     */
    using MessageCallback  = std::function<void(const Message& message)>;
    using DeliveryCallback = std::function<void(const uint8_t* mac, bool success)>;

    /**
     * @brief Initialize ESP-NOW service
     * @param master Whether device runs as master
     * @return true if initialization succeeded
     */
    static bool init(bool master = true);

    /**
     * @brief Send sensor data (slave mode)
     * @param slaveId Slave device ID
     * @param temp Temperature value
     * @param hum Humidity value
     * @return true if sent successfully
     */
    static bool send(uint8_t slaveId, float temp, float hum);

    /**
     * @brief Set master/slave mode
     * @param isMaster True for master mode, false for slave mode
     * @return True if mode was set successfully
     */
    static bool setMasterMode(bool isMaster);
    
    /**
     * @brief Check if device is in master mode
     * @return True if device is master, false if slave
     */
    static bool isMaster();
    
    /**
     * @brief Set master device MAC address (for slaves)
     * @param mac Master device MAC address
     */
    static void setMasterMac(const uint8_t* mac);

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

    /**
     * @brief Debug function to print current ESP-NOW status and configuration
     */
    static void debugStatus();

    /**
     * @brief Get the current device MAC address
     * @param mac Buffer to store MAC address (6 bytes)
     */
    static void getDeviceMAC(uint8_t* mac);

    /**
     * @brief Register callback for incoming messages
     */
    static void setMessageCallback(MessageCallback cb);

    /**
     * @brief Register callback for delivery status
     */
    static void setDeliveryCallback(DeliveryCallback cb);

private:
    ESPNowService() = default; // prevent instantiation
};

} // namespace SVC

#endif // CONFIG_ENABLE_ESP_NOW
