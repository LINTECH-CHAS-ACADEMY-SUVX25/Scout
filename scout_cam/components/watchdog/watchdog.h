#pragma once

// Registers the calling task with the task watchdog timer.
void watchdog_register(void);

// Resets the watchdog for the calling task. Must be called within the WDT timeout period.
void watchdog_reset(void);
