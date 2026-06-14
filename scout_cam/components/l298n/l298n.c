#include "l298n.h"
#include "rc_protocol.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

// L298N H-bridge driver for the AI-Thinker ESP32-CAM board.
// ENA and ENB are wired together to GPIO 1 (UART TX — logging stops after init).
// Speed is controlled via LEDC duty cycle; direction via IN1-IN4.

#define PIN_IN1  GPIO_NUM_12
#define PIN_IN2  GPIO_NUM_13
#define PIN_IN3  GPIO_NUM_14
#define PIN_IN4  GPIO_NUM_15
#define PIN_ENA  GPIO_NUM_1   // ENA and ENB tied together; takes over UART TX

#define ENA_SPEED_MODE  LEDC_LOW_SPEED_MODE
#define ENA_TIMER       LEDC_TIMER_1      // TIMER_0/CHANNEL_0 used by camera
#define ENA_CHANNEL     LEDC_CHANNEL_1

void l298n_init(void)
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

    ledc_timer_config_t timer = {
        .speed_mode      = ENA_SPEED_MODE,
        .timer_num       = ENA_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz         = 1000,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch = {
        .speed_mode = ENA_SPEED_MODE,
        .channel    = ENA_CHANNEL,
        .timer_sel  = ENA_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = PIN_ENA,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    l298n_apply(CMD_STOP, 0);
}

void l298n_apply(uint8_t cmd, uint8_t speed)
{
    ledc_set_duty(ENA_SPEED_MODE, ENA_CHANNEL, speed);
    ledc_update_duty(ENA_SPEED_MODE, ENA_CHANNEL);

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
