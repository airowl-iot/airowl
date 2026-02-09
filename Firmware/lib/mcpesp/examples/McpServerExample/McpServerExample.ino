#include <WiFi.h>
#include <mcpesp.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Create MCP server instance
Mcpesp mcpServer;

// LED pin for demonstration
#define LED_PIN 8

// Tool callback function for LED control
void toggleLed(JsonObject arguments, JsonObject result) {
  bool state = arguments["state"].as<bool>(); 
  bool ledState = state;
  
  digitalWrite(LED_PIN, ledState ? LOW : HIGH); // LOW = ON for most ESP32 boards
  
  
  
  result["text"] = "LED turned " + String(state) + " successfully!";
  Serial.println("LED state changed to: " + state);
}

// Tool callback function for getting sensor data
void getSensorData(JsonObject arguments, JsonObject result) {
  // Simulate sensor reading
  float temperature = 25.5 + random(-50, 50) / 10.0; // Random temperature around 25.5°C
  int humidity = 60 + random(-20, 20); // Random humidity around 60%
  
  
  result["text"] = "Temperature: " + String(temperature, 1) + "°C, Humidity: " + String(humidity) + "%";
  Serial.println("Sensor data requested");
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(250);
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Start with LED off
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize MCP server
  mcpServer.begin("ESP32 LED & Sensor MCP Server", "1.0.0");
  
  // Create input schema for LED control tool using helper functions
  Schema ledInSchema;
  ledInSchema.addBooleanProperty("state", "LED state");
  
  // Create input schema for sensor data tool using helper functions
  Schema sensorInSchema;
  sensorInSchema.addStringProperty("sensor", "Sensor type to read");

  
  // Add tools to the server
  mcpServer.addTool(
    "toggle_led",
    "Toggle the LED on or off",
    ledInSchema,
    toggleLed
  );
  
  mcpServer.addTool(
    "get_sensor_data",
    "Get sensor data (temperature, humidity, or both)",
    sensorInSchema,
    getSensorData
  );
  
  Serial.println("MCP Server setup complete!");
  Serial.println("Available tools:");
  Serial.println("- toggle_led: Control the LED");
  Serial.println("- get_sensor_data: Read sensor data");
}

void loop() {
  // Handle client requests
  mcpServer.handleClient();
  
  // Add any other loop functionality here
  delay(10);
}
