SET(SOURCES ui_Intro.c
    ui_qrcode.c
    ui_owl.c
    ui_dashboard.c
    ui_PM1graph.c
    ui_PM4graph.c
    ui_PM25graph.c
    ui_PM10graph.c
    ui_TVOCgraph.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    ui_img_oizom_logo_png.c
    ui_img_airowl_1_png.c
    ui_img_582762761.c)

add_library(ui ${SOURCES})
