// config.h - Configuration for Airowl 3.0 HAL
#pragma once

#ifndef __CONFIG_H
#define __CONFIG_H

#define SYS_I2C_PORT 0

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_CS    39
#define TFT_DC    21
#define TFT_RST   47
#define TFT_SCLK  7
#define TFT_MOSI  6
#define TFT_BL    38

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TOUCH_SDA 4// I2C SDA pin
#define TOUCH_SCL 5 // I2C SCL pin
#define TOUCH_RST 42
#define TOUCH_INT 41

#define PMS_SERIAL_BAUD 9600

#define PMS_RX_PIN 44

#define CONFIG_ENS160_SDA 4
#define CONFIG_ENS160_SCL 5

#define CONFIG_AHT_SDA 4
#define CONFIG_AHT_SCL 5

// esp-now configuration
#define MAX_SLAVES 9
#define ESPNOW_RETRY_COUNT 3
#define ESPNOW_TIMEOUT_MS 5000
#define SLAVE_DATA_TIMEOUT 30000
#define MQTT_PUBLISH_INTERVAL 10000

// OTA Update Configuration
#ifdef CONFIG_ENABLE_OTA_ANEDYA
#define OTA_UPDATE_SERVER "https://airowl-updates.example.com/firmware"
#endif

#endif  // __CONFIG_H
