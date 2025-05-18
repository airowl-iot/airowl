SET(SOURCES screens/ui_Intro.c
    screens/ui_qrcode.c
    screens/ui_owl.c
    screens/ui_dashboard.c
    screens/ui_PM1graph.c
    screens/ui_PM4graph.c
    screens/ui_PM25graph.c
    screens/ui_PM10graph.c
    screens/ui_TVOCgraph.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_loho2_png.c
    images/ui_img_airowl_1_png.c
    images/ui_img_582762761.c
    fonts/ui_font_clock24.c
    fonts/ui_font_clock48.c
    fonts/ui_font_clock_66.c)

# Register the component with ESP-IDF
idf_component_register(SRCS "${SOURCES}"
                       INCLUDE_DIRS "."
                       REQUIRES lvgl)