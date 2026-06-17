#include "motor.h"
#include "motor_queue.h"
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
// Deadzone is enforced on the screen side; x=0,y=0 maps to CMD_STOP here.
void joy_to_motor(int16_t x, int16_t y, uint8_t *cmd, uint8_t *speed)
{
    *cmd = CMD_STOP;
    if(y > 0) *cmd |= CMD_FORWARD;
    if(y < 0) *cmd |= CMD_BACKWARD;
    if(x < 0) *cmd |= CMD_LEFT;
    if(x > 0) *cmd |= CMD_RIGHT;
    float mag = sqrtf((float)(x * x + y * y));
    *speed = (*cmd == CMD_STOP) ? 0 : (mag > 255.0f ? 255 : (uint8_t)mag);
}

void motor_init(void)
{
    l298n_init();
    motor_queue_init();
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
        if(motor_queue_recv(&x, &y, 500)) {
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
