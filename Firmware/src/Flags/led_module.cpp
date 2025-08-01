#ifdef CONFIG_FASTLED
#include "led_module.h"

#ifdef CONFIG_ENABLE_ESP_NOW
#include "espnow_module.h"
#endif

// #include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <Arduino.h>
#include <esp_task_wdt.h>

#include "mqtt_module.h"

#define NUM_LEDS    1
#define COLOR_ORDER GRB       // or BGR if colors are wrong
#define LED_TYPE    WS2812B

CRGB leds[NUM_LEDS];

// Adafruit_NeoPixel led = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

static device_state_t current_state = DEVICE_STATE_DISCONNECTED;
static unsigned long last_update = 0;
static uint8_t breathe_brightness = 0;
static int8_t breathe_direction = 1;
static bool led_on = false;

extern PubSubClient mqttClient; 

// FreeRTOS task handle
TaskHandle_t ledTaskHandle = nullptr;


// void led_status_init(uint8_t pin) {
//     pinMode(LED_PIN, OUTPUT);
//     digitalWrite(LED_PIN, LOW);
//     led.begin();
//     led.setBrightness(255); // Max brightness
//     led.show();
//     Serial.println("[LED] LED initialized");
// }

void led_status_init(uint8_t pin) {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(255);  // Optional
    leds[0] = CRGB::Black;
    FastLED.show();
    Serial.println("[LED] LED initialized (FastLED)");
}

void led_status_update(device_state_t state) {
    current_state = state;
}

device_state_t get_device_state() {
    bool mqttOk = mqtt_connected();
    if (WiFi.status() != WL_CONNECTED) {
        // Serial.println("[LED] WiFi disconnected");
        return DEVICE_STATE_DISCONNECTED;
    } else if (!mqttOk) {
        // Serial.println("[LED] WiFi OK, MQTT NOT connected");
        return DEVICE_STATE_WIFI_CONNECTED;
    } else {
        // Serial.println("[LED] MQTT connected");
        return DEVICE_STATE_MQTT_PUBLISHING;
    }
}


// void led_status_loop() {
//     unsigned long now = millis();
//     switch (current_state) {
//         case DEVICE_STATE_DISCONNECTED:
//             // Blink green
//             if (now - last_update > 500) {
//                 led_on = !led_on;
//                 led.setPixelColor(0, led_on ? led.Color(0, 0, 255) : 0);
//                 led.show();
//                 last_update = now;
//             }
//             break;
//         case DEVICE_STATE_WIFI_CONNECTED:
//             // Breathe blue
//             if (now - last_update > 30) {
//                 breathe_brightness += breathe_direction;
//                 if (breathe_brightness <= 5 || breathe_brightness >= 255) {
//                     breathe_direction = -breathe_direction;
//                 }
//                 led.setPixelColor(0, led.Color(0, 0, breathe_brightness));
//                 led.show();
//                 last_update = now;
//             }
//             break;
//         case DEVICE_STATE_MQTT_PUBLISHING:
//             // Breathe cyan
//             if (now - last_update > 30) {
//                 breathe_brightness += breathe_direction;
//                 if (breathe_brightness <= 5 || breathe_brightness >= 255) {
//                     breathe_direction = -breathe_direction;
//                 }
//                 led.setPixelColor(0, led.Color(0, breathe_brightness, breathe_brightness));
//                 led.show();
//                 last_update = now;
//             }
//             break;
//     }
// }

void led_status_loop() {
    static unsigned long last_blink = 0;
    static bool led_on = false;
    unsigned long now = millis();
    CRGB color = CRGB::Black;

    switch (current_state) {
        case DEVICE_STATE_DISCONNECTED:
            color = CRGB(0, 255, 0); // Green
            break;
        case DEVICE_STATE_WIFI_CONNECTED:
            color = CRGB(255, 0, 0); // Red
            break;
        case DEVICE_STATE_MQTT_PUBLISHING:
            color = CRGB(0, 0, 255); // Blue
            break;
        default:
            color = CRGB::Black;
            break;
    }
    if (now - last_blink >= 1000) { // 1 second interval
        led_on = !led_on;
        last_blink = now;
    }

    leds[0] = led_on ? color : CRGB::Black;
    FastLED.show();
}

// FreeRTOS LED task
static void led_task(void *param) {
    // if (!esp_task_wdt_status(NULL)) esp_task_wdt_add(NULL);
    while (true) {
        current_state = get_device_state(); // Check live status
        led_status_loop();
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void restartLEDTask() {
    // if (ledTaskHandle) {
    //     esp_task_wdt_delete(ledTaskHandle);
    //     vTaskDelete(ledTaskHandle);
    //     ledTaskHandle = nullptr;
    // }

    BaseType_t result = xTaskCreatePinnedToCore(led_task, "LEDTask", 4096, NULL, 1, &ledTaskHandle, 1);
    if (result == pdPASS && ledTaskHandle) {
        esp_task_wdt_add(ledTaskHandle);
        Serial.println("[LED] LED task restarted");
    } else {
        Serial.println("[LED] Failed to restart LED task!");
        ledTaskHandle = nullptr;
    }
}
#endif
