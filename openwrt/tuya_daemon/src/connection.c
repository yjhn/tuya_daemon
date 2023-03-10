#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <stdio.h>

#include <tuya_cacert.h>
#include <libubus.h>
#include <cJSON.h>

#include "connection.h"
#include "send_info.h"
#include "signals.h"
/*
int send_current_time(tuya_mqtt_context_t *context)
{
	char buf[200] = "{\"time\":\"";
	const size_t used_buf_length = strlen(buf);
	int time_length = 0;
	if (time_length < 0) {
		return -1;
	}
	if (used_buf_length + (size_t)time_length + 2 >= sizeof(buf)) {
		syslog(LOG_ERR,
		       "Time report message does not fit in buffer with length %zu: %s",
		       sizeof(buf), buf);
		return -2;
	}
	buf[used_buf_length + (size_t)time_length] = '\"';
	buf[used_buf_length + (size_t)time_length + 1] = '}';
	buf[used_buf_length + (size_t)time_length + 2] = '\0';

	// All Tuya error codes are < 0,
	// they are defined in tuya-iot-core-sdk/utils/tuya_error_code.h
	if (tuyalink_thing_property_report_with_ack(context, NULL, buf) < 0) {
		syslog(LOG_ERR, "Failed to send current time: %s", buf);
		return -3;
	}
	return 0;
}*/

void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
	struct ubus_context *ubus_ctx = (struct ubus_context *)user_data;
	syslog(LOG_INFO, "Connected to Tuya cloud");
	send_info(context, ubus_ctx);
}

void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
	// Silence unused variable warning.
	(void)context;

	struct ubus_context *ubus_ctx = (struct ubus_context *)user_data;

	ubus_free(ubus_ctx);
	syslog(LOG_INFO, "Disconnected from Tuya cloud");
}

void on_messages(tuya_mqtt_context_t *context, void *user_data,
		 const tuyalink_message_t *msg)
{
	// Silence unused variable warning.
	(void)user_data;
	(void)context;

	syslog(LOG_INFO, "Got message: id: %s, type: %u, code: %u", msg->msgid,
	       msg->type, msg->code);

	switch (msg->type) {
	case THING_TYPE_PROPERTY_REPORT_RSP:
		syslog(LOG_INFO, "Got message type PROPERTY_REPORT_RSP");
		break;

	default:
		syslog(LOG_ALERT, "unrecognized message type received: %s",
		       msg->data_string);
		break;
	}
}

int init_connections(struct tuya_mqtt_context *tuya_ctx, const char *device_id,
		     const char *device_secret, struct ubus_context **ubus_ctx)
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
		.on_messages = on_messages,
	};
	if (tuya_mqtt_init(tuya_ctx, &config) != OPRT_OK) {
		syslog(LOG_ERR, "Failed to initialize Tuya MQTT");
		return EXIT_FAILURE;
	}
	// Connect to ubus
	*ubus_ctx = ubus_connect(NULL);
	if (*ubus_ctx == NULL) {
		syslog(LOG_ERR, "Failed to connect to ubus");
		return EXIT_FAILURE;
	}
	// This is what gets passed as user_data to on_* callbacks.
	tuya_ctx->user_data = *ubus_ctx;

	return EXIT_SUCCESS;
}

int connect_to_tuya(struct tuya_mqtt_context *context)
{
	unsigned int sleep_seconds = 2;
	int ret = tuya_mqtt_connect(context);
	for (int i = 0; i < 10 && ret != OPRT_OK && keep_running == 1; ++i) {
		syslog(LOG_ERR,
		       "%s(): Failed to connect to Tuya, retrying in %u seconds",
		       __func__, sleep_seconds);
		sleep(sleep_seconds);
		sleep_seconds *= 2;
		ret = tuya_mqtt_connect(context);
	}
	if (ret != OPRT_OK) {
		syslog(LOG_ERR, "Failed to connect to Tuya");
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
