#include "udp_stream.h"
#include "frag_tx.h"
#include "camera.h"
#include "motor_cmd.h"
#include "udp.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

// Task — captures JPEG frames from the camera and streams them to the dashboard via UDP.
// Also drains inbound CMD bytes from the dashboard and forwards them to the motor task.
// Exists because camera capture and UDP I/O both block and must run together.

static const char *TAG = "udp_stream";

void udp_stream_run(void *arg)
{
    struct sockaddr_in dest = udp_addr(S3_IP, VID_PORT);

    int sock = -1;
    while(sock < 0) {
        sock = udp_open(CMD_PORT);
        if(sock < 0) {
            ESP_LOGE(TAG, "socket setup failed, retrying");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    udp_set_send_timeout(sock, 1);
    ESP_LOGI(TAG, "streaming to %s:%d, commands on port %d", S3_IP, VID_PORT, CMD_PORT);

    esp_task_wdt_add(NULL);
    uint16_t seq = 0;

    while(1) {
        esp_task_wdt_reset();

        const uint8_t *buf;
        size_t len;
        if(!camera_capture(&buf, &len)) {
            ESP_LOGE(TAG, "camera capture failed");
            motor_cmd_send(CMD_STOP);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if(len > FRAME_MAX) {
            ESP_LOGW(TAG, "frame too large (%u B > %u) — lower jpeg_quality or frame_size",
                     (unsigned)len, FRAME_MAX);
            camera_release();
            continue;
        }

        frag_tx(sock, &dest, buf, (uint32_t)len, seq++);
        camera_release();

        uint8_t cmd;
        while(udp_try_recv(sock, &cmd, 1) == 1)
            motor_cmd_send(cmd);
    }
}

void udp_stream_init(void)
{
}
