#include "event_bus.h"

#define EVENT_QUEUE_SIZE 10

K_MSGQ_DEFINE(event_queue, sizeof(app_event_t), EVENT_QUEUE_SIZE, 8);

int event_bus_post(app_event_t *evt) {
    return k_msgq_put(&event_queue, evt, K_NO_WAIT);
}

int event_bus_get(app_event_t *evt, k_timeout_t timeout) {
    return k_msgq_get(&event_queue, evt, timeout);
}