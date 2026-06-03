#include "test_runner.hpp"
#include "MotorMock.hpp"

// CMD_* bits from rc_protocol.h — duplicated here to avoid ESP-IDF dependency
static constexpr uint8_t CMD_STOP     = 0x00;
static constexpr uint8_t CMD_FORWARD  = 0x01;
static constexpr uint8_t CMD_BACKWARD = 0x02;
static constexpr uint8_t CMD_LEFT     = 0x04;
static constexpr uint8_t CMD_RIGHT    = 0x08;

TEST(test_motor_init_sets_flag)
{
    MotorMock motor;
    ASSERT_TRUE(!motor.initialized);
    motor.init();
    ASSERT_TRUE(motor.initialized);
}

TEST(test_motor_apply_stores_direction)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD, 100);
    ASSERT_EQ(motor.last_direction, CMD_FORWARD);
}

TEST(test_motor_apply_stores_speed)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD, 75);
    ASSERT_EQ(motor.last_speed, 75);
}

TEST(test_motor_stop_clears_direction)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD, 100);
    motor.apply(CMD_STOP, 0);
    ASSERT_EQ(motor.last_direction, CMD_STOP);
    ASSERT_EQ(motor.last_speed, 0);
}

TEST(test_motor_combined_direction)
{
    MotorMock motor;
    motor.init();
    motor.apply(CMD_FORWARD | CMD_LEFT, 50);
    ASSERT_EQ(motor.last_direction, CMD_FORWARD | CMD_LEFT);
    ASSERT_EQ(motor.last_speed, 50);
}

void run_motor_tests()
{
    printf("Motor:\n");
    RUN(test_motor_init_sets_flag);
    RUN(test_motor_apply_stores_direction);
    RUN(test_motor_apply_stores_speed);
    RUN(test_motor_stop_clears_direction);
    RUN(test_motor_combined_direction);
}
