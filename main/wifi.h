// wifi.h
#ifndef WIFI_H
#define WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

// Initialize Wi-Fi
void wifi_init(void);

// Get the AP SSID (used as device ID)
const char* wifi_get_ap_ssid(void);

// Check if Wi-Fi is connected in STA mode
bool wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_H