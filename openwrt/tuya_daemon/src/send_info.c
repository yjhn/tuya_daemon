#include <syslog.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <tuya_error_code.h>

#include "send_info.h"

#define MAX_DEVICE_COUNT 50

struct ubus_response {
	bool request_successful;
	char *data_json;
};

enum {
	TOTAL_MEMORY,
	FREE_MEMORY,
	SHARED_MEMORY,
	BUFFERED_MEMORY,
	AVAILABLE_MEMORY,
	CACHED_MEMORY,
	__MEMORY_MAX,
};

enum {
	MEMORY_DATA,
	__INFO_MAX,
};

static const struct blobmsg_policy memory_policy[__MEMORY_MAX] = {
	[TOTAL_MEMORY] = { .name = "total", .type = BLOBMSG_TYPE_INT64 },
	[FREE_MEMORY] = { .name = "free", .type = BLOBMSG_TYPE_INT64 },
	[SHARED_MEMORY] = { .name = "shared", .type = BLOBMSG_TYPE_INT64 },
	[BUFFERED_MEMORY] = { .name = "buffered", .type = BLOBMSG_TYPE_INT64 },
	[AVAILABLE_MEMORY] = { .name = "available",
			       .type = BLOBMSG_TYPE_INT64 },
	[CACHED_MEMORY] = { .name = "cached", .type = BLOBMSG_TYPE_INT64 }
};

static const struct blobmsg_policy info_policy[__INFO_MAX] = {
	[MEMORY_DATA] = { .name = "memory", .type = BLOBMSG_TYPE_TABLE },
};

enum { DEVCTL_DEVICE_COUNT, DEVCTL_DEVICE_LIST, __DEVCTL_MAX };

static const struct blobmsg_policy devctl_policy[__DEVCTL_MAX] = {
	[DEVCTL_DEVICE_COUNT] = { .name = "count", .type = BLOBMSG_TYPE_INT32 },
	[DEVCTL_DEVICE_LIST] = { .name = "devices", .type = BLOBMSG_TYPE_ARRAY }
};

enum { DEVCTL_CTL_STATUS, DEVCTL_CTL_MESSAGE, __DEVCTL_CTL_MAX };

static const struct blobmsg_policy devctl_ctl_policy[__DEVCTL_CTL_MAX] = {
	[DEVCTL_CTL_STATUS] = { .name = "status", .type = BLOBMSG_TYPE_INT32 },
	[DEVCTL_CTL_MESSAGE] = { .name = "message",
				 .type = BLOBMSG_TYPE_STRING }
};

// Populates passed in id with ubus service id. On success returns true
// and populates the id. On failure returns false, id values is undefined.
static bool lookup_ubus_service(struct ubus_context *ubus_ctx,
				const char *const ubus_service_name,
				uint32_t *id)
{
	int ret_val = ubus_lookup_id(ubus_ctx, ubus_service_name, id);
	if (ret_val != UBUS_STATUS_OK) {
		syslog(LOG_ERR, "ubus_lookup_id(%s) returned %d: %s",
		       ubus_service_name, ret_val, ubus_strerror(ret_val));
		return false;
	}
	return true;
}

static bool report_property_to_tuya(tuya_mqtt_context_t *tuya_ctx,
				    const char *msg)
{
	// tuyalink_thing_property_report() returns either the (positive) message id
	// or an error code. All Tuya error codes are < 0,
	// they are defined in tuya-iot-core-sdk/utils/tuya_error_code.h
	int ret_val = tuyalink_thing_property_report(tuya_ctx, NULL, msg);
	if (ret_val < 0) {
		syslog(LOG_ERR,
		       "Failed to send message to Tuya. Error core: %d, message data: %s",
		       ret_val, msg);
		return false;
	}

	return true;
}

// req contains request info
// req->priv is data passed to priv parameter of ubus_invoke
static void ubus_received_mem_data_cb(struct ubus_request *req, int type,
				      struct blob_attr *msg)
{
	(void)type;

	struct ubus_response *resp = (struct ubus_response *)req->priv;
	resp->request_successful = false;
	struct blob_attr *tb[__INFO_MAX];
	struct blob_attr *memory[__MEMORY_MAX];

	if (blobmsg_parse(info_policy, __INFO_MAX, tb, blob_data(msg),
			  blob_len(msg)) != 0) {
		syslog(LOG_ERR, "Failed to parse answer from ubus");
		return;
	}

	if (tb[MEMORY_DATA] == NULL) {
		syslog(LOG_ERR, "No memory data received from ubus");
		return;
	}

	if (blobmsg_parse(memory_policy, __MEMORY_MAX, memory,
			  blobmsg_data(tb[MEMORY_DATA]),
			  blobmsg_data_len(tb[MEMORY_DATA])) != 0 ||
	    memory[TOTAL_MEMORY] == NULL || memory[FREE_MEMORY] == NULL ||
	    memory[SHARED_MEMORY] == NULL || memory[BUFFERED_MEMORY] == NULL ||
	    memory[AVAILABLE_MEMORY] == NULL || memory[CACHED_MEMORY] == NULL) {
		syslog(LOG_ERR, "Failed to parse memory data received");
		return;
	}

	resp->request_successful = true;
	resp->data_json = blobmsg_format_json(tb[MEMORY_DATA], true);
}

bool send_memory_info(tuya_mqtt_context_t *tuya_ctx,
		      struct ubus_context *ubus_ctx)
{
	uint32_t id;
	if (!lookup_ubus_service(ubus_ctx, "system", &id)) {
		return false;
	}

	struct ubus_response resp;
	int ret_val = ubus_invoke(ubus_ctx, id, "info", NULL,
				  ubus_received_mem_data_cb, &resp, 10000);
	if (ret_val != UBUS_STATUS_OK) {
		syslog(LOG_ERR, "Failed to get memory info from ubus: %s",
		       ubus_strerror(ret_val));
		return false;
	} else if (!resp.request_successful) {
		return false;
	}

	bool ret = report_property_to_tuya(tuya_ctx, resp.data_json);
	free(resp.data_json);
	return ret;
}

// req contains request info
// req->priv is data passed to priv parameter of ubus_invoke
static void ubus_received_dev_list_cb(struct ubus_request *req, int type,
				      struct blob_attr *msg)
{
	(void)type;

	struct ubus_response *resp = (struct ubus_response *)req->priv;
	resp->request_successful = false;
	struct blob_attr *tb[__DEVCTL_MAX];

	// Check if the answer is valid.
	if (blobmsg_parse(devctl_policy, __DEVCTL_MAX, tb, blob_data(msg),
			  blob_len(msg)) != 0 ||
	    tb[DEVCTL_DEVICE_COUNT] == NULL || tb[DEVCTL_DEVICE_LIST] == NULL) {
		syslog(LOG_ERR, "Failed to parse answer from ubus");
		return;
	}
	resp->request_successful = true;
	resp->data_json = blobmsg_format_json(msg, true);
}

bool send_connected_devices_list(tuya_mqtt_context_t *tuya_ctx,
				 struct ubus_context *ubus_ctx)
{
	uint32_t id;
	if (!lookup_ubus_service(ubus_ctx, "devctl", &id)) {
		return false;
	}

	struct ubus_response resp;
	int ret_val = ubus_invoke(ubus_ctx, id, "list_devices", NULL,
				  ubus_received_dev_list_cb, &resp, 10000);
	if (ret_val != UBUS_STATUS_OK) {
		syslog(LOG_ERR, "Failed to get device list from ubus: %s",
		       ubus_strerror(ret_val));
		return false;
	} else if (!resp.request_successful) {
		return false;
	}
	syslog(LOG_DEBUG, "Device list from devctl: %s", resp.data_json);

	// Explanation for the message type. Tuya defines THING_TYPE_ACTION_EXECUTE_RSP
	// message type, but does not expose a way to send a message of this type.
	// So we report a property when we get THING_TYPE_ACTION_EXECUTE message.
	bool ret = report_property_to_tuya(tuya_ctx, resp.data_json);
	free(resp.data_json);
	return ret;
}

static void ubus_dev_command_resp_cb(struct ubus_request *req, int type,
				     struct blob_attr *msg)
{
	(void)type;

	struct ubus_response *resp = (struct ubus_response *)req->priv;
	resp->request_successful = false;
	struct blob_attr *tb[__DEVCTL_CTL_MAX];

	// Check if the answer is valid.
	if (blobmsg_parse(devctl_ctl_policy, __DEVCTL_CTL_MAX, tb,
			  blob_data(msg), blob_len(msg)) != 0 ||
	    tb[DEVCTL_CTL_STATUS] == NULL || tb[DEVCTL_CTL_MESSAGE] == NULL) {
		syslog(LOG_ERR, "Failed to parse answer from ubus");
		return;
	}
	resp->request_successful = true;
	resp->data_json = blobmsg_format_json(msg, true);
}

bool control_device_pin(tuya_mqtt_context_t *tuya_ctx,
			struct ubus_context *ubus_ctx, bool turn_on,
			const char *const device, const uint32_t pin)
{
	uint32_t id;
	if (!lookup_ubus_service(ubus_ctx, "devctl", &id)) {
		return false;
	}

	struct blob_buf b = { 0 }; // mysterious segfaults if uninitialized
	blob_buf_init(&b, 0);
	if (blobmsg_add_string(&b, "device", device) == -1 ||
	    blobmsg_add_u32(&b, "pin", pin) == -1) {
		syslog(LOG_ERR, "Failed to add required fields");
		return false;
	}

	const char *on_off = turn_on ? "turn_on_pin" : "turn_off_pin";
	struct ubus_response resp;
	int ret_val = ubus_invoke(ubus_ctx, id, on_off, b.head,
				  ubus_dev_command_resp_cb, &resp, 10000);
	blob_buf_free(&b);
	if (ret_val != UBUS_STATUS_OK) {
		syslog(LOG_ERR, "Failed to send command to devctl via ubus: %s",
		       ubus_strerror(ret_val));
		return false;
	} else if (!resp.request_successful) {
		return false;
	}
	// Explanation for the message type. Tuya defines THING_TYPE_ACTION_EXECUTE_RSP
	// message type, but does not expose a way to send a message of this type.
	// So we report a property when we get THING_TYPE_ACTION_EXECUTE message.
	bool ret = report_property_to_tuya(tuya_ctx, resp.data_json);
	free(resp.data_json);
	return ret;
}
