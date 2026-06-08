idf_component_register(
    SRCS "motor.c" "motor_task.cpp" "MotorEsp32.cpp"
    INCLUDE_DIRS "."
    REQUIRES rc_protocol esp_driver_gpio freertos scout_hal
)
