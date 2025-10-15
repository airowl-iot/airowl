#include "service_manager.h"
#include "hal/hal_wifi.h"
#include "service/wifi_service.h"
#include "service/mqtt_service.h"
#include "service/ota_service.h"
#include <WiFi.h>

namespace APP {

ServiceManager& ServiceManager::getInstance() {
    static ServiceManager instance;
    return instance;
}

bool ServiceManager::init() {
    if (initialized) return true;

    if (!ConfigManager::getInstance()->isInitialized()) {
        Serial.println("[SERVICE] ConfigManager not initialized");
        return false;
    }

    if (!loadConfigFromManager()) {
        Serial.println("[SERVICE] Failed to load service configurations");
        return false;
    }

    CORE::EventBus& eventBus = CORE::EventBus::getInstance();
    wifiEventSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::WIFI_STATE_CHANGED,
        [this](const std::shared_ptr<const CORE::Event>& event) {
            auto wifiEvent = std::static_pointer_cast<const CORE::WiFiStateChangedEvent>(event);
            if (wifiEvent->getState() == CORE::WiFiStateChangedEvent::WiFiState::CONNECTED) {
                Serial.println("[SERVICE] WiFi connected, checking for dependent services...");
                onWiFiConnected();
            }
        }
    );
    Serial.printf("[SERVICE] Subscribed to WiFi events (ID: %u)\n", wifiEventSubscriptionId);

    if (isServiceEnabled(ServiceType::MQTT)) {
        Serial.println("[SERVICE] MQTT service enabled, will initialize after WiFi connection");
        if (!SVC::MQTTService::init("")) {
            Serial.println("[SERVICE] Failed to initialize MQTT service structure");
        }
    }

    if (isServiceEnabled(ServiceType::OTA)) {
        
        Serial.println("[SERVICE] OTA service enabled (HAL not yet implemented)");
    }

    initialized = true;
    Serial.println("[SERVICE] ServiceManager initialized");
    return true;
}

bool ServiceManager::loadConfigFromManager() {
    services.clear();
    auto& config = ConfigManager::getInstance()->getConfig();

    for (const auto& [name, svcConfig] : config.services) {
        ServiceType type = stringToServiceType(name);
        if (type != ServiceType::UNKNOWN) {
            ServiceConfig sc;
            sc.enabled = svcConfig.enabled;
            sc.params  = svcConfig.params;
            services[type] = sc;
            
            Serial.printf("[SERVICE] Loaded config for %s: enabled=%d\n",
                          name.c_str(), svcConfig.enabled);
        }
    }
    return true;
}

bool ServiceManager::start() {
    if (!initialized) {
        Serial.println("[SERVICE] ServiceManager not initialized");
        return false;
    }

    bool allStarted = true;
    for (const auto& [type, config] : services) {
        if (config.enabled) {
            if (type == ServiceType::MQTT) {
                Serial.println("[SERVICE] MQTT service will start after WiFi connection");
                continue;
            }
            if (type == ServiceType::OTA) {
                Serial.println("[SERVICE] OTA service will start after WiFi connection");
                continue;
            }

            Serial.printf("[SERVICE] Starting %s service...\n",
                          serviceTypeToString(type).c_str());

            if (!startService(type)) {
                Serial.printf("[SERVICE] Failed to start %s\n",
                              serviceTypeToString(type).c_str());
                allStarted = false;
            } else {
                Serial.printf("[SERVICE] %s service started successfully\n",
                              serviceTypeToString(type).c_str());
            }
        } else {
            Serial.printf("[SERVICE] %s service is disabled\n",
                          serviceTypeToString(type).c_str());
        }
    }

    running = allStarted;
    return allStarted;
}

void ServiceManager::stop() {
    if (!running) return;

    if (wifiEventSubscriptionId != 0) {
        CORE::EventBus& eventBus = CORE::EventBus::getInstance();
        eventBus.unsubscribe(wifiEventSubscriptionId);
        Serial.printf("[SERVICE] Unsubscribed from WiFi events (ID: %u)\n", wifiEventSubscriptionId);
        wifiEventSubscriptionId = 0;
    }

    for (const auto& [type, config] : services) {
        if (config.enabled) stopService(type);
    }
    running = false;
}

bool ServiceManager::isServiceEnabled(ServiceType type) const {
    auto it = services.find(type);
    return it != services.end() && it->second.enabled;
}

const ServiceManager::ServiceConfig* ServiceManager::getServiceConfig(ServiceType type) const {
    auto it = services.find(type);
    return it != services.end() ? &it->second : nullptr;
}

ServiceManager::ServiceType ServiceManager::stringToServiceType(const String& type) {
    if (type == "wifi") return ServiceType::WIFI;
    if (type == "mqtt") return ServiceType::MQTT;
    if (type == "ota") return ServiceType::OTA;
    return ServiceType::UNKNOWN;
}

String ServiceManager::serviceTypeToString(ServiceType type) {
    switch (type) {
        case ServiceType::WIFI: return "wifi";
        case ServiceType::MQTT: return "mqtt";
        case ServiceType::OTA: return "ota";
        default: return "unknown";
    }
}

bool ServiceManager::startService(ServiceType type) {
    if (!isServiceEnabled(type)) {
        Serial.printf("[SERVICE] %s is not enabled in configuration\n",
                      serviceTypeToString(type).c_str());
        return false;
    }

    switch (type) {
        case ServiceType::WIFI: {
            auto* cfg = getServiceConfig(ServiceType::WIFI);
            if (!cfg) {
                Serial.println("[SERVICE] WiFi config not found");
                return false;
            }

            String hostname = "airowl";
            if (cfg->params.count("hostname")) {
                hostname = cfg->params.at("hostname");
            }

            if (!SVC::WiFiService::init(hostname.c_str())) {
                Serial.println("[SERVICE] WiFi service init failed");
                return false;
            }


            vTaskDelay(pdMS_TO_TICKS(100));

            if (!SVC::WiFiService::startTask()) {
                Serial.println("[SERVICE] WiFi task start failed");
                return false;
            }

            vTaskDelay(pdMS_TO_TICKS(100));

            if (cfg->params.count("ssid") && cfg->params.count("password")) {
                String ssid = cfg->params.at("ssid");
                String pass = cfg->params.at("password");

                if (ssid.length() > 0) {
                    Serial.printf("[SERVICE] Connecting to WiFi from config.json: %s\n", ssid.c_str());
                    if (SVC::WiFiService::connect(ssid.c_str(), pass.c_str())) {
                        return true;
                    }
                    Serial.println("[SERVICE] Failed to connect with config.json credentials");
                }
            }
            Serial.println("[SERVICE] Attempting auto-connect with saved credentials or starting portal...");
            return SVC::WiFiService::connect();
        }

        case ServiceType::MQTT: {
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[SERVICE] MQTT service requires WiFi connection, deferring initialization");
                return false;
            }

            auto* cfg = getServiceConfig(ServiceType::MQTT);
            if (!cfg) {
                Serial.println("[SERVICE] MQTT config not found");
                return false;
            }

            String broker = "";  
            uint16_t port = 1883; 
            String username = "";
            String password = "";
            String clientId = "airowl_" + String(ESP.getEfuseMac(), HEX);

            if (cfg->params.count("server")) {
                broker = cfg->params.at("server");
            }
            if (cfg->params.count("port")) {
                port = cfg->params.at("port").toInt();
            }
            if (cfg->params.count("username")) {
                username = cfg->params.at("username");
            }
            if (cfg->params.count("password")) {
                password = cfg->params.at("password");
            }
            if (cfg->params.count("client_id")) {
                clientId = cfg->params.at("client_id");
            }

            if (broker.length() == 0) {
                Serial.println("[SERVICE] ERROR: MQTT server not configured in config.json");
                return false;
            }

            Serial.printf("[SERVICE] Initializing MQTT service with broker: %s:%d\n",
                          broker.c_str(), port);
            if (username.length() > 0) {
                Serial.printf("[SERVICE] Using authentication with username: %s\n", username.c_str());
            }

            if (!SVC::MQTTService::init(clientId.c_str())) {
                Serial.println("[SERVICE] Failed to initialize MQTT service");
                return false;
            }

            if (!SVC::MQTTService::startTask()) {
                Serial.println("[SERVICE] Failed to start MQTT task");
                return false;
            }

            bool connected = SVC::MQTTService::connect(
                broker.c_str(),
                port,
                username.length() > 0 ? username.c_str() : nullptr,
                password.length() > 0 ? password.c_str() : nullptr
            );

            if (!connected) {
                Serial.println("[SERVICE] MQTT initial connection failed, will retry in background");
            }

            return true;
        }

        case ServiceType::OTA: {
            static bool otaServiceStarted = false;
            if (otaServiceStarted) {
                Serial.println("[SERVICE] OTA service already started");
                return true;
            }

            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[SERVICE] OTA service requires WiFi connection, deferring initialization");
                return false;
            }

            auto* cfg = getServiceConfig(ServiceType::OTA);
            if (!cfg) {
                Serial.println("[SERVICE] OTA config not found");
                return false;
            }

            Serial.println("[SERVICE] Initializing OTA service...");

            String versionURL = "";
            String firmwareURL = "";

            if (cfg->params.count("versionURL")) {
                versionURL = cfg->params.at("versionURL");
            }
            if (cfg->params.count("firmwareURL")) {
                firmwareURL = cfg->params.at("firmwareURL");
            }

            if (!versionURL.isEmpty() || !firmwareURL.isEmpty()) {
                SVC::OTA::setURLs(versionURL.c_str(), firmwareURL.c_str());
                Serial.printf("[SERVICE] OTA URLs set - version: %s, firmware: %s\n",
                              versionURL.c_str(), firmwareURL.c_str());
            }

            if (!SVC::OTA::init()) {
                Serial.println("[SERVICE] Failed to initialize OTA service");
                return false;
            }

            if (!SVC::OTA::startTask()) {
                Serial.println("[SERVICE] Failed to start OTA task");
                return false;
            }

            Serial.println("[SERVICE] OTA will check for updates on every WiFi connection");

            otaServiceStarted = true;
            Serial.println("[SERVICE] OTA service started successfully");
            return true;
        }

        default:
            Serial.printf("[SERVICE] Unknown service type: %d\n", (int)type);
            return false;
    }
}

bool ServiceManager::stopService(ServiceType type) {
    if (!isServiceEnabled(type)) {
        Serial.printf("[SERVICE] %s is not enabled, cannot stop\n",
                      serviceTypeToString(type).c_str());
        return false;
    }

    switch (type) {
        case ServiceType::WIFI: {
            Serial.println("[SERVICE] Stopping WiFi service...");
            bool result = SVC::WiFiService::disconnect();
            if (result) {
                Serial.println("[SERVICE] WiFi service stopped");
            } else {
                Serial.println("[SERVICE] Failed to stop WiFi service");
            }
            return result;
        }

        case ServiceType::MQTT: {
            Serial.println("[SERVICE] MQTT stop not yet implemented");
            return false;
        }

        case ServiceType::OTA: {
            Serial.println("[SERVICE] OTA stop not yet implemented");
            return false;
        }

        default:
            Serial.printf("[SERVICE] Unknown service type: %d\n", (int)type);
            return false;
    }
}

void ServiceManager::updateFromConfigManager() {
    Serial.println("[SERVICE] Updating configuration from ConfigManager");

    std::map<ServiceType, bool> previousStates;
    for (const auto& [type, config] : services) {
        previousStates[type] = config.enabled;
    }
    if (!loadConfigFromManager()) {
        Serial.println("[SERVICE] Failed to reload configuration");
        return;
    }

    for (const auto& [type, config] : services) {
        bool wasEnabled = previousStates[type];
        bool isEnabled = config.enabled;

        if (wasEnabled != isEnabled) {
            Serial.printf("[SERVICE] %s state changed: %s -> %s\n",
                          serviceTypeToString(type).c_str(),
                          wasEnabled ? "enabled" : "disabled",
                          isEnabled ? "enabled" : "disabled");

            if (running) {
                if (isEnabled) {
                    startService(type);
                } else {
                    stopService(type);
                }
            }
        }
    }
}

void ServiceManager::onWiFiConnected() {
    Serial.println("[SERVICE] WiFi connected, checking for dependent services...");

    if (isServiceEnabled(ServiceType::OTA)) {
        static bool otaServiceInitialized = false;

        if (!otaServiceInitialized) {
            Serial.println("[SERVICE] Starting OTA service after WiFi connection...");
            if (startService(ServiceType::OTA)) {
                otaServiceInitialized = true;
                Serial.println("[SERVICE] OTA service started successfully");
            } else {
                Serial.println("[SERVICE] Failed to start OTA service");
                return;  
            }
        }

        Serial.println("[SERVICE] Checking for OTA updates...");
        SVC::OTA::onWiFiConnected();
        Serial.println("[SERVICE] OTA update check completed");
    }

    if (isServiceEnabled(ServiceType::MQTT)) {
        Serial.println("[SERVICE] MQTT service is enabled, attempting to start...");
        if (startService(ServiceType::MQTT)) {
            Serial.println("[SERVICE] MQTT service started successfully after WiFi connection");
        } else {
            Serial.println("[SERVICE] Failed to start MQTT service after WiFi connection");
        }
    }
}

} // namespace APP
