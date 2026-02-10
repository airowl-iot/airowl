#include "service/mcp_server.h"

#include <Arduino.h>
#include <mcpesp.h>
#include <WebServer.h>
#include "manager/sensor_manager.h"
#include "manager/config_manager.h"
#include "manager/ui_manager.h"

namespace SVC {

static Mcpesp mcp;
static WebServer aliasServer(8080); 

static bool initialized = false;
static bool running = false;

static void registerTools()
{
    Serial.println("[MCP] Registering tools...");

    static Schema empty;

    // Schema for change_screen tool
    static Schema screenSchema;
    screenSchema.addStringProperty("screen", "Screen name to display (dashboard, pm25graph, pm10graph, tempgraph, humdgraph, tvocgraph, co2graph, owl)", true);

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

    mcp.addTool(
        "get_tvoc",
        "Get tvoc",
        empty,
        [](JsonObject, JsonObject out) {
            out["value"] = APP::SensorManager::getTVOC();
        }
    );

    mcp.addTool(
        "get_eco2",
        "Get eCO2",
        empty,
        [](JsonObject, JsonObject out) {
            out["value"] = APP::SensorManager::getCO2();
        }
    );
    
    mcp.addTool(
        "get_pm25_history",
        "Get PM2.5 historical data (2-hour history, 2-minute intervals)",
        empty,
        [](JsonObject, JsonObject out) {
            float history[60];
            int count = APP::SensorManager::getPM25History(history, 60);

            JsonArray values = out.createNestedArray("values");
            for (int i = 0; i < count; i++) {
                values.add(history[i]);
            }
            out["count"] = count;
            out["interval_minutes"] = 2;
        }
    );

    mcp.addTool(
        "get_pm10_history",
        "Get PM10 historical data (2-hour history, 2-minute intervals)",
        empty,
        [](JsonObject, JsonObject out) {
            float history[60];
            int count = APP::SensorManager::getPM10History(history, 60);

            JsonArray values = out.createNestedArray("values");
            for (int i = 0; i < count; i++) {
                values.add(history[i]);
            }
            out["count"] = count;
            out["interval_minutes"] = 2;
        }
    );

    mcp.addTool(
        "change_screen",
        "Change the display screen on the device. Available screens: dashboard, pm25graph, pm10graph, tempgraph, humdgraph, tvocgraph, co2graph, owl",
        screenSchema,
        [](JsonObject args, JsonObject out) {
            // Debug: Print received arguments
            String argsStr;
            serializeJson(args, argsStr);
            Serial.print("[MCP] change_screen called with args: ");
            Serial.println(argsStr);

            String screenName = args["screen"] | "";

            Serial.print("[MCP] Extracted screen name: '");
            Serial.print(screenName);
            Serial.println("'");

            if (screenName.length() == 0) {
                out["success"] = false;
                out["error"] = "screen parameter is required";
                out["received_args"] = argsStr;
                return;
            }

            // Call UIController to switch screen
            bool success = APP::UIController::switchScreen(screenName.c_str());

            if (success) {
                out["success"] = true;
                out["screen"] = screenName;
                out["message"] = "Screen changed successfully";
            } else {
                out["success"] = false;
                out["error"] = "Invalid screen name or screen change failed";
                out["available_screens"] = "dashboard, pm25graph, pm10graph, tempgraph, humdgraph, tvocgraph, co2graph, owl";
            }
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

    aliasServer.on("/mcp", HTTP_ANY, []() {
        mcp.handleClient();   
    });

    aliasServer.begin();

    running = true;
    Serial.println("[MCP] MCP server is LIVE on port 8080");
    return true;
}

static void mcpTask(void* parameter)
{
    Serial.println("[MCP] Task started");
    
    while (true) {
        if (running) {
             mcp.handleClient();
            aliasServer.handleClient();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); 
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
    aliasServer.handleClient();  
}

} // namespace SVC
