cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    "../../shared_components"
    "../../scout_cam/components"
    "../../scout_cam/components/motor_HAL"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_motor)
