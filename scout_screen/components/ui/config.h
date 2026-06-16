#pragma once
#include <stdbool.h>
#include <stdint.h>

/** @brief Builds the camera config panel (initially hidden). */
void config_panel_build(void);

/** @brief Shows the config panel if hidden, hides it if visible. */
void ui_config_toggle(void);

bool scout_ui_cfg_dirty_take(void);
void scout_ui_get_cam_cfg(bool *cam_on, int8_t *quality, int8_t *ae_level, int8_t *agc_gain);
