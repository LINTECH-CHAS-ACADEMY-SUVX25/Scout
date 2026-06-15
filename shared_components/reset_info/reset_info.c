#include "reset_info.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "reset_info";

static const char *reason_name(esp_reset_reason_t r)
{
    switch(r) {
        case ESP_RST_POWERON:   return "power-on";
        case ESP_RST_EXT:       return "external pin";
        case ESP_RST_SW:        return "software restart";
        case ESP_RST_PANIC:     return "panic";
        case ESP_RST_INT_WDT:   return "interrupt watchdog";
        case ESP_RST_TASK_WDT:  return "task watchdog";
        case ESP_RST_WDT:       return "other watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep wake";
        case ESP_RST_BROWNOUT:  return "brownout";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "unknown";
    }
}

void reset_info_log(void)
{
    esp_reset_reason_t r = esp_reset_reason();
    bool abnormal = r == ESP_RST_PANIC || r == ESP_RST_INT_WDT || r == ESP_RST_TASK_WDT ||
                    r == ESP_RST_WDT || r == ESP_RST_BROWNOUT;
    if(abnormal)
        ESP_LOGW(TAG, "reset reason: %s (%d)", reason_name(r), r);
    else
        ESP_LOGI(TAG, "reset reason: %s (%d)", reason_name(r), r);
}
