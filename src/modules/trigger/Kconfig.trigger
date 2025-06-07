menu "Trigger"

config TRIGGER_THREAD_STACK_SIZE
	int "Thread stack size"
	default 512

config TRIGGER_TIMEOUT_SECONDS
	int "Trigger timer timeout"
	default 30

module = MUUVI_TRIGGER
module-str = Trigger
source "subsys/logging/Kconfig.template.log_config"

endmenu # Trigger
