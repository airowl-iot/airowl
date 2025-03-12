#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"  // Add this line to define esp_err_t

void wifi_manager_start(void);
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

#endif // WIFI_MANAGER_H
