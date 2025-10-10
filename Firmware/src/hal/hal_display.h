// hal_display.h - Display HAL for Airowl 3.0
#pragma once

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
     * @brief Deinitialize the display and free resources
     */
    static void deinit();

};

} // namespace HAL
