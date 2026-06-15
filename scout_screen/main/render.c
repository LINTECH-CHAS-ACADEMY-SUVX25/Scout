#include "render.h"
#include "frame_pool.h"
#include "screen_state.h"
#include "screen_stats.h"
#include "scene.h"
#include "rc_tx.h"
#include "cfg_tx.h"
#include "lvgl_port.h"
#include "scout_ui.h"
#include "display.h"
#include "jpeg.h"
#include "watchdog.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — drives the display on core 1.
// Each tick: applies scene changes, renders the LVGL frame, sends the current RC
// command, and blits any newly decoded camera frame into the framebuffer.
// Exists to keep all display and RC output on a dedicated core away from networking.

#define CAM_X  ((SCREEN_W - CAM_W) / 2)
#define CAM_Y  ((SCREEN_H - CAM_H) / 2)

static const char *TAG = "render";
static screen_tick_t  s_tick;

static uint8_t rssi_bars(int8_t rssi_dbm)
{
    if(rssi_dbm >= 0 || rssi_dbm < -80) return 1;
    if(rssi_dbm < -65)                  return 2;
    if(rssi_dbm < -50)                  return 3;
    return 4;
}

static void render_run(void *arg);

void render_init(void)
{
    jpeg_init_canvas(CAM_W, CAM_H);
    screen_stats_render_tick_init(&s_tick);
    lvgl_port_set_video_region(CAM_X, CAM_Y, CAM_W, CAM_H);
    TaskHandle_t handle;
    xTaskCreatePinnedToCore(render_run, "render", 8192, NULL, 4, &handle, 1);
    frame_pool_set_render_task(handle);
    ESP_LOGI(TAG, "canvas %dx%d allocated", CAM_W, CAM_H);
}

static void render_run(void *arg)
{
    watchdog_register();

    while(1) {
        watchdog_reset();
        screen_stats_tick(&s_tick);

        if(screen_state_cam_dirty_take()) {
            cam_diag_pkt_t cam;
            screen_state_get_cam(&cam);
            scout_ui_update_telemetry(&cam);
            if(screen_state_get_scene() == SCENE_STREAMING)
                scout_ui_update(rssi_bars(cam.rssi_dbm));
        }

        scene_render();

        // Protect the camera region from LVGL while a live frame owns it, so a
        // full-screen redraw (e.g. a theme switch) can't paint over the video.
        bool streaming = screen_status.streaming;
        lvgl_port_video_lock(streaming);

        lvgl_port_render_frame();
        screen_stats_tick_split(&s_tick, &s_tick.lvgl);

        int16_t jx, jy;
        scout_ui_get_joy(&jx, &jy);
        rc_tx_send_throttled(jx, jy);
        cfg_tx_flush();

        // Only blit when a new frame was decoded. LVGL redraws just its dirty areas which don't overlap the camera region
        const uint8_t *src;
        uint32_t       src_len;
        if(frame_pool_try_acquire(&src, &src_len)) {
            bool ok = jpeg_decode_rgb565(src, (int)src_len,
                                         (uint8_t *)jpeg_canvas_get(),
                                         CAM_W * CAM_H * sizeof(uint16_t), NULL, NULL);
            screen_stats_tick_split(&s_tick, &s_tick.decode);
            frame_pool_release();

            // Gated on streaming so a buffered frame can't overwrite the scene overlay
            // (the blit bypasses LVGL, so LVGL would not know to redraw the overlay).
            if(ok && streaming) {
                display_blit_region(CAM_X, CAM_Y, CAM_W, CAM_H, jpeg_canvas_get());
                screen_stats_tick_split(&s_tick, &s_tick.blit);
            }
        }

        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
    }
}
