#pragma once

#include <Arduino.h>

#ifdef CONFIG_ENABLE_LVGL
#include "ui/ui.h"
#endif

//  #define NTP_TIMEZONE  "IST-5:30"
//  #define NTP_SERVER1   "0.pool.ntp.org"
//  #define NTP_SERVER2   "1.pool.ntp.org"
//  #define NTP_SERVER3   "2.pool.ntp.org"

 #if __has_include (<esp_sntp.h>)
  #include <esp_sntp.h>
  #define SNTP_ENABLED 1
 #elif __has_include (<sntp.h>)
  #include <sntp.h>
  #define SNTP_ENABLED 1
 #endif

#ifndef SNTP_ENABLED
#define SNTP_ENABLED 0
#endif

#include <WiFi.h>
#include <WiFiManagerTz.h>


void time_init()
{
    if(WiFi.status() == WL_CONNECTED){
        WiFiManagerNS::configTime();
        #ifdef CONFIG_ENABLE_LVGL
            lv_img_set_src(ui_nose, &ui_img_airowl_2_png);
        #endif
    }
    else
    {
        #ifdef CONFIG_ENABLE_LVGL
            lv_img_set_src(ui_nose, &ui_img_airowl_1_png);
        #endif
    }
}

void update_time()
{   
    auto t = time(nullptr);
    auto tm = localtime(&t); 

    char time_buf[9];

    // Format the time string
    sprintf(time_buf, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

    #ifdef CONFIG_ENABLE_LVGL
        lv_label_set_text(ui_time, time_buf);
    #endif
}