#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <syslog.h>

#include <cJSON.h>

#include "connection.h"

char *date_time_format = "%F %T%z";

int send_current_time(tuya_mqtt_context_t *context)
{
	char buf[200] = "{\"time\":\"";
	const size_t used_buf_length = strlen(buf);
	int time_length = get_time_string(buf + used_buf_length,
					  sizeof(buf) - used_buf_length,
					  date_time_format);
	if (time_length < 0) {
		return -1;
	}
	if (used_buf_length + (size_t)time_length + 2 >= sizeof(buf)) {
		syslog(LOG_ERR,
		       "Time report message does not fit in buffer with length %ld: %s",
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
}

// Get current time as string.
// Returns 0 on success, -1, -2 or -3 on failure.
int get_time_string(char *str, size_t str_size, const char *format)
{
	const time_t current_time_epoch = time(NULL);
	if (current_time_epoch == (time_t)(-1)) {
		syslog(LOG_ERR, "failed to get current time");
		return -1;
	}
	const struct tm *local_time = localtime(&current_time_epoch);
	if (local_time == NULL) {
		syslog(LOG_ERR, "failed to convert current time to local time");
		return -2;
	}
	int ret = (int)strftime(str, str_size, format, local_time);
	if (ret == 0) {
		syslog(LOG_ERR,
		       "string representation of date (format '%s') does not fit into a buffer of size %lu",
		       format, str_size);
		return -3;
	}
	return ret;
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
		syslog(LOG_INFO, "Got message type PROPERTY_SET: %s",
		       msg->data_string);
		// Set date and time format.
		// Message format: {"date_time_format":"<format>"}
		// Cannot use msg->data_json because the field is always NULL.
		// This will always successfully parse the data because JSON
		// is validated by the lib when message is received.
		struct cJSON *data = cJSON_Parse(msg->data_string);
		data = data->child;
		if (data == NULL) {
			syslog(LOG_ERR,
			       "PROPERTY_SET message has unexpected structure");
			break;
		}
		// Field name.
		if (strcmp(data->string, "date_time_format") != 0) {
			syslog(LOG_ERR,
			       "PROPERTY_SET wants to set unknown property with name: %s",
			       data->string);
			break;
		}
		date_time_format = data->valuestring;
		syslog(LOG_INFO, "Setting date time format to '%s'",
		       data->valuestring);
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
