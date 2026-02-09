#include "ui.h"

lv_obj_t * ui_Intro = NULL;
lv_obj_t * ui_logo = NULL;
lv_obj_t * ui_firmwareversion = NULL;
lv_obj_t * ui_devicename = NULL;

// Forward declarations for C++ helper functions (defined in ui_manager.cpp)
#ifdef __cplusplus
extern "C" {
#endif
    bool isWiFiConnected(void);
    bool isMatterEnabled(void);
    void debugLog(const char* msg);  // Simple logging helper
#ifdef __cplusplus
}
#endif

// WiFi check retry counter
static int wifi_check_attempts = 0;
static const int MAX_WIFI_CHECKS = 5;  // Check up to 5 times (10 seconds total)

// Timer callback to check WiFi after delay (with retries)
static void wifi_check_timer_cb(lv_timer_t * timer)
{
    wifi_check_attempts++;

    // Check WiFi status
    bool wifiConnected = isWiFiConnected();

    if (wifiConnected) {
        // WiFi connected - delete timer and proceed
        debugLog("[ui_Intro] WiFi CONNECTED!");
        lv_timer_del(timer);
        wifi_check_attempts = 0;  // Reset for next boot

        // Check Matter next
        bool matterEnabled = isMatterEnabled();
        debugLog(matterEnabled ? "[ui_Intro] Matter ENABLED" : "[ui_Intro] Matter DISABLED");

        if (!matterEnabled) {
            // Matter disabled - go directly to owl animation
            debugLog("[ui_Intro] -> OWL screen");
            _ui_screen_change(&ui_owl, LV_SCR_LOAD_ANIM_FADE_ON, 500, 6000, &ui_owl_screen_init);
        } else {
            // Matter enabled - check commissioning
            debugLog("[ui_Intro] -> QRCODE screen (Matter check)");
            _ui_screen_change(&ui_qrcode, LV_SCR_LOAD_ANIM_FADE_ON, 500, 6000, &ui_qrcode_screen_init);
        }
    } else if (wifi_check_attempts >= MAX_WIFI_CHECKS) {
        // Timeout - WiFi not connected after multiple attempts
        debugLog("[ui_Intro] WiFi timeout, showing AP QR");
        lv_timer_del(timer);
        wifi_check_attempts = 0;  // Reset for next boot

        // Show WiFi QR code (AP mode)
        debugLog("[ui_Intro] -> QRCODE screen (AP mode)");
        _ui_screen_change(&ui_qrcode, LV_SCR_LOAD_ANIM_FADE_ON, 500, 6000, &ui_qrcode_screen_init);
    } else {
        // Still waiting for WiFi, timer will fire again
        debugLog("[ui_Intro] WiFi check: not connected, waiting...");
    }
}

void ui_event_Intro(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_SCREEN_LOADED) {
        debugLog("[ui_Intro] Screen loaded, starting WiFi checks");
        wifi_check_attempts = 0;

        // Check WiFi status every 2 seconds, up to 5 times (10 seconds total)
        // Timer will auto-delete when WiFi connects or timeout
        lv_timer_create(wifi_check_timer_cb, 2000, NULL);
    }
}

void ui_event_logo(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_PRESSED) {
        logoPressed(e);
        _ui_opacity_set(ui_logo, 100);
    }
    if(event_code == LV_EVENT_RELEASED) {
        _ui_opacity_set(ui_logo, 255);
    }
}

// build funtions

void ui_Intro_screen_init(void)
{
    ui_Intro = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Intro, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Intro, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Intro, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_logo = lv_img_create(ui_Intro);
    lv_img_set_src(ui_logo, &ui_img_airowl_logo_png);
    lv_obj_set_width(ui_logo, LV_SIZE_CONTENT);   /// 250
    lv_obj_set_height(ui_logo, LV_SIZE_CONTENT);    /// 250
    lv_obj_set_align(ui_logo, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_logo, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_logo, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_firmwareversion = lv_label_create(ui_Intro);
    lv_obj_set_width(ui_firmwareversion, 100);
    lv_obj_set_height(ui_firmwareversion, 20);
    lv_obj_set_x(ui_firmwareversion, 0);
    lv_obj_set_y(ui_firmwareversion, -120);
    lv_obj_set_align(ui_firmwareversion, LV_ALIGN_CENTER);
    lv_label_set_text(ui_firmwareversion, "Version- 3.0.0");
    lv_obj_set_style_text_color(ui_firmwareversion, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_firmwareversion, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_firmwareversion, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_devicename = lv_label_create(ui_Intro);
    lv_obj_set_width(ui_devicename, LV_SIZE_CONTENT);   
    lv_obj_set_height(ui_devicename, LV_SIZE_CONTENT);    
    lv_obj_set_x(ui_devicename, 0);
    lv_obj_set_y(ui_devicename, 120);
    lv_obj_set_align(ui_devicename, LV_ALIGN_CENTER);
    lv_label_set_text(ui_devicename, "OIZOM AIROWL");
    lv_obj_set_style_text_color(ui_devicename, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_devicename, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_devicename, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_logo, ui_event_logo, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_Intro, ui_event_Intro, LV_EVENT_ALL, NULL);

}

void ui_Intro_screen_destroy(void)
{
    if(ui_Intro) lv_obj_del(ui_Intro);

    // NULL screen variables
    ui_Intro = NULL;
    ui_logo = NULL;
    ui_firmwareversion = NULL;
    ui_devicename = NULL;

}
