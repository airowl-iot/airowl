#ifndef _UI_MATTER_QRCODE_H
#define _UI_MATTER_QRCODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_matter_qrcode;
extern lv_obj_t *ui_matter_qr_title;
extern lv_obj_t *ui_matter_qr_widget;

void ui_matter_qrcode_screen_init(void);
void ui_matter_qrcode_screen_destroy(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif