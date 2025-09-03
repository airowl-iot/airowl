// mqtt_service.h - MQTT Service for Airowl 3.0
#pragma once

#include <Arduino.h>
#include <PubSubClient.h>

namespace SVC {

class MQTTService {
public:
    /**
     * @brief MQTT connection states
     */
    enum class State {
        DISCONNECTED,    // Not connected to broker
        CONNECTING,      // Connection attempt in progress
        CONNECTED,       // Connected to broker
        FAILED           // Connection attempt failed
    };
    
    /**
     * @brief MQTT QoS levels
     */
    enum class QoS {
        AT_MOST_ONCE = 0,     // Fire and forget
        AT_LEAST_ONCE = 1,     // Guaranteed delivery, may duplicate
        EXACTLY_ONCE = 2       // Guaranteed delivery, no duplicates
    };
    
    /**
     * @brief MQTT message callback type
     */
    using MessageCallback = std::function<void(const char* topic, const uint8_t* payload, unsigned int length)>;
    
    /**
     * @brief MQTT state callback type
     */
    using StateCallback = std::function<void(State state)>;
    
    /**
     * @brief Initialize MQTT service
     * @param clientId MQTT client ID
     * @return true if initialization was successful, false otherwise
     */
    static bool init(const char* clientId = "airowl");
    
    /**
     * @brief Connect to MQTT broker
     * @param server Broker hostname or IP
     * @param port Broker port
     * @param username Username (nullptr for no auth)
     * @param password Password (nullptr for no auth)
     * @return true if connection process started, false otherwise
     */
    static bool connect(const char* server, uint16_t port = 1883, 
                        const char* username = nullptr, const char* password = nullptr);
    
    // static bool connect();
    
    /**
     * @brief Disconnect from MQTT broker
     * @return true if disconnection was successful, false otherwise
     */
    static bool disconnect();
    
    /**
     * @brief Get current MQTT state
     * @return Current state
     */
    static State getState();
    
    /**
     * @brief Subscribe to MQTT topic
     * @param topic Topic to subscribe to
     * @param qos Quality of Service level
     * @return true if subscription was successful, false otherwise
     */
    static bool subscribe(const char* topic, QoS qos = QoS::AT_MOST_ONCE);
    
    /**
     * @brief Unsubscribe from MQTT topic
     * @param topic Topic to unsubscribe from
     * @return true if unsubscription was successful, false otherwise
     */
    static bool unsubscribe(const char* topic);
    
    /**
     * @brief Publish message to MQTT topic
     * @param topic Topic to publish to
     * @param payload Message payload
     * @param length Payload length
     * @param qos Quality of Service level
     * @param retain Whether to retain the message
     * @return true if publish was successful, false otherwise
     */
    static bool publish(const char* topic, const uint8_t* payload, unsigned int length, 
                        QoS qos = QoS::AT_MOST_ONCE, bool retain = false);
    
    /**
     * @brief Publish string message to MQTT topic
     * @param topic Topic to publish to
     * @param message String message
     * @param qos Quality of Service level
     * @param retain Whether to retain the message
     * @return true if publish was successful, false otherwise
     */
    static bool publish(const char* topic, const char* message, 
                        QoS qos = QoS::AT_MOST_ONCE, bool retain = false);
    
    /**
     * @brief Connect to Oizom MQTT broker with predefined credentials
     * @return true if connection process started, false otherwise
     */
    static bool connectToOizom();
    
    /**
     * @brief Publish sensor data to MQTT broker
     * @param deviceId Device identifier
     * @param pm25 PM2.5 value
     * @param pm10 PM10.0 value
     * @param tvoc TVOC value
     * @return true if publish was successful, false otherwise
     */
    static bool publishSensorData(const char* deviceId, float pm25, float pm10, float tvoc);
    
    /**
     * @brief Register message callback
     * @param callback Function to call when messages are received
     */
    static void onMessage(MessageCallback callback);
    
    /**
     * @brief Register state callback
     * @param callback Function to call on state changes
     */
    static void onStateChange(StateCallback callback);
    
    /**
     * @brief Service task that handles MQTT processing and reconnection
     * @param parameter Task parameter (unused)
     */
    static void task(void* parameter);
    
    /**
     * @brief Start the MQTT service task
     * @return true if task started successfully, false otherwise
     */
    static bool startTask();
    
    /**
     * @brief Restart the MQTT service task (after OTA, crash, etc.)
     * @return true if restart was successful, false otherwise
     */
    static bool restartTask();
};

} // namespace SVC