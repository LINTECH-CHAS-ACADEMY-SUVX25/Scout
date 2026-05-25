#include "motor.h"
#include "rc_protocol.h"
#include "driver/gpio.h"

// L298N motor driver pins
#define PIN_IN1  GPIO_NUM_12
#define PIN_IN2  GPIO_NUM_13
#define PIN_IN3  GPIO_NUM_14
#define PIN_IN4  GPIO_NUM_15

void motor_apply(uint8_t cmd)
{
    int in1, in2, in3, in4;
    switch (cmd) {
        case CMD_FORWARD:  in1=1; in2=0; in3=1; in4=0; break;
        case CMD_BACKWARD: in1=0; in2=1; in3=0; in4=1; break;
        case CMD_LEFT:     in1=1; in2=0; in3=0; in4=1; break;
        case CMD_RIGHT:    in1=0; in2=1; in3=1; in4=0; break;
        default:           in1=0; in2=0; in3=0; in4=0; break;
    }
    gpio_set_level(PIN_IN1, in1);
    gpio_set_level(PIN_IN2, in2);
    gpio_set_level(PIN_IN3, in3);
    gpio_set_level(PIN_IN4, in4);
}

void motor_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = BIT64(PIN_IN1) | BIT64(PIN_IN2) |
                        BIT64(PIN_IN3) | BIT64(PIN_IN4),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    motor_apply(CMD_STOP); // start with motors stopped
}
