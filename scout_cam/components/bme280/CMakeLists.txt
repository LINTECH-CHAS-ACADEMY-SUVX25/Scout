idf_component_register(
    SRCS "bme280.c" "Bme280.cpp"
    INCLUDE_DIRS "." "../../../shared_components/scout_hal"
    REQUIRES esp_driver_i2c
)
