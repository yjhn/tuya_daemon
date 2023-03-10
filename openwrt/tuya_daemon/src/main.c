#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

#include <uci.h>
#include <libubus.h>

#include "args.h"
#include "connection.h"
#include "become_daemon.h"
#include "send_info.h"
#include "signals.h"

const char *options_const[] = {
	"tuya.tuya_daemon.device_id",	  "tuya.tuya_daemon.device_secret",
	"tuya.tuya_daemon.product_id",	  "tuya.tuya_daemon.autostart",
	"tuya.tuya_daemon.become_daemon", "tuya.tuya_daemon.log_level"
};
const size_t options_count = sizeof(options_const) / sizeof(options_const[0]);

const int log_priorities[8] = { LOG_EMERG,   LOG_ALERT,	 LOG_CRIT, LOG_ERR,
				LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG };

int main(void)
{
	int ret_val = EXIT_SUCCESS;
	set_up_signal_handler();

	openlog("tuya_daemon", LOG_PID | LOG_CONS | LOG_PERROR, LOG_LOCAL0);

	char *option_names[] = {
		strdup(options_const[0]), strdup(options_const[1]),
		strdup(options_const[2]), strdup(options_const[3]),
		strdup(options_const[4]), strdup(options_const[5])
	};
	// Get program settings from UCI.
	struct uci_context *uci_ctx = uci_alloc_context();
	if (uci_ctx == NULL) {
		syslog(LOG_ERR, "Failed to allocate UCI context");
		ret_val = EXIT_FAILURE;
		goto cleanup_end;
	}
	struct uci_ptr uci_ptr;
	char *device_id = uci_get_option(uci_ctx, &uci_ptr, option_names[0],
					 options_const[0]);
	char *device_secret = uci_get_option(uci_ctx, &uci_ptr, option_names[1],
					     options_const[1]);
	char *product_id = uci_get_option(uci_ctx, &uci_ptr, option_names[2],
					  options_const[2]);
	char *autostart_str = uci_get_option(uci_ctx, &uci_ptr, option_names[3],
					     options_const[3]);
	char *become_daemon_str = uci_get_option(
		uci_ctx, &uci_ptr, option_names[4], options_const[4]);
	char *log_level_str = uci_get_option(uci_ctx, &uci_ptr, option_names[5],
					     options_const[5]);
	// All options must be specified.
	if (device_id == NULL || device_secret == NULL || product_id == NULL ||
	    autostart_str == NULL || become_daemon_str == NULL ||
	    log_level_str == NULL) {
		ret_val = EXIT_FAILURE;
		goto cleanup_end;
	}
	bool autostart;
	if (!str_to_bool(autostart_str, &autostart)) {
		syslog(LOG_ERR, "Unrecognized value for option 'autostart': %s",
		       autostart_str);
		ret_val = EXIT_FAILURE;
		goto cleanup_end;
	}
	bool be_daemon;
	if (!str_to_bool(become_daemon_str, &be_daemon)) {
		syslog(LOG_ERR,
		       "Unrecognized value for option 'become_daemon': %s",
		       become_daemon_str);
		ret_val = EXIT_FAILURE;
		goto cleanup_end;
	}
	int log_level;
	if (!str_to_digit(log_level_str, &log_level) || log_level > 7) {
		syslog(LOG_ERR, "Unrecognized value for option 'log_level': %s",
		       log_level_str);
		ret_val = EXIT_FAILURE;
		goto cleanup_end;
	}
	setlogmask(LOG_UPTO(log_priorities[log_level]));

	syslog(LOG_DEBUG,
	       "Options: device ID: %s, device secret: %s,"
	       " product ID: %s, autostart: %d, become_daemon: %d, log_level: %d",
	       device_id, device_secret, product_id, autostart, be_daemon,
	       log_level);

	int daemon = 0;
	if (be_daemon) {
		daemon = become_daemon();
	}
	if (ret_val != 0) {
		syslog(LOG_ERR,
		       "Failed to become a daemon, become_daemon() returned %d",
		       daemon);
		ret_val = EXIT_FAILURE;
		goto cleanup_end;
	}

	tuya_mqtt_context_t mqtt_context;
	struct ubus_context *ubus_ctx;
	ret_val = init_connections(&mqtt_context, device_id, device_secret,
				   &ubus_ctx);
	if (ret_val != EXIT_SUCCESS) {
		goto cleanup_mqtt;
	}

	ret_val = connect_to_tuya(&mqtt_context);
	if (ret_val != EXIT_SUCCESS) {
		goto cleanup_mqtt;
	}

	unsigned int send_fail_count = 0;
	// Main loop.
	while (keep_running == 1) {
		// Loop to receive packets and handle client keepalive.
		if (tuya_mqtt_loop(&mqtt_context) != OPRT_OK) {
			syslog(LOG_ERR, "Tuya MQTT error");
			ret_val = EXIT_FAILURE;
			goto cleanup_mqtt;
		}

		// Send current time every 10 seconds.
		// Sleep afer calling tuya_mqtt_loop. Since sleep() can be interrupted
		// by a signal, this allows for a potentilly faster reaction to it.
		sleep(10);
		if (!send_info(&mqtt_context, ubus_ctx)) {
			syslog(LOG_ERR,
			       "Error sending information, retrying in 10 seconds");
			send_fail_count += 1;
			if (send_fail_count == 10) {
				ret_val = EXIT_FAILURE;
				goto cleanup_mqtt;
			}
		} else {
			send_fail_count = 0;
		}
	}

	if (keep_running == 0) {
		syslog(LOG_INFO, "Got signal to exit");
	}
cleanup_mqtt:
	tuya_mqtt_disconnect(&mqtt_context);
	tuya_mqtt_deinit(&mqtt_context);
cleanup_end:
	syslog(LOG_INFO, "Cleaning up resources and exiting");

	for (size_t i = 0; i < options_count; ++i) {
		free(option_names[i]);
	}
	uci_free_context(uci_ctx);
	closelog();
	return ret_val;
}
