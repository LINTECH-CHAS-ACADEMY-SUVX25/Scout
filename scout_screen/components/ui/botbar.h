#pragma once

/** @brief Builds the bottom status bar and attaches it to s_root. */
void botbar_build(void);

/** @brief Updates the active theme name label in the bottom bar. */
void botbar_set_theme_name(const char *name);
