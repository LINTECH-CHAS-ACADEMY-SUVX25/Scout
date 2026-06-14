#include "motor.h"
#include "motor_cmd.h"
#include "l298n.h"
#include "watchdog.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

// Task — receives joystick x/y packets from the command queue and drives the
// L298N H-bridge. Times out after 500 ms and stops as a safety measure so the
// car doesn't run away if the dashboard disconnects.

static const char *TAG = "motor";

static void motor_run(void *arg);

// Converts joystick position to direction bitmask and speed (0-255).
// Deadzone: ±112 (~44% of full deflection). Speed = magnitude of (x,y).
static void joy_to_motor(int16_t x, int16_t y, uint8_t *cmd, uint8_t *speed)
{
    *cmd = CMD_STOP;
    if(y >  112) *cmd |= CMD_FORWARD;
    if(y < -112) *cmd |= CMD_BACKWARD;
    if(x < -112) *cmd |= CMD_LEFT;
    if(x >  112) *cmd |= CMD_RIGHT;
    *speed = (*cmd == CMD_STOP) ? 0 : (uint8_t)sqrtf((float)(x * x + y * y));
}

void motor_init(void)
{
    l298n_init();
    motor_cmd_init();
    xTaskCreate(motor_run, "motor", 2048, NULL, 6, NULL);
    ESP_LOGI(TAG, "motor ready");
}

static void motor_run(void *arg)
{
    bool moving = false;
    watchdog_register();
    l298n_apply(CMD_STOP, 0);

    while(1) {
        int16_t x, y;
        if(motor_cmd_recv(&x, &y, 500)) {
            uint8_t cmd, speed;
            joy_to_motor(x, y, &cmd, &speed);
            l298n_apply(cmd, speed);
            moving = (cmd != CMD_STOP);
        } else if(moving) {
            l298n_apply(CMD_STOP, 0);
            moving = false;
            ESP_LOGW(TAG, "command timeout — motors stopped");
        }
        watchdog_reset();
    }
}
