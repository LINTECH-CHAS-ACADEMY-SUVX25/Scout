idf_component_register(
    SRCS "main.c" "wifi.c" "motor.c" "camera.c" "stream.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip esp_driver_gpio rc_protocol
)
