#include "screen_state.h"
#include "esp_timer.h"
#include "esp_log.h"

// Shared state hub for scout_screen tasks.
// Stores connection status, the active scene, and cam-diag telemetry.
// Liveness (is_streaming/has_streamed) is updated via screen_state_mark_rx_time,
// called by screen_stats when a transfer slot commits.

static const char *TAG = "screen_state";

screen_status_t screen_status;

static volatile scene_t s_scene = SCENE_BOOTING;

static const char *s_scene_names[SCENE_COUNT] = {
    [SCENE_BOOTING]      = "booting",
    [SCENE_WAITING]      = "waiting",
    [SCENE_STREAMING]    = "streaming",
    [SCENE_DISCONNECTED] = "disconnected",
};

static volatile uint32_t s_last_rx_ms;

static cam_diag_pkt_t s_cam_status;
static volatile bool  s_cam_dirty;

void screen_state_mark_rx_time(uint32_t now_ms)
{
    s_last_rx_ms = now_ms;
}

void screen_state_set_scene(scene_t s)
{
    if(s == s_scene) return;
    ESP_LOGI(TAG, "scene %s -> %s", s_scene_names[s_scene], s_scene_names[s]);
    s_scene = s;
}

scene_t screen_state_get_scene(void)
{
    return s_scene;
}

const char *screen_state_scene_name(scene_t s)
{
    return s < SCENE_COUNT ? s_scene_names[s] : "?";
}

bool screen_state_is_streaming(void)
{
    return s_last_rx_ms &&
           (uint32_t)(esp_timer_get_time() / 1000) - s_last_rx_ms < 2000;
}

bool screen_state_has_streamed(void)
{
    return s_last_rx_ms != 0;
}

void screen_state_set_cam(const cam_diag_pkt_t *pkt)
{
    s_cam_status = *pkt;
    s_cam_dirty  = true;
}

void screen_state_get_cam(cam_diag_pkt_t *out)
{
    *out = s_cam_status;
}

bool screen_state_cam_dirty_take(void)
{
    bool dirty  = s_cam_dirty;
    s_cam_dirty = false;
    return dirty;
}
