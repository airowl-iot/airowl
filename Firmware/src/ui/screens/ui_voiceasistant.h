#ifndef UI_VOICEASISTANT_H
#define UI_VOICEASISTANT_H

#ifdef __cplusplus
extern "C" {
#endif

// SCREEN: ui_voiceasistant
extern void ui_voiceasistant_screen_init(void);
extern void ui_voiceasistant_screen_destroy(void);
extern lv_obj_t * ui_voiceasistant;
extern void ui_event_speakerbutton(lv_event_t * e);
extern lv_obj_t * ui_speakerbutton;
// extern bool elato_active;
// CUSTOM VARIABLES

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

