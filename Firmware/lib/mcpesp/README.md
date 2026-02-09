# Mcpesp - ESP32 Model Context Protocol Library

A configurable Arduino library for implementing Model Context Protocol (MCP) servers on ESP32 devices. This library enables you to expose ESP32 hardware capabilities as MCP tools that can be called via HTTP/JSON-RPC, allowing AI models and other clients to interact with your ESP32 through a standardized protocol.

## Description

Mcpesp provides a complete implementation of the Model Context Protocol server for ESP32. It handles JSON-RPC requests, tool registration, schema validation, and CORS headers, making it easy to create MCP servers that expose ESP32 functionality to external clients.

## Features

- **MCP Protocol Support**: Basic implementation of MCP protocol
- **Tool Registration**: Easily register custom tools with callback functions
- **Schema Validation**: Built-in schema builder for defining tool input parameters
- **HTTP Server**: Integrates into the ESP32's WebServer for handling HTTP/JSON-RPC requests
- **JSON-RPC 2.0**: Compatible with JSON-RPC 2.0 specification
- **WiFi Integration**: Works with ESP32 WiFi capabilities

## Installation

### Arduino IDE

1. Download or clone this repository
2. Place the `mcpesp` folder in your Arduino `libraries` directory
3. Restart the Arduino IDE

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps = 
    bblanchon/ArduinoJson@^7.0.0
```

## Dependencies

- **ArduinoJson**: JSON parsing and generation library (version 7.x or compatible)
- **ESP32 Board Support**: Requires ESP32 board support package for Arduino IDE
- **WebServer**: Included with ESP32 core libraries

## Quick Start

```cpp
#include <WiFi.h>
#include <mcpesp.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Create MCP server instance
Mcpesp mcpServer;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  
  // Initialize MCP server
  mcpServer.begin("My MCP Server", "1.0.0");
  
  // Create tool schema
  Schema toolSchema;
  toolSchema.addBooleanProperty("enabled", "Enable or disable feature");
  
  // Register tool
  mcpServer.addTool(
    "my_tool",
    "Description of my tool",
    toolSchema,
    myToolCallback
  );
}

void loop() {
  mcpServer.handleClient();
}
```

## API Reference

### Mcpesp Class

#### Constructor
```cpp
Mcpesp()
```
Creates a new MCP server instance.

#### Methods

##### `begin(serverName, version, port)`
Initializes the MCP server.

- **Parameters**:
  - `serverName` (const char*): Name of the server (default: "ESP32 MCP Server")
  - `version` (const char*): Server version (default: "1.0.0")
  - `port` (int): HTTP server port (default: 80)
- **Returns**: void

##### `addTool(name, description, inputSchema, callback)`
Registers a new tool with the server.

- **Parameters**:
  - `name` (const String&): Tool name (must be unique)
  - `description` (const String&): Tool description
  - `inputSchema` (Schema&): Input parameter schema
  - `callback` (toolCallback): Callback function to execute when tool is called
- **Returns**: void

##### `handleClient()`
Handles incoming client requests. Call this in your `loop()` function.

- **Returns**: void

##### `setServerInfo(name, version)`
Updates server name and version.

- **Parameters**:
  - `name` (const String&): Server name
  - `version` (const String&): Server version
- **Returns**: void

##### `setProtocolVersion(version)`
Sets the MCP protocol version.

- **Parameters**:
  - `version` (const String&): Protocol version (default: "2025-03-26")
- **Returns**: void

##### `isInitialized()`
Checks if the server is initialized.

- **Returns**: bool

##### `getServer()`
Gets the underlying WebServer instance for advanced usage.

- **Returns**: WebServer*

### Schema Class

The Schema class is used to define input parameter schemas for tools.

#### Constructor
```cpp
Schema()
```
Creates a new schema instance.

#### Methods

##### `addStringProperty(name, description, required)`
Adds a string property to the schema.

- **Parameters**:
  - `name` (String): Property name
  - `description` (String): Property description
  - `required` (bool): Whether the property is required (default: true)
- **Returns**: void

##### `addStringEnumProperty(name, description, options, required)`
Adds a string enum property to the schema.

- **Parameters**:
  - `name` (String): Property name
  - `description` (String): Property description
  - `options` (String[]): Array of allowed values
  - `required` (bool): Whether the property is required (default: true)
- **Returns**: void

##### `addNumberProperty(name, description, required)`
Adds a number property to the schema.

- **Parameters**:
  - `name` (String): Property name
  - `description` (String): Property description
  - `required` (bool): Whether the property is required (default: true)
- **Returns**: void

##### `addNumberProperty(name, description, min, max, def, required)`
Adds a number property with constraints to the schema.

- **Parameters**:
  - `name` (String): Property name
  - `description` (String): Property description
  - `min` (int): Minimum value
  - `max` (int): Maximum value
  - `def` (int): Default value
  - `required` (bool): Whether the property is required (default: true)
- **Returns**: void

##### `addBooleanProperty(name, description, required)`
Adds a boolean property to the schema.

- **Parameters**:
  - `name` (String): Property name
  - `description` (String): Property description
  - `required` (bool): Whether the property is required (default: true)
- **Returns**: void

##### `getSchema()`
Returns the JSON schema document.

- **Returns**: JsonDocument

### Tool Callback Function

Tool callbacks have the following signature:

```cpp
void toolCallback(JsonObject arguments, JsonObject result)
```

- **Parameters**:
  - `arguments` (JsonObject): Input parameters from the client
  - `result` (JsonObject): Result object to populate (set `result["text"]` for the response)
- **Returns**: void

## Examples

See the `examples/McpServerExample/` directory for a complete example demonstrating:
- WiFi connection
- Server initialization
- Tool registration with different schema types
- Tool callback implementation

## MCP Protocol

This library implements the following MCP methods:
- `initialize`: Initializes the MCP connection
- `tools/list`: Lists all registered tools
- `tools/call`: Calls a registered tool

All methods follow the JSON-RPC 2.0 specification and return appropriate responses.

## License

This library is licensed under the GNU General Public License v2.0 (GPLv2). See the [LICENSE](LICENSE) file for details.

This library is based on the work from https://github.com/MrLaki5/ESP32-MCP-air-conditioner

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Support

For any bug reports or feature requests, please open an issue on the repository.