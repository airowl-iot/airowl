idf_component_register(SRCS "main.c" "network_manager.c" "sensor_manager.c" "web_server.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server json nvs_flash esp_wifi esp_timer) 