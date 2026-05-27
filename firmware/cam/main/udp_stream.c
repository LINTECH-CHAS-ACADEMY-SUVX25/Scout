#include "udp_stream.h"
#include "motor_task.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>

// Strömmar JPEG-frames från kameran till screenen via UDP.
//
// Paketformat: [seq: 2B] [frag-index: 1B] [antal frags: 1B] [data]
// Fragment 0 börjar med: [FRAME_MAGIC: 1B] [jpeg-längd: 4B BE] [jpeg...]
//
// Video → S3_IP:VID_PORT, styrkommandon ← CMD_PORT
// Fragmentstorlek 1460: WiFi MTU 1500 minus IP/UDP-header (28B) minus marginal

#define FRAG_SIZE   1460
#define FIRST_DATA  (FRAG_SIZE - 5)   /* fragment 0 bär 5 byte mindre data */
#define PKT_MAX     (4 + 5 + FRAG_SIZE)

static const char *TAG = "udp_stream";

static uint8_t s_pkt[PKT_MAX];

static void udp_stream_task(void *arg)
{
    // Skicka videobilder till screenen på VID_PORT
    struct sockaddr_in dest =
    {
        .sin_family = AF_INET,
        .sin_port   = htons(VID_PORT),
    };
    inet_pton(AF_INET, S3_IP, &dest.sin_addr);

    // Lyssna på RC-styrkommandon från screenen på CMD_PORT
    struct sockaddr_in local =
    {
        .sin_family      = AF_INET,
        .sin_port        = htons(CMD_PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) 
    {
        ESP_LOGE(TAG, "socket() failed");
        vTaskDelete(NULL);
        return;
    }
    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) != 0) 
    {
        ESP_LOGE(TAG, "bind() failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "UDP stream: video->%s:%d, cmd port %d", S3_IP, VID_PORT, CMD_PORT);

    uint16_t frame_seq = 0;

    while (1)
    {
        // Hämta senaste bildruta — CAMERA_GRAB_LATEST ser till att vi aldrig
        // skickar en gammal bild som legat i kamerabufferten
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGE(TAG, "Camera capture failed");
            motor_cmd_send(CMD_STOP);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        uint32_t total  = fb->len;
        uint8_t  n_frags = (total <= FIRST_DATA) ? 1
            : (uint8_t)(1 + (total - FIRST_DATA + FRAG_SIZE - 1) / FRAG_SIZE);
        uint32_t len_be = htonl(total);
        uint32_t sent   = 0;

        // Skicka alla fragment för den här bilden.
        // Fragment 0 får magic+längd-header så mottagaren vet bildstorlek direkt.
        for (uint8_t fi = 0; fi < n_frags; fi++)
        {
            uint8_t *p = s_pkt;
            // Gemensam header: [seq: 2 BE][fragment-index: 1][totalt: 1]
            uint16_t seq_be = htons(frame_seq);
            memcpy(p, &seq_be, 2); p += 2;
            *p++ = fi;
            *p++ = n_frags;

            uint32_t chunk;
            if (fi == 0)
            {
                // Fragment 0 bär magic-byte och total JPEG-längd
                *p++ = FRAME_MAGIC;
                memcpy(p, &len_be, 4); p += 4;
                chunk = (total < FIRST_DATA) ? total : (uint32_t)FIRST_DATA;
            } else
            {
                uint32_t remaining = total - sent;
                chunk = (remaining < FRAG_SIZE) ? remaining : (uint32_t)FRAG_SIZE;
            }
            memcpy(p, fb->buf + sent, chunk);
            p += chunk;
            sent += chunk;

            sendto(sock, s_pkt, (size_t)(p - s_pkt), 0, (struct sockaddr *)&dest, sizeof(dest));
        }

        esp_camera_fb_return(fb);
        frame_seq++;

        // Töm inkommande styrkommandon utan att blockera.
        // Görs efter varje bild — fördröjningen är försumbar vid 30 fps.
        uint8_t cmd;
        while (recv(sock, &cmd, 1, MSG_DONTWAIT) == 1)
            motor_cmd_send(cmd);
    }
}

void udp_stream_start(void)
{
    // Prioritet 5 — högst av alla tasks, kameran ska aldrig svältas på CPU
    xTaskCreate(udp_stream_task, "udp_stream", 4096, NULL, 5, NULL);
}
