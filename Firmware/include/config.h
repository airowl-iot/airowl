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

// Sensor I2C Bus
#define I2C_SENSOR_SDA 18  // Use different pins for sensors
#define I2C_SENSOR_SCL 19

#define PMS_SERIAL_BAUD 9600
#define PMS_RX_PIN 44

#define PM700_SERIAL_BAUD 9600
#define PM700_RX_PIN 15

#define CONFIG_ENS160_SDA 4
#define CONFIG_ENS160_SCL 5

#define CONFIG_AHT_SDA 4
#define CONFIG_AHT_SCL 5

#define SLAVE_DATA_TIMEOUT 30000
#define MQTT_PUBLISH_INTERVAL 10000

#ifdef CONFIG_ENABLE_OTA_ANEDYA
#define REGION_CODE "ap-in-1"

static const char *CONNECTION_KEY = "bf84ac9d5e797a798429628d5b2abdab";
static const char *PHYSICAL_DEVICE_ID = "1970582f-42c4-4d67-82e9-b3b3df260125";

// Anedya API helpers
inline String anedyaDeviceHost() {
  return "device." + String(REGION_CODE) + ".anedya.io";
}

inline String anedyaApi(const char *path) {
  return String("https://") + anedyaDeviceHost() + path;
}

#endif // CONFIG_ENABLE_OTA_ANEDYA
#endif 