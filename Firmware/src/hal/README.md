# Airowl 3.0 Hardware Abstraction Layer (HAL)

This directory contains the Hardware Abstraction Layer (HAL) modules for the Airowl 3.0 project. These modules provide a clean interface to hardware components, hiding implementation details and making the higher-level code more maintainable.

## Design Principles

1. **Encapsulation**: Each HAL module hides hardware details and exposes only high-level methods.
2. **Lightweight**: Implemented as C++ classes with static methods to minimize overhead.
3. **Error Handling**: Returns boolean success/failure or enum-based error codes.
4. **Non-blocking**: Avoids delays in HAL methods, letting the application control timing.
5. **Conditional Compilation**: Uses build flags to include/exclude modules as needed.

## Available HAL Modules

### Display (`hal_display.h`)

Provides an interface to the display hardware:

```cpp
HAL::Display::init();       // Initialize display
HAL::Display::update();     // Update display (if needed)
HAL::Display::restartTask(); // Restart display task after OTA
```

### Touch (`hal_touch.h`)

Provides an interface to the touch controller:

```cpp
HAL::Touch::init();         // Initialize touch controller
HAL::Touch::Point point = HAL::Touch::read(); // Read touch coordinates
if (point.pressed) {         // Check if screen is touched
    // Handle touch event at (point.x, point.y)
}
```

### PMS Sensor (`hal_pms.h`)

Provides an interface to the PMS air quality sensor:

```cpp
HAL::PMS::init();           // Initialize PMS sensor
HAL::PMS::Data data;        // Create data structure
if (HAL::PMS::isDataAvailable()) {
    HAL::PMS::read(&data);   // Read sensor data
    // Use data.pm25, data.pm10, etc.
}
```

### AHT Sensor (`hal_aht.h`)

Provides an interface to the AHT temperature/humidity sensor:

```cpp
HAL::AHT::init();           // Initialize AHT sensor
HAL::AHT::Data data;        // Create data structure
if (HAL::AHT::read(&data) == HAL::AHT::Error::NONE) {
    // Use data.temperature, data.humidity
}
```

### WiFi (`hal_wifi.h`)

Provides an interface to the WiFi functionality:

```cpp
HAL::WiFi::init();          // Initialize WiFi
HAL::WiFi::connect("SSID", "password"); // Connect to network
HAL::WiFi::ConnectionInfo info;
HAL::WiFi::getConnectionInfo(&info); // Get connection details
```

## Integration

To use these HAL modules in your application:

1. Include the appropriate header files
2. Initialize the modules in your setup code
3. Use the provided methods to interact with hardware

Example:

```cpp
#include "hal/hal_display.h"
#include "hal/hal_touch.h"
#include "hal/hal_pms.h"

void setup() {
    // Initialize hardware
    HAL::Display::init();
    HAL::Touch::init();
    HAL::PMS::init();
}

void loop() {
    // Update display
    HAL::Display::update();
    
    // Handle touch input
    HAL::Touch::Point touchPoint = HAL::Touch::read();
    if (touchPoint.pressed) {
        // Handle touch event
    }
    
    // Read sensor data
    if (HAL::PMS::isDataAvailable()) {
        HAL::PMS::Data pmsData;
        HAL::PMS::read(&pmsData);
        // Process sensor data
    }
}
```

## Configuration

HAL modules use configuration values from `config.h`. Make sure to define the necessary pins and parameters in your configuration file.