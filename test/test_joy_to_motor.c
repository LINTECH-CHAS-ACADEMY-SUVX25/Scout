#include "unity.h"
#include "motor.h"
#include "rc_protocol.h"

void test_joy_center_is_stop(void)
{
    uint8_t cmd, speed;
    joy_to_motor(0, 0, &cmd, &speed);
    TEST_ASSERT_EQUAL(CMD_STOP, cmd);
    TEST_ASSERT_EQUAL(0, speed);
}

void test_joy_forward(void)
{
    uint8_t cmd, speed;
    joy_to_motor(0, 100, &cmd, &speed);
    TEST_ASSERT_EQUAL(CMD_FORWARD, cmd);
    TEST_ASSERT_EQUAL(100, speed);
}

void test_joy_backward(void)
{
    uint8_t cmd, speed;
    joy_to_motor(0, -100, &cmd, &speed);
    TEST_ASSERT_EQUAL(CMD_BACKWARD, cmd);
    TEST_ASSERT_EQUAL(100, speed);
}

void test_joy_left(void)
{
    uint8_t cmd, speed;
    joy_to_motor(-100, 0, &cmd, &speed);
    TEST_ASSERT_EQUAL(CMD_LEFT, cmd);
    TEST_ASSERT_EQUAL(100, speed);
}

void test_joy_right(void)
{
    uint8_t cmd, speed;
    joy_to_motor(100, 0, &cmd, &speed);
    TEST_ASSERT_EQUAL(CMD_RIGHT, cmd);
    TEST_ASSERT_EQUAL(100, speed);
}

void test_joy_forward_left_combined(void)
{
    uint8_t cmd, speed;
    joy_to_motor(-100, 100, &cmd, &speed);
    TEST_ASSERT_EQUAL(CMD_FORWARD | CMD_LEFT, cmd);
}

void test_joy_speed_capped_at_255(void)
{
    uint8_t cmd, speed;
    joy_to_motor(0, 1000, &cmd, &speed);
    TEST_ASSERT_EQUAL(255, speed);
}
