#include "network_task.h"
#include "motor_task.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "esp_task_wdt.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

static const char *TAG = "network";

typedef enum {
    STATE_DISCONNECTED,
    STATE_CONNECTING,
    STATE_STREAMING,
} net_state_t;

static esp_err_t send_all(int sock, const void *buf, size_t len)
{
    const uint8_t *p = buf;
    while (len > 0) {
        int n = send(sock, p, len, 0);
        if (n < 0) return ESP_FAIL;
        p += n;
        len -= n;
    }
    return ESP_OK;
}

static void network_task(void *arg)
{
    struct sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_port   = htons(VID_PORT),
    };
    inet_pton(AF_INET, S3_IP, &dest.sin_addr);

    net_state_t state = STATE_DISCONNECTED;
    int sock = -1;
    esp_task_wdt_add(NULL);

    while (1) {
        esp_task_wdt_reset();
        switch (state) {

            case STATE_DISCONNECTED:
                sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (sock < 0) {
                    ESP_LOGE(TAG, "socket() failed");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    break;
                }
                state = STATE_CONNECTING;
                break;

            case STATE_CONNECTING:
                ESP_LOGI(TAG, "Connecting to S3...");
                if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
                    ESP_LOGW(TAG, "Connect failed, retrying in 2s");
                    close(sock);
                    sock = -1;
                    state = STATE_DISCONNECTED;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    break;
                }
                struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
                setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                ESP_LOGI(TAG, "Connected — streaming");
                state = STATE_STREAMING;
                break;

            case STATE_STREAMING: {
                camera_fb_t *fb = esp_camera_fb_get();
                if (!fb) {
                    ESP_LOGE(TAG, "Camera capture failed");
                    motor_cmd_send(CMD_STOP);
                    close(sock);
                    sock = -1;
                    state = STATE_DISCONNECTED;
                    break;
                }

                uint8_t magic = FRAME_MAGIC;
                uint32_t len_net = htonl((uint32_t)fb->len);
                esp_err_t err = send_all(sock, &magic, 1);
                if (err == ESP_OK) err = send_all(sock, &len_net, 4);
                if (err == ESP_OK) err = send_all(sock, fb->buf, fb->len);
                esp_camera_fb_return(fb);

                if (err != ESP_OK) {
                    motor_cmd_send(CMD_STOP);
                    ESP_LOGW(TAG, "Send failed — reconnecting");
                    close(sock);
                    sock = -1;
                    state = STATE_DISCONNECTED;
                    break;
                }

                uint8_t cmd;
                bool got_cmd = false;
                while (recv(sock, &cmd, 1, MSG_DONTWAIT) == 1)
                    got_cmd = true;
                if (got_cmd)
                    motor_cmd_send(cmd);
                break;
            }
        }
    }
}

void network_task_start(void)
{
    xTaskCreate(network_task, "network_task", 4096, NULL, 5, NULL);
}
