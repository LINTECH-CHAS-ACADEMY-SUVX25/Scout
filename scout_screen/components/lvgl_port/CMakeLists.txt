set(LVGL_DIR ${CMAKE_CURRENT_LIST_DIR}/lvgl__lvgl)
file(GLOB_RECURSE LVGL_SRCS ${LVGL_DIR}/src/*.c)

idf_component_register(
    SRCS "lvgl_port.c" ${LVGL_SRCS}
    INCLUDE_DIRS "." "${LVGL_DIR}" "${LVGL_DIR}/src"
    REQUIRES display touch rgb_lcd_port rc_protocol esp_timer
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_CONF_INCLUDE_SIMPLE")
