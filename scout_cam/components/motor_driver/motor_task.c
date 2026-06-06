#include "motor_task.h"
#include "motor.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

static const char *TAG = "motor";

static QueueHandle_t s_queue;

void motor_cmd_send(uint8_t cmd)
{
    if (xQueueSend(s_queue, &cmd, 0) != pdTRUE)
        ESP_LOGW(TAG, "cmd queue full, dropped 0x%02x", cmd);
}

static void motor_task(void *arg)
{
    uint8_t cmd;
    bool moving = false;
    esp_task_wdt_add(NULL);
    motor_apply(CMD_STOP);

    while (1)
    {
        if (xQueueReceive(s_queue, &cmd, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            motor_apply(cmd);
            moving = (cmd != CMD_STOP);
        } else if (moving)
        {
            motor_apply(CMD_STOP);
            moving = false;
            ESP_LOGW(TAG, "Command timeout — motors stopped");
        }
        esp_task_wdt_reset();
    }
}

void motor_task_start(void)
{
    s_queue = xQueueCreate(4, sizeof(uint8_t));
    xTaskCreate(motor_task, "motor_task", 2048, NULL, 6, NULL);
}
