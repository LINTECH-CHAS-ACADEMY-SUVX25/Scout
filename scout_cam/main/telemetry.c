#include "telemetry.h"
#include "cam_state.h"
#include "udp.h"
#include "rc_protocol.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — periodically sends a cam_diag_pkt_t to scout_screen over UDP.
// Exists so the UART monitor on the screen side can show cam health without
// requiring a separate serial connection to the cam node.

#define DIAG_INTERVAL_MS 2000

static const char *TAG = "telemetry";

static void telemetry_run(void *arg)
{
    int sock = udp_open(0);
    if(sock < 0) { ESP_LOGE(TAG, "failed to open socket"); vTaskDelete(NULL); return; }
    struct sockaddr_in dest = udp_addr(S3_IP, DIAG_PORT);

    while(1) {
        cam_state_update_rssi();
        cam_diag_pkt_t pkt = {
            .temp_cdeg    = 0,
            .humidity_pct = 0,
            .pressure_pa  = 0,
            .free_heap    = esp_get_free_heap_size(),
            .rssi_dbm     = cam_status.rssi_dbm,
            .uptime_s     = (uint32_t)(esp_timer_get_time() / 1000000),
        };
        udp_tx(sock, &dest, &pkt, sizeof(pkt));
        vTaskDelay(pdMS_TO_TICKS(DIAG_INTERVAL_MS));
    }
}

void telemetry_init(void)
{
    xTaskCreate(telemetry_run, "telemetry", 2048, NULL, 2, NULL);
    ESP_LOGI(TAG, "sending diagnostics to %s:%d every %d ms", S3_IP, DIAG_PORT, DIAG_INTERVAL_MS);
}
