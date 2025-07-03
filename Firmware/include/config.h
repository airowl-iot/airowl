#ifndef __CONFIG_H
#define __CONFIG_H

// I2C Pin Config 
#define SYS_I2C_PORT 0

// TFT Display Configuration 
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_CS     39
#define TFT_DC     21
#define TFT_RST    47
#define TFT_SCLK   36
#define TFT_MOSI   35
#define TFT_BL     38

// CST836U Touch Configuration
#define TOUCH_SDA  4
#define TOUCH_SCL  5
#define TOUCH_RST  6
#define TOUCH_INT  7

// Optional features
#define MONKEY_TEST_ENABLE 0
#define MIC_BUF_SIZE 256  // Keep only if you're adding audio in future

#endif  // __CONFIG_H
