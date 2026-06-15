#include "stream.h"
#include "cam_state.h"
#include "rc_rx.h"
#include "frag_tx.h"
#include "camera.h"
#include "motor_queue.h"
#include "bme280.h"
#include "watchdog.h"
#include "udp.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <math.h>

// Task — captures JPEG frames from the camera and streams them to the dashboard via UDP.
// Also drains inbound RC commands from the dashboard and forwards them to the motor task.
// Exists because camera capture and UDP I/O both block and must run together.

static const char *TAG = "stream";

static void stream_run(void *arg);

void stream_init(void)
{
    frag_tx_init(S3_IP, VID_PORT);
    rc_rx_init();
    xTaskCreate(stream_run, "stream", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "streaming to %s:%d, commands on :%d", S3_IP, VID_PORT, CMD_PORT);
}

static void stream_run(void *arg)
{
    int sock = -1;
    while(sock < 0) {
        sock = udp_open(CMD_PORT);
        if(sock < 0) {
            ESP_LOGE(TAG, "socket setup failed, retrying");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    udp_set_send_timeout(sock, 1);

    int ctrl_sock = udp_open(CTRL_PORT);
    if(ctrl_sock < 0) ESP_LOGW(TAG, "failed to open CTRL socket — camera control unavailable");

    watchdog_register();
    uint16_t           seq          = 0;
    struct sockaddr_in diag_dest    = udp_addr(S3_IP, DIAG_PORT);
    int64_t            last_diag_us = 0;
    cam_status.streaming = true;

    while(1) {
        watchdog_reset();

        int64_t now_us = esp_timer_get_time();
        if(now_us - last_diag_us >= 2000000) {
            cam_state_update_rssi();
            cam_diag_pkt_t pkt = {
                .free_heap = esp_get_free_heap_size(),
                .rssi_dbm  = cam_status.rssi_dbm,
                .uptime_s  = (uint32_t)(now_us / 1000000),
            };
            float t, h, p;
            if(cam_status.sensor_enabled && bme280_read(&t, &h, &p)) {
                pkt.temp_cdeg    = (int16_t)lroundf(t * 100.0f);
                pkt.humidity_pct = (uint8_t)lroundf(h);
                pkt.pressure_pa  = (uint32_t)lroundf(p);
            }
            udp_tx(sock, &diag_dest, &pkt, sizeof(pkt));
            last_diag_us = now_us;
        }

        if(ctrl_sock >= 0) {
            cam_ctrl_pkt_t cpkt;
            if(udp_try_recv(ctrl_sock, &cpkt, sizeof(cpkt)) == (int)sizeof(cpkt))
                cam_state_apply_ctrl(&cpkt);
        }

        if(!cam_status.streaming) {
            cam_state_try_resume();
            rc_rx_process(sock);
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if(!cam_status.camera_enabled) {
            rc_rx_process(sock);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        const uint8_t *buf;
        size_t len;
        if(!camera_capture(&buf, &len)) {
            ESP_LOGE(TAG, "camera capture failed");
            motor_queue_send(0, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if(len > FRAME_MAX) {
            ESP_LOGW(TAG, "frame too large (%u B > %u) — lower jpeg_quality or frame_size",
                     (unsigned)len, FRAME_MAX);
            camera_release();
            continue;
        }

        frag_tx(sock, buf, (uint32_t)len, seq++);
        camera_release();
        rc_rx_process(sock);
    }
}
