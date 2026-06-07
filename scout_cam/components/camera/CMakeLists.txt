idf_component_register(
    SRCS "camera.c" "CameraEsp32.cpp"
    INCLUDE_DIRS "."
    REQUIRES espressif__esp32-camera scout_hal
)
