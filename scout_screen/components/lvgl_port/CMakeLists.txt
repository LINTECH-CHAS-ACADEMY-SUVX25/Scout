idf_component_register(
    SRCS "lvgl_port.c" "ui.c"
    INCLUDE_DIRS "."
    REQUIRES display touch lvgl rgb_lcd_port rc_protocol esp_timer
)
