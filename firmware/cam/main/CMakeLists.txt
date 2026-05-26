idf_component_register(
    SRCS "main.c" "wifi.c" "motor.c" "camera.c" "motor_task.c" "network_task.c" "monitor.c" "telemetry.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip esp_driver_gpio rc_protocol
)
