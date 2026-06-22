#include "motor_driver.hpp"
#include "rc_protocol.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "MotorDriver";

#define PIN_IN1  GPIO_NUM_12
#define PIN_IN2  GPIO_NUM_13
#define PIN_IN3  GPIO_NUM_14
#define PIN_IN4  GPIO_NUM_15
#define PIN_ENA  GPIO_NUM_1   // ENA and ENB tied together; takes over UART TX

#define ENA_SPEED_MODE  LEDC_LOW_SPEED_MODE
#define ENA_TIMER       LEDC_TIMER_1      // TIMER_0/CHANNEL_0 used by camera
#define ENA_CHANNEL     LEDC_CHANNEL_1

#define L298N_DUTY_MIN  130   // ≈ 1.7 V at 3.3 V logic
#define L298N_DUTY_MAX  255

esp_err_t MotorDriver::init()
{
	gpio_config_t cfg = {};
	cfg.pin_bit_mask = BIT64(PIN_IN1) | BIT64(PIN_IN2) |
	                   BIT64(PIN_IN3) | BIT64(PIN_IN4);
	cfg.mode         = GPIO_MODE_OUTPUT;
	cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
	cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
	cfg.intr_type    = GPIO_INTR_DISABLE;

	esp_err_t err = gpio_config(&cfg);
	if(err != ESP_OK) return err;

	ledc_timer_config_t timer = {};
	timer.speed_mode      = ENA_SPEED_MODE;
	timer.timer_num       = ENA_TIMER;
	timer.duty_resolution = LEDC_TIMER_8_BIT;
	timer.freq_hz         = 1000;
	timer.clk_cfg         = LEDC_AUTO_CLK;

	err = ledc_timer_config(&timer);
	if(err != ESP_OK) return err;

	ledc_channel_config_t ch = {};
	ch.speed_mode = ENA_SPEED_MODE;
	ch.channel    = ENA_CHANNEL;
	ch.timer_sel  = ENA_TIMER;
	ch.intr_type  = LEDC_INTR_DISABLE;
	ch.gpio_num   = PIN_ENA;
	ch.duty       = 0;
	ch.hpoint     = 0;

	err = ledc_channel_config(&ch);
	if(err != ESP_OK) return err;

	return this->stop();
}

esp_err_t MotorDriver::drive(uint8_t cmd, uint8_t speed)
{
	esp_err_t err = this->set_speed(speed);
	if(err != ESP_OK) return err;

	this->set_direction(cmd);
	return ESP_OK;
}

esp_err_t MotorDriver::stop()
{
	return this->drive(CMD_STOP, 0);
}

esp_err_t MotorDriver::set_speed(uint8_t speed)
{
	uint8_t duty;
	if(speed == 0) {
		duty = 0;
	} else if(speed <= JOY_SPEED_MIN) {
		duty = L298N_DUTY_MIN;
	} else {
		duty = (uint8_t)(L298N_DUTY_MIN +
		       (uint32_t)(speed - JOY_SPEED_MIN) * (L298N_DUTY_MAX - L298N_DUTY_MIN) /
		       (255 - JOY_SPEED_MIN));
	}

	esp_err_t err = ledc_set_duty(ENA_SPEED_MODE, ENA_CHANNEL, duty);
	if(err != ESP_OK) return err;
	return ledc_update_duty(ENA_SPEED_MODE, ENA_CHANNEL);
}

void MotorDriver::set_direction(uint8_t cmd)
{
	bool fwd = (cmd & CMD_FORWARD) != 0;
	bool bwd = (cmd & CMD_BACKWARD) != 0;
	bool lft = (cmd & CMD_LEFT) != 0;
	bool rgt = (cmd & CMD_RIGHT) != 0;

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

	ESP_LOGD(TAG, "cmd=0x%02x IN=%d%d%d%d", cmd, in1, in2, in3, in4);
}
