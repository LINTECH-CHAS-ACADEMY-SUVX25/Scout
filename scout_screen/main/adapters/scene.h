#pragma once

/**
 * @brief Applies widget changes for the active scene when it differs from the one currently shown.
 *        Single edge-detect for all UI mode changes. Call once per render loop tick,
 *        from the render task only (LVGL is not thread-safe).
 */
void scene_render(void);
