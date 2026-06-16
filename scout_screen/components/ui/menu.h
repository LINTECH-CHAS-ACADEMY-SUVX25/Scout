#pragma once
#include <stdint.h>

/** @brief Builds the theme-selector dropdown and attaches it to s_root. */
void theme_menu_build(void);

/** @brief Shows the theme menu if hidden, hides it if visible. */
void ui_menu_toggle(void);

/** @brief Marks theme entry @p idx as active (shows dot), clears all others. */
void menu_set_active(uint8_t idx);
