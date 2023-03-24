#ifndef SEND_INFO_H
#define SEND_INFO_H

#include <stdbool.h>
#include <stdint.h>

#include <tuyalink_core.h>
#include <libubus.h>

/*
 * Sends device memory info to Tuya.
 * Returns true on success, false on failure.
 */
bool send_memory_info(tuya_mqtt_context_t *tuya_ctx,
		      struct ubus_context *ubus_ctx);

bool send_connected_devices_list(tuya_mqtt_context_t *tuya_ctx,
				 struct ubus_context *ubus_ctx);

// if(turn_on)
//	turn on pin
// else
//	turn off pin
bool control_device_pin(tuya_mqtt_context_t *tuya_ctx,
			struct ubus_context *ubus_ctx, bool turn_on,
			const char *const device, const uint32_t pin);

#endif
