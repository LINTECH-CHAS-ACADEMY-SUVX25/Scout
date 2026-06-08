idf_component_register(
    SRCS "main.c" "stream.c" "motor.c"
         "adapters/frag_tx.c" "adapters/motor_cmd.c"
    INCLUDE_DIRS "." "adapters"
    REQUIRES rc_protocol udp camera motor_driver wifi_sta wdt
)
