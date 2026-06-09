idf_component_register(
    SRCS "l298n.c" "MotorEsp32.cpp"
    INCLUDE_DIRS "."
    REQUIRES rc_protocol esp_driver_gpio scout_hal
)
