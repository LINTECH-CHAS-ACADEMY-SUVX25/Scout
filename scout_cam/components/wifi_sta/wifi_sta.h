#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inits NVS, the network interface, and connects to the Scout AP as STA.
 *        Blocks until an IP address is assigned.
 */
void wifi_connect(void);

bool wifi_sta_is_connected(void);

/** @brief Returns the RSSI of the connected AP in dBm, or 0 if not connected. */
int8_t wifi_sta_get_rssi(void);
