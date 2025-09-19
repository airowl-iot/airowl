// hal_aht.h - AHT + ENS160 Sensor HAL for Airowl 3.0
#pragma once

#include <Arduino.h>

namespace HAL {

class AHT {
public:
    /**
     * @brief AHT + ENS160 sensor data structure
     */
    struct Data {
        float temperature;   // Temperature in Celsius
        float humidity;      // Relative humidity in percent
        uint16_t aqi;        // Air Quality Index (ENS160)
        uint16_t tvoc;       // Total Volatile Organic Compounds (ppb)
        uint16_t eco2;       // Equivalent CO2 (ppm)
    };
    
    /**
     * @brief Error codes for AHT operations
     */
    enum class Error {
        NONE,            // No error
        NOT_INITIALIZED, // Sensor not initialized
        COMM_ERROR,      // Communication error
        TIMEOUT,         // Read timeout
        INVALID_DATA     // Invalid data received
    };
    
    /**
     * @brief Initialize the AHT + ENS160 sensors
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
     * @brief Check if sensors are initialized
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
