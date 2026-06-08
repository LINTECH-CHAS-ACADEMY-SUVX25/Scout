#include "stream.h"
#include "frame_buf.h"
#include "cam_cmd.h"
#include "frag_rx.h"
#include "udp.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — UDP video receiver for the dashboard side.
// Reassembles JPEG fragments from the camera into complete frames and hands them
// to the render task through ping-pong buffers in frame_buf.
// Camera IP is learned from the first incoming packet's source address.

static const char *TAG = "stream";

void stream_init(void)
{
    frame_buf_init();
    cam_cmd_init();
}

void stream_run(void *arg)
{
    int sock = udp_open(VID_PORT);
    if(sock < 0) { ESP_LOGE(TAG, "failed to open UDP socket"); vTaskDelete(NULL); return; }
    udp_set_rcvbuf(sock, MAX_FRAGS * PKT_MAX);
    cam_cmd_bind(sock);
    ESP_LOGI(TAG, "UDP video server on port %d", VID_PORT);

    while(1) {
        struct sockaddr_in src;
        int n = udp_rx(sock, frame_buf_pkt(), PKT_MAX, &src);

        uint32_t      frame_len;
        int32_t       transfer_ms;
        frag_result_t result = frag_rx(frame_buf_pkt(), n, &frame_len, &transfer_ms);
        if(result == FRAG_DISCARD) continue;

        cam_cmd_learn(&src);
        if(result == FRAG_COMPLETE)
            frame_buf_publish(frame_len, transfer_ms);
    }
}
