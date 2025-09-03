// hal_pm700.h - Temtop PM700 Sensor HAL for Airowl 3.0
#pragma once

#include <Arduino.h>

namespace HAL {

class PM700 {
public:
    /**
     * @brief PM700 sensor data structure
     */
    struct Data {
        float pm1;    // PM1.0 concentration (μg/m³)
        float pm25;   // PM2.5 concentration (μg/m³)
        float pm4;    // PM4.0 concentration (μg/m³) - not available on PM700
        float pm10;   // PM10 concentration (μg/m³)
        float p03;    // 0.3μm particle count (pcs/L)
        uint32_t timestamp; // Timestamp of reading
    };
    
    /**
     * @brief Error codes for PM700 operations
     */
    enum class Error {
        NONE,           // No error
        NOT_INITIALIZED,// Sensor not initialized
        COMM_ERROR,     // Communication error
        TIMEOUT,        // Read timeout
        INVALID_DATA,   // Invalid data received
        CHECKSUM_ERROR  // Checksum validation failed
    };
    
    /**
     * @brief Initialize the PM700 sensor
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
     * @brief Check if sensor is initialized
     * @return true if initialized, false otherwise
     */
    static bool isInitialized();
    
    /**
     * @brief Get last error code
     * @return Last error code
     */
    static Error getLastError();
    
    /**
     * @brief Get sensor type string
     * @return Sensor type identifier
     */
    static const char* getSensorType();
};

} // namespace HAL
