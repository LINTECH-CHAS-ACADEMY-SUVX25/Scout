#include "cam_cmd.h"
#include "udp.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

// Shared camera address and socket for the command channel.
// The stream task learns the camera IP from incoming video packets and writes it here;
// the render task reads it to send RC commands — hence the mutex.

static const char *TAG = "cam_cmd";

static int                s_sock  = -1;
static struct sockaddr_in s_addr;
static bool               s_known = false;
static SemaphoreHandle_t  s_mutex;

void cam_cmd_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
}

void cam_cmd_bind(int sock)
{
    s_sock = sock;
}

void cam_cmd_learn(const struct sockaddr_in *src)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if(!s_known) {
        s_addr          = *src;
        s_addr.sin_port = htons(CMD_PORT);
        s_known         = true;
        ESP_LOGI(TAG, "camera at %s", udp_ip_str(src));
    }
    xSemaphoreGive(s_mutex);
}

void cam_cmd_send(uint8_t cmd)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool               known = s_known;
    struct sockaddr_in addr  = s_addr;
    xSemaphoreGive(s_mutex);

    if(!known || s_sock < 0) return;
    udp_tx(s_sock, &addr, &cmd, 1);
}
