#include "scene.h"
#include "screen_state.h"
#include "lvgl_port.h"

// Maps each scene to its UI reaction. Tasks signal mode changes from anywhere via
// screen_state_set_scene; this file is the only place widgets react to them.
// Adding a scene = one scene_t value, one table row, and the sites that set it.

typedef struct {
    bool        cam_connected;  // connection indicator state (dot + label)
    const char *overlay_text;   // text over the camera region; NULL hides the overlay
} scene_cfg_t;

static const scene_cfg_t s_scenes[SCENE_COUNT] = {
    [SCENE_BOOTING]      = { .cam_connected = false, .overlay_text = NULL },
    [SCENE_WAITING]      = { .cam_connected = false, .overlay_text = "WAITING FOR CAM" },
    [SCENE_STREAMING]    = { .cam_connected = true,  .overlay_text = NULL },
    [SCENE_DISCONNECTED] = { .cam_connected = false, .overlay_text = "CAM DISCONNECTED" },
};

static scene_t s_shown = SCENE_BOOTING;

void scene_render(void)
{
    scene_t want = screen_state_get_scene();
    if(want == s_shown) return;

    const scene_cfg_t *cfg = &s_scenes[want];
    lvgl_port_ui_update(cfg->cam_connected);
    lvgl_port_overlay(cfg->overlay_text);
    s_shown = want;
}
