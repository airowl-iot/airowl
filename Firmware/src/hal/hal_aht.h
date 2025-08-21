// hal_aht.h - AHT Temperature/Humidity Sensor HAL for Airowl 3.0
#pragma once

#include <Arduino.h>

namespace HAL {

class AHT {
public:
    /**
     * @brief AHT sensor data structure
     */
    struct Data {
        float temperature; // Temperature in Celsius
        float humidity;    // Relative humidity in percent
        uint32_t timestamp; // Timestamp of reading
    };
    
    /**
     * @brief Error codes for AHT operations
     */
    enum class Error {
        NONE,           // No error
        NOT_INITIALIZED,// Sensor not initialized
        COMM_ERROR,     // Communication error
        TIMEOUT,        // Read timeout
        INVALID_DATA    // Invalid data received
    };
    
    /**
     * @brief Initialize the AHT sensor
     * @return true if initialization was successful, false otherwise
     */
    static bool init();
    
    /**
     * @brief Read sensor data (non-blocking if possible)
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