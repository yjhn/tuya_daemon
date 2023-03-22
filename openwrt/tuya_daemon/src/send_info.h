#ifndef SEND_INFO_H
#define SEND_INFO_H

#include <stdbool.h>

#include <tuyalink_core.h>
#include <libubus.h>

/*
 * Sends device memory info to Tuya.
 * Returns true on success, false on failure.
 */
bool send_memory_info(tuya_mqtt_context_t *tuya_ctx,
		      struct ubus_context *ubus_ctx);

int send_connected_devices_list(tuya_mqtt_context_t *tuya_ctx,
				struct ubus_context *ubus_ctx);
#endif
