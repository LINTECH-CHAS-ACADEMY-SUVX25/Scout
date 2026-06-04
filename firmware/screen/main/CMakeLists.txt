idf_component_register(
    SRCS "main.c" "wifi.c" "display.c" "lvgl_port.c" "ui.c" "stream.c" "render.c" "jpeg_decode.c"
    INCLUDE_DIRS "."
    REQUIRES rgb_lcd_port lvgl touch esp_wifi nvs_flash esp_netif lwip
             espressif__esp_new_jpeg rc_protocol
)
