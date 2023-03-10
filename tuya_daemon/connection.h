#ifndef CONNECTION_H
#define CONNECTION_H

#include <tuya_error_code.h>
#include <system_interface.h>
#include <mqtt_client_interface.h>
#include <tuyalink_core.h>

void set_date_time_format(char *format);
void free_date_time_format(void);
void on_connected(tuya_mqtt_context_t *context, void *user_data);
void on_disconnect(tuya_mqtt_context_t *context, void *user_data);
void on_messages(tuya_mqtt_context_t *context, void *user_data,
		 const tuyalink_message_t *msg);
int get_time_string(char *str, size_t str_size, const char *format);

int send_current_time(tuya_mqtt_context_t *context);

#endif
