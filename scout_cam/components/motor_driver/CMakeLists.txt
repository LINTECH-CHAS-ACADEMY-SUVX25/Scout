idf_component_register(
    SRCS "motor.c" "motor_cmd.c" "motor_task.c"
    INCLUDE_DIRS "."
    REQUIRES rc_protocol esp_driver_gpio freertos
)
