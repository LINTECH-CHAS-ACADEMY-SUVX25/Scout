idf_component_register(
    SRCS "main.c" "stream.c" "motor.cpp"
         "adapters/frag_tx.c" "adapters/motor_queue.c" "adapters/cam_state.c" "adapters/rc_rx.c"
    INCLUDE_DIRS "." "adapters"
    REQUIRES rc_protocol udp camera motor_HAL wifi_sta watchdog bme280 reset_info
)
