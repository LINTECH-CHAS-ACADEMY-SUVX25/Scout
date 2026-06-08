idf_component_register(
    SRCS "l298n.c"
    INCLUDE_DIRS "."
    REQUIRES rc_protocol esp_driver_gpio
)
