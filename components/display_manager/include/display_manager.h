#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <esp_err.h>
#include "sensor_manager.h"

// Constants for chart data normalization
#define TEMP_MAX        40.0f    // Maximum temperature in °C
#define PRESSURE_MIN    900.0f   // Minimum pressure in hPa
#define PRESSURE_MAX    1100.0f  // Maximum pressure in hPa
#define GAS_MIN         1000.0f  // Minimum gas resistance in ohms (legacy)
#define GAS_MAX         500000.0f// Maximum gas resistance in ohms (legacy)
#define IAQ_MIN         0.0f     // Minimum IAQ value (legacy)
#define IAQ_MAX         300.0f   // Maximum IAQ value (legacy)
#define PM_MIN          0.0f     // Minimum particulate matter value in μg/m³
#define PM_MAX          300.0f   // Maximum particulate matter value in μg/m³
#define AQI_MIN         0.0f     // Minimum Air Quality Index value
#define AQI_MAX         500.0f   // Maximum Air Quality Index value
#define TVOC_MIN        0.0f     // Minimum TVOC value in ppb
#define TVOC_MAX        2000.0f  // Maximum TVOC value in ppb

/**
 * @brief Initialize the display and LVGL 
 * 
 * @return esp_err_t ESP_OK if successful, error code otherwise
 */
esp_err_t display_manager_init(void);

/**
 * @brief Update the display with latest sensor data
 * 
 * @param data Pointer to sensor data structure
 * @return esp_err_t ESP_OK if successful, error code otherwise
 */
esp_err_t display_manager_update(sensor_data_t *data);

/**
 * @brief Deinitialize the display manager
 * 
 * @return esp_err_t ESP_OK if successful, error code otherwise
 */
esp_err_t display_manager_deinit(void);

#endif // DISPLAY_MANAGER_H 