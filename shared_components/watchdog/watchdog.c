#include "watchdog.h"
#include "freertos/FreeRTOS.h"
#include "esp_task_wdt.h"
#include "esp_log.h"

static const char *TAG = "watchdog";

void watchdog_init(const watchdog_config_t *cfg)
{
    esp_task_wdt_config_t config = {
        .timeout_ms     = cfg ? cfg->timeout_ms     : WATCHDOG_TIMEOUT_MS,
        .idle_core_mask = cfg ? cfg->idle_core_mask : (uint32_t)((1 << configNUMBER_OF_CORES) - 1),
        .trigger_panic  = true,
    };

    // The TWDT is auto-started by CONFIG_ESP_TASK_WDT_INIT, so init returns
    // INVALID_STATE — reconfigure the running instance in that case.
    esp_err_t err = esp_task_wdt_init(&config);
    if(err == ESP_ERR_INVALID_STATE) err = esp_task_wdt_reconfigure(&config);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "init failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "armed: %d ms timeout, reboot on hang", WATCHDOG_TIMEOUT_MS);
}

void watchdog_register(void) { esp_task_wdt_add(NULL); }
void watchdog_reset(void)    { esp_task_wdt_reset(); }
