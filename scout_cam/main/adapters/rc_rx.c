#include "rc_rx.h"
#include "cam_state.h"
#include "motor_queue.h"
#include "udp.h"
#include "rc_protocol.h"
#include "esp_log.h"

#define SILENT_FRAMES_MAX 150

static const char *TAG = "rc_rx";
static int s_silent_frames;

void rc_rx_init(void)
{
    s_silent_frames = 0;
}

void rc_rx_process(int cmd_sock)
{
    joy_pkt_t pkt;
    if(udp_try_recv(cmd_sock, &pkt, sizeof(pkt)) == sizeof(pkt)) {
        motor_queue_send(pkt.x, pkt.y);
        if(!cam_status.screen_online) { ESP_LOGI(TAG, "screen online"); cam_status.screen_online = true; }
        cam_status.streaming = true;
        s_silent_frames = 0;
        while(udp_try_recv(cmd_sock, &pkt, sizeof(pkt)) == sizeof(pkt))
            motor_queue_send(pkt.x, pkt.y);
    } else if(cam_status.screen_online && ++s_silent_frames == SILENT_FRAMES_MAX) {
        ESP_LOGW(TAG, "screen silent — pausing stream");
        cam_status.screen_online = false;
        cam_status.streaming     = false;
        s_silent_frames          = 0;
    }
}
