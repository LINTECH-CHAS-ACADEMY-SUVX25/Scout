#include "motor_task.h"
#include "motor_cmd.h"
#include "motor.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

// Task — receives CMD_* bytes from the stream task via motor_cmd and applies them
// to the L298N H-bridge. Times out after 500 ms and sends CMD_STOP as a safety measure
// so the car doesn't run away if the dashboard disconnects.

static const char *TAG = "motor";

void motor_task_run(void *arg)
{
    bool moving = false;
    esp_task_wdt_add(NULL);
    motor_apply(CMD_STOP);

    while(1) {
        uint8_t cmd;
        if(motor_cmd_recv(&cmd, 500)) {
            motor_apply(cmd);
            moving = (cmd != CMD_STOP);
        } else if(moving) {
            motor_apply(CMD_STOP);
            moving = false;
            ESP_LOGW(TAG, "command timeout — motors stopped");
        }
        esp_task_wdt_reset();
    }
}

void motor_task_init(void)
{
    motor_cmd_init();
}
