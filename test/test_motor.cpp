#include "unity.h"
#include "MotorMock.hpp"

// CMD_* bits from rc_protocol.h — duplicated here to avoid ESP-IDF dependency
static constexpr uint8_t CMD_STOP     = 0x00;
static constexpr uint8_t CMD_FORWARD  = 0x01;
static constexpr uint8_t CMD_BACKWARD = 0x02;
static constexpr uint8_t CMD_LEFT     = 0x04;
static constexpr uint8_t CMD_RIGHT    = 0x08;

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
    motor.apply(CMD_FORWARD, 100);
    TEST_ASSERT_EQUAL(CMD_FORWARD, motor.last_direction);
}

void test_motor_apply_stores_speed(void)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD, 75);
    TEST_ASSERT_EQUAL(75, motor.last_speed);
}

void test_motor_stop_clears_direction(void)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD, 100);
    motor.apply(CMD_STOP, 0);
    TEST_ASSERT_EQUAL(CMD_STOP, motor.last_direction);
    TEST_ASSERT_EQUAL(0, motor.last_speed);
}

void test_motor_combined_direction(void)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD | CMD_LEFT, 50);
    TEST_ASSERT_EQUAL(CMD_FORWARD | CMD_LEFT, motor.last_direction);
    TEST_ASSERT_EQUAL(50, motor.last_speed);
}
