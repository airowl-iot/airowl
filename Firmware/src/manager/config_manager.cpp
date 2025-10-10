#include "config_manager.h"

ConfigManager* ConfigManager::instance = nullptr;

ConfigManager::ConfigManager() : initialized(false) {}

ConfigManager* ConfigManager::getInstance() {
    if (instance == nullptr) {
        try {
            instance = new ConfigManager();
        } catch (const std::bad_alloc& e) {
            Serial.println("[CONFIG] CRITICAL: Failed to allocate memory for ConfigManager!");
            Serial.printf("[CONFIG] Exception: %s\n", e.what());
            Serial.println("[CONFIG] System will restart in 3 seconds...");
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_restart();
        }
    }
    return instance;
}

bool ConfigManager::initialize() {
    if (initialized) return true;

    if (!SPIFFS.begin(true)) {
        Serial.println("[CONFIG] Failed to initialize SPIFFS");
        return false;
    }

    if (!loadConfigFromFile()) {
        Serial.println("[CONFIG] Failed to load config");
        return false;
    }

    initialized = true;
    Serial.println("[CONFIG] ConfigManager initialized successfully");
    return true;
}

bool ConfigManager::loadConfigFromFile() {
    if (!SPIFFS.exists(CONFIG_FILE_PATH)) {
        Serial.println("[CONFIG] Configuration file not found: " + String(CONFIG_FILE_PATH));
        return false;
    }

    File configFile = SPIFFS.open(CONFIG_FILE_PATH, "r");
    if (!configFile) {
        Serial.println("[CONFIG] Failed to open configuration file");
        return false;
    }

    String jsonString = configFile.readString();
    configFile.close();

    return parseJsonConfig(jsonString);
}

bool ConfigManager::parseJsonConfig(const String& jsonString) {
    DynamicJsonDocument* doc = new DynamicJsonDocument(4096);
    if (!doc) {
        Serial.println("[CONFIG] Failed to allocate memory for JSON document");
        return false;
    }

    DeserializationError error = deserializeJson(*doc, jsonString);
    if (error) {
        Serial.println("[CONFIG] Failed to parse JSON: " + String(error.c_str()));
        delete doc;
        return false;
    }

    config.device_profile = (*doc)["device_profile"].as<String>();

    if (doc->containsKey("hardware")) {
        JsonObject hardware = (*doc)["hardware"];
        parseHardwareConfig(hardware);
    }

    if (doc->containsKey("sensors")) {
        JsonArray sensors = (*doc)["sensors"];
        parseSensorsConfig(sensors);
    }

    if (doc->containsKey("services")) {
        JsonObject services = (*doc)["services"];
        parseServicesConfig(services);
    }

    delete doc; 
    return true;
}

// ---------------- Hardware ----------------
void ConfigManager::parseHardwareConfig(JsonObject& hardware) {
    config.hardware.firmwareVersion   = hardware["firmwareVersion"] | "";
    config.hardware.display           = hardware["display"] | "";
    config.hardware.touch_controller  = hardware["touch_controller"] | "";

    config.hardware.additional_params.clear();
    for (JsonPair kv : hardware) {
        String key = kv.key().c_str();
        if (key != "firmwareVersion" && key != "display" && key != "touch_controller") {
            config.hardware.additional_params[key] = kv.value().as<String>();
        }
    }
}

// ---------------- Sensors ----------------
void ConfigManager::parseSensorsConfig(JsonArray& sensors) {
    config.sensors.clear();

    for (JsonObject sensorObj : sensors) {
        SensorConfig sensor;
        sensor.type            = sensorObj["type"] | "";
        sensor.enabled         = sensorObj["enabled"] | false;
        sensor.read_interval   = sensorObj["read_interval"] | 5000;
        sensor.publish_interval= sensorObj["publish_interval"] | 60000;

        for (JsonPair kv : sensorObj) {
            String key = kv.key().c_str();
            if (key != "type" && key != "enabled" && key != "read_interval" && key != "publish_interval") {
                sensor.additional_params[key] = kv.value().as<String>();
            }
        }

        config.sensors.push_back(sensor);
    }
}

// ---------------- Services ----------------
void ConfigManager::parseServicesConfig(JsonObject& services) {
    config.services.clear();

    for (JsonPair service : services) {
        ServiceConfig sc;
        JsonObject serviceObj = service.value().as<JsonObject>();

        sc.enabled = serviceObj["enabled"] | false;

        for (JsonPair kv : serviceObj) {
            String key = kv.key().c_str();
            if (key != "enabled") {
                sc.params[key] = kv.value().as<String>();
            }
        }

        config.services[service.key().c_str()] = sc;
    }
}

// ---------------- Getters ----------------
String ConfigManager::getDeviceProfile() const { return config.device_profile; }
HardwareConfig ConfigManager::getHardwareConfig() const { return config.hardware; }
String ConfigManager::getFirmwareVersion() const { return config.hardware.firmwareVersion; }
String ConfigManager::getDisplayType() const { return config.hardware.display; }
String ConfigManager::getTouchController() const { return config.hardware.touch_controller; }
std::map<String, String> ConfigManager::getHardwareParams() const { return config.hardware.additional_params; }
String ConfigManager::getHardwareParam(const String& key) const {
    auto it = config.hardware.additional_params.find(key);
    return (it != config.hardware.additional_params.end()) ? it->second : "";
}

// Sensors
std::vector<SensorConfig> ConfigManager::getAllSensors() const { return config.sensors; }
std::vector<SensorConfig> ConfigManager::getEnabledSensors() const {
    std::vector<SensorConfig> enabled;
    for (auto& s : config.sensors) if (s.enabled) enabled.push_back(s);
    return enabled;
}
bool ConfigManager::isSensorEnabled(const String& sensorType) const {
    for (auto& s : config.sensors) if (s.type == sensorType) return s.enabled;
    return false;
}
SensorConfig ConfigManager::getSensorConfig(const String& sensorType) const {
    for (auto& s : config.sensors) if (s.type == sensorType) return s;
    return SensorConfig{}; 
}
unsigned long ConfigManager::getSensorReadInterval(const String& type) const {
    return getSensorConfig(type).read_interval;
}
unsigned long ConfigManager::getSensorPublishInterval(const String& type) const {
    return getSensorConfig(type).publish_interval;
}
std::map<String, String> ConfigManager::getSensorAdditionalParams(const String& type) const {
    return getSensorConfig(type).additional_params;
}
String ConfigManager::getSensorParam(const String& type, const String& key) const {
    auto params = getSensorAdditionalParams(type);
    auto it = params.find(key);
    return (it != params.end()) ? it->second : "";
}

// Services
std::map<String, ServiceConfig> ConfigManager::getAllServices() const { return config.services; }
std::vector<String> ConfigManager::getEnabledServices() const {
    std::vector<String> enabled;
    for (auto& s : config.services) if (s.second.enabled) enabled.push_back(s.first);
    return enabled;
}
bool ConfigManager::isServiceEnabled(const String& name) const {
    auto it = config.services.find(name);
    return (it != config.services.end()) ? it->second.enabled : false;
}
ServiceConfig ConfigManager::getServiceConfig(const String& name) const {
    auto it = config.services.find(name);
    return (it != config.services.end()) ? it->second : ServiceConfig{};
}
std::map<String, String> ConfigManager::getServiceParams(const String& name) const {
    return getServiceConfig(name).params;
}
String ConfigManager::getServiceParam(const String& name, const String& key) const {
    auto params = getServiceParams(name);
    auto it = params.find(key);
    return (it != params.end()) ? it->second : "";
}

// ---------------- Updates ----------------
bool ConfigManager::updateSensorEnabled(const String& type, bool enabled) {
    for (auto& s : config.sensors) if (s.type == type) { s.enabled = enabled; return true; }
    return false;
}
bool ConfigManager::updateServiceEnabled(const String& name, bool enabled) {
    auto it = config.services.find(name);
    if (it != config.services.end()) { it->second.enabled = enabled; return true; }
    return false;
}

// ---------------- Save ----------------
bool ConfigManager::saveConfigToFile() {
    // Use heap allocation to avoid stack overflow (4KB is too large for stack)
    DynamicJsonDocument* doc = new DynamicJsonDocument(4096);
    if (!doc) {
        Serial.println("[CONFIG] Failed to allocate memory for JSON document");
        return false;
    }

    (*doc)["device_profile"] = config.device_profile;

    JsonObject hardware = doc->createNestedObject("hardware");
    hardware["firmwareVersion"] = config.hardware.firmwareVersion;
    hardware["display"] = config.hardware.display;
    hardware["touch_controller"] = config.hardware.touch_controller;
    for (auto& kv : config.hardware.additional_params) hardware[kv.first] = kv.second;

    JsonArray sensors = doc->createNestedArray("sensors");
    for (auto& s : config.sensors) {
        JsonObject obj = sensors.createNestedObject();
        obj["type"] = s.type;
        obj["enabled"] = s.enabled;
        obj["read_interval"] = s.read_interval;
        obj["publish_interval"] = s.publish_interval;
        for (auto& kv : s.additional_params) obj[kv.first] = kv.second;
    }

    JsonObject services = doc->createNestedObject("services");
    for (auto& svc : config.services) {
        JsonObject obj = services.createNestedObject(svc.first);
        obj["enabled"] = svc.second.enabled;
        for (auto& kv : svc.second.params) obj[kv.first] = kv.second;
    }

    File configFile = SPIFFS.open(CONFIG_FILE_PATH, "w");
    if (!configFile) {
        Serial.println("[CONFIG] Failed to open config file for writing");
        delete doc;
        return false;
    }

    size_t bytes = serializeJsonPretty(*doc, configFile);
    configFile.close();

    delete doc;

    Serial.printf("[CONFIG] Configuration saved successfully (%d bytes)\n", (int)bytes);
    return bytes > 0;
}

// ---------------- Utility ----------------
void ConfigManager::printConfig() const {
    Serial.println("\n[CONFIG] Device Configuration:");
    Serial.println("Device Profile: " + config.device_profile);
    Serial.println("Firmware Version: " + config.hardware.firmwareVersion);
    Serial.println("Display: " + config.hardware.display);
    Serial.println("Touch Controller: " + config.hardware.touch_controller);

    Serial.println("\nEnabled Sensors:");
    for (auto& s : getEnabledSensors()) {
        Serial.printf("  - %s (Read: %lums, Publish: %lums)\n", 
                      s.type.c_str(), s.read_interval, s.publish_interval);
    }

    Serial.println("\nEnabled Services:");
    for (auto& svc : getEnabledServices()) Serial.println("  - " + svc);
    Serial.println();
}

bool ConfigManager::isInitialized() const { return initialized; }
