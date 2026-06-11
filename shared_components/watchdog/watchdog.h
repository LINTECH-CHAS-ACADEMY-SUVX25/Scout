#pragma once
#include <stdint.h>

#define WATCHDOG_TIMEOUT_MS 5000

typedef struct {
    uint32_t timeout_ms;
    uint32_t idle_core_mask;
} watchdog_config_t;

// Arms the task watchdog. Pass NULL to use defaults (WATCHDOG_TIMEOUT_MS,
// idle tasks on all cores monitored). Call once at startup before any task registers.
void watchdog_init(const watchdog_config_t *cfg);

// Registers the calling task with the task watchdog timer.
void watchdog_register(void);

// Resets the watchdog for the calling task. Must be called within the WDT timeout period.
void watchdog_reset(void);
