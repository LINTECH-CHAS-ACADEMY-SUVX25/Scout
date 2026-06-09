#include "motor.h"
#include "motor_cmd.h"
#include "l298n.h"
#include "watchdog.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — receives CMD_* bytes from stream via the motor command queue and applies
// them to the L298N H-bridge. Times out after 500 ms and stops as a safety measure
// so the car doesn't run away if the dashboard disconnects.

static const char *TAG = "motor";

static void motor_run(void *arg);

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
    l298n_apply(CMD_STOP);

    while(1) {
        uint8_t cmd;
        if(motor_cmd_recv(&cmd, 500)) {
            l298n_apply(cmd);
            moving = (cmd != CMD_STOP);
        } else if(moving) {
            l298n_apply(CMD_STOP);
            moving = false;
            ESP_LOGW(TAG, "command timeout — motors stopped");
        }
        watchdog_reset();
    }
}
