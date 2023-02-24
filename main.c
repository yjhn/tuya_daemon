#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>

#include <tuya_cacert.h>

#include "args.h"
#include "become_daemon.h"
#include "connection.h"

void sigint_handler(int signum);

volatile sig_atomic_t keep_running = 1;

int main(int argc, char *argv[])
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	int return_value = EXIT_SUCCESS;

	struct Args arguments = { .device_id = NULL,
				  .device_secret = NULL,
				  .product_id = NULL };
	if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0) {
		puts("Argument parsing failure");
		return EXIT_FAILURE;
	}

	int daemon = 0; //become_daemon();

	openlog(program_name, LOG_PID | LOG_PERROR, LOG_LOCAL0);

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

	int ret = tuya_mqtt_init(
		&mqtt_context,
		&(const tuya_mqtt_config_t){
			.host = "m1.tuyacn.com",
			.port = 8883,
			.cacert = (const unsigned char *)tuya_cacert_pem,
			.cacert_len = sizeof(tuya_cacert_pem),
			.device_id = arguments.device_id,
			.device_secret = arguments.device_secret,
			.keepalive = 100,
			.timeout_ms = 2000,
			.on_connected = on_connected,
			.on_disconnect = on_disconnect,
			.on_messages = on_messages });
	if (ret != OPRT_OK) {
		syslog(LOG_ERR, "Failed to initialize Tuya MQTT\n");
		return EXIT_FAILURE;
	}

	if (tuya_mqtt_connect(&mqtt_context) != OPRT_OK) {
		syslog(LOG_ERR, "Failed to connect to Tuya\n");
		return_value = EXIT_FAILURE;
		goto cleanup;
	}
	syslog(LOG_INFO, "Connection to Tuya cloud initialized");

	// Loop a few times to connect.
	for (int i = 0; i < 5; ++i) {
		// Loop to receive packets, and handle client keepalive.
		if (tuya_mqtt_loop(&mqtt_context) != OPRT_OK) {
			syslog(LOG_ERR, "Tuya MQTT error");
			return_value = EXIT_FAILURE;
			goto cleanup;
		}
	}

	while (keep_running == 1) {
		// Loop to receive packets, and handle client keepalive.
		if (tuya_mqtt_loop(&mqtt_context) != OPRT_OK) {
			syslog(LOG_ERR, "Tuya MQTT error");
			return_value = EXIT_FAILURE;
			break;
		}

		// Send current time every 10 seconds.
		// Sleep afer calling tuya_mqtt_loop. Since sleep() can be interrupted
		// by a signal, this allows for a potentilly faster reaction to it.
		sleep(10);
		send_current_time(&mqtt_context);
	}

cleanup:
	syslog(LOG_INFO, "Cleaning up resources and exiting");
	tuya_mqtt_disconnect(&mqtt_context);
	tuya_mqtt_deinit(&mqtt_context);
	closelog();
	return return_value;
}

void sigint_handler(int signum)
{
	// Avoid unused parameter warning.
	(void)signum;
	keep_running = 0;
}
