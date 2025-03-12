
#include <Arduino.h>
#if defined(ARDUINO_M5STACK_Core2)
#include <M5Core2.h>
#endif
#if defined(ARDUINO_M5STACK_CORES3)
// #include <M5CoreS3.h>
#include <M5Unified.h>
#endif
#include <lvgl.h>

/* Core2 screen size */
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

/* Display flushing */
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    M5.Lcd.startWrite();
    M5.Lcd.setAddrWindow(area->x1, area->y1, w, h);
    M5.Lcd.pushColors((uint16_t *)&color_p->full, w * h, true);
    M5.Lcd.endWrite();

    lv_disp_flush_ready(disp);
}

/* Read the touchpad */
static void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX = 0, touchY = 0;
#ifdef M5UNIFIED
    M5.update();
    auto count = M5.Touch.getCount();
    if (!count) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    auto t = M5.Touch.getDetail();
    if (t.wasPressed())  {
        data->state = LV_INDEV_STATE_PR;
        /*Set the coordinates*/
        data->point.x = t.x;
        data->point.y = t.y;
        // Serial.println("x: " + String(data->point.x) + " y: " + String(data->point.y));
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
#else
    bool touched = M5.Lcd.getTouch(&touchX, &touchY, 600);
    if (M5.Lcd.getTouch(&touchX, &touchY, 600))
    {
        data->state = LV_INDEV_STATE_PR;
        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;
        // Serial.println("x: " + String(touchX) + " y: " + String(touchY));
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
#endif
}

/* Setup lvgl with display and touch pad */
void lv_begin()
{
    lv_init(); // call this before any other lvgl function

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10); // initialize the display buffer

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    Serial.printf("LVGL v%d.%d.%d initialized\n", lv_version_major(), lv_version_minor(), lv_version_patch());
}

/* Handles updating the display and touch events */
void lv_handler()
{
    static uint32_t previousUpdate = 0;
    static uint32_t interval = 0;

    if (millis() - previousUpdate > interval)
    {
        previousUpdate = millis();
        uint32_t interval = lv_timer_handler(); // Update the UI
    }
}
