#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

extern void ui_dashboard_screen_init(void);
extern void ui_dashboard_screen_destroy(void);
extern void ui_event_dashboard(lv_event_t * e);
extern lv_obj_t * ui_dashboard;
extern lv_obj_t * ui_headerContainer;
extern void ui_event_Button3(lv_event_t * e);
extern lv_obj_t * ui_Button3;
extern lv_obj_t * ui_time;
extern lv_obj_t * ui_date;
extern lv_obj_t * ui_wifi;
extern void ui_event_PM1Button(lv_event_t * e);
extern lv_obj_t * ui_pm1Button;
extern lv_obj_t * ui_pm1label;
extern lv_obj_t * ui_pm1unit;
extern lv_obj_t * ui_pm1cube;
extern lv_obj_t * ui_pm1value;
extern lv_obj_t * ui_pm1Container;
extern void ui_event_PM25Button(lv_event_t * e);
extern lv_obj_t * ui_pm25Button;
extern lv_obj_t * ui_pm25label;
extern lv_obj_t * ui_pm25unit;
extern lv_obj_t * ui_pm25cube;
extern lv_obj_t * ui_pm25value;
extern lv_obj_t * ui_pm25Container;
extern lv_obj_t * ui_pm10Container;
extern void ui_event_PM10Button(lv_event_t * e);
extern lv_obj_t * ui_pm10Button;
extern lv_obj_t * ui_pm10label;
extern lv_obj_t * ui_pm10unit;
extern lv_obj_t * ui_pm10cube;
extern lv_obj_t * ui_pm10value;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

