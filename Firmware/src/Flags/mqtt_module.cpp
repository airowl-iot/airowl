#include "mqtt_module.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// mqtt credentials
const char *mqtt_server = "mqtt.oizom.com";
const char *mqtt_username = "oizom";
const char *mqtt_password = "12345678";

void mqtt_callback(char* topic, byte* message, unsigned int length) {
    // MQTT message handler
}

void mqtt_setup() {
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(mqtt_callback);
}

bool mqtt_reconnect(const char* clientId) {
    if (!mqttClient.connected()) {
        String clientId = "AIROWL";
        clientId += String(random(0xffffff), HEX);
        if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
            Serial.print("MQTT Connected");
        }
    }
    return mqttClient.connected();
}

bool mqtt_publish(const char* topic, const char* payload) {
    if (mqttClient.connected()) {
        return mqttClient.publish(topic, payload);
    }
    return false;
}

bool mqtt_connected() {
    return mqttClient.connected();
}