#pragma once
#include <stdint.h>

// Initialises NVS, the network interface, and starts a WPA2 WiFi AP.
// Blocks until the AP is up. SSID and password are defined in rc_protocol.h.
void wifi_ap_start(void);

// Returns the number of stations currently connected to the AP.
int wifi_ap_sta_count(void);
