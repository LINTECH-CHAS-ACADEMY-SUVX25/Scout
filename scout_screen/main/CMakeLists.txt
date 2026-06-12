idf_component_register(
    SRCS "main.c" "stream.c" "render.c" "monitor.c"
         "adapters/frame_buf.c" "adapters/ring_buffer.c" "adapters/screen_state.c" "adapters/cam_cmd.c" "adapters/frag_rx.c" "adapters/monitor_cmds.c" "adapters/cam_diag.c"
    INCLUDE_DIRS "." "adapters"
    REQUIRES lwip rc_protocol udp display jpeg lvgl_port ui uart_console wifi_ap watchdog
)
