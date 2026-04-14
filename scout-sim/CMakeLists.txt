cmake_minimum_required(VERSION 3.14)
project(scout_sim C)

set(CMAKE_C_STANDARD 11)

# ── Paths ──
set(LVGL_DIR     ${CMAKE_SOURCE_DIR}/lvgl       CACHE PATH "Path to lvgl repo")
set(LV_DRIVERS   ${CMAKE_SOURCE_DIR}/lv_drivers  CACHE PATH "Path to lv_drivers repo")

# ── LVGL config ──
set(LV_CONF_PATH ${CMAKE_SOURCE_DIR}/lv_conf.h CACHE STRING "" FORCE)

# ── LVGL core ──
add_subdirectory(${LVGL_DIR} lvgl)

# ── SDL2 ──
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# ── lv_drivers (SDL backend) ──
file(GLOB_RECURSE LV_DRV_SRC ${LV_DRIVERS}/sdl/*.c)

# ── Scout sources ──
set(SCOUT_SRC
    src/main.c
    src/sensor_sim.c
    src/ui_dashboard.c
)

add_executable(scout_sim ${SCOUT_SRC} ${LV_DRV_SRC})

target_include_directories(scout_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${LVGL_DIR}
    ${LV_DRIVERS}
    ${SDL2_INCLUDE_DIRS}
)

target_compile_definitions(scout_sim PRIVATE
    SCOUT_SIM=1
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
)

target_link_libraries(scout_sim PRIVATE
    lvgl
    ${SDL2_LIBRARIES}
    m
)
