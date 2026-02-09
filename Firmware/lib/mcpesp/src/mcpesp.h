#ifndef MCPESP_H
#define MCPESP_H

#include <Arduino.h>
#include <WebServer.h>
#include "ArduinoJson.h"
#include "schema.h"

typedef void (*toolCallback)(JsonObject arguments, JsonObject result);
struct McpTool;
struct McpTool {
  String name;
  String description;
  JsonDocument inputSchema;
  //JsonDocument outputSchema;
  toolCallback callback;
  McpTool* next;
};

class Mcpesp {
public:
  // Constructor
  Mcpesp();
  
  // Destructor
  ~Mcpesp();
  
  // Initialize the MCP server (assumes WiFi is already connected)
  void begin(const char* serverName = "ESP32 MCP Server", const char* version = "1.0.0", int port = 80);
  
  // Add a tool to the server
  void addTool(const String& name, const String& description, Schema& inputSchema, toolCallback callback);
  
  // Handle client requests (call this in loop())
  void handleClient();
  
  // Get the server instance for advanced usage
  WebServer* getServer() { return server; }
  
  // Set server info
  void setServerInfo(const String& name, const String& version);
  
  // Set protocol version
  void setProtocolVersion(const String& version);
  
  // Check if server is initialized
  bool isInitialized() const { return initialized; }
  
  

private:
  WebServer* server;
  McpTool* toolList;
  String serverName;
  String serverVersion;
  String protocolVersion;
  bool initialized;
  
  // Internal methods
  void handleRoot();
  void handleInitialize(JsonDocument& req, JsonDocument& res);
  void handleToolsList(JsonDocument& req, JsonDocument& res);
  void handleToolsCall(JsonDocument& req, JsonDocument& res);
  void sendCorsHeaders();
  McpTool* findTool(const String& name);
};

#endif