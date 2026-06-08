idf_component_register(
    SRCS "main.cpp" "wifi_ap.c" "uart_console.c" "stream.c" "render.cpp" "monitor.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_netif lwip rc_protocol udp display jpeg lvgl_port driver esp_driver_uart
)
