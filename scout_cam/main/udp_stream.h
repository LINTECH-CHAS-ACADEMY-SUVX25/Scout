#pragma once

// Starts the UDP stream task: captures JPEG frames, fragments them, and sends
// them to the screen. Also drains inbound CMD bytes from CMD_PORT and forwards
// them to the motor task.
void udp_stream_start(void);
