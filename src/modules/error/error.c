#include <zephyr/sys/reboot.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include "message_channel.h"

LOG_MODULE_REGISTER(error);

void error_callback(const struct zbus_channel *chan)
{
	if (&FATAL_ERROR_CHAN == chan) {
		if (IS_ENABLED(CONFIG_ERROR_REBOOT_ON_FATAL)) {
			LOG_ERR("FATAL error, rebooting");
			LOG_PANIC();
			sys_reboot(0);
		}
	}
}

ZBUS_LISTENER_DEFINE(error, error_callback);
