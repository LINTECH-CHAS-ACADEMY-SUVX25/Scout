idf_component_register(
    SRCS "frag_rx.c" "cam_cmd.c" "frame_buf.c" "main.c" "wifi_ap.c" "uart_console.c" "stream.c" "render.c" "monitor.c" "monitor_cmds.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_netif lwip rc_protocol udp display jpeg lvgl_port driver esp_driver_uart
)
