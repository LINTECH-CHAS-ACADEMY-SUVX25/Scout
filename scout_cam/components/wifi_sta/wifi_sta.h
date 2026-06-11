#pragma once

// Initialises NVS, the network interface, and connects to the Scout AP as STA.
// Blocks until an IP address is assigned. SSID and password are in rc_protocol.h.
void wifi_connect(void);

// Returns true while the station has an IP address assigned.
bool wifi_sta_is_connected(void);

// Returns the RSSI of the connected AP in dBm, or 0 if not connected.
int8_t wifi_sta_get_rssi(void);
