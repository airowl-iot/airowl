cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(m5core-s3-hello-world)

# Keep either LovyanGFX or M5GFX, not both
set(EXTRA_COMPONENT_DIRS
    components/M5Unified
    components/M5GFX    # Keep this OR LovyanGFX
    # components/LovyanGFX  # Remove/comment this line
    driver
    esp_log
    freertos
    # SensirionI2CSen5x
    # Wire
)