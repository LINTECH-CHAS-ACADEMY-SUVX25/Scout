#include "cam_state.h"
#include "motor_queue.h"
#include "rc_protocol.h"
#include "wifi_sta.h"
#include "camera.h"
#include "udp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define SILENT_FRAMES_MAX     150
#define CAMERA_INIT_ATTEMPTS  3

// Backoff before each attempt — power-on glitches often succeed on attempt 2.
static const uint16_t s_camera_backoff_ms[CAMERA_INIT_ATTEMPTS] = { 0, 500, 2000 };

static const char *TAG = "cam_state";

cam_status_t cam_status;

bool cam_state_camera_start(void)
{
    for(int i = 0; i < CAMERA_INIT_ATTEMPTS; i++) {
        if(s_camera_backoff_ms[i]) vTaskDelay(pdMS_TO_TICKS(s_camera_backoff_ms[i]));
        if(camera_init() == ESP_OK) return true;
        ESP_LOGW(TAG, "camera init attempt %d/%d failed", i + 1, CAMERA_INIT_ATTEMPTS);
    }
    ESP_LOGE(TAG, "camera unavailable — running degraded (motors + telemetry, no video)");
    cam_status.camera_fault = true;
    ESP_LOGE(TAG, "camera init failed — rebooting");
    esp_restart();
    return false;
}

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
        motor_queue_send(pkt.x, pkt.y);
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
        motor_queue_send(pkt.x, pkt.y);
        if(!cam_status.screen_online) { ESP_LOGI(TAG, "screen online"); cam_status.screen_online = true; }
        s_silent_frames = 0;
        while(udp_try_recv(sock, &pkt, sizeof(pkt)) == sizeof(pkt))
            motor_queue_send(pkt.x, pkt.y);
    } else if(cam_status.screen_online && ++s_silent_frames == SILENT_FRAMES_MAX) {
        ESP_LOGW(TAG, "screen silent — pausing stream");
        cam_status.screen_online = false;
        cam_status.streaming = false;
        s_silent_frames = 0;
    }
}
