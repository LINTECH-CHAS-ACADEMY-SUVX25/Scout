#include "stream.h"
#include "frame_buf.h"
#include "screen_state.h"
#include "cam_cmd.h"
#include "frag_rx.h"
#include "udp.h"
#include "wifi_ap.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

// Task — UDP video receiver for the dashboard side.
// Reassembles JPEG fragments from the camera into complete frames and hands them
// to the render task through ping-pong buffers in frame_buf.
// Camera IP is learned from the first incoming packet's source address.

static const char *TAG = "stream";

static void stream_run(void *arg);

void stream_init(void)
{
    frame_buf_init();
    cam_cmd_init();
    xTaskCreatePinnedToCore(stream_run, "udp_server", 4096, NULL, 5, NULL, 0);
}

static void stream_run(void *arg)
{
    int sock = udp_open(VID_PORT);
    if(sock < 0) { ESP_LOGE(TAG, "failed to open UDP socket"); vTaskDelete(NULL); return; }
    udp_set_rcvbuf(sock, MAX_FRAGS * PKT_MAX);
    cam_cmd_bind(sock);
    ESP_LOGI(TAG, "UDP video server on port %d", VID_PORT);

    while(1) {
        struct sockaddr_in src;
        int n = udp_rx(sock, frame_buf_pkt(), PKT_MAX, &src);

        screen_status.cam_connected = wifi_ap_sta_count() > 0;
        screen_status.streaming     = frame_buf_is_streaming();

        uint32_t      frame_len;
        int32_t       transfer_ms;
        frag_result_t result = frag_rx(frame_buf_pkt(), n, &frame_len, &transfer_ms);
        if(result == FRAG_DISCARD) continue;

        cam_cmd_learn(&src);
        if(result == FRAG_COMPLETE) {
            uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
            frame_buf_publish(now_ms, frame_len);
            screen_state_push_transfer(now_ms, frame_len, transfer_ms);
        }
    }
}
