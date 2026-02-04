#include "service/mcp_server.h"

#include <Arduino.h>
#include <mcpesp.h>

#include "manager/sensor_manager.h"
#include "manager/config_manager.h"

namespace SVC {

static Mcpesp mcp;
static bool initialized = false;
static bool running = false;

static void registerTools()
{
    Serial.println("[MCP] Registering tools...");

    static Schema empty;

    mcp.addTool(
        "get_pm25",
        "Get PM2.5 concentration",
        empty,
        [](JsonObject, JsonObject out) {
            out["value"] = APP::SensorManager::getPM25();
        }
    );

    mcp.addTool(
        "get_pm10",
        "Get PM10 concentration",
        empty,
        [](JsonObject, JsonObject out) {
            out["value"] = APP::SensorManager::getPM10();
        }
    );

    mcp.addTool(
        "get_temp",
        "Get temperature",
        empty,
        [](JsonObject, JsonObject out) {
            out["value"] = APP::SensorManager::getTemperature();
        }
    );

    mcp.addTool(
        "get_humd",
        "Get humidity",
        empty,
        [](JsonObject, JsonObject out) {
            out["value"] = APP::SensorManager::getHumidity();
        }
    );

    Serial.println("[MCP] Tools registered");
}

bool MCPServer::init()
{
    if (initialized) return true;
    initialized = true;
    Serial.println("[MCP] MCP initialized");
    return true;
}

bool MCPServer::start()
{
    if (!initialized || running) return true;

    Serial.println("[MCP] Binding MCP HTTP server on port 8080");

    mcp.begin("Airowl", "1.0.0", 8080);
    registerTools();

    running = true;
    Serial.println("[MCP] MCP server is LIVE on port 8080");
    return true;
}

static void mcpTask(void* parameter)
{
    Serial.println("[MCP] Task started");
    
    while (true) {
        if (running) {
            MCPServer::loop();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent watchdog issues
    }
}

bool MCPServer::startTask()
{
    if (!running) {
        Serial.println("[MCP] Cannot start task - server not running");
        return false;
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        mcpTask,
        "MCP_Task",
        8192,           // Stack size
        nullptr,
        1,              // Priority
        nullptr,
        1               // Core 1
    );

    if (result != pdPASS) {
        Serial.println("[MCP] Failed to create MCP task");
        return false;
    }

    Serial.println("[MCP] Task created successfully");
    return true;
}

void MCPServer::loop()
{
    mcp.handleClient();
}

} // namespace SVC
