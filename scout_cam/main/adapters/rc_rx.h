#pragma once

// Initialises internal state (silent-frame counter). Call from stream_init().
void rc_rx_init(void);

// Drains all pending joy_pkt_t from cmd_sock and forwards each to the motor task.
// Updates cam_status.screen_online and cam_status.streaming based on packet activity.
// Call after each frame transmission and in the pause/resume path.
void rc_rx_process(int cmd_sock);
