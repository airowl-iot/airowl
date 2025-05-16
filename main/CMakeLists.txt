idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "." "${ESP_MATTER_PATH}/examples/common/utils"
    PRIV_REQUIRES esp_matter ui espressif__cmake_utilities espressif__button app_reset
)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
