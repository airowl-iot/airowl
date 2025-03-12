idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES wifi_manager ui_display sensor_module nvs_flash web_server)
