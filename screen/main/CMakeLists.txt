idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "."
                        REQUIRES rgb_lcd_port lvgl touch esp_wifi nvs_flash esp_netif lwip)
