#pragma once

// Logs why the chip last reset (power-on, panic, watchdog, brownout, ...).
// Call first thing in app_main — turns silent reboot loops into a visible story.
// Abnormal reasons (panic, watchdog, brownout) log at WARN so they show even
// with the default log level set to WARN.
void reset_info_log(void);
