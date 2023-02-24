#include <time.h>
#include <syslog.h>

#include "connection.h"

int send_current_time(tuya_mqtt_context_t *context)
{
	char buf[200];
	get_time_string(buf, sizeof(buf));

	return tuyalink_thing_property_report_with_ack(context, NULL, buf);
}

// Get current time as string.
void get_time_string(char *str, size_t str_size)
{
	const time_t current_time_epoch = time(NULL);
	const struct tm *local_time = localtime(&current_time_epoch);
	if (strftime(str, str_size, "{\"time\":\"%F %T%z\"}", local_time) ==
	    0) {
		syslog(LOG_ERR,
		       "string representation of date does not fit into a buffer of size %lu",
		       str_size);
		str[0] = '\0';
		return;
	}
}

void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
	// Silence unused variable warning.
	(void)user_data;
	syslog(LOG_INFO, "connected to Tuya cloud");
	send_current_time(context);
}

void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
	// Silence unused variable warning.
	(void)user_data;
	(void)context;

	syslog(LOG_INFO, "Disconnected from Tuya cloud");
}

void on_messages(tuya_mqtt_context_t *context, void *user_data,
		 const tuyalink_message_t *msg)
{
	// Silence unused variable warning.
	(void)user_data;
	(void)context;

	syslog(LOG_INFO, "on message id:%s, type:%d, code:%d", msg->msgid,
	       msg->type, msg->code);

	switch (msg->type) {
	case THING_TYPE_PROPERTY_SET:
		syslog(LOG_ALERT,
		       "Got message type PROPERTY_SET, do not support property setting: %s",
		       msg->data_string);
		break;

	case THING_TYPE_PROPERTY_REPORT_RSP:
		syslog(LOG_INFO, "Got message type PROPERTY_REPORT_RSP");
		break;

	default:
		syslog(LOG_ALERT, "unrecognized message type received: %s",
		       msg->data_string);
		break;
	}
}
