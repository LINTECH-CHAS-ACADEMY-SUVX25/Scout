#include "unity.h"
#include "MotorMock.hpp"

static constexpr uint8_t CMD_STOP    = 0x00;
static constexpr uint8_t CMD_FORWARD = 0x01;

void test_motor_init_sets_flag(void)
{
    MotorMock motor;
    TEST_ASSERT_FALSE(motor.initialized);
    motor.init();
    TEST_ASSERT_TRUE(motor.initialized);
}

void test_motor_apply_stores_direction(void)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD, 50);
    TEST_ASSERT_EQUAL(CMD_FORWARD, motor.last_direction);
}

void test_motor_apply_stores_speed(void)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD, 75);
    TEST_ASSERT_EQUAL(75, motor.last_speed);
}

void test_motor_stop_clears_state(void)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD, 100);
    motor.apply(CMD_STOP, 0);
    TEST_ASSERT_EQUAL(CMD_STOP, motor.last_direction);
    TEST_ASSERT_EQUAL(0, motor.last_speed);
}
