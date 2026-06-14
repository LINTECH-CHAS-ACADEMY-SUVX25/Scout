#pragma once

// Render-owned scene reactions. Applies the widget changes for the active scene
// when it differs from the one currently shown — the single edge-detect for all
// UI mode changes. Call once per render loop tick, from the render task only
// (LVGL is core 1-only and not thread-safe).
void scene_render(void);
