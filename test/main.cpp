#include "test_runner.hpp"

void run_motor_tests();
void run_camera_tests();
void run_network_tests();
void run_display_tests();

int main()
{
    run_motor_tests();
    run_camera_tests();
    run_network_tests();
    run_display_tests();
    print_summary();
    return 0;
}
