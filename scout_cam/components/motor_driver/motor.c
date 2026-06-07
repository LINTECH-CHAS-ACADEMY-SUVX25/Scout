#include "motor.h"
#include "rc_protocol.h"
#include "driver/gpio.h"

// L298N H-bridge driver for the AI-Thinker ESP32-CAM board.

#define PIN_IN1  GPIO_NUM_12
#define PIN_IN2  GPIO_NUM_13
#define PIN_IN3  GPIO_NUM_14
#define PIN_IN4  GPIO_NUM_15

void motor_apply(uint8_t cmd)
{
    bool fwd = cmd & CMD_FORWARD;
    bool bwd = cmd & CMD_BACKWARD;
    bool lft = cmd & CMD_LEFT;
    bool rgt = cmd & CMD_RIGHT;

    if(fwd && bwd) { fwd = false; bwd = false; }
    if(lft && rgt) { lft = false; rgt = false; }

    int in1, in2, in3, in4;
    if(fwd && lft)      { in1 = 1; in2 = 0; in3 = 0; in4 = 0; }
    else if(fwd && rgt) { in1 = 0; in2 = 0; in3 = 1; in4 = 0; }
    else if(bwd && lft) { in1 = 0; in2 = 1; in3 = 0; in4 = 0; }
    else if(bwd && rgt) { in1 = 0; in2 = 0; in3 = 0; in4 = 1; }
    else if(fwd)        { in1 = 1; in2 = 0; in3 = 1; in4 = 0; }
    else if(bwd)        { in1 = 0; in2 = 1; in3 = 0; in4 = 1; }
    else if(lft)        { in1 = 1; in2 = 0; in3 = 0; in4 = 1; }
    else if(rgt)        { in1 = 0; in2 = 1; in3 = 1; in4 = 0; }
    else                { in1 = 0; in2 = 0; in3 = 0; in4 = 0; }

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
    ESP_ERROR_CHECK(gpio_config(&cfg));
    motor_apply(CMD_STOP);
}
