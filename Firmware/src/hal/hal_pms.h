// hal_pms.h - PMS Sensor HAL for Airowl 3.0
#pragma once

#include <Arduino.h>

namespace HAL {

class PMS {
public:
    /**
     * @brief PMS sensor data structure
     */
    struct Data {
        float pm1;    // PM1.0 concentration (μg/m³)
        float pm25;   // PM2.5 concentration (μg/m³)
        float pm4;    // PM4.0 concentration (μg/m³)
        float pm10;   // PM10 concentration (μg/m³)
        uint32_t timestamp; // Timestamp of reading
    };
    
    /**
     * @brief Error codes for PMS operations
     */
    enum class Error {
        NONE,           // No error
        NOT_INITIALIZED,// Sensor not initialized
        COMM_ERROR,     // Communication error
        TIMEOUT,        // Read timeout
        INVALID_DATA    // Invalid data received
    };
    
    /**
     * @brief Initialize the PMS sensor
     * @return true if initialization was successful, false otherwise
     */
    static bool init();
    
    /**
     * @brief Read sensor data (non-blocking)
     * @param data Pointer to data structure to fill
     * @return Error code
     */
    static Error read(Data* data);
    
    /**
     * @brief Check if new data is available
     * @return true if new data is available, false otherwise
     */
    static bool isDataAvailable();
    
    /**
     * @brief Put sensor to sleep mode to save power
     * @return true if successful, false otherwise
     */
    static bool sleep();
    
    /**
     * @brief Wake up sensor from sleep mode
     * @return true if successful, false otherwise
     */
    static bool wakeup();
    
    /**
     * @brief Check if sensor is initialized
     * @return true if initialized, false otherwise
     */
    static bool isInitialized();
    
    /**
     * @brief Get last error code
     * @return Last error code
     */
    static Error getLastError();
};

} // namespace HAL