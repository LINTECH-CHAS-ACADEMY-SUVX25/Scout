cmake_minimum_required(VERSION 3.16)
project(scout_tests C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(HAL_DIR ${CMAKE_SOURCE_DIR}/../shared_components/scout_hal)

add_executable(scout_tests
    main.cpp
    test_motor.cpp
    test_camera.cpp
    test_network.cpp
    test_display.cpp
    unity/unity.c
    ${HAL_DIR}/mock/MotorMock.cpp
    ${HAL_DIR}/mock/CameraMock.cpp
    ${HAL_DIR}/mock/NetworkMock.cpp
    ${HAL_DIR}/mock/DisplayMock.cpp
)

target_include_directories(scout_tests PRIVATE
    ${HAL_DIR}
    ${HAL_DIR}/mock
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/unity
)
