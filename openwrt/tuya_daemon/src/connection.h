#ifndef CONNECTION_H
#define CONNECTION_H

#include <tuya_error_code.h>
#include <tuyalink_core.h>
#include <libubus.h>

int init_connections(struct tuya_mqtt_context *tuya_ctx, const char *device_id,
		     const char *device_secret, struct ubus_context **ubus_ctx);
int connect_to_tuya(struct tuya_mqtt_context *context);

void on_connected(tuya_mqtt_context_t *context, void *user_data);
void on_disconnect(tuya_mqtt_context_t *context, void *user_data);
void on_messages(tuya_mqtt_context_t *context, void *user_data,
		 const tuyalink_message_t *msg);

#endif
