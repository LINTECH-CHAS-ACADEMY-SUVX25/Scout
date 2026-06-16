#pragma once
#include <stdint.h>

/**
 * @brief Inits NVS, the network interface, and starts a WPA2 WiFi AP.
 *        Blocks until the AP is up.
 */
void wifi_ap_start(void);

int wifi_ap_sta_count(void);
