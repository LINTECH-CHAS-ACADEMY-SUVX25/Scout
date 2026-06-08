idf_component_register(
    SRCS "wifi_ap.c"
    INCLUDE_DIRS "."
    REQUIRES rc_protocol esp_wifi nvs_flash esp_event esp_netif
)
