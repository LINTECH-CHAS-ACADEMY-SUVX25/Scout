idf_component_register(
    SRCS "main.c" "stream.c" "motor.c"
         "adapters/frag_tx.c" "adapters/motor_cmd.c"
    INCLUDE_DIRS "." "adapters"
    REQUIRES rc_protocol udp camera l298n wifi_sta watchdog
)
