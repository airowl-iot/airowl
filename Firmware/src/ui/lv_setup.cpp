#ifdef CONFIG_ENABLE_LVGL
#include "lv_setup.h"

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_CST8XX.h>
#include <esp_task_wdt.h>
#include "configure.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 2, true, TFT_WIDTH, TFT_HEIGHT);

Adafruit_CST8XX cst8xx;
static CST_TS_Point last_point;

extern TwoWire Wire;

#define TOUCH_RAW_MIN_X 0
#define TOUCH_RAW_MAX_X 240
#define TOUCH_RAW_MIN_Y 0
#define TOUCH_RAW_MAX_Y 320

// -------------------- LVGL Buffers --------------------
#define DISP_BUF_SIZE (TFT_WIDTH * 20)  // Further reduced buffer size to save memory
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[DISP_BUF_SIZE];

// ----------- External Task Handle ----
TaskHandle_t lvglTaskHandle = nullptr;

// -------------------- Display Flush --------------------
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p,
                            area->x2 - area->x1 + 1,
                            area->y2 - area->y1 + 1);
    lv_disp_flush_ready(disp);
}

// -------------------- Touch Coordinate Mapping --------------------
void map_touch_coordinates(int raw_x, int raw_y, int* mapped_x, int* mapped_y) {
    // Option 2 with extended range for full screen coverage
    // Standard rotation transformation (90 degrees counterclockwise) with calibration
    
    // First, apply the rotation transformation
    int temp_x = TFT_HEIGHT - raw_y;
    int temp_y = raw_x;
    
    // Apply calibration mapping to ensure full screen coverage
    // Based on your touch data, we need to map the full touch sensor range
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

// -------------------- Touch Read --------------------
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

// ----------- LVGL Task --------------
void lvgl_task(void *pv) {
  Serial.println("[LVGL] LVGL task started");
  
  // Add to watchdog only if not already added
  if (!esp_task_wdt_status(NULL)) {
    esp_err_t err = esp_task_wdt_add(NULL);
    if (err != ESP_OK) {
      Serial.printf("[LVGL] Failed to add task to watchdog: %s\n", esp_err_to_name(err));
    }
  }
  
  const TickType_t d = pdMS_TO_TICKS(16);
  while (true) {
    lv_handler();
    esp_task_wdt_reset();
    vTaskDelay(d);
  }
}

// ----------- Task Restart ----------
void restartLVGLTask() {
    // Clean up existing task if it exists
    if (lvglTaskHandle) {
        esp_task_wdt_delete(lvglTaskHandle);
        vTaskDelete(lvglTaskHandle);
        lvglTaskHandle = nullptr;
    }

    BaseType_t result = xTaskCreatePinnedToCore(lvgl_task, "LVGL", 8192, nullptr, 1, &lvglTaskHandle, 0);
    if (result == pdPASS && lvglTaskHandle) {
        // Add to watchdog only if not already added
        if (!esp_task_wdt_status(lvglTaskHandle)) {
            esp_err_t err = esp_task_wdt_add(lvglTaskHandle);
            if (err != ESP_OK) {
                Serial.printf("[LVGL] Failed to add LVGL task to watchdog: %s\n", esp_err_to_name(err));
            }
        }
        Serial.println("[LVGL] LVGL task restarted");
    } else {
        Serial.println("[LVGL] Failed to restart LVGL task!");
        lvglTaskHandle = nullptr;
    }
}

// -------------------- LVGL + Display + Touch Init --------------------
void lv_begin() {
    Serial.println("[LVGL] Initializing LVGL...");
    lv_init();
    Serial.println("[LVGL] LVGL core initialized");
    
    // Initialize display with error handling
    if (!gfx->begin()) {
        Serial.println("[LVGL] ERROR: Display initialization failed!");
        return;
    }
    Serial.println("[LVGL] Display initialized");
    gfx->fillScreen(BLACK);
    Serial.println("[LVGL] Display cleared");

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(50);
    digitalWrite(TOUCH_RST, HIGH);
    delay(200);

    // Wire is already initialized in main.cpp, so we don't need to initialize it again
    // Wire.begin(TOUCH_SDA, TOUCH_SCL);

    Serial.println("[LVGL] Initializing touch sensor...");
    if (!cst8xx.begin(&Wire, 0x15)) {
        Serial.println("[LVGL] CST836U NOT FOUND");
    } else {
        Serial.println("[LVGL] CST836U INITIALIZED");
    }

    Serial.println("[LVGL] Setting up display buffer...");
    Serial.printf("[LVGL] Buffer size: %d bytes\n", DISP_BUF_SIZE * sizeof(lv_color_t));
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, DISP_BUF_SIZE);
    Serial.println("[LVGL] Display buffer initialized");

    Serial.println("[LVGL] Registering display driver...");
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_t* disp = lv_disp_drv_register(&disp_drv);
    if (disp == NULL) {
        Serial.println("[LVGL] ERROR: Failed to register display driver!");
        return;
    }
    Serial.println("[LVGL] Display driver registered");

    Serial.println("[LVGL] Registering input driver...");
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_t* indev = lv_indev_drv_register(&indev_drv);
    if (indev == NULL) {
        Serial.println("[LVGL] ERROR: Failed to register input driver!");
        return;
    }
    Serial.println("[LVGL] Input driver registered");

    Serial.printf("[LVGL] LVGL v%d.%d.%d initialized\n", lv_version_major(), lv_version_minor(), lv_version_patch());
    Serial.println("[LVGL] LVGL initialization completed successfully");
}

// -------------------- LVGL Tick Handling --------------------
void lv_handler()
{
    static uint32_t previousUpdate = 0;
    static uint32_t interval = 0;

    if (millis() - previousUpdate > interval)
    {
        previousUpdate = millis();
        interval = lv_timer_handler(); 
    }
}

#endif // CONFIG_ENABLE_LVGL