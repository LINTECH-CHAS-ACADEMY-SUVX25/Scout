idf_component_register(
    SRCS "display.c" "DisplayEsp32.cpp"
    INCLUDE_DIRS "."
    REQUIRES touch rgb_lcd_port scout_hal
)
