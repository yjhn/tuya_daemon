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

// req contains request info
// req->priv is data passed to priv parameter of ubus_invoke
static void ubus_received_mem_data_process_cb(struct ubus_request *req,
					      int type, struct blob_attr *msg)
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
	int ret_val = ubus_lookup_id(ubus_ctx, "system", &id);
	if (ret_val != UBUS_STATUS_OK) {
		syslog(LOG_ERR, "ubus_lookup_id() returned %d: %s", ret_val,
		       ubus_strerror(ret_val));
		return false;
	}

	struct ubus_response resp;
	ret_val = ubus_invoke(ubus_ctx, id, "info", NULL,
			      ubus_received_mem_data_process_cb, &resp, 10000);
	if (ret_val != UBUS_STATUS_OK) {
		syslog(LOG_ERR, "Failed to get memory info from ubus: %s",
		       ubus_strerror(ret_val));
		return false;
	} else if (!resp.request_successful) {
		return false;
	}

	// tuyalink_thing_property_report() returns either the (positive) message id
	// or an error code. All Tuya error codes are < 0,
	// they are defined in tuya-iot-core-sdk/utils/tuya_error_code.h
	ret_val =
		tuyalink_thing_property_report(tuya_ctx, NULL, resp.data_json);
	if (ret_val < 0) {
		syslog(LOG_ERR,
		       "Failed to send message to Tuya. Error core: %d, message data: %s",
		       ret_val, resp.data_json);
		return false;
	}

	return true;
}

// req contains request info
// req->priv is data passed to priv parameter of ubus_invoke
static void ubus_received_dev_list_process_cb(struct ubus_request *req,
					      int type, struct blob_attr *msg)
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

int send_connected_devices_list(tuya_mqtt_context_t *tuya_ctx,
				struct ubus_context *ubus_ctx)
{
	uint32_t id;
	int ret_val = ubus_lookup_id(ubus_ctx, "devctl", &id);
	if (ret_val != UBUS_STATUS_OK) {
		syslog(LOG_ERR, "ubus_lookup_id() returned %d: %s", ret_val,
		       ubus_strerror(ret_val));
		return false;
	}

	struct ubus_response resp;
	ret_val = ubus_invoke(ubus_ctx, id, "list_devices", NULL,
			      ubus_received_dev_list_process_cb, &resp, 10000);
	if (ret_val != UBUS_STATUS_OK) {
		syslog(LOG_ERR, "Failed to get device list from ubus: %s",
		       ubus_strerror(ret_val));
		return false;
	} else if (!resp.request_successful) {
		return false;
	}
	syslog(LOG_DEBUG, "Device list from devctl: %s", resp.data_json);

	// tuyalink_thing_property_report() returns either the (positive) message id
	// or an error code. All Tuya error codes are < 0,
	// they are defined in tuya-iot-core-sdk/utils/tuya_error_code.h

	// Explanation for the message type. Tuya defines THING_TYPE_ACTION_EXECUTE_RSP
	// message type, but does not expose a way to send a message of this type.
	// So we report a property when we get this action message.
	ret_val =
		tuyalink_thing_property_report(tuya_ctx, NULL, resp.data_json);
	if (ret_val < 0) {
		syslog(LOG_ERR,
		       "Failed to send message to Tuya. Error core: %d, message data: %s",
		       ret_val, resp.data_json);
		return false;
	}

	return true;
}
