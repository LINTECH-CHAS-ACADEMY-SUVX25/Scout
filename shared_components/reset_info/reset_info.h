#pragma once

/**
 * @brief Logs why the chip last reset (power-on, panic, watchdog, brownout, ...).
 *        Call first thing in app_main. Abnormal reasons log at WARN level.
 */
void reset_info_log(void);
