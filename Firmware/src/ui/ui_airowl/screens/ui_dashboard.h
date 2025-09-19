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
extern lv_obj_t * ui_eCO2Container;
extern void ui_event_eCO2Button(lv_event_t * e);
extern lv_obj_t * ui_eCO2Button;
extern lv_obj_t * ui_eCO2label;
extern lv_obj_t * ui_eCO2unit;
extern lv_obj_t * ui_eCO2value;
extern lv_obj_t * ui_tvocContainer;
extern void ui_event_TVOCButton(lv_event_t * e);
extern lv_obj_t * ui_TVOCButton;
extern lv_obj_t * ui_tvoclabel;
extern lv_obj_t * ui_tvocunit;
extern lv_obj_t * ui_tvocvalue;
extern lv_obj_t * ui_tempContainer;
extern lv_obj_t * ui_tempunit;
extern void ui_event_tempButton(lv_event_t * e);
extern lv_obj_t * ui_tempButton;
extern lv_obj_t * ui_tempvalue;
extern lv_obj_t * ui_templabel;
extern lv_obj_t * ui_humdContainer;
extern void ui_event_humdButton(lv_event_t * e);
extern lv_obj_t * ui_humdButton;
extern lv_obj_t * ui_humdlabel;
extern lv_obj_t * ui_humdunit;
extern lv_obj_t * ui_humdcube;
extern lv_obj_t * ui_humdvalue;

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

