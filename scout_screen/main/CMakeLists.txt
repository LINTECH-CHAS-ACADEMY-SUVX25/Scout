idf_component_register(
    SRCS "main.c" "wifi_ap.c" "uart_console.c" "stream.c" "render.c" "monitor.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_netif lwip rc_protocol transport display jpeg lvgl_port driver esp_driver_uart
)
