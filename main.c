#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>

#include <tuya_cacert.h>

#include "args.h"
#include "become_daemon.h"
#include "connection.h"

void signal_handler(int signum);
void set_up_signal_handler(void);
int init_tuya(struct tuya_mqtt_context *context, const char *device_id,
	      const char *device_secret);
int connect_to_tuya(struct tuya_mqtt_context *context);

volatile sig_atomic_t keep_running = 1;

int main(int argc, char *argv[])
{
	set_up_signal_handler();

	int ret_val = EXIT_SUCCESS;

	struct Args arguments = { .device_id = NULL,
				  .device_secret = NULL,
				  .product_id = NULL,
				  .become_daemon = false };
	if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0) {
		puts("Argument parsing failure");
		return EXIT_FAILURE;
	}

	int daemon = 0;
	if (arguments.become_daemon) {
		daemon = become_daemon();
	}

	openlog(program_name, LOG_PID, LOG_LOCAL0);

	syslog(LOG_INFO, "Device ID: %s, device secret: %s, product ID: %s\n",
	       arguments.device_id, arguments.device_secret,
	       arguments.product_id);

	if (daemon != 0) {
		syslog(LOG_ERR,
		       "Failed to become a daemon: become_daemon() returned %d\n",
		       daemon);
		return EXIT_FAILURE;
	}

	tuya_mqtt_context_t mqtt_context;
	ret_val = init_tuya(&mqtt_context, arguments.device_id,
			    arguments.device_secret);
	if (ret_val != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	ret_val = connect_to_tuya(&mqtt_context);
	if (ret_val != EXIT_SUCCESS) {
		goto cleanup;
	}

	unsigned int send_fail_count = 0;
	// Main loop.
	while (keep_running == 1) {
		// Loop to receive packets and handle client keepalive.
		if (tuya_mqtt_loop(&mqtt_context) != OPRT_OK) {
			syslog(LOG_ERR, "Tuya MQTT error");
			ret_val = EXIT_FAILURE;
			goto cleanup;
		}

		// Send current time every 10 seconds.
		// Sleep afer calling tuya_mqtt_loop. Since sleep() can be interrupted
		// by a signal, this allows for a potentilly faster reaction to it.
		sleep(10);
		int ret = send_current_time(&mqtt_context);
		if (ret != 0) {
			syslog(LOG_ERR,
			       "Error sending current time, retrying in 10 seconds");
			send_fail_count += 1;
			if (send_fail_count == 10) {
				ret_val = EXIT_FAILURE;
				goto cleanup;
			}
		} else {
			send_fail_count = 0;
		}
	}

cleanup:
	if (keep_running == 0) {
		syslog(LOG_INFO, "Got signal to exit");
	}
	syslog(LOG_INFO, "Cleaning up resources and exiting");
	tuya_mqtt_disconnect(&mqtt_context);
	tuya_mqtt_deinit(&mqtt_context);
	free_date_time_format();
	closelog();
	return ret_val;
}

void signal_handler(int signum)
{
	// Avoid unused parameter warning.
	(void)signum;
	keep_running = 0;
}

void set_up_signal_handler(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

int init_tuya(struct tuya_mqtt_context *context, const char *device_id,
	      const char *device_secret)
{
	const tuya_mqtt_config_t config = {

		.host = "m1.tuyacn.com",
		.port = 8883,
		.cacert = (const unsigned char *)tuya_cacert_pem,
		.cacert_len = sizeof(tuya_cacert_pem),
		.device_id = device_id,
		.device_secret = device_secret,
		.keepalive = 100,
		.timeout_ms = 2000,
		.on_connected = on_connected,
		.on_disconnect = on_disconnect,
		.on_messages = on_messages
	};
	if (tuya_mqtt_init(context, &config) != OPRT_OK) {
		syslog(LOG_ERR, "Failed to initialize Tuya MQTT, exiting");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int connect_to_tuya(struct tuya_mqtt_context *context)
{
	unsigned int sleep_seconds = 2;
	int ret = tuya_mqtt_connect(context);
	for (int i = 0; i < 10 && ret != OPRT_OK && keep_running == 1; ++i) {
		syslog(LOG_ERR,
		       "Failed to connect to Tuya, retrying in %d seconds",
		       sleep_seconds);
		sleep(sleep_seconds);
		sleep_seconds *= 2;
		ret = tuya_mqtt_connect(context);
	}
	if (ret != OPRT_OK) {
		syslog(LOG_ERR, "Failed to connect to Tuya cloud, exiting");
		return EXIT_FAILURE;
	}
	syslog(LOG_INFO, "Connection to Tuya cloud initialized");

	// Loop a few times to connect. tuya_mqtt_connect does not fully set up
	// the connection, only initializes it, so a few loop iterations are needed
	// to finish the setup.
	for (int i = 0; i < 5 && keep_running == 1; ++i) {
		// Loop to receive packets and handle client keepalive.
		if (tuya_mqtt_loop(context) != OPRT_OK) {
			syslog(LOG_ERR, "Tuya MQTT error");
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
