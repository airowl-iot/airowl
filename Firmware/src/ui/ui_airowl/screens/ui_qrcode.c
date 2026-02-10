#include "ui.h"
#include "../../service/matter_service.h"

lv_obj_t * ui_qrcode = NULL;
lv_obj_t * ui_qrcodename = NULL;
lv_obj_t * ui_name = NULL;
lv_obj_t * ui_IPlabel = NULL;

// Forward declarations for C++ helper functions (defined in ui_manager.cpp)
#ifdef __cplusplus
extern "C" {
#endif
    bool isMatterEnabled(void);
#ifdef __cplusplus
}
#endif

void ui_event_qrcode(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_SCREEN_LOADED) {
        // Check if Matter is enabled using C-compatible helper
        bool matterEnabled = isMatterEnabled();

        if (!matterEnabled) {
            // Matter disabled - skip to owl animation
            _ui_screen_change(&ui_owl, LV_SCR_LOAD_ANIM_FADE_ON, 500, 8000, &ui_owl_screen_init);
        } else {
            // Matter enabled - check commissioning status
            bool commissioned = is_matter_commissioned();

            if (commissioned) {
                // Already commissioned - go directly to owl animation
                _ui_screen_change(&ui_owl, LV_SCR_LOAD_ANIM_FADE_ON, 500, 8000, &ui_owl_screen_init);
            } else {
                // Not commissioned - show Matter QR code
                _ui_screen_change(&ui_matter, LV_SCR_LOAD_ANIM_FADE_ON, 500, 8000, &ui_matter_screen_init);
            }
        }
    }
}

void ui_qrcode_screen_init(void)
{
    ui_qrcode = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_qrcode, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_qrcode, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_qrcode, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_qrcodename = lv_label_create(ui_qrcode);
    lv_obj_set_width(ui_qrcodename, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_qrcodename, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_qrcodename, 0);
    lv_obj_set_y(ui_qrcodename, 117);
    lv_obj_set_align(ui_qrcodename, LV_ALIGN_CENTER);
    lv_label_set_text(ui_qrcodename, "");
    lv_obj_set_style_text_color(ui_qrcodename, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_qrcodename, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_qrcodename, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_IPlabel = lv_label_create(ui_qrcode);
    lv_obj_set_width(ui_IPlabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_IPlabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_IPlabel, 0);
    lv_obj_set_y(ui_IPlabel, -116);
    lv_obj_set_align(ui_IPlabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_IPlabel, "--");
    lv_obj_set_style_text_color(ui_IPlabel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_IPlabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_IPlabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_qrcode, ui_event_qrcode, LV_EVENT_ALL, NULL);

}

void ui_qrcode_screen_destroy(void)
{
    if(ui_qrcode) lv_obj_del(ui_qrcode);

    ui_qrcode = NULL;
    ui_qrcodename = NULL;
    ui_name = NULL;
    ui_IPlabel = NULL;
}
