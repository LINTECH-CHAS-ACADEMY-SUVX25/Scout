idf_component_register(SRCS "main.c"
                        INCLUDE_DIRS "."
                        REQUIRES rgb_lcd_port io_extension i2c
                                  esp_wifi nvs_flash esp_event esp_netif lwip)
