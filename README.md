# LVGL ported to M5Stack CoreS3

## Overview

This is LVGL ported to [M5Stack CoreS3](https://github.com/espressif/esp-bsp/tree/master/bsp/m5stack_core_s3) with using Espressif [BSP](https://github.com/espressif/esp-bsp). This example shows LVGL music demo.

<div align=center><img src="https://raw.githubusercontent.com/espressif/esp-bsp/master/bsp/m5stack_core_s3/pic.webp" width=800/></div>

## Buy

You can purchase M5Stack CoreS3 from [Official shop](https://shop.m5stack.com/products/m5stack-cores3-esp32s3-lotdevelopment-kit).

## Benchmark

You can find more about performance in [BSP repository](https://github.com/espressif/esp-bsp/blob/master/components/esp_lvgl_port/docs/performance.md).

## Specification

### CPU and Memory
- **MCU:** ESP32-S3
- **RAM:** 512 KB internal SRAM,  8/16 MB external PSRAM
- **Flash:** 2/4/8/16 MB

### Display and Touch
- **Resolution:** 320x240
- **Display Size:** 2.0"
- **Interface:** SPI (ILI9341)
- **Color Depth:** 24-bit
- **Touch Pad:** Capacitive (FT5x06)

### Connectivity
- Onboard audio codec + audio amplifier
- Onboard dual microphone pickup
- Onboard camera
- Onboard uSD card slot
- Onboard IMU
- USB type-C interface download and debugging

## Getting started

### Hardware setup
- Connect USB-C (next to the screen) to PC
- [Board User Guide](https://docs.m5stack.com/en/core/CoreS3)

### Software setup
- Prepare environment for compiling ESP-IDF - follow this [guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).

### Run the project
- Clone this repository
- Compile and flash
```
idf.py -p COMx flash monitor
```

## Contribution and Support

If you find any issues with the development board feel free to open an Issue in this repository. For LVGL related issues (features, bugs, etc) please use the main [lvgl repository](https://github.com/lvgl/lvgl).

If you found a bug and found a solution too please send a Pull request. If you are new to Pull requests refer to [Our Guide](https://docs.lvgl.io/master/CONTRIBUTING.html#pull-request) to learn the basics.

