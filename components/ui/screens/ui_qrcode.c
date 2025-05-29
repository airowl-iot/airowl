#include <string.h>
#include "../ui.h"

lv_obj_t * ui_qrcode;
lv_obj_t * ui_qrcodename;
lv_obj_t * ui_qrcode_widget; // Added for QR code widget

// event functions
void ui_event_qrcode(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_SCREEN_LOADED) {
        _ui_screen_change(&ui_owl, LV_SCR_LOAD_ANIM_FADE_ON, 500, 8000, &ui_owl_screen_init);
    }
}

// build functions
void ui_qrcode_screen_init(void)
{
    ui_qrcode = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_qrcode, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_qrcode, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_qrcode, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create QR code widget
    ui_qrcode_widget = lv_qrcode_create(ui_qrcode);
    lv_obj_set_size(ui_qrcode_widget, 200, 200); // Set QR code size
    lv_obj_set_style_bg_color(ui_qrcode_widget, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT); // White background
    lv_obj_set_style_text_color(ui_qrcode_widget, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT); // Black foreground
    lv_obj_set_align(ui_qrcode_widget, LV_ALIGN_CENTER);
    lv_obj_set_y(ui_qrcode_widget, -20); // Position above the label
    const char * url = "https://opendata.oizom.com/";
    lv_qrcode_update(ui_qrcode_widget, url, strlen(url));

    ui_qrcodename = lv_label_create(ui_qrcode);
    lv_obj_set_width(ui_qrcodename, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_qrcodename, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_qrcodename, 0);
    lv_obj_set_y(ui_qrcodename, 99);
    lv_obj_set_align(ui_qrcodename, LV_ALIGN_CENTER);
    lv_label_set_text(ui_qrcodename, "Scan to visit Oizom Data");
    lv_obj_set_style_text_font(ui_qrcodename, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_qrcode, ui_event_qrcode, LV_EVENT_ALL, NULL);
}

void ui_qrcode_screen_destroy(void)
{
    if(ui_qrcode) {
        lv_obj_del(ui_qrcode);
    }

    // NULL screen variables
    ui_qrcode = NULL;
    ui_qrcodename = NULL;
    ui_qrcode_widget = NULL; // Clear QR code widget
}