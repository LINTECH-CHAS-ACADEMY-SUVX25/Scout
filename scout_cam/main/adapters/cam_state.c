#include "cam_state.h"
#include "camera.h"
#include "wifi_sta.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CAMERA_INIT_ATTEMPTS  3

static const uint16_t s_camera_backoff_ms[CAMERA_INIT_ATTEMPTS] = { 0, 500, 2000 };

static const char *TAG = "cam_state";

cam_status_t cam_status;

void cam_state_init(void)
{
    cam_status.camera_enabled = true;
    cam_status.sensor_enabled = true;
}

bool cam_state_camera_start(void)
{
    for(int i = 0; i < CAMERA_INIT_ATTEMPTS; i++) {
        if(s_camera_backoff_ms[i]) vTaskDelay(pdMS_TO_TICKS(s_camera_backoff_ms[i]));
        if(camera_init() == ESP_OK) return true;
        ESP_LOGW(TAG, "camera init attempt %d/%d failed", i + 1, CAMERA_INIT_ATTEMPTS);
    }
    ESP_LOGE(TAG, "camera init failed after %d attempts — rebooting", CAMERA_INIT_ATTEMPTS);
    esp_restart();
    return false;
}

static bool s_reconnect_pending;

void cam_state_update_rssi(void)
{
    cam_status.rssi_dbm = wifi_sta_get_rssi();
}

void cam_state_try_resume(void)
{
    bool now_connected = wifi_sta_is_connected();
    if(!now_connected) s_reconnect_pending = true;
    if(s_reconnect_pending && now_connected) {
        ESP_LOGI(TAG, "wifi reconnected — resuming stream");
        cam_status.streaming = true;
        s_reconnect_pending = false;
    }
}

void cam_state_apply_ctrl(const cam_ctrl_pkt_t *pkt)
{
    switch(pkt->cmd) {
        case CAM_CTRL_CAMERA_ON:  cam_status.camera_enabled = true;  break;
        case CAM_CTRL_CAMERA_OFF: cam_status.camera_enabled = false; break;
        case CAM_CTRL_SENSOR_ON:  cam_status.sensor_enabled = true;  break;
        case CAM_CTRL_SENSOR_OFF: cam_status.sensor_enabled = false; break;
        default: camera_apply_setting(pkt->cmd, pkt->value);         break;
    }
}
