#include "motor_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "motor_cmd";

static QueueHandle_t s_queue;

void motor_cmd_init(void)
{
    s_queue = xQueueCreate(4, sizeof(uint8_t));
}

void motor_cmd_send(uint8_t cmd)
{
    if(xQueueSend(s_queue, &cmd, 0) != pdTRUE)
        ESP_LOGW(TAG, "cmd queue full, dropped 0x%02x", cmd);
}

bool motor_cmd_recv(uint8_t *cmd, uint32_t timeout_ms)
{
    return xQueueReceive(s_queue, cmd, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}
