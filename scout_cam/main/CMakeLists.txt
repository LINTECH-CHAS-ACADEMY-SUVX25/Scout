idf_component_register(
    SRCS "main.c" "wifi_sta.c" "frag_tx.c" "udp_stream.c" "monitor.c" "telemetry.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip rc_protocol udp camera motor_driver
)
