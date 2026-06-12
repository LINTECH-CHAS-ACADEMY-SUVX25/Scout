#include "cam_diag.h"
#include "udp.h"
#include "rc_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Task — listens for cam_diag_pkt_t packets from scout_cam on DIAG_PORT.
// Updates cam_diag_latest on each valid packet; the monitor task reads it for CAMDIAG.

static const char *TAG = "cam_diag";

cam_diag_pkt_t cam_diag_latest;

static void cam_diag_run(void *arg)
{
    int sock = udp_open(DIAG_PORT);
    if(sock < 0) { ESP_LOGE(TAG, "failed to open socket"); vTaskDelete(NULL); return; }
    ESP_LOGI(TAG, "listening on port %d", DIAG_PORT);

    while(1) {
        cam_diag_pkt_t pkt;
        int n = udp_rx(sock, &pkt, sizeof(pkt), NULL);
        if(n == (int)sizeof(pkt))
            cam_diag_latest = pkt;
    }
}

void cam_diag_init(void)
{
    xTaskCreatePinnedToCore(cam_diag_run, "cam_diag", 2048, NULL, 3, NULL, 0);
}
