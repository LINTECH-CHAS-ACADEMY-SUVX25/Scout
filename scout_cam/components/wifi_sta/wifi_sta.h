#pragma once

// Initialises NVS, the network interface, and connects to the Scout AP as STA.
// Blocks until an IP address is assigned. SSID and password are in rc_protocol.h.
void wifi_connect(void);
