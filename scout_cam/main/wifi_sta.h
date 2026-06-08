#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

// Initialises NVS, the network interface, and connects to the Scout AP as STA.
// Blocks until an IP address is assigned. SSID and password are in rc_protocol.h.
void wifi_connect(void);

// Returns true if currently associated with an AP.
bool wifi_sta_is_connected(void);

// Returns the RSSI of the current AP in dBm. Only valid when wifi_sta_is_connected().
int8_t wifi_sta_rssi(void);

// Returns the primary channel of the current AP.
uint8_t wifi_sta_channel(void);

#ifdef __cplusplus
}
#endif
