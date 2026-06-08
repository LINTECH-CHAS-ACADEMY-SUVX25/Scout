#include "watchdog.h"
#include "esp_task_wdt.h"

void watchdog_register(void) { esp_task_wdt_add(NULL); }
void watchdog_reset(void)    { esp_task_wdt_reset(); }
