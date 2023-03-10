#ifndef CONNECTION_H
#define CONNECTION_H

#include <tuya_error_code.h>
#include <tuyalink_core.h>
#include <libubus.h>

int init_connections(struct tuya_mqtt_context *tuya_ctx, const char *device_id,
		     const char *device_secret, struct ubus_context **ubus_ctx);
int connect_to_tuya(struct tuya_mqtt_context *context);
void set_date_time_format(char *format);
void free_date_time_format(void);

// user_data is actually 'struct my_data *'
// user_data can be set by
void on_connected(tuya_mqtt_context_t *context, void *user_data);
void on_disconnect(tuya_mqtt_context_t *context, void *user_data);
void on_messages(tuya_mqtt_context_t *context, void *user_data,
		 const tuyalink_message_t *msg);
int get_time_string(char *str, size_t str_size, const char *format);

int send_current_time(tuya_mqtt_context_t *context);

#endif
