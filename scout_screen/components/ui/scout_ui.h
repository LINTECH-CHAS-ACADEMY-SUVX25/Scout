#pragma once
#include "rc_protocol.h"
#include <stdint.h>
#include <stdbool.h>

/** @brief Builds the full UI layout on the active screen. LVGL must be initialised. */
void    scout_ui_init(void);

/**
 * @brief Shows the intro overlay with a loading bar that fills over total_steps
 *        calls to scout_ui_intro_step(). Not thread safe — call both functions
 *        before the render task starts.
 */
void    scout_ui_intro_screen(uint8_t total_steps);

/**
 * @brief Advances the loading bar one step and renders to the screen immediately.
 *        After the last step the overlay removes itself once the render loop is running.
 */
void    scout_ui_intro_step(const char *label);

/** @brief Updates the WiFi signal symbol: 0 none (red dot), 1 low, 2 mid, 3 full. */
void    scout_ui_update(uint8_t wifi_level);

/** @brief Updates the temperature/humidity/pressure readouts from a cam diag packet. Render task only. */
void    scout_ui_update_telemetry(const cam_diag_pkt_t *d);

/** @brief Shows text on the overlay covering the camera region, or hides it when NULL. Render task only. */
void    scout_ui_overlay(const char *text);

void    scout_ui_set_theme(uint8_t idx);

/**
 * @brief Returns the current joystick position scaled to -255..255 per axis.
 *        Positive x = right, positive y = forward (away from player).
 */
void scout_ui_get_joy(int16_t *x, int16_t *y);

/** @brief Returns true (once) when the APPLY button was pressed since the last call. */
bool scout_ui_cfg_dirty_take(void);

/** @brief Reads current config panel widget values. Only valid after scout_ui_init. */
void scout_ui_get_cam_cfg(bool *cam_on, int8_t *quality,
                           int8_t *ae_level, int8_t *agc_gain);
