#include "motor_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "motor_queue";

static QueueHandle_t s_queue;

void motor_queue_init(void)
{
    s_queue = xQueueCreate(4, sizeof(joy_pkt_t));
}

void motor_queue_send(int16_t x, int16_t y)
{
    joy_pkt_t pkt = { .x = x, .y = y };
    if(xQueueSend(s_queue, &pkt, 0) != pdTRUE)
        ESP_LOGW(TAG, "cmd queue full, dropped");
}

bool motor_queue_recv(int16_t *x, int16_t *y, uint32_t timeout_ms)
{
    joy_pkt_t pkt;
    if(xQueueReceive(s_queue, &pkt, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) return false;
    *x = pkt.x;
    *y = pkt.y;
    return true;
}
