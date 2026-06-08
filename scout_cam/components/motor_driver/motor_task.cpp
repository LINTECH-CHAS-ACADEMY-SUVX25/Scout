#include "motor_task.hpp"

extern "C" {
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
}

// Receives CMD_* bytes from udp_stream via a queue and applies them via IMotor.
// Times out after 500 ms without a command and sends CMD_STOP as a safety measure.
// motor_task never includes motor.h — it only knows IMotor.

static const char   *TAG = "motor";
static QueueHandle_t s_queue;
static IMotor       *s_motor;
static uint8_t       s_last_cmd = CMD_STOP;

static void motor_task(void *arg)
{
    uint8_t cmd;
    bool    moving = false;
    esp_task_wdt_add(NULL);
    s_motor->apply(CMD_STOP, 0);

    while(1) {
        if(xQueueReceive(s_queue, &cmd, pdMS_TO_TICKS(500)) == pdTRUE) {
            s_motor->apply(cmd, 100);
            s_last_cmd = cmd;
            moving = (cmd != CMD_STOP);
        } else if(moving) {
            s_motor->apply(CMD_STOP, 0);
            s_last_cmd = CMD_STOP;
            moving = false;
            ESP_LOGW(TAG, "Command timeout — motors stopped");
        }
        esp_task_wdt_reset();
    }
}

uint8_t motor_task_get_last_cmd(void)
{
    return s_last_cmd;
}

void motor_cmd_send(uint8_t cmd)
{
    if(xQueueSend(s_queue, &cmd, 0) != pdTRUE)
        ESP_LOGW(TAG, "cmd queue full, dropped 0x%02x", cmd);
}

void motor_task_start(IMotor *motor)
{
    s_motor = motor;
    s_queue = xQueueCreate(4, sizeof(uint8_t));
    xTaskCreate(motor_task, "motor_task", 2048, NULL, 6, NULL);
}
