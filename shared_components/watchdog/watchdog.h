#pragma once

// Timeout applied to every registered task and to the per-core idle tasks.
// Every registered task loops well under this; a longer stall means a real hang.
#define WATCHDOG_TIMEOUT_MS 5000

// Arms the task watchdog and enables a panic-reboot on timeout, so a task that
// stops feeding it forces the system to recover. Call once at startup, before
// any task registers.
void watchdog_init(void);

// Registers the calling task with the task watchdog timer.
void watchdog_register(void);

// Resets the watchdog for the calling task. Must be called within the WDT timeout period.
void watchdog_reset(void);
