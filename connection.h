#ifndef CONNECTION_H
#define CONNECTION_H

#include <tuya_error_code.h>
#include <system_interface.h>
#include <mqtt_client_interface.h>
#include <tuyalink_core.h>

void on_connected(tuya_mqtt_context_t *context, void *user_data);
void on_disconnect(tuya_mqtt_context_t *context, void *user_data);
void on_messages(tuya_mqtt_context_t *context, void *user_data,
		 const tuyalink_message_t *msg);
void get_time_string(char *str, size_t str_size);

int send_current_time(tuya_mqtt_context_t *context);

#endif
