#include "ui.h"
#include "../../service/matter_service.h"

lv_obj_t * ui_matter = NULL;
lv_obj_t * ui_matter_label_top = NULL;
lv_obj_t * ui_matter_qrcode_obj = NULL;
lv_obj_t * ui_matter_label_bottom= NULL;

void ui_event_matter(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    
    if (event_code == LV_EVENT_SCREEN_LOADED) {
        // Auto-transition to owl animation after showing QR for 3 seconds
        _ui_screen_change(&ui_owl, LV_SCR_LOAD_ANIM_FADE_ON, 500, 3000, &ui_owl_screen_init);
    }
    else if (event_code == LV_EVENT_CLICKED) {
        // Manual skip to owl animation
        _ui_screen_change(&ui_owl, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, &ui_owl_screen_init);
    }
}

void ui_matter_screen_init(void)
{
    ui_matter = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_matter, LV_OBJ_FLAG_SCROLLABLE);

    ui_matter_label_top = lv_label_create(ui_matter);
    lv_label_set_text(ui_matter_label_top, "Matter Device");
    lv_obj_set_align(ui_matter_label_top, LV_ALIGN_TOP_MID);
    lv_obj_set_y(ui_matter_label_top, 20);

    const char* qrcode_data = get_matter_qrcode_data();
    ui_matter_qrcode_obj = lv_qrcode_create(ui_matter, 150, lv_color_black(), lv_color_white());
    lv_qrcode_update(ui_matter_qrcode_obj, qrcode_data, strlen(qrcode_data));
    lv_obj_center(ui_matter_qrcode_obj);

    const char* pairing_code = get_matter_pairing_code();
    ui_matter_label_bottom = lv_label_create(ui_matter);

    char pairing_label[64];
    snprintf(pairing_label, sizeof(pairing_label), "pairing code: %s", pairing_code);
    lv_label_set_text(ui_matter_label_bottom, pairing_label);

    lv_obj_set_align(ui_matter_label_bottom, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(ui_matter_label_bottom, -20);

    lv_obj_add_event_cb(ui_matter, ui_event_matter, LV_EVENT_ALL, NULL);
}

void ui_matter_screen_destroy(void)
{
    if(ui_matter) lv_obj_del(ui_matter);

    ui_matter = NULL;
    ui_matter_label_top = NULL;
    ui_matter_qrcode_obj = NULL;
    ui_matter_label_bottom= NULL;

}
