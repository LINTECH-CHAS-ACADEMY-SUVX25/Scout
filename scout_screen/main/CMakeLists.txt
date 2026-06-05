idf_component_register(
    SRCS "main.c" "wifi.c" "stream.c" "render.c" "monitor.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_netif lwip rc_protocol udp display jpeg_decode lvgl_port driver esp_driver_uart
)
