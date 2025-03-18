typedef struct {
    bool ap_mode_enabled;
    char ap_ssid[32];
    char ap_password[64];
    uint8_t ap_max_connections;
    char sta_ssid[32];
    char sta_password[64];
} network_manager_config_t;

esp_err_t network_manager_init(void);
esp_err_t network_manager_start_ap(const char* ssid, const char* password, uint8_t max_connections);
esp_err_t network_manager_stop_ap(void);
esp_err_t network_manager_connect_sta(const char* ssid, const char* password);
esp_err_t network_manager_disconnect_sta(void);
esp_err_t network_manager_get_status(bool* is_ap_active, bool* is_sta_connected);
esp_err_t network_manager_save_wifi_config(const network_manager_config_t* config);
esp_err_t network_manager_load_wifi_config(network_manager_config_t* config); 