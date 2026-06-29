cmake_minimum_required(VERSION 3.16)
project(scout_tests C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.0
)
FetchContent_MakeAvailable(unity)

set(HAL_DIR ${CMAKE_SOURCE_DIR}/../shared_components/scout_hal)

add_executable(scout_tests
    main.cpp
    test_motor.cpp
    test_camera.cpp
    test_display.cpp
    test_network.cpp
    test_bme280.cpp
    ${HAL_DIR}/mock/MotorMock.cpp
    ${HAL_DIR}/mock/CameraMock.cpp
    ${HAL_DIR}/mock/DisplayMock.cpp
    ${HAL_DIR}/mock/NetworkMock.cpp
    ${HAL_DIR}/mock/Bme280Mock.cpp
)

target_include_directories(scout_tests PRIVATE
    ${HAL_DIR}
    ${HAL_DIR}/mock
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(scout_tests PRIVATE unity)
