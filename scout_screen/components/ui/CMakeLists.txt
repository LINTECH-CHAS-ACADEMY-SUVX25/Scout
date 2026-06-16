idf_component_register(
    SRCS
        "scout_ui.c"
        "themes.c"
        "topbar.c"
        "botbar.c"
        "tele.c"
        "joy.c"
        "cam.c"
        "config.c"
        "menu.c"
        "intro.c"
        "fonts/press_start_2p_8.c"
        "fonts/press_start_2p_24.c"
        "fonts/press_start_2p_96.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl_port display rc_protocol
)
