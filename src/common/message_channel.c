#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "message_channel.h"
#include "utils.h"
#include "led.h"

ZBUS_CHAN_DEFINE(TRIGGER_CHAN,                         /* Name */
		 uint16_t,                             /* Message type */
		 NULL,                                 /* Validator */
		 NULL,                                 /* User data */
		 ZBUS_OBSERVERS(sampler),              /* Observers */
		 ZBUS_MSG_INIT(SEQUENCE_NUMER_INVALID) /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(PAYLOAD_CHAN,                /* Name */
		 measurement_t,               /* Message type */
		 NULL,                        /* Validator */
		 NULL,                        /* User data */
		 ZBUS_OBSERVERS(advertising), /* Observers */
		 ZBUS_MSG_INIT(.temperature = TEMP_INVALID_MEASUREMENT,
			       .humidity = HUM_INVALID_MEASUREMENT,
			       .sequence_number = SEQUENCE_NUMER_INVALID)); /* Initial value */

ZBUS_CHAN_DEFINE(FATAL_ERROR_CHAN,      /* Name */
		 int,                   /* Message type */
		 NULL,                  /* Validator */
		 NULL,                  /* User data */
		 ZBUS_OBSERVERS(error), /* Observers */
		 ZBUS_MSG_INIT(0));     /* Initial value {0} */

ZBUS_CHAN_DEFINE(LED_PATTERN_CHAN,               /* Name */
		 led_pattern_t,                  /* Message type */
		 NULL,                           /* Validator */
		 NULL,                           /* User data */
		 ZBUS_OBSERVERS(led_controller), /* Observers */
		 ZBUS_MSG_INIT(0));              /* Initial value {0} */
