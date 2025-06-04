#ifndef MESSAGE_CHANNEL_H
#define MESSAGE_CHANNEL_H

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/sys/reboot.h>

#ifdef __cplusplus
extern "C" {
#endif


#define PAYLOAD_CHANNEL_STRING_MAX_SIZE 100

/** @brief Macro used to send a message on the FATAL_ERROR_CHANNEL.
 *	   The message will be handled in the error module.
 */
#define SEND_FATAL_ERROR()									\
	int not_used = -1;									\
	if (zbus_chan_pub(&FATAL_ERROR_CHAN, &not_used, K_SECONDS(10))) {			\
		LOG_ERR("Sending a message on the fatal error channel failed, rebooting");	\
		IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)));					\
	}

typedef struct {
	int16_t temperature;
	uint16_t humidity;
	uint16_t sequence_number;
} measurement_t;

ZBUS_CHAN_DECLARE(TRIGGER_CHAN, PAYLOAD_CHAN, FATAL_ERROR_CHAN);

#ifdef __cplusplus
}
#endif

#endif // MESSAGE_CHANNEL_H
