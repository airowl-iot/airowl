#include <lvgl.h>
#include "airowl_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_CST8XX.h>
#include <esp_task_wdt.h>

#include "hal_display.h"

#define DISP_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10) 
static lv_color_t buf[DISP_BUF_SIZE];

static Arduino_DataBus* bus = nullptr;
static Arduino_GFX* gfx = nullptr;
static Adafruit_CST8XX cst8xx;
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static bool initializedDisplay = false;

static void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    if (gfx == nullptr) {
        Serial.println("[Display] ERROR: my_disp_flush called with null gfx pointer!");
        lv_disp_flush_ready(disp);
        return;
    }

    esp_task_wdt_reset(); 
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)color_p,
                            area->x2 - area->x1 + 1,
                            area->y2 - area->y1 + 1);
    lv_disp_flush_ready(disp);
}

static void my_touchpad_read(lv_indev_drv_t* indev_driver, lv_indev_data_t* data) {
    if (cst8xx.touched()) {
        auto p = cst8xx.getPoint(0);
        int mapped_x = map(TFT_HEIGHT - p.y, 0, TFT_HEIGHT, 0, TFT_WIDTH);
        int mapped_y = map(p.x, 0, TFT_WIDTH, 0, TFT_HEIGHT);
        data->point.x = constrain(mapped_x, 0, TFT_WIDTH - 1);
        data->point.y = constrain(mapped_y, 0, TFT_HEIGHT - 1);
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

namespace HAL {

bool Display::init() {
    if (initializedDisplay) {
        return true;
    }

    bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI);
    if (bus == nullptr) {
        Serial.println("[Display] CRITICAL: Failed to allocate memory for display bus!");
        return false;
    }

    gfx = new Arduino_ST7789(bus, TFT_RST, 2, true, TFT_WIDTH, TFT_HEIGHT);
    if (gfx == nullptr) {
        Serial.println("[Display] CRITICAL: Failed to allocate memory for display driver!");
        delete bus;
        bus = nullptr;
        return false;
    }

    if (!gfx->begin()) {
        Serial.println("[Display] ERROR: Display initialization failed (gfx->begin() returned false)");
        delete gfx;
        gfx = nullptr;
        delete bus;
        bus = nullptr;
        return false;
    }
    gfx->setRotation(2);
    gfx->fillScreen(WHITE);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    vTaskDelay(pdMS_TO_TICKS(50));
    digitalWrite(TOUCH_RST, HIGH);
    vTaskDelay(pdMS_TO_TICKS(200));

    lv_init();

     if (!cst8xx.begin(&Wire, 0x15)) {
        Serial.println("CST836U NOT FOUND");
    } else {
        Serial.println("CST836U INITIALIZED");
    }

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, DISP_BUF_SIZE);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    if (disp) {
        lv_disp_set_default(disp);
    }

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    (void)indev;

    Serial.println("[HAL] Display driver registered");
    initializedDisplay = true;
    return true;
}

bool Display::lvHandler(){
    lv_timer_handler();
    return true;
}

void Display::deinit() {
    if (!initializedDisplay) {
        return;
    }

    Serial.println("[HAL] Deinitializing display...");
    Serial.println("[HAL] LVGL deinitialized");
    digitalWrite(TFT_BL, LOW);

    if (gfx != nullptr) {
        delete gfx;
        gfx = nullptr;
    }

    if (bus != nullptr) {
        delete bus;
        bus = nullptr;
    }

    initializedDisplay = false;
    Serial.println("[HAL] Display deinitialized");
}

} //namespace HAL