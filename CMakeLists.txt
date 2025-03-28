cmake_minimum_required(VERSION 3.16)

# Set partition table file
set(PARTITION_TABLE_CSV "partitions.csv")
set(PARTITION_TABLE "${CMAKE_CURRENT_LIST_DIR}/partitions.csv")

# Include necessary build scripts for ESP-IDF
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Add custom component directories
set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_CURRENT_LIST_DIR}/components
)

# Set compile options for LVGL
add_compile_options("-Wno-attributes")

# Define the project
project(air_owl_firmware) 