
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"
#include "utils.h"

LOG_MODULE_REGISTER(trigger);

// sequence number, will be updated whenever a new measurement is recorded
// set initial value to 65535 to indicate "not available" if, for some reason, the sequence number
// is not updated
static uint16_t sequence_number = SEQUENCE_NUMER_INVALID;

static void message_send(void)
{
	// update sequence number in advertisement data
	sequence_number++;
	// reset sequence number if its > 65534, as the max allowed value is 65534
	// 65535 is reserved for "not available"
	if (sequence_number > SEQUENCE_NUMER_INVALID - 1) {
		// reset sequence number, if greater than allowed value
		// sequence number will be reset every ~23 days with advertisements every 30 seconds
		sequence_number = 0;
	}

	int err = zbus_chan_pub(&TRIGGER_CHAN, &sequence_number, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}
}

static void trigger_task(void)
{
	while (true) {
		message_send();
		k_sleep(K_SECONDS(CONFIG_TRIGGER_TIMEOUT_SECONDS));
	}
}

K_THREAD_DEFINE(trigger_task_id, CONFIG_TRIGGER_THREAD_STACK_SIZE, trigger_task, NULL, NULL, NULL,
		3, 0, 0);
