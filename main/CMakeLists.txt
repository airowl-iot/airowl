idf_component_register(
    SRCS "main.cpp" "wifi.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "." "${ESP_MATTER_PATH}/examples/common/utils"
    REQUIRES ui sensor esp_matter esp_wifi esp_event esp_netif esp_http_server nvs_flash wifi_provisioning lwip
)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H" "-DCHIP_ADDRESS_RESOLVE_IMPL_INCLUDE_HEADER=<lib/address_resolve/AddressResolve_DefaultImpl.h>")
add_definitions(-DCHIP_DEVICE_CONFIG_DEVICE_PRODUCT_NAME="AirOwl")
add_definitions(-DCHIP_DEVICE_CONFIG_DEVICE_VENDOR_NAME="Atomo Innovations")
add_definitions(-DCHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING="2.0")