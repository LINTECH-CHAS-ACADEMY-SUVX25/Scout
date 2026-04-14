/**
 * @file ui_dashboard.h
 * @brief Scout LVGL Dashboard – 1024×600 split layout
 *
 * Layout:
 *  ┌──────── 640 px ────────┬────── 384 px ──────┐
 *  │                        │  TELEMETRI header   │
 *  │   CAMERA FEED 640×480  │  Miljö  │  Gas      │
 *  │   + HUD overlay        │  Termisk│  IMU      │
 *  │   + dropdown sensors   │  Partik │  Range    │
 *  │                        │  [ALERT BAR]        │
 *  ├────────────────────────│  ─────────────────  │
 *  │  sensor chips │ readout│  Btns   │ Joystick  │
 *  └────────────────────────┴─────────────────────┘
 *         120 px high                 ~160 px
 */

#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#include "lvgl.h"
#include "sensor_hal.h"

/** Screen dimensions (Waveshare 7" LCD) */
#define SCREEN_W    1024
#define SCREEN_H    600

/** Camera feed area */
#define CAM_W       640
#define CAM_H       480

/** Right panel */
#define RPANEL_W    (SCREEN_W - CAM_W)          /* 384 */
#define RPANEL_H    SCREEN_H                    /* 600 */

/** Bottom bar (under camera) */
#define BBAR_W      CAM_W
#define BBAR_H      (SCREEN_H - CAM_H)         /* 120 */

/**
 * Create the full Scout UI on the given screen.
 */
void ui_dashboard_create(lv_obj_t *parent);

/**
 * Update all widgets with new data.
 */
void ui_dashboard_update(const scout_sensors_t *data,
                         alert_level_t alert,
                         const joystick_input_t *joy);

/**
 * Toggle the sensor-data dropdown overlay on the camera.
 */
void ui_dashboard_toggle_dropdown(void);

#endif /* UI_DASHBOARD_H */
