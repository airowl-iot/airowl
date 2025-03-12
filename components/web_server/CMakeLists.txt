idf_component_register(SRCS "web_server.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_http_server esp_wifi)
