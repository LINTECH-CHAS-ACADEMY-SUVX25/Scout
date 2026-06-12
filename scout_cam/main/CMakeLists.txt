idf_component_register(
    SRCS "main.c" "stream.c" "motor.c" "telemetry.c"
         "adapters/frag_tx.c" "adapters/motor_cmd.c" "adapters/cam_state.c"
    INCLUDE_DIRS "." "adapters"
    REQUIRES rc_protocol udp camera l298n wifi_sta watchdog
)
