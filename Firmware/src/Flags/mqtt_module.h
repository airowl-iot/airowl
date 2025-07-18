#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

extern WiFiClient espClient;
extern PubSubClient mqttClient;

extern const char* mqtt_server;
extern const char* mqtt_username;
extern const char* mqtt_password;

void mqtt_setup();
bool mqtt_reconnect(const char* clientId);
bool mqtt_publish(const char* topic, const char* payload);
bool mqtt_connected();