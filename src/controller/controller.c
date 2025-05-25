#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "controller.h"
#include "../events/event_bus.h"

LOG_MODULE_REGISTER(orchestrator);

#define MEASUREMENT_INTERVAL K_SECONDS(30)
#define INITIAL_DELAY        K_SECONDS(2)

static void sample_timer_handler(struct k_timer *timer_id);

K_TIMER_DEFINE(sample_timer, sample_timer_handler, NULL);

static void sample_timer_handler(struct k_timer *timer_id)
{
	app_event_t evt = {.type = SENSOR_SAMPLE_EVENT};
	int ret = event_bus_post(&evt);
	if (ret == 0) {
		LOG_INF("Posted SENSOR_SAMPLE_EVENT");
	} else {
		LOG_WRN("Failed to post SENSOR_SAMPLE_EVENT");
	}
}

void controller_init(void)
{
	k_timer_start(&sample_timer, INITIAL_DELAY, MEASUREMENT_INTERVAL);
	LOG_INF("Measurement timer cycle initialized");
}

void controller_run(void)
{
}