#pragma once
#include <stdint.h>

#define WATCHDOG_TIMEOUT_MS 5000

typedef struct {
    uint32_t timeout_ms;
    uint32_t idle_core_mask;
} watchdog_config_t;

/**
 * @brief Arms the task watchdog. Pass NULL to use defaults (WATCHDOG_TIMEOUT_MS,
 *        idle tasks on all cores monitored). Call once at startup before any task registers.
 */
void watchdog_init(const watchdog_config_t *cfg);

void watchdog_register(void);
void watchdog_reset(void);
