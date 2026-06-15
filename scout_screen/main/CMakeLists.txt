idf_component_register(
    SRCS "main.c" "stream.c" "render.c" "monitor.c"
         "adapters/frame_pool.c" "adapters/screen_state.c" "adapters/screen_stats.c" "adapters/scene.c" "adapters/rc_tx.c" "adapters/frag_rx.c"
         "adapters/console.c"
    INCLUDE_DIRS "." "adapters"
    REQUIRES lwip rc_protocol udp display jpeg lvgl_port ui wifi_ap watchdog ring_buffer esp_driver_uart
)
