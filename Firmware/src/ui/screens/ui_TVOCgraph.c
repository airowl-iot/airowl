
#include "../ui.h"

lv_obj_t * ui_TVOCgraph;
lv_obj_t * ui_TVOCchart;
lv_obj_t * ui_graphparameter4;
lv_obj_t * ui_Container16;
lv_obj_t * ui_gpunit4;
lv_obj_t * ui_return4;
lv_obj_t * ui_returnlabel4;
lv_obj_t * ui_graphparameter12;
lv_obj_t * ui_graphparameter15;
lv_obj_t * ui_tvocavg;
lv_obj_t * ui_tvocmax1;
lv_obj_t * ui_tvocmax2;

// event funtions
void ui_event_return4(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if(event_code == LV_EVENT_CLICKED) {
        _ui_screen_change(&ui_dashboard, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, &ui_dashboard_screen_init);
    }
}


void ui_TVOCgraph_screen_init(void)
{
    ui_TVOCgraph = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_TVOCgraph, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_TVOCgraph, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TVOCgraph, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_TVOCchart = lv_chart_create(ui_TVOCgraph);
    lv_obj_set_width(ui_TVOCchart, 201);
    lv_obj_set_height(ui_TVOCchart, 195);
    lv_obj_set_x(ui_TVOCchart, 14);
    lv_obj_set_y(ui_TVOCchart, 4);
    lv_obj_set_align(ui_TVOCchart, LV_ALIGN_CENTER);
    lv_chart_set_type(ui_TVOCchart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(ui_TVOCchart, 12);
    lv_chart_set_range(ui_TVOCchart, LV_CHART_AXIS_PRIMARY_Y, 500, 1800);
    lv_chart_set_div_line_count(ui_TVOCchart, 0, 0);
    lv_chart_set_zoom_x(ui_TVOCchart, 250);
    lv_chart_set_zoom_y(ui_TVOCchart, 250);
    lv_chart_set_axis_tick(ui_TVOCchart, LV_CHART_AXIS_PRIMARY_X, 0, 0, 6, 1, false, 20);
    lv_chart_set_axis_tick(ui_TVOCchart, LV_CHART_AXIS_PRIMARY_Y, 4, 2, 6, 3, true, 45);
    lv_chart_set_axis_tick(ui_TVOCchart, LV_CHART_AXIS_SECONDARY_Y, 0, 0, 0, 0, false, 0);
    lv_obj_set_style_bg_color(ui_TVOCchart, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TVOCchart, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_TVOCchart, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_TVOCchart, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_TVOCchart, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_TVOCchart, lv_color_hex(0xFFFFFF), LV_PART_TICKS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_TVOCchart, 255, LV_PART_TICKS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TVOCchart, &lv_font_montserrat_16, LV_PART_TICKS | LV_STATE_DEFAULT);

    ui_Container16 = lv_obj_create(ui_TVOCgraph);
    lv_obj_remove_style_all(ui_Container16);
    lv_obj_set_width(ui_Container16, 238);
    lv_obj_set_height(ui_Container16, 59);
    lv_obj_set_x(ui_Container16, 1);
    lv_obj_set_y(ui_Container16, -129);
    lv_obj_set_align(ui_Container16, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Container16, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_border_color(ui_Container16, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_Container16, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Container16, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_graphparameter4 = lv_label_create(ui_Container16);
    lv_obj_set_width(ui_graphparameter4, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_graphparameter4, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_graphparameter4, -74);
    lv_obj_set_y(ui_graphparameter4, 2);
    lv_obj_set_align(ui_graphparameter4, LV_ALIGN_CENTER);
    lv_label_set_text(ui_graphparameter4, "PM10.0");
    lv_obj_set_style_text_color(ui_graphparameter4, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_graphparameter4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_graphparameter4, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_graphparameter4, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_gpunit4 = lv_label_create(ui_Container16);
    lv_obj_set_width(ui_gpunit4, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_gpunit4, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_gpunit4, -4);
    lv_obj_set_y(ui_gpunit4, 2);
    lv_obj_set_align(ui_gpunit4, LV_ALIGN_CENTER);
    lv_label_set_text(ui_gpunit4, "ug/m");
    lv_obj_set_style_text_color(ui_gpunit4, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_gpunit4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_gpunit4, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);


    ui_tvocmax1 = lv_label_create(ui_Container16);
    lv_obj_set_width(ui_tvocmax1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_tvocmax1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_tvocmax1, 49);
    lv_obj_set_y(ui_tvocmax1, 2);
    lv_obj_set_align(ui_tvocmax1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_tvocmax1, "max");
    lv_obj_set_style_text_color(ui_tvocmax1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_tvocmax1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_tvocmax1, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_tvocmax2 = lv_label_create(ui_Container16);
    lv_obj_set_width(ui_tvocmax2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_tvocmax2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_tvocmax2, 85);
    lv_obj_set_y(ui_tvocmax2, 1);
    lv_obj_set_align(ui_tvocmax2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_tvocmax2, "-");
    lv_obj_set_style_text_color(ui_tvocmax2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_tvocmax2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_tvocmax2, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_return4 = lv_btn_create(ui_TVOCgraph);
    lv_obj_set_width(ui_return4, 160);
    lv_obj_set_height(ui_return4, 39);
    lv_obj_set_x(ui_return4, 0);
    lv_obj_set_y(ui_return4, 127);
    lv_obj_set_align(ui_return4, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_return4, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_return4, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_return4, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_return4, lv_color_hex(0x41B4D1), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_return4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_returnlabel4 = lv_label_create(ui_return4);
    lv_obj_set_width(ui_returnlabel4, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_returnlabel4, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_returnlabel4, LV_ALIGN_CENTER);
    lv_label_set_text(ui_returnlabel4, "RETURN");
    lv_obj_set_style_text_color(ui_returnlabel4, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_returnlabel4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_returnlabel4, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_return4, ui_event_return3, LV_EVENT_ALL, NULL);

}


void ui_TVOCgraph_screen_destroy(void)
{
    if(ui_TVOCgraph) lv_obj_del(ui_TVOCgraph);

    // NULL screen variables
    ui_TVOCgraph = NULL;
    ui_TVOCchart = NULL;
    ui_graphparameter4 = NULL;
    ui_Container16 = NULL;
    ui_gpunit4 = NULL;
    ui_return4 = NULL;
    ui_returnlabel4 = NULL;
    ui_graphparameter12 = NULL;
    ui_graphparameter15 = NULL;
    ui_tvocavg = NULL;
    ui_tvocmax = NULL;

}
