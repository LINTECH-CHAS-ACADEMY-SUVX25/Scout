idf_component_register(
    SRCS "main.c" "stream.c" "render.c" "monitor.c"
         "adapters/frame_buf.c" "adapters/cam_cmd.c" "adapters/frag_rx.c" "adapters/monitor_cmds.c"
    INCLUDE_DIRS "." "adapters"
    REQUIRES lwip rc_protocol udp display jpeg lvgl_port uart_console wifi_ap watchdog
)
