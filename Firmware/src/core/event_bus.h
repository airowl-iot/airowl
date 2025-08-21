// event_bus.h - Event Bus interface for Airowl 3.0
#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>
#include <map>
#include <mutex>

namespace CORE {

/**
 * @brief Base Event class that all specific events inherit from
 */
class Event {
public:
    /**
     * @brief Event types enumeration
     */
    enum class Type {
        SENSOR_READING,     ///< New sensor reading available
        WIFI_STATE_CHANGED, ///< WiFi connection state changed
        MQTT_CONNECTED,     ///< MQTT connection established
        MQTT_DISCONNECTED,  ///< MQTT connection lost
        OTA_PROGRESS,       ///< OTA update progress
        COMMAND_RECEIVED,   ///< Command received from external source
        UI_EVENT,           ///< User interface event
        SYSTEM_ERROR        ///< System error occurred
    };

    /**
     * @brief Constructor
     * @param type Event type
     */
    Event(Type type) : type_(type) {}
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Event() {}
    
    /**
     * @brief Get event type
     * @return Event type
     */
    Type getType() const { return type_; }
    
    /**
     * @brief Get timestamp when event was created
     * @return Timestamp in milliseconds
     */
    uint64_t getTimestamp() const { return timestamp_; }

private:
    Type type_;                      ///< Event type
    uint64_t timestamp_ = millis(); ///< Event creation timestamp
};

/**
 * @brief Sensor Reading Event
 */
class SensorReadingEvent : public Event {
public:
    /**
     * @brief Sensor types
     */
    enum class SensorType {
        PMS,        ///< Particulate Matter Sensor
        AHT,        ///< Temperature/Humidity Sensor
        OTHER       ///< Other sensor type
    };
    
    /**
     * @brief Constructor
     * @param sensorType Type of sensor
     * @param sensorId Unique sensor identifier
     * @param values Array of sensor values
     * @param valueCount Number of values in the array
     */
    SensorReadingEvent(SensorType sensorType, uint8_t sensorId, 
                       const float* values, uint8_t valueCount)
        : Event(Type::SENSOR_READING), 
          sensorType_(sensorType), 
          sensorId_(sensorId),
          valueCount_(valueCount) {
        // Copy values (up to MAX_VALUES)
        for (uint8_t i = 0; i < valueCount_ && i < MAX_VALUES; i++) {
            values_[i] = values[i];
        }
    }
    
    /**
     * @brief Get sensor type
     * @return Sensor type
     */
    SensorType getSensorType() const { return sensorType_; }
    
    /**
     * @brief Get sensor ID
     * @return Sensor ID
     */
    uint8_t getSensorId() const { return sensorId_; }
    
    /**
     * @brief Get sensor values
     * @return Pointer to sensor values array
     */
    const float* getValues() const { return values_; }
    
    /**
     * @brief Get number of sensor values
     * @return Number of values
     */
    uint8_t getValueCount() const { return valueCount_; }

private:
    static constexpr uint8_t MAX_VALUES = 8; ///< Maximum number of values
    SensorType sensorType_;                  ///< Type of sensor
    uint8_t sensorId_;                       ///< Sensor identifier
    float values_[MAX_VALUES];               ///< Sensor values
    uint8_t valueCount_;                     ///< Number of values
};

/**
 * @brief WiFi State Changed Event
 */
class WiFiStateChangedEvent : public Event {
public:
    /**
     * @brief WiFi states
     */
    enum class WiFiState {
        DISCONNECTED,    ///< WiFi disconnected
        CONNECTING,      ///< WiFi connecting
        CONNECTED,       ///< WiFi connected
        PROVISIONING,    ///< WiFi in provisioning mode
        FAILED           ///< WiFi connection failed
    };
    
    /**
     * @brief Constructor
     * @param state New WiFi state
     * @param ssid SSID when connected (empty otherwise)
     * @param rssi RSSI value when connected
     */
    WiFiStateChangedEvent(WiFiState state, const String& ssid = "", int rssi = 0)
        : Event(Type::WIFI_STATE_CHANGED), 
          state_(state), 
          ssid_(ssid),
          rssi_(rssi) {}
    
    /**
     * @brief Get WiFi state
     * @return WiFi state
     */
    WiFiState getState() const { return state_; }
    
    /**
     * @brief Get SSID
     * @return SSID string
     */
    const String& getSSID() const { return ssid_; }
    
    /**
     * @brief Get RSSI
     * @return RSSI value
     */
    int getRSSI() const { return rssi_; }

private:
    WiFiState state_; ///< WiFi state
    String ssid_;    ///< SSID when connected
    int rssi_;       ///< RSSI when connected
};

/**
 * @brief MQTT Connection Event
 */
class MQTTConnectionEvent : public Event {
public:
    /**
     * @brief Constructor for connected event
     * @param broker Broker address
     * @param clientId Client ID used for connection
     */
    MQTTConnectionEvent(bool connected, const String& broker, const String& clientId)
        : Event(connected ? Type::MQTT_CONNECTED : Type::MQTT_DISCONNECTED), 
          broker_(broker), 
          clientId_(clientId) {}
    
    /**
     * @brief Get broker address
     * @return Broker address
     */
    const String& getBroker() const { return broker_; }
    
    /**
     * @brief Get client ID
     * @return Client ID
     */
    const String& getClientId() const { return clientId_; }

private:
    String broker_;    ///< Broker address
    String clientId_;  ///< Client ID
};

/**
 * @brief OTA Progress Event
 */
class OTAProgressEvent : public Event {
public:
    /**
     * @brief OTA states
     */
    enum class OTAState {
        CHECKING,    ///< Checking for updates
        DOWNLOADING, ///< Downloading update
        VERIFYING,   ///< Verifying update
        READY,       ///< Update ready to apply
        UPDATING,    ///< Applying update
        COMPLETE,    ///< Update complete
        FAILED       ///< Update failed
    };
    
    /**
     * @brief Constructor
     * @param state OTA state
     * @param progress Progress percentage (0-100)
     * @param message Status message
     */
    OTAProgressEvent(OTAState state, int progress, const String& message)
        : Event(Type::OTA_PROGRESS), 
          state_(state), 
          progress_(progress),
          message_(message) {}
    
    /**
     * @brief Get OTA state
     * @return OTA state
     */
    OTAState getState() const { return state_; }
    
    /**
     * @brief Get progress percentage
     * @return Progress (0-100)
     */
    int getProgress() const { return progress_; }
    
    /**
     * @brief Get status message
     * @return Status message
     */
    const String& getMessage() const { return message_; }

private:
    OTAState state_;   ///< OTA state
    int progress_;     ///< Progress percentage
    String message_;   ///< Status message
};

/**
 * @brief Command Received Event
 */
class CommandReceivedEvent : public Event {
public:
    /**
     * @brief Command source
     */
    enum class Source {
        MQTT,       ///< Command received via MQTT
        ESP_NOW,    ///< Command received via ESP-NOW
        SERIAL_PORT,     ///< Command received via Serial
        BLE,        ///< Command received via BLE
        OTHER       ///< Other source
    };
    
    /**
     * @brief Constructor
     * @param source Command source
     * @param command Command string
     * @param payload Command payload
     */
    CommandReceivedEvent(Source source, const String& command, const String& payload)
        : Event(Type::COMMAND_RECEIVED), 
          source_(source), 
          command_(command),
          payload_(payload) {}
    
    /**
     * @brief Get command source
     * @return Command source
     */
    Source getSource() const { return source_; }
    
    /**
     * @brief Get command string
     * @return Command string
     */
    const String& getCommand() const { return command_; }
    
    /**
     * @brief Get command payload
     * @return Command payload
     */
    const String& getPayload() const { return payload_; }

private:
    Source source_;     ///< Command source
    String command_;    ///< Command string
    String payload_;    ///< Command payload
};

/**
 * @brief UI Event
 */
class UIEvent : public Event {
public:
    /**
     * @brief UI event types
     */
    enum class UIEventType {
        BUTTON_PRESSED,   ///< Button pressed
        SCREEN_CHANGED,   ///< Screen changed
        VALUE_CHANGED,    ///< Value changed
        GESTURE,          ///< Gesture detected
        OTHER             ///< Other UI event
    };
    
    /**
     * @brief Constructor
     * @param uiEventType UI event type
     * @param elementId UI element identifier
     * @param value Event value
     */
    UIEvent(UIEventType uiEventType, uint16_t elementId, int32_t value)
        : Event(Type::UI_EVENT), 
          uiEventType_(uiEventType), 
          elementId_(elementId),
          value_(value) {}
    
    /**
     * @brief Get UI event type
     * @return UI event type
     */
    UIEventType getUIEventType() const { return uiEventType_; }
    
    /**
     * @brief Get UI element ID
     * @return Element ID
     */
    uint16_t getElementId() const { return elementId_; }
    
    /**
     * @brief Get event value
     * @return Event value
     */
    int32_t getValue() const { return value_; }

private:
    UIEventType uiEventType_; ///< UI event type
    uint16_t elementId_;      ///< UI element ID
    int32_t value_;           ///< Event value
};

/**
 * @brief Event callback type
 */
using EventCallback = std::function<void(const Event&)>;

/**
 * @brief Event Bus class for publish-subscribe pattern
 */
class EventBus {
public:
    /**
     * @brief Get singleton instance
     * @return EventBus instance
     */
    static EventBus& getInstance();
    
    /**
     * @brief Subscribe to events of a specific type
     * @param type Event type to subscribe to
     * @param callback Function to call when event occurs
     * @return Subscription ID (used for unsubscribing)
     */
    uint32_t subscribe(Event::Type type, EventCallback callback);
    
    /**
     * @brief Unsubscribe from events
     * @param subscriptionId Subscription ID returned from subscribe
     * @return True if successfully unsubscribed
     */
    bool unsubscribe(uint32_t subscriptionId);
    
    /**
     * @brief Publish an event to subscribers
     * @param event Event to publish
     */
    void publish(const Event& event);
    
    /**
     * @brief Clear all subscriptions
     */
    void clear();

private:
    /**
     * @brief Constructor (private for singleton)
     */
    EventBus() = default;
    
    /**
     * @brief Copy constructor (deleted)
     */
    EventBus(const EventBus&) = delete;
    
    /**
     * @brief Assignment operator (deleted)
     */
    EventBus& operator=(const EventBus&) = delete;
    
    /**
     * @brief Subscription information
     */
    struct Subscription {
        uint32_t id;             ///< Subscription ID
        Event::Type type;        ///< Event type
        EventCallback callback;  ///< Callback function
    };
    
    std::vector<Subscription> subscriptions_; ///< List of subscriptions
    std::mutex mutex_;                        ///< Mutex for thread safety
    uint32_t nextSubscriptionId_ = 1;         ///< Next subscription ID
};

} // namespace CORE