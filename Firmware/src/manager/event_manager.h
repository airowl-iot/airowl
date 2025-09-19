// event_bus.h - Unified Event Bus System
#pragma once

#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <Arduino.h>

namespace CORE {

    class Event {
    public:
        enum class Type {
            NONE,
            WIFI_STATE_CHANGED,
            COMMAND_RECEIVED,
            OTA_PROGRESS,
            OTA_COMPLETE,
            SENSOR_READING,
            MODE_CHANGED,
            MQTT_STATE_CHANGED
        };

        explicit Event(Type type) : type_(type), timestamp_(millis()) {}
        virtual ~Event() = default;

        Type getType() const { return type_; }
        uint64_t getTimestamp() const { return timestamp_; }

    private:
        Type type_;
        uint64_t timestamp_;
    };

    class WiFiStateChangedEvent : public Event {
    public:
        enum class WiFiState { DISCONNECTED, CONNECTING, CONNECTED, FAILED, PROVISIONING };

        WiFiStateChangedEvent(WiFiState state, const String& ssid = "", int32_t rssi = 0)
            : Event(Type::WIFI_STATE_CHANGED), state_(state), ssid_(ssid), rssi_(rssi) {}

        WiFiState getState() const { return state_; }
        const String& getSSID() const { return ssid_; }
        int32_t getRSSI() const { return rssi_; }

    private:
        WiFiState state_;
        String ssid_;
        int32_t rssi_;
    };

    class CommandReceivedEvent : public Event {
    public:
        CommandReceivedEvent(const String& cmd, const String& payload = "")
            : Event(Type::COMMAND_RECEIVED), command_(cmd), payload_(payload) {}

        const String& getCommand() const { return command_; }
        const String& getPayload() const { return payload_; }

    private:
        String command_;
        String payload_;
    };

    class OTAProgressEvent : public Event {
    public:
        OTAProgressEvent(size_t written, size_t total)
            : Event(Type::OTA_PROGRESS), written_(written), total_(total) {}

        size_t getWritten() const { return written_; }
        size_t getTotal() const { return total_; }

    private:
        size_t written_;
        size_t total_;
    };

    class OTACompleteEvent : public Event {
    public:
        explicit OTACompleteEvent(bool success)
            : Event(Type::OTA_COMPLETE), success_(success) {}

        bool isSuccess() const { return success_; }

    private:
        bool success_;
    };

    class MQTTStateChangedEvent : public Event {
    public:
        enum class MQTTState { DISCONNECTED, CONNECTING, CONNECTED, FAILED, RECONNECTING };

        MQTTStateChangedEvent(MQTTState state, const String& broker = "", uint16_t port = 0)
            : Event(Type::MQTT_STATE_CHANGED), state_(state), broker_(broker), port_(port) {}

        MQTTState getState() const { return state_; }
        const String& getBroker() const { return broker_; }
        uint16_t getPort() const { return port_; }

    private:
        MQTTState state_;
        String broker_;
        uint16_t port_;
    };

    class SensorReadingEvent : public Event {
    public:
        enum class SensorType {
            PM700,
            AHT,
            OTHER
        };

        SensorReadingEvent(SensorType sensor, uint32_t id, const float* values, size_t count)
            : Event(Event::Type::SENSOR_READING), sensor_(sensor), sensorId_(id) 
        {
            readings_.assign(values, values + count);
        }

        SensorType getSensor() const { return sensor_; }
        uint32_t getSensorId() const { return sensorId_; }
        const std::vector<float>& getReadings() const { return readings_; }

    private:
        SensorType sensor_;
        uint32_t sensorId_;
        std::vector<float> readings_;
    };
    
    using EventCallback = std::function<void(const std::shared_ptr<const Event>&)>;

    class EventBus {
    public:
        static EventBus& getInstance();

        uint32_t subscribe(Event::Type type, EventCallback callback);
        bool unsubscribe(uint32_t subscriptionId);
        void publish(const std::shared_ptr<const Event>& event);
        void clear();

    private:
        EventBus() = default;

        struct Subscription {
            uint32_t id;
            Event::Type type;
            EventCallback callback;
        };

        uint32_t nextSubscriptionId_ = 1;
        std::vector<Subscription> subscriptions_;
        std::mutex mutex_;
    };

} // namespace CORE
