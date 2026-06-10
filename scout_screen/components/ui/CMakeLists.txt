idf_component_register(
    SRCS "scout_ui.c" "fonts/press_start_2p_8.c" "fonts/press_start_2p_96.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl_port display rc_protocol
)
