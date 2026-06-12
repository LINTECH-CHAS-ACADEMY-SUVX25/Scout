#pragma once

// Starts the background task that listens on DIAG_PORT and writes each received
// diagnostics packet into screen_state's cam_status.
void cam_diag_init(void);
