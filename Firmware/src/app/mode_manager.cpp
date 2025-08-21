// mode_manager.cpp - Mode Manager implementation for Airowl 3.0
#include "mode_manager.h"
#include <esp_task_wdt.h>
#include "sensor_manager.h"
#include "ui_controller.h"
#include "svc/wifi_service.h"
#include "svc/mqtt_service.h"
#include "svc/ota_service.h"

static void task(void* parameter);

namespace {
    // Private variables
    bool initialized = false;
    APP::ModeManager::Mode currentMode = APP::ModeManager::Mode::NORMAL;
    APP::ModeManager::ModeChangeCallback modeChangeCallback = nullptr;
    TaskHandle_t modeTaskHandle = nullptr;
    
    uint32_t wifiStateChangedSubscriptionId = 0;
    #ifdef CONFIG_ENABLE_OTA_ANEDYA
        uint32_t otaProgressSubscriptionId = 0;
    #endif
    uint32_t commandReceivedSubscriptionId = 0;

    bool provisioningRequested = false;
    bool normalModeRequested = false;
    
    #ifdef CONFIG_ENABLE_OTA_ANEDYA
        bool otaUpdateRequested = false;
    #endif

    void enterMode(APP::ModeManager::Mode newMode) {
        if (newMode == currentMode) return;
        
        APP::ModeManager::Mode previousMode = currentMode;
        currentMode = newMode;
        
        switch (newMode) {
            case APP::ModeManager::Mode::NORMAL:
                APP::SensorManager::start();
                APP::UIController::startTask();
                SVC::WiFiService::connect();
                SVC::MQTTService::connect("143.198.24.17", 1883, "oizom", "12345678");
                break;
                
            case APP::ModeManager::Mode::PROVISIONING:
                APP::SensorManager::stop();
                APP::UIController::startTask();
                SVC::WiFiService::startProvisioning();
                SVC::MQTTService::disconnect();
                break;
                
    #ifdef CONFIG_ENABLE_OTA_ANEDYA
            case APP::ModeManager::Mode::OTA_UPDATE:
                APP::SensorManager::stop();
                APP::UIController::startTask();
                SVC::WiFiService::connect();
                SVC::MQTTService::disconnect();
                break;
    #endif
            
        }
        if (modeChangeCallback) {
            modeChangeCallback(previousMode, newMode);
        }
    }
}

namespace APP {

bool ModeManager::init(Mode initialMode) {
    if (initialized) return true;
    
    // Subscribe to events
    CORE::EventBus& eventBus = CORE::EventBus::getInstance();
    
    wifiStateChangedSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::WIFI_STATE_CHANGED, 
        handleWiFiStateChangedEvent
    );
    
#ifdef CONFIG_ENABLE_OTA_ANEDYA
    otaProgressSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::OTA_PROGRESS, 
        handleOTAProgressEvent
    );
#endif
    
    commandReceivedSubscriptionId = eventBus.subscribe(
        CORE::Event::Type::COMMAND_RECEIVED, 
        handleCommandReceivedEvent
    );
    
    currentMode = initialMode;
    initialized = true;
    return true;
}

ModeManager::Mode ModeManager::getCurrentMode() {
    return currentMode;
}

bool ModeManager::changeMode(Mode newMode) {
    if (!initialized) return false;
    switch (newMode) {
        case Mode::NORMAL:
            normalModeRequested = true;
            break;
            
        case Mode::PROVISIONING:
            provisioningRequested = true;
            break;
            
    #ifdef CONFIG_ENABLE_OTA_ANEDYA
        case Mode::OTA_UPDATE:
            otaUpdateRequested = true;
            break;
    #endif     
    }
    return true;
}

void ModeManager::onModeChange(ModeChangeCallback callback) {
    modeChangeCallback = callback;
}

void ModeManager::handleWiFiStateChangedEvent(const CORE::Event& event) {
    const CORE::WiFiStateChangedEvent& wifiEvent = static_cast<const CORE::WiFiStateChangedEvent&>(event);

    switch (wifiEvent.getState()) {
        case CORE::WiFiStateChangedEvent::WiFiState::PROVISIONING:
            if (currentMode != Mode::PROVISIONING) {
                provisioningRequested = true;
            }
            break;
            
        case CORE::WiFiStateChangedEvent::WiFiState::CONNECTED:
            if (currentMode == Mode::PROVISIONING) {
                normalModeRequested = true;
            }
            break;
            
        default:
            break;
    }
}

#ifdef CONFIG_ENABLE_OTA_ANEDYA
void ModeManager::handleOTAProgressEvent(const CORE::Event& event) {
    const CORE::OTAProgressEvent& otaEvent = static_cast<const CORE::OTAProgressEvent&>(event);

    switch (otaEvent.getState()) {
        case CORE::OTAProgressEvent::OTAState::DOWNLOADING:
        case CORE::OTAProgressEvent::OTAState::VERIFYING:
        case CORE::OTAProgressEvent::OTAState::UPDATING:
            if (currentMode != Mode::OTA_UPDATE) {
                otaUpdateRequested = true;
            }
            break;
            
        case CORE::OTAProgressEvent::OTAState::COMPLETE:
        case CORE::OTAProgressEvent::OTAState::FAILED:
            if (currentMode == Mode::OTA_UPDATE) {
                normalModeRequested = true;
            }
            break;
            
        default:
            break;
    }
}
#endif

void ModeManager::handleCommandReceivedEvent(const CORE::Event& event) {
    const CORE::CommandReceivedEvent& cmdEvent = static_cast<const CORE::CommandReceivedEvent&>(event);
    
    const String& command = cmdEvent.getCommand();
    
    if (command == "mode") {
        const String& payload = cmdEvent.getPayload();
        
        if (payload == "normal") {
            normalModeRequested = true;
        } else if (payload == "provisioning") {
            provisioningRequested = true;

            #ifdef CONFIG_ENABLE_OTA_ANEDYA
        } else if (payload == "ota") {
            otaUpdateRequested = true;
            #endif

        } 
    }
}

void ModeManager::task(void* parameter) {
    esp_task_wdt_add(NULL);
    
    while (true) {
        esp_task_wdt_reset();
        
        if (normalModeRequested) {
            normalModeRequested = false;
            enterMode(Mode::NORMAL);
        } else if (provisioningRequested) {
            provisioningRequested = false;
            enterMode(Mode::PROVISIONING);

            #ifdef CONFIG_ENABLE_OTA_ANEDYA

        } else if (otaUpdateRequested) {
            otaUpdateRequested = false;
            enterMode(Mode::OTA_UPDATE);

            #endif
        } 
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

bool ModeManager::startTask() {
    if (modeTaskHandle != nullptr) {
        return true; 
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        task,
        "ModeManager",
        4096,
        NULL,
        1,
        &modeTaskHandle,
        0
    );
    
    return (result == pdPASS);
}

bool ModeManager::restartTask() {
    if (modeTaskHandle != nullptr) {
        vTaskDelete(modeTaskHandle);
        modeTaskHandle = nullptr;
    }
    return startTask();
}
}
 // namespace APP