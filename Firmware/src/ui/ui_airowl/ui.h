#ifndef _SQUARELINE_PROJECT_UI_H
#define _SQUARELINE_PROJECT_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined __has_include
#if __has_include("lvgl.h")
#include "lvgl.h"
#elif __has_include("lvgl/lvgl.h")
#include "lvgl/lvgl.h"
#else
#include "lvgl.h"
#endif
#else
#include "lvgl.h"
#endif

#include "ui_helpers.h"
#include "ui_events.h"

///////////////////// SCREENS ////////////////////

#include "screens/ui_dashboard.h"
#include "screens/ui_Intro.h"
#include "screens/ui_qrcode.h"
#include "screens/ui_owl.h"
#include "screens/ui_dashboard.h"
#include "screens/ui_Tempgraph.h"
#include "screens/ui_Humdgraph.h"
#include "screens/ui_PM25graph.h"
#include "screens/ui_PM10graph.h"
#include "screens/ui_TVOCgraph.h"
#include "screens/ui_eCO2graph.h"
#include "screens/ui_ota.h"

///////////////////// VARIABLES ////////////////////

extern void blink_Animation(lv_obj_t * TargetObject, int delay);
extern void open_Animation(lv_obj_t * TargetObject, int delay);
extern void blinkx_Animation(lv_obj_t * TargetObject, int delay);
extern void openx_Animation(lv_obj_t * TargetObject, int delay);
extern void eyeR_Animation(lv_obj_t * TargetObject, int delay);
extern void eyeL_Animation(lv_obj_t * TargetObject, int delay);
extern void eyeright_Animation(lv_obj_t * TargetObject, int delay);
extern void eyeleft_Animation(lv_obj_t * TargetObject, int delay);

// EVENTS

extern lv_obj_t * ui____initial_actions0;

// IMAGES AND IMAGE SETS
LV_IMG_DECLARE(ui_img_oizom_logo_png);    // assets/oizom_logo.png
LV_IMG_DECLARE(ui_img_airowl_1_png);    // assets/airowl_1.png
LV_IMG_DECLARE(ui_img_airowl_2_png);    // assets/airowl_2.png
LV_IMG_DECLARE(ui_img_582762761);    // assets/celsius-degrees-symbol-of-temperature.png
LV_IMG_DECLARE(ui_img_26368195); 
LV_IMG_DECLARE(ui_img_26368195); 
LV_IMG_DECLARE(ui_img_wifi_off_png);
LV_IMG_DECLARE(ui_img_wifi_on_png);
LV_IMG_DECLARE(ui_img_ota_icon_png);

// UI INIT
void ui_init(void);
void ui_destroy(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
