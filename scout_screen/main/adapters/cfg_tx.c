#include "cfg_tx.h"
#include "udp.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static int                s_sock  = -1;
static struct sockaddr_in s_addr;
static bool               s_known = false;
static SemaphoreHandle_t  s_mutex;

#define CTRL_QUEUE_MAX 8
typedef struct { uint8_t cmd; int8_t value; } ctrl_entry_t;
static ctrl_entry_t s_queue[CTRL_QUEUE_MAX];
static int          s_queue_len;
static bool         s_dirty;

void cfg_tx_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
}

void cfg_tx_bind(int sock)
{
    s_sock = sock;
}

void cfg_tx_learn(const struct sockaddr_in *src)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if(!s_known) {
        s_addr          = *src;
        s_addr.sin_port = htons(CTRL_PORT);
        s_known         = true;
    }
    xSemaphoreGive(s_mutex);
}

void cfg_tx_send(uint8_t cmd, int8_t value)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool               known = s_known;
    struct sockaddr_in addr  = s_addr;
    xSemaphoreGive(s_mutex);

    if(!known || s_sock < 0) return;
    cam_ctrl_pkt_t pkt = { .cmd = cmd, .value = value };
    udp_tx(s_sock, &addr, &pkt, sizeof(pkt));
}

void cfg_tx_push(uint8_t cmd, int8_t value)
{
    if(s_queue_len < CTRL_QUEUE_MAX) {
        s_queue[s_queue_len++] = (ctrl_entry_t){ cmd, value };
        s_dirty = true;
    }
}

void cfg_tx_flush(void)
{
    if(!s_dirty) return;
    for(int i = 0; i < s_queue_len; i++)
        cfg_tx_send(s_queue[i].cmd, s_queue[i].value);
    s_queue_len = 0;
    s_dirty     = false;
}
