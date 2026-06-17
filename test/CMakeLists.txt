cmake_minimum_required(VERSION 3.16)
project(scout_tests C)
set(CMAKE_C_STANDARD 11)

add_executable(scout_tests
    main.c
    test_frag_tx.c
    test_frag_rx.c
    test_joy_to_motor.c
    unity/unity.c
    stubs/udp_stub.c
    stubs/frame_pool_stub.c
    stubs/motor_queue_stub.c
    stubs/l298n_stub.c
    stubs/watchdog_stub.c
    ../scout_cam/main/adapters/frag_tx.c
    ../scout_screen/main/adapters/frag_rx.c
    ../scout_cam/main/motor.c
)

target_include_directories(scout_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/unity
    ${CMAKE_SOURCE_DIR}/stubs
    ../shared_components/rc_protocol
    ../shared_components/udp
    ../shared_components/watchdog
    ../scout_cam/main/adapters
    ../scout_screen/main/adapters
    ../scout_cam/main
    ../scout_cam/components/l298n
)

target_link_libraries(scout_tests m)
