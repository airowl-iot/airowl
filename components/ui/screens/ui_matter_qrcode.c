#include <string.h>
#include "../ui.h"

lv_obj_t *ui_matter_qrcode;
lv_obj_t *ui_matter_qr_title;
lv_obj_t *ui_matter_qr_widget; // QR code widget

void ui_event_matter_qrcode(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if (event_code == LV_EVENT_SCREEN_LOADED) {
        // Transition to dashboard after 7 seconds if commissioning is active, otherwise immediately
        extern bool g_commissioning_window_open;
        _ui_screen_change(&ui_dashboard, LV_SCR_LOAD_ANIM_FADE_ON, 500, g_commissioning_window_open ? 7000 : 0, &ui_dashboard_screen_init);
    }
}

void ui_matter_qrcode_screen_init(void)
{
    ui_matter_qrcode = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_matter_qrcode, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_matter_qrcode, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_matter_qrcode, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create QR code widget
    ui_matter_qr_widget = lv_qrcode_create(ui_matter_qrcode);
    lv_obj_set_size(ui_matter_qr_widget, 200, 200); // Set QR code size
    lv_obj_set_style_bg_color(ui_matter_qr_widget, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT); // White background
    lv_obj_set_style_text_color(ui_matter_qr_widget, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT); // Black foreground
    lv_obj_set_align(ui_matter_qr_widget, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_matter_qr_widget, -20); // Position above the label
    const char *matter_payload = "MT:Y.K9042C00KA0648G00";
    lv_qrcode_update(ui_matter_qr_widget, matter_payload, strlen(matter_payload));

    // Create label below QR code
    ui_matter_qr_title = lv_label_create(ui_matter_qrcode);
    lv_obj_set_width(ui_matter_qr_title, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_matter_qr_title, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_matter_qr_title, 0);
    lv_obj_set_y(ui_matter_qr_title, 99); // Position below QR code
    lv_obj_set_align(ui_matter_qr_title, LV_ALIGN_CENTER);
    lv_label_set_text(ui_matter_qr_title, "Scan to pair with Matter");
    lv_obj_set_style_text_color(ui_matter_qr_title, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_matter_qr_title, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_matter_qr_title, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_matter_qrcode, ui_event_matter_qrcode, LV_EVENT_ALL, NULL);
}

void ui_matter_qrcode_screen_destroy(void)
{
    if (ui_matter_qrcode) {
        lv_obj_del(ui_matter_qrcode);
    }

    ui_matter_qrcode = NULL;
    ui_matter_qr_title = NULL;
    ui_matter_qr_widget = NULL;
}