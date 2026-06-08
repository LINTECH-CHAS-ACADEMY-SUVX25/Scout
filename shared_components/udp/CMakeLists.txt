idf_component_register(
    SRCS "udp.c" "NetworkEsp32.cpp"
    INCLUDE_DIRS "."
    REQUIRES lwip scout_hal
)
