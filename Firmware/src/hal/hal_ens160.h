// hal_ens160.h - AHT Temperature/Humidity Sensor HAL for Airowl 3.0
#pragma once

#include <Arduino.h>

namespace HAL {

class ENS160 {
public:
    /**
     * @brief ENS160 sensor data structure
     */
    struct Data {
        uint16_t aqi;       // Air Quality Index (1-5)
        uint16_t tvoc;      // TVOC in ppb
        uint16_t eco2;      // eCO2 in ppm
        uint32_t timestamp; // Timestamp of reading
    };
    
    /**
     * @brief Error codes for ENS160 operations
     */
    enum class Error {
        NONE,           // No error
        NOT_INITIALIZED,// Sensor not initialized
        COMM_ERROR,     // Communication error
        TIMEOUT,        // Read timeout
        INVALID_DATA    // Invalid data received
    };
    
    /**
     * @brief Initialize the ENS160 sensor
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
     * @brief Set temperature and humidity data for compensation
     * @param temperature Temperature in Celsius
     * @param humidity Relative humidity in percent
     * @return Error code
     */
    static Error setEnvironmentalData(float temperature, float humidity);
    
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
};

} // namespace HAL