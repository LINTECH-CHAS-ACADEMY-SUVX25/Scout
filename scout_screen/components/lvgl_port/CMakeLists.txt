set(LVGL_DIR ${CMAKE_CURRENT_LIST_DIR}/lvgl__lvgl)
file(GLOB_RECURSE LVGL_SRCS ${LVGL_DIR}/src/*.c)

idf_component_register(
    SRCS "lvgl_port.c" "lvgl_port_ui.c" "press_start_2p_8.c" "press_start_2p_96.c" ${LVGL_SRCS}
    INCLUDE_DIRS "." "${LVGL_DIR}" "${LVGL_DIR}/src"
    REQUIRES display touch rgb_lcd_port rc_protocol esp_timer
)

# LV_LVGL_H_INCLUDE_SIMPLE — the generated font files include "lvgl.h" via this guard
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_CONF_INCLUDE_SIMPLE" "-DLV_LVGL_H_INCLUDE_SIMPLE")
