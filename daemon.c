#include <stdlib.h>
#include <argp.h>
#include <sys/syslog.h>
#include <syslog.h>

#include <tuya_cacert.h>
#include <tuya_error_code.h>
#include <system_interface.h>
#include <mqtt_client_interface.h>
#include <tuyalink_core.h>

#include "become_daemon.h"

const char *program_name = "tuya_daemon";
const char *argp_program_version = "tuya_daemon 0.1";
const char doc[] = "Daemon program that talks to Tuya cloud";

const struct argp_option options[] = { { .name = "dev-id",
					 .key = 'i',
					 .arg = "DEVICE_ID",
					 .flags = 0,
					 .doc = "Device ID. Required",
					 .group = 0 },
				       { .name = "dev-secret",
					 .key = 's',
					 .arg = "DEVICE_SECRET",
					 .flags = 0,
					 "Device secret. Required",
					 .group = 0 },
				       { .name = "product-id",
					 .key = 'p',
					 .arg = "PRODUCT_ID",
					 .flags = 0,
					 "Product ID. Required",
					 .group = 0 },
				       { 0 } };

struct Args {
	char *device_id;
	char *device_secret;
	char *product_id;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct Args *arguments = state->input;

	switch (key) {
	case 'i':
		arguments->device_id = arg;
		break;
	case 's':
		arguments->device_secret = arg;
		break;
	case 'p':
		arguments->product_id = arg;
		break;
	case ARGP_KEY_ARG:
		// We don't take any positional arguments.
		argp_usage(state);
		break;
	case ARGP_KEY_END:
		if (arguments->device_id == NULL) {
			argp_error(state,
				   "Missing required program option --dev-id");
		} else if (arguments->device_secret == NULL) {
			argp_error(
				state,
				"Missing required program option --dev-secret");
		} else if (arguments->product_id == NULL) {
			argp_error(
				state,
				"Missing required program option --product-id");
		}
	case ARGP_KEY_NO_ARGS:
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { .options = options,
			    .parser = parse_opt,
			    .args_doc = NULL,
			    .doc = doc,
			    .children = NULL,
			    .help_filter = NULL,
			    .argp_domain = NULL };

void on_connected(tuya_mqtt_context_t *context, void *user_data);
void on_disconnect(tuya_mqtt_context_t *context, void *user_data);
void on_messages(tuya_mqtt_context_t *context, void *user_data,
		 const tuyalink_message_t *msg);

void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
	syslog(LOG_INFO, "on connected");

	/* data model test code */
	tuyalink_thing_data_model_get(context, NULL);
	tuyalink_thing_desired_get(context, NULL, "[\"power\"]");
	tuyalink_thing_property_report(
		context, NULL,
		"{\"power\":{\"value\":1234,\"time\":1631708204231}}");
	tuyalink_thing_property_report_with_ack(
		context, NULL,
		"{\"power\":{\"value\":1234,\"time\":1631708204231}}");
	tuyalink_thing_event_trigger(
		context, NULL,
		"{\"eventCode\":\"boom\",\"eventTime\":1626197189630,\"outputParams\":{\"param1\":100}}");
	tuyalink_thing_batch_report(
		context,
		"{\"msgId\":\"45lkj3551234001\",\"time\":1626197189638,\"sys\":{\"ack\":1},\"data\":{\"properties\":{\"power\":{\"value\":11,\"time\":1626197189638}},\"events\":{\"boom\":{\"outputParams\":{\"param1\":\"10\"},\"eventTime\":1626197189001}}}}");
}

void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
	syslog(LOG_INFO, "on disconnect");
}

void on_messages(tuya_mqtt_context_t *context, void *user_data,
		 const tuyalink_message_t *msg)
{
	syslog(LOG_INFO, "on message id:%s, type:%d, code:%d", msg->msgid,
	       msg->type, msg->code);
	switch (msg->type) {
	case THING_TYPE_MODEL_RSP:
		syslog(LOG_INFO, "Model data:%s", msg->data_string);
		break;

	case THING_TYPE_PROPERTY_SET:
		syslog(LOG_INFO, "property set:%s", msg->data_string);
		break;

	case THING_TYPE_PROPERTY_REPORT_RSP:
		syslog(LOG_INFO, "property report");
		break;

	default:
		break;
	}
	syslog(LOG_INFO, "\n");
}

int main(int argc, char *argv[])
{
	struct Args arguments = { .device_id = NULL,
				  .device_secret = NULL,
				  .product_id = NULL };
	if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0) {
		puts("Argument parsing failure");
		return EXIT_FAILURE;
	}

	int daemon = become_daemon();

	openlog(program_name, LOG_PID | LOG_PERROR, LOG_LOCAL0);

	syslog(LOG_INFO, "Device ID: %s, device secret: %s, product ID: %s\n",
	       arguments.device_id, arguments.device_secret,
	       arguments.product_id);

	if (daemon != 0) {
		syslog(LOG_ERR,
		       "Failed to become a daemon: become_daemon() returned %d\n",
		       daemon);
		return EXIT_FAILURE;
	}

	// TODO: the actual program logic.
	tuya_mqtt_context_t client_instance;

	int ret = tuya_mqtt_init(
		&client_instance,
		&(const tuya_mqtt_config_t){
			.host = "m1.tuyacn.com",
			.port = 8883,
			.cacert = (const unsigned char *)tuya_cacert_pem,
			.cacert_len = sizeof(tuya_cacert_pem),
			.device_id = arguments.device_id,
			.device_secret = arguments.device_secret,
			.keepalive = 100,
			.timeout_ms = 2000,
			.on_connected = on_connected,
			.on_disconnect = on_disconnect,
			.on_messages = on_messages });
	if (ret != OPRT_OK) {
		syslog(LOG_ERR, "Failed to initialize Tuya MQTT\n");
		return EXIT_FAILURE;
	}

	ret = tuya_mqtt_connect(&client_instance);
	if (ret != OPRT_OK) {
		syslog(LOG_ERR, "Failed to connect to Tuya\n");
		return EXIT_FAILURE;
	}

	while (true) {
		/* Loop to receive packets, and handles client keepalive */
		tuya_mqtt_loop(&client_instance);
	}

	closelog();
	return EXIT_SUCCESS;
}