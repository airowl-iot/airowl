#ifndef UI_MATTER_H
#define UI_MATTER_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_matter_screen_init(void);
extern void ui_matter_screen_destroy(void);
extern lv_obj_t *ui_matter;
extern lv_obj_t *ui_matter_label_top;
extern lv_obj_t *ui_matter_qrcode_obj;
extern lv_obj_t *ui_matter_label_bottom;
void ui_event_matter_(lv_event_t * e);
extern const char *ui_matter_qrcodedata;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

