#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(trigger);

static void message_send(void)
{
	int not_used = -1;
	int err;

	err = zbus_chan_pub(&TRIGGER_CHAN, &not_used, K_SECONDS(1));
	if (err) {
		LOG_ERR("zbus_chan_pub, error: %d", err);
		SEND_FATAL_ERROR();
	}
}

static void trigger_task(void)
{
	while (true) {
		message_send();
		k_sleep(K_SECONDS(30));
	}
}

K_THREAD_DEFINE(trigger_task_id,
		1024,
		trigger_task, NULL, NULL, NULL, 3, 0, 0);