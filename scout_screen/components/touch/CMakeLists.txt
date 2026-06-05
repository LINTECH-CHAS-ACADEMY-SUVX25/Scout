if(IDF_TARGET STREQUAL "esp32s3")
    idf_component_register(SRCS "gt911.c" "touch.c"
                            INCLUDE_DIRS "."
                            REQUIRES driver esp_lcd i2c gpio io_extension rgb_lcd_port)
else()
    idf_component_register(INCLUDE_DIRS ".")
endif()
