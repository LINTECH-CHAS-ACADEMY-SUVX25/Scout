#include "cam_state.h"
#include "motor_cmd.h"
#include "wifi_sta.h"
#include "udp.h"
#include "esp_log.h"
#include <stdint.h>

#define SILENT_FRAMES_MAX 150

static const char *TAG = "cam_state";

cam_status_t cam_status;

void cam_state_update_rssi(void)
{
    cam_status.rssi_dbm = wifi_sta_get_rssi();
}

static bool s_reconnect_pending;
static int  s_silent_frames;

void cam_state_try_resume(int sock)
{
    bool now_connected = wifi_sta_is_connected();
    if(!now_connected) s_reconnect_pending = true;
    if(s_reconnect_pending && now_connected) {
        ESP_LOGI(TAG, "wifi reconnected — resuming stream");
        cam_status.streaming = true;
        s_reconnect_pending = false;
    }
    uint8_t cmd;
    if(udp_try_recv(sock, &cmd, 1) == 1) {
        ESP_LOGD(TAG, "cmd: 0x%02x", cmd);
        motor_cmd_send(cmd);
        cam_status.screen_online = true;
        cam_status.streaming = true;
        s_silent_frames = 0;
        ESP_LOGI(TAG, "screen online — resuming stream");
    }
}

void cam_state_process_cmds(int sock)
{
    uint8_t cmd;
    if(udp_try_recv(sock, &cmd, 1) == 1) {
        ESP_LOGD(TAG, "cmd: 0x%02x", cmd);
        motor_cmd_send(cmd);
        if(!cam_status.screen_online) { ESP_LOGI(TAG, "screen online"); cam_status.screen_online = true; }
        s_silent_frames = 0;
        while(udp_try_recv(sock, &cmd, 1) == 1) {
            ESP_LOGD(TAG, "cmd: 0x%02x", cmd);
            motor_cmd_send(cmd);
        }
    } else if(cam_status.screen_online && ++s_silent_frames == SILENT_FRAMES_MAX) {
        ESP_LOGW(TAG, "screen silent — pausing stream");
        cam_status.screen_online = false;
        cam_status.streaming = false;
        s_silent_frames = 0;
    }
}
