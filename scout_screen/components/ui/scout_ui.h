#pragma once
#include "rc_protocol.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Builds the full UI layout on the active screen.
 * @pre    LVGL must be initialised.
 */
void    scout_ui_init(void);

/**
 * @brief  Shows the intro overlay with a loading bar that fills over total_steps
 *         calls to scout_ui_intro_step().
 * @param  total_steps  Number of steps before the overlay auto-hides.
 * @note   Not thread-safe — call before the render task starts.
 */
void    scout_ui_intro_screen(uint8_t total_steps);

/**
 * @brief  Advances the loading bar one step and renders to the screen immediately.
 *         After the last step the overlay removes itself once the render loop is running.
 * @param  label  Text shown next to the progress step (e.g. "WIFI", "READY").
 * @note   Not thread-safe — call before the render task starts.
 */
void    scout_ui_intro_step(const char *label);

/**
 * @brief  Updates the WiFi signal symbol.
 * @param  wifi_level  0 = no signal (red dot), 1 = low, 2 = mid, 3 = full.
 */
void    scout_ui_update(uint8_t wifi_level);

/**
 * @brief  Updates the temperature/humidity/pressure readouts from a cam diag packet.
 * @param  d  Latest diagnostics packet from the camera.
 * @note   Render task only.
 */
void    scout_ui_update_telemetry(const cam_diag_pkt_t *d);

/**
 * @brief  Shows text on the overlay covering the camera region, or hides it when NULL.
 * @param  text  String to display, or NULL to hide the overlay.
 * @note   Render task only.
 */
void    scout_ui_overlay(const char *text);

void    scout_ui_set_theme(uint8_t idx);

/**
 * @brief  Returns the current joystick position scaled to -255..255 per axis.
 *         Positive x = right, positive y = forward (away from player).
 * @param[out] x  Horizontal axis value.
 * @param[out] y  Vertical axis value.
 */
void scout_ui_get_joy(int16_t *x, int16_t *y);

/** @brief Returns true (once) when the APPLY button was pressed since the last call. */
bool scout_ui_cfg_dirty_take(void);

/**
 * @brief  Reads current config panel widget values.
 * @param[out] cam_on    Whether the camera stream is enabled.
 * @param[out] quality   JPEG quality (0–63, lower = better).
 * @param[out] ae_level  Auto-exposure level (0 = AEC, 1–10 = manual).
 * @param[out] agc_gain  Auto-gain level (0 = AGC, 1–10 = manual).
 * @pre    scout_ui_init() must have been called.
 */
void scout_ui_get_cam_cfg(bool *cam_on, int8_t *quality, int8_t *ae_level, int8_t *agc_gain);
