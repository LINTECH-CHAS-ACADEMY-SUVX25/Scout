idf_component_register(
    SRCS "main.c" "stream.c" "render.c" "monitor.c"
         "adapters/frame_pool.c" "adapters/screen_state.c" "adapters/scene.c" "adapters/rc_tx.c" "adapters/frag_rx.c" "adapters/monitor_cmds.c" "adapters/cam_diag.c"
    INCLUDE_DIRS "." "adapters"
    REQUIRES lwip rc_protocol udp display jpeg lvgl_port ui uart_console wifi_ap watchdog cam_diag_fmt ring_buffer
)
