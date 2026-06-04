#include "udp_stream.h"
#include "motor_task.h"
#include "rc_protocol.h"
#include "udp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "esp_task_wdt.h"
#include <string.h>

// A JPEG frame is split across UDP fragments to stay under the WiFi MTU:
// Fragment 0's payload is prefixed with [FRAME_MAGIC:1][frame_len:4] so the
// receiver knows the full size before reassembling.

#define FRAG_SIZE   1460
#define FIRST_DATA  (FRAG_SIZE - 5)
#define PKT_MAX     (4 + 5 + FRAG_SIZE)

static const char *TAG = "udp_stream";

static uint8_t s_pkt[PKT_MAX];

static void send_frame(int sock, const struct sockaddr_in *dest, const uint8_t *buf, uint32_t len, uint16_t seq)
{
    uint8_t n_frags = (len <= FIRST_DATA) ? 1 : 1 + (len - FIRST_DATA + FRAG_SIZE - 1) / FRAG_SIZE;
    uint32_t len_be = htonl(len);
    uint32_t sent   = 0;

    for (uint8_t fi = 0; fi < n_frags; fi++)
    {
        uint8_t *p = s_pkt;
        uint16_t seq_be = htons(seq);
        memcpy(p, &seq_be, 2); p += 2;
        *p++ = fi;
        *p++ = n_frags;

        uint32_t chunk;
        if (fi == 0)
        {
            *p++ = FRAME_MAGIC;
            memcpy(p, &len_be, 4); p += 4;
            chunk = (len < FIRST_DATA) ? len : FIRST_DATA;
        } else
        {
            uint32_t remaining = len - sent;
            chunk = (remaining < FRAG_SIZE) ? remaining : FRAG_SIZE;
        }
        memcpy(p, buf + sent, chunk);
        p   += chunk;
        sent += chunk;

        udp_send(sock, dest, s_pkt, p - s_pkt);
    }
}

static void drain_commands(int sock)
{
    uint8_t cmd;
    while (udp_try_recv(sock, &cmd, 1) == 1)
        motor_cmd_send(cmd);
}

static void udp_stream_task(void *arg)
{
    struct sockaddr_in dest = udp_addr(S3_IP, VID_PORT);

    int sock = -1;
    while (sock < 0)
    {
        sock = udp_open(CMD_PORT);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "socket setup failed, retrying");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    ESP_LOGI(TAG, "streaming to %s:%d, commands on port %d", S3_IP, VID_PORT, CMD_PORT);

    esp_task_wdt_add(NULL);
    uint16_t seq = 0;

    while (1)
    {
        esp_task_wdt_reset();

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGE(TAG, "camera capture failed");
            motor_cmd_send(CMD_STOP);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        send_frame(sock, &dest, fb->buf, fb->len, seq++);
        esp_camera_fb_return(fb);

        drain_commands(sock);
    }
}

void udp_stream_start(void)
{
    xTaskCreate(udp_stream_task, "udp_stream", 4096, NULL, 5, NULL);
}
