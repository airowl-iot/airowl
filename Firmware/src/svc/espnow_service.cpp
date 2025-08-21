// espnow_service.cpp - ESP-NOW Service implementation for Airowl 3.0
#include "espnow_service.h"

#ifdef CONFIG_ENABLE_ESP_NOW

#include <WiFi.h>
#include <esp_task_wdt.h>

namespace {
    // Private variables
    bool initialized = false;
    bool isMasterDevice = false;
    SVC::ESPNowService::MessageCallback messageCallback = nullptr;
    SVC::ESPNowService::DeliveryCallback deliveryCallback = nullptr;
    TaskHandle_t espnowTaskHandle = nullptr;
    
    // Message tracking
    struct PendingMessage {
        uint8_t id;
        uint8_t type;
        uint8_t macAddress[6];
        uint8_t retriesLeft;
        unsigned long timeout;
        unsigned long sentTime;
        SVC::ESPNowService::DeliveryStatus status;
    };
    
    std::vector<PendingMessage> pendingMessages;
    uint8_t nextMessageId = 1; // Start from 1, 0 is reserved for errors
    
    // ESP-NOW callbacks
    void onDataSent(const uint8_t *macAddr, esp_now_send_status_t status) {
        // Find the message in pending messages
        for (auto& msg : pendingMessages) {
            if (memcmp(msg.macAddress, macAddr, 6) == 0 && 
                msg.status == SVC::ESPNowService::DeliveryStatus::PENDING) {
                
                if (status == ESP_NOW_SEND_SUCCESS) {
                    msg.status = SVC::ESPNowService::DeliveryStatus::DELIVERED;
                    
                    // Call delivery callback if registered
                    if (deliveryCallback) {
                        deliveryCallback(msg.id, msg.status);
                    }
                } else {
                    // Check if retries are available
                    if (msg.retriesLeft > 0) {
                        msg.retriesLeft--;
                        msg.sentTime = millis(); // Reset timeout
                        
                        // Message will be retried by the task
                    } else {
                        msg.status = SVC::ESPNowService::DeliveryStatus::FAILED;
                        
                        // Call delivery callback if registered
                        if (deliveryCallback) {
                            deliveryCallback(msg.id, msg.status);
                        }
                    }
                }
                
                break;
            }
        }
    }
    
    void onDataReceived(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
        // Ensure we have enough data for the message header
        if (len < 2) return;
        
        // Parse message
        SVC::ESPNowService::Message message;
        message.type = data[0];
        message.id = data[1];
        
        // Copy data payload
        size_t dataLen = len - 2;
        if (dataLen > SVC::ESPNowService::MAX_MESSAGE_SIZE) {
            dataLen = SVC::ESPNowService::MAX_MESSAGE_SIZE;
        }
        
        memcpy(message.data, data + 2, dataLen);
        message.length = dataLen;
        
        // Copy sender MAC address
        // memcpy(message.macAddress, macAddr, 6);
        memcpy(message.macAddress, recv_info->src_addr, 6);

        
        // Call message callback if registered
        if (messageCallback) {
            messageCallback(message);
        }
    }
    
    // Helper function to send a message
    bool sendESPNowMessage(uint8_t id, uint8_t type, const uint8_t* data, size_t length, 
                          const uint8_t* macAddress, uint8_t retries, unsigned long timeout) {
        // Prepare message buffer
        uint8_t buffer[SVC::ESPNowService::MAX_MESSAGE_SIZE + 2]; // +2 for type and id
        buffer[0] = type;
        buffer[1] = id;
        
        // Copy data payload
        size_t dataLen = length;
        if (dataLen > SVC::ESPNowService::MAX_MESSAGE_SIZE) {
            dataLen = SVC::ESPNowService::MAX_MESSAGE_SIZE;
        }
        
        memcpy(buffer + 2, data, dataLen);
        
        // Send message
        esp_err_t result = esp_now_send(macAddress, buffer, dataLen + 2);
        
        if (result == ESP_OK) {
            // Add to pending messages for tracking
            PendingMessage pending;
            pending.id = id;
            pending.type = type;
            memcpy(pending.macAddress, macAddress, 6);
            pending.retriesLeft = retries;
            pending.timeout = timeout;
            pending.sentTime = millis();
            pending.status = SVC::ESPNowService::DeliveryStatus::PENDING;
            
            pendingMessages.push_back(pending);
            return true;
        }
        
        return false;
    }
}

namespace SVC {

bool ESPNowService::init(bool isMaster) {
    if (initialized) return true;
    
    // Store master/slave status
    isMasterDevice = isMaster;
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        return false;
    }
    
    // Register callbacks
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceived);
    
    initialized = true;
    return true;
}

uint8_t ESPNowService::sendMessage(uint8_t type, const uint8_t* data, size_t length, 
                                  const uint8_t* macAddress, uint8_t retries, 
                                  unsigned long timeout) {
    if (!initialized) return 0;
    
    // Get next message ID
    uint8_t id = nextMessageId++;
    if (nextMessageId == 0) nextMessageId = 1; // Skip 0 as it's reserved for errors
    
    // Send message
    if (sendESPNowMessage(id, type, data, length, macAddress, retries, timeout)) {
        return id;
    }
    
    return 0; // Failed to send
}

uint8_t ESPNowService::broadcastMessage(uint8_t type, const uint8_t* data, size_t length) {
    if (!initialized) return 0;
    
    // Get next message ID
    uint8_t id = nextMessageId++;
    if (nextMessageId == 0) nextMessageId = 1; // Skip 0 as it's reserved for errors
    
    // Broadcast address (all 0xFF)
    uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // Send message (no retries for broadcast)
    if (sendESPNowMessage(id, type, data, length, broadcastAddr, 0, 0)) {
        return id;
    }
    
    return 0; // Failed to send
}

bool ESPNowService::addPeer(const uint8_t* macAddress, uint8_t channel, 
                           bool encrypt, const uint8_t* key) {
    if (!initialized) return false;
    
    // Prepare peer info
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = channel;
    peerInfo.encrypt = encrypt;
    
    if (encrypt && key) {
        memcpy(peerInfo.lmk, key, 16);
    }
    
    // Add peer
    if (esp_now_is_peer_exist(macAddress)) {
        return true; // Already exists
    }
    
    return (esp_now_add_peer(&peerInfo) == ESP_OK);
}

bool ESPNowService::removePeer(const uint8_t* macAddress) {
    if (!initialized) return false;
    
    // Remove peer
    return (esp_now_del_peer(macAddress) == ESP_OK);
}

void ESPNowService::onMessage(MessageCallback callback) {
    messageCallback = callback;
}

void ESPNowService::onDeliveryStatus(DeliveryCallback callback) {
    deliveryCallback = callback;
}

void ESPNowService::task(void* parameter) {
    esp_task_wdt_add(NULL);
    
    while (true) {
        // Reset watchdog
        esp_task_wdt_reset();
        
        // Check for timed out messages
        unsigned long currentTime = millis();
        
        for (auto it = pendingMessages.begin(); it != pendingMessages.end();) {
            if (it->status == DeliveryStatus::PENDING) {
                // Check for timeout
                if (currentTime - it->sentTime > it->timeout) {
                    // Check if retries are available
                    if (it->retriesLeft > 0) {
                        // Retry sending
                        it->retriesLeft--;
                        it->sentTime = currentTime;
                        
                        // Resend message
                        uint8_t buffer[MAX_MESSAGE_SIZE + 2];
                        buffer[0] = it->type;
                        buffer[1] = it->id;
                        
                        esp_now_send(it->macAddress, buffer, 2); // Minimal retry (header only)
                    } else {
                        // Mark as timed out
                        it->status = DeliveryStatus::TIMEOUT;
                        
                        // Call delivery callback if registered
                        if (deliveryCallback) {
                            deliveryCallback(it->id, it->status);
                        }
                    }
                }
                
                ++it; // Move to next message
            } else {
                // Remove completed messages
                it = pendingMessages.erase(it);
            }
        }
        
        // Task delay
        vTaskDelay(pdMS_TO_TICKS(10)); // Check frequently
    }
}

bool ESPNowService::startTask() {
    if (espnowTaskHandle != nullptr) {
        return true; // Task already running
    }
    
    // Create ESP-NOW task
    BaseType_t result = xTaskCreatePinnedToCore(
        task,
        "ESPNowService",
        4096,
        NULL,
        1,
        &espnowTaskHandle,
        0
    );
    
    return (result == pdPASS);
}

bool ESPNowService::restartTask() {
    if (espnowTaskHandle != nullptr) {
        vTaskDelete(espnowTaskHandle);
        espnowTaskHandle = nullptr;
    }
    
    return startTask();
}

} // namespace SVC

#endif // CONFIG_ENABLE_ESP_NOW