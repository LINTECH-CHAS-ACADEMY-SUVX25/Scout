#include "cam_state.h"
#include "motor_cmd.h"
#include "rc_protocol.h"
#include "wifi_sta.h"
#include "udp.h"
#include "esp_log.h"

#define SILENT_FRAMES_MAX 150

static const char *TAG = "cam_state";

cam_status_t cam_status;

static bool s_reconnect_pending;
static int  s_silent_frames;

void cam_state_update_rssi(void)
{
    cam_status.rssi_dbm = wifi_sta_get_rssi();
}

void cam_state_try_resume(int sock)
{
    bool now_connected = wifi_sta_is_connected();
    if(!now_connected) s_reconnect_pending = true;
    if(s_reconnect_pending && now_connected) {
        ESP_LOGI(TAG, "wifi reconnected — resuming stream");
        cam_status.streaming = true;
        s_reconnect_pending = false;
    }
    joy_pkt_t pkt;
    if(udp_try_recv(sock, &pkt, sizeof(pkt)) == sizeof(pkt)) {
        motor_cmd_send(pkt.x, pkt.y);
        cam_status.screen_online = true;
        cam_status.streaming = true;
        s_silent_frames = 0;
        ESP_LOGI(TAG, "screen online — resuming stream");
    }
}

void cam_state_process_cmds(int sock)
{
    joy_pkt_t pkt;
    if(udp_try_recv(sock, &pkt, sizeof(pkt)) == sizeof(pkt)) {
        motor_cmd_send(pkt.x, pkt.y);
        if(!cam_status.screen_online) { ESP_LOGI(TAG, "screen online"); cam_status.screen_online = true; }
        s_silent_frames = 0;
        while(udp_try_recv(sock, &pkt, sizeof(pkt)) == sizeof(pkt))
            motor_cmd_send(pkt.x, pkt.y);
    } else if(cam_status.screen_online && ++s_silent_frames == SILENT_FRAMES_MAX) {
        ESP_LOGW(TAG, "screen silent — pausing stream");
        cam_status.screen_online = false;
        cam_status.streaming = false;
        s_silent_frames = 0;
    }
}
