#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <SPIFFS.h>
#include <FS.h>

// -------------------- Structs --------------------
struct HardwareConfig {
    String firmwareVersion;
    String display;
    String touch_controller;
    std::map<String, String> additional_params;
};

struct SensorConfig {
    String type;
    bool enabled;
    unsigned long read_interval;
    std::map<String, String> additional_params;
};

struct ServiceConfig {
    bool enabled;
    std::map<String, String> params;
};

struct DeviceConfig {
    String device_profile;
    HardwareConfig hardware;
    std::vector<SensorConfig> sensors;
    std::map<String, ServiceConfig> services;
};

// -------------------- Config Manager --------------------
class ConfigManager {
public:
    static ConfigManager* getInstance();

    bool initialize();
    bool isInitialized() const;

    // Load / Save
    bool loadConfigFromFile();
    bool saveConfigToFile();

    // Print for debug
    void printConfig() const;

    // --------- Device Profile ---------
    String getDeviceProfile() const;

    const DeviceConfig& getConfig() const { return config; }


    // --------- Hardware ---------
    HardwareConfig getHardwareConfig() const;
    String getFirmwareVersion() const;
    String getDisplayType() const;
    String getTouchController() const;
    std::map<String, String> getHardwareParams() const;
    String getHardwareParam(const String& key) const;

    // --------- Sensors ---------
    std::vector<SensorConfig> getAllSensors() const;
    std::vector<SensorConfig> getEnabledSensors() const;
    bool isSensorEnabled(const String& sensorType) const;
    SensorConfig getSensorConfig(const String& sensorType) const;
    unsigned long getSensorReadInterval(const String& sensorType) const;
    std::map<String, String> getSensorAdditionalParams(const String& sensorType) const;
    String getSensorParam(const String& sensorType, const String& key) const;

    // --------- Services ---------
    std::map<String, ServiceConfig> getAllServices() const;
    std::vector<String> getEnabledServices() const;
    bool isServiceEnabled(const String& serviceName) const;
    ServiceConfig getServiceConfig(const String& serviceName) const;
    std::map<String, String> getServiceParams(const String& serviceName) const;
    String getServiceParam(const String& serviceName, const String& key) const;

    // --------- Update methods ---------
    bool updateSensorEnabled(const String& sensorType, bool enabled);
    bool updateServiceEnabled(const String& serviceName, bool enabled);

private:
    ConfigManager();
    static ConfigManager* instance;

    bool initialized;
    DeviceConfig config;

    bool parseJsonConfig(const String& jsonString);
    void parseHardwareConfig(JsonObject& hardware);
    void parseSensorsConfig(JsonArray& sensors);
    void parseServicesConfig(JsonObject& services);
};

// Path for config file
#define CONFIG_FILE_PATH "/config.json"
