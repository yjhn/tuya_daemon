#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <time.h>
#include <syslog.h>
#include <stdio.h>

#include <cJSON.h>
#include <tuya_cacert.h>
#include <tuyalink_core.h>
#include <libubus.h>

#include "connection.h"
#include "send_info.h"
#include "signals.h"

void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
	struct ubus_context *ubus_ctx = (struct ubus_context *)user_data;
	syslog(LOG_INFO, "Connected to Tuya cloud");
	send_memory_info(context, ubus_ctx);
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
	struct ubus_context *ubus_ctx = (struct ubus_context *)user_data;
	syslog(LOG_DEBUG, "Got message: id: %s, type: %s, code: %u, data: %s",
	       msg->msgid, THING_TYPE_ID2STR(msg->type), msg->code,
	       msg->data_string);

	switch (msg->type) {
	case THING_TYPE_ACTION_EXECUTE:; // Empty statement, otherwise an error is thrown.
		// Cannot use msg->data_json because the field is always NULL.
		// This will always successfully parse the data because JSON
		// is validated by the lib when message is received.
		struct cJSON *full_data = cJSON_Parse(msg->data_string);
		struct cJSON *action =
			cJSON_GetObjectItem(full_data, "actionCode");
		if (action == NULL) {
			syslog(LOG_ERR,
			       "ACTION_EXECUTE does not contain action code");
			goto json_cleanup;
		}
		const char *action_name = cJSON_GetStringValue(action);
		if (action_name == NULL ||
		    strcmp(action_name, "list_devices") != 0) {
			syslog(LOG_ERR, "Unrecognized action requested: %s",
			       action_name);
			goto json_cleanup;
		}
		send_connected_devices_list(context, ubus_ctx);
		// TODO: call devctl via ubus.
		// Set it to null to prevent from being freed by cJSON_Delete.
		// data->valuestring = NULL;
json_cleanup:
		cJSON_Delete(full_data);
		break;
	default:
		syslog(LOG_WARNING, "unrecognized message type received: %s",
		       msg->data_string);
		break;
	}
}

// Return values:
//  0 - OK
// -1 - failed to initialize Tuya connection
// -2 - failed to connect to ubus
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
		return -1;
	}
	// Connect to ubus
	*ubus_ctx = ubus_connect(NULL);
	if (*ubus_ctx == NULL) {
		syslog(LOG_ERR, "Failed to connect to ubus");
		return -2;
	}
	// tuya_ctx->user_data gets passed as user_data to on_* callbacks.
	tuya_ctx->user_data = *ubus_ctx;

	return 0;
}

// Return values:
//  0 - OK
// -1 - failed to connect to Tuya
int connect_to_tuya(struct tuya_mqtt_context *context)
{
	unsigned int sleep_seconds = 2;
	int ret = tuya_mqtt_connect(context);
	for (int i = 0; ret != OPRT_OK && keep_running == 1 && i < 10; ++i) {
		syslog(LOG_ERR,
		       "%s(): Failed to connect to Tuya, retrying in %u seconds",
		       __func__, sleep_seconds);
		sleep(sleep_seconds);
		sleep_seconds *= 2;
		ret = tuya_mqtt_connect(context);
	}
	if (ret != OPRT_OK) {
		syslog(LOG_ERR, "Failed to connect to Tuya");
		return -1;
	}
	syslog(LOG_INFO, "Connection to Tuya cloud initialized");

	// Loop a few times to connect. tuya_mqtt_connect does not fully set up
	// the connection, only initializes it, so a few loop iterations are needed
	// to finish the setup.
	// When the connection is fully set up, context->is_connected is set to true.
	while (!tuya_mqtt_connected(context) && keep_running == 1) {
		// Loop to receive packets and handle client keepalive.
		if (tuya_mqtt_loop(context) != OPRT_OK) {
			syslog(LOG_ERR, "Tuya MQTT error");
			return -1;
		}
	}
	return 0;
}
