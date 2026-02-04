#pragma once

#include <Arduino.h>
#include "config_manager.h"
#include "event_manager.h"
#include "service/wifi_service.h"
#include <map>
#include <memory>

namespace APP {

class ServiceManager {
public:
    enum class ServiceType {
        WIFI,
        MQTT,
        MCP,
        OTA,
        MATTER,
        UNKNOWN
    };

    struct ServiceConfig {
        bool enabled{false};
        std::map<String, String> params;
    };

    static ServiceManager& getInstance();

    bool init();
    bool start();
    void stop();
    bool isRunning() const { return running; }

    // Service control
    bool isServiceEnabled(ServiceType type) const;
    const ServiceConfig* getServiceConfig(ServiceType type) const;
    bool startService(ServiceType type);
    bool stopService(ServiceType type);

    // Config updates
    void updateFromConfigManager();

    void onWiFiConnected();

private:
    ServiceManager() : initialized(false), running(false) {}
    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;

    bool loadConfigFromManager();
    ServiceType stringToServiceType(const String& type);
    String serviceTypeToString(ServiceType type);

    bool initialized;
    bool running;
    std::map<ServiceType, ServiceConfig> services;
    uint32_t wifiEventSubscriptionId = 0; 
};

} // namespace APP