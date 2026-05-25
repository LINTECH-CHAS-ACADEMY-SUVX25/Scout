#include "stream.h"
#include "motor.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

static const char *TAG = "stream";

#define S3_IP "192.168.4.1"

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

static void stream_task(void *arg)
{
    struct sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_port   = htons(VID_PORT),
    };
    inet_pton(AF_INET, S3_IP, &dest.sin_addr);

    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        ESP_LOGI(TAG, "Connecting to S3...");

        if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
            ESP_LOGW(TAG, "Connect failed, retrying in 2s");
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        ESP_LOGI(TAG, "Connected — streaming");

        while (1) {
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGE(TAG, "Camera capture failed");
                break;
            }

            uint8_t magic = FRAME_MAGIC;
            uint32_t len_net = htonl((uint32_t)fb->len);
            esp_err_t err = send_all(sock, &magic, 1);
            if (err == ESP_OK) err = send_all(sock, &len_net, 4);
            if (err == ESP_OK) err = send_all(sock, fb->buf, fb->len);

            esp_camera_fb_return(fb);

            if (err != ESP_OK) {
                motor_apply(CMD_STOP); // stop before dropping the connection
                ESP_LOGW(TAG, "Send failed — reconnecting");
                break;
            }

            // drain queue, keep only the latest cmd
            uint8_t cmd;
            int last = -1;
            while (recv(sock, &cmd, 1, MSG_DONTWAIT) == 1)
                last = cmd;
            if (last >= 0)
                motor_apply((uint8_t)last);
        }

        close(sock);
    }
}

void stream_start(void)
{
    xTaskCreate(stream_task, "stream", 4096, NULL, 5, NULL);
}
