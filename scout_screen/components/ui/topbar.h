#pragma once
#include <stdint.h>

/** @brief Builds all top-bar widgets and attaches them to s_root. */
void topbar_build(void);

/** @brief Repaints wifi arc and slash colours after a theme change. */
void topbar_refresh_theme(void);

void scout_ui_update(uint8_t wifi_level);
