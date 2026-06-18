#include "motor_queue.h"

void motor_queue_init(void) {}
void motor_queue_send(int16_t x, int16_t y) { (void)x; (void)y; }
bool motor_queue_recv(int16_t *x, int16_t *y, uint32_t timeout_ms)
    { (void)x; (void)y; (void)timeout_ms; return false; }
