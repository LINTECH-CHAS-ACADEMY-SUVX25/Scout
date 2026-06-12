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

// Task — UDP video receiver for the dashboard side.
// Reassembles JPEG fragments from the camera into complete frames and hands them
// to the render task through ping-pong buffers in frame_buf.
// Camera IP is learned from the first incoming packet's source address.

static const char    *TAG    = "stream";
static screen_tick_t  s_tick = {0};

static void stream_run(void *arg);

void stream_init(void)
{
    frame_buf_init();
    cam_cmd_init();
    screen_state_stream_tick_init(&s_tick);
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
        screen_state_tick(&s_tick);

        struct sockaddr_in src;
        int n = udp_rx(sock, frame_buf_pkt(), PKT_MAX, &src);

        bool cam_connected = wifi_ap_sta_count() > 0;
        bool streaming     = screen_state_is_streaming();
        screen_status.cam_connected = cam_connected;
        screen_status.streaming     = streaming;

        if(!screen_state_has_streamed())  screen_state_set_scene(SCENE_WAITING);
        else if(streaming)                screen_state_set_scene(SCENE_STREAMING);
        else                              screen_state_set_scene(SCENE_DISCONNECTED);

        uint32_t      frame_len;
        int32_t       transfer_ms;
        frag_result_t result = frag_rx(frame_buf_pkt(), n, &frame_len, &transfer_ms);
        if(result == FRAG_DISCARD) continue;

        cam_cmd_learn(&src);
        if(result == FRAG_COMPLETE) {
            s_tick.transfer.ms   = transfer_ms;
            s_tick.transfer.done = true;
            s_tick.bytes.ms      = (int32_t)frame_len;
            s_tick.bytes.done    = true;
            frame_buf_publish(frame_len);
        }
    }
}
