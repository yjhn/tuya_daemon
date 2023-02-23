#include <stdlib.h>
#include <argp.h>
#include <sys/syslog.h>
#include <syslog.h>

#include "become_daemon.h"

const char *program_name = "tuya_daemon";
const char *argp_program_version = "tuya_daemon 0.1";
static char doc[] = "Daemon program that talks to Tuya cloud";

static struct argp_option options[] = { { .name = "dev-id",
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

int main(int argc, char *argv[])
{
	struct Args arguments = { .device_id = NULL,
				  .device_secret = NULL,
				  .product_id = NULL };
	if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0) {
		puts("Argument parsing failure");
		return EXIT_FAILURE;
	}

	int daemon = 0; //become_daemon();

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

	closelog();
	return EXIT_SUCCESS;
}