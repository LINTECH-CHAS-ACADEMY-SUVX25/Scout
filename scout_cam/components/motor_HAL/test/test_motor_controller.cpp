#include "unity.h"
#include "MotorController.hpp"
#include "rc_protocol.h"
#include "esp_err.h"

struct MockMotor {
    uint8_t last_cmd   = CMD_STOP;
    uint8_t last_speed = 0;

    esp_err_t init()                             { return ESP_OK; }
    esp_err_t drive(uint8_t cmd, uint8_t speed)  { last_cmd = cmd; last_speed = speed; return ESP_OK; }
    esp_err_t stop()                             { return ESP_OK; }
};

TEST_CASE("zero joystick input stops motors", "[motor]")
{
    MockMotor mock;
    MotorController<MockMotor> ctrl(mock);

    ctrl.drive(0, 0);

    TEST_ASSERT_EQUAL(CMD_STOP, mock.last_cmd);
    TEST_ASSERT_EQUAL(0, mock.last_speed);
    TEST_ASSERT_FALSE(ctrl.isMoving());
}

TEST_CASE("forward joystick sets CMD_FORWARD", "[motor]")
{
    MockMotor mock;
    MotorController<MockMotor> ctrl(mock);

    ctrl.drive(0, 100);

    TEST_ASSERT_TRUE(mock.last_cmd & CMD_FORWARD);
    TEST_ASSERT_TRUE(ctrl.isMoving());
}

TEST_CASE("backward joystick sets CMD_BACKWARD", "[motor]")
{
    MockMotor mock;
    MotorController<MockMotor> ctrl(mock);

    ctrl.drive(0, -100);

    TEST_ASSERT_TRUE(mock.last_cmd & CMD_BACKWARD);
    TEST_ASSERT_TRUE(ctrl.isMoving());
}

TEST_CASE("right joystick sets CMD_RIGHT", "[motor]")
{
    MockMotor mock;
    MotorController<MockMotor> ctrl(mock);

    ctrl.drive(100, 0);

    TEST_ASSERT_TRUE(mock.last_cmd & CMD_RIGHT);
}

TEST_CASE("left joystick sets CMD_LEFT", "[motor]")
{
    MockMotor mock;
    MotorController<MockMotor> ctrl(mock);

    ctrl.drive(-100, 0);

    TEST_ASSERT_TRUE(mock.last_cmd & CMD_LEFT);
}

TEST_CASE("stop clears moving flag", "[motor]")
{
    MockMotor mock;
    MotorController<MockMotor> ctrl(mock);

    ctrl.drive(0, 100);
    TEST_ASSERT_TRUE(ctrl.isMoving());

    ctrl.stop();
    TEST_ASSERT_FALSE(ctrl.isMoving());
}
