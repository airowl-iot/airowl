#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_CST8XX.h>
#include "config.h"

// pin configurations in the config.h file
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 2, true, TFT_WIDTH, TFT_HEIGHT);

Adafruit_CST8XX cst8xx;
static CST_TS_Point last_point;

// Touch Calibration Parameters
#define TOUCH_RAW_MIN_X 0
#define TOUCH_RAW_MAX_X 240
#define TOUCH_RAW_MIN_Y 0
#define TOUCH_RAW_MAX_Y 320

// LVGL Buffers
#define DISP_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 8) 
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[DISP_BUF_SIZE];

// Display Flush
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p,
                            area->x2 - area->x1 + 1,
                            area->y2 - area->y1 + 1);
    lv_disp_flush_ready(disp);
}

// Touch Coordinate Mapping
void map_touch_coordinates(int raw_x, int raw_y, int* mapped_x, int* mapped_y) {
    
    // First, apply the rotation transformation
    int temp_x = TFT_HEIGHT - raw_y;
    int temp_y = raw_x;
    
    // Applied calibration mapping to ensure full screen coverage
    *mapped_x = map(temp_x, 0, TFT_HEIGHT, 0, TFT_WIDTH);
    *mapped_y = map(temp_y, 0, TFT_WIDTH, 0, TFT_HEIGHT);
    
    // Apply bounds checking with slight tolerance for edge cases
    *mapped_x = constrain(*mapped_x, 0, TFT_WIDTH - 1);
    *mapped_y = constrain(*mapped_y, 0, TFT_HEIGHT - 1);
    
    // Alternative Option 2B - Try this if above doesn't work for TVOC button:
    /*
    *mapped_x = raw_y;
    *mapped_y = TFT_HEIGHT - raw_x;
    
    *mapped_x = constrain(*mapped_x, 0, TFT_WIDTH - 1);
    *mapped_y = constrain(*mapped_y, 0, TFT_HEIGHT - 1);
    */
    
    // Option 3: Fine-tuned linear mapping based on your actual touch data
    /*
    // From your data analysis, it seems like:
    // Top area (PM1.0): raw_y ~239, should map to y ~0-80
    // Bottom area (TVOC): raw_y ~224, should map to y ~280-320
    
    *mapped_x = map(raw_y, 0, 320, 0, TFT_WIDTH);
    *mapped_y = map(raw_x, 240, 0, 0, TFT_HEIGHT);  // Note: inverted range for raw_x
    
    *mapped_x = constrain(*mapped_x, 0, TFT_WIDTH - 1);
    *mapped_y = constrain(*mapped_y, 0, TFT_HEIGHT - 1);
    */
}

// Touch Read
void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
    if (cst8xx.touched()) {
        CST_TS_Point p = cst8xx.getPoint(0);
        last_point = p;

        int mapped_x, mapped_y;
        map_touch_coordinates(p.x, p.y, &mapped_x, &mapped_y);
        
        data->point.x = mapped_x;
        data->point.y = mapped_y;
        data->state = LV_INDEV_STATE_PR;

        // Serial.printf("Touch raw=(%d,%d) -> mapped=(%d,%d)\n", p.x, p.y, mapped_x, mapped_y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// LVGL + Display + Touch Init 
void lv_begin() {
    lv_init();
    gfx->begin();
    gfx->fillScreen(BLACK);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(50);
    digitalWrite(TOUCH_RST, HIGH);
    delay(200);

    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    if (!cst8xx.begin(&Wire, 0x15)) {
        // Serial.println("CST836U NOT FOUND");
    } else {
        // Serial.println("CST836U INITIALIZED");
    }

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    Serial.printf("LVGL v%d.%d.%d initialized\n", lv_version_major(), lv_version_minor(), lv_version_patch());
}

// LVGL Tick Handling
void lv_handler()
{
    static uint32_t previousUpdate = 0;
    static uint32_t interval = 0;

    if (millis() - previousUpdate > interval)
    {
        previousUpdate = millis();
        uint32_t interval = lv_timer_handler(); 
    }
}
