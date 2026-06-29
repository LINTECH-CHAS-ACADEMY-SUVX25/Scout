#include "unity.h"

void test_motor_init_sets_flag(void);
void test_motor_apply_stores_direction(void);
void test_motor_apply_stores_speed(void);
void test_motor_stop_clears_state(void);

void test_camera_init_sets_flag(void);
void test_camera_capture_returns_data(void);
void test_camera_capture_increments_count(void);

void test_display_init_sets_flag(void);
void test_display_draw_increments_count(void);

void test_network_connect_sets_flag(void);
void test_network_disconnect_clears_flag(void);
void test_network_send_increments_count(void);
void test_network_send_failure_returns_false(void);

void test_bme280_init_sets_flag(void);
void test_bme280_read_returns_injected_values(void);
void test_bme280_read_failure_returns_false(void);

void setUp(void)    {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_motor_init_sets_flag);
    RUN_TEST(test_motor_apply_stores_direction);
    RUN_TEST(test_motor_apply_stores_speed);
    RUN_TEST(test_motor_stop_clears_state);

    RUN_TEST(test_camera_init_sets_flag);
    RUN_TEST(test_camera_capture_returns_data);
    RUN_TEST(test_camera_capture_increments_count);

    RUN_TEST(test_display_init_sets_flag);
    RUN_TEST(test_display_draw_increments_count);

    RUN_TEST(test_network_connect_sets_flag);
    RUN_TEST(test_network_disconnect_clears_flag);
    RUN_TEST(test_network_send_increments_count);
    RUN_TEST(test_network_send_failure_returns_false);

    RUN_TEST(test_bme280_init_sets_flag);
    RUN_TEST(test_bme280_read_returns_injected_values);
    RUN_TEST(test_bme280_read_failure_returns_false);

    return UNITY_END();
}
