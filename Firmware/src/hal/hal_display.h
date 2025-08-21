// hal_display.h - Display HAL for Airowl 3.0
#pragma once

#ifdef CONFIG_ENABLE_LVGL

#include <Arduino.h>

namespace HAL {

class Display {
public:
    /**
     * @brief Initialize the display hardware
     * @return true if initialization was successful, false otherwise
     */
    static bool init();
    
     /**
     * @brief Check if display is initialized
     * @return true if display is initialized, false otherwise
     */
    static bool lvHandler();
    
    /**
     * @brief Restart the display task (after OTA, crash, etc.)
     * @return true if restart was successful, false otherwise
     */
    static bool restartTask();
};

} // namespace HAL

#endif