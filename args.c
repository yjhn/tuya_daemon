#include "args.h"

const char *const program_name = "tuya_daemon";
const char *argp_program_version = "tuya_daemon 0.1";
const char *const doc = "Daemon program that talks to Tuya cloud";

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
					 .doc = "Device secret. Required",
					 .group = 0 },
				       { .name = "product-id",
					 .key = 'p',
					 .arg = "PRODUCT_ID",
					 .flags = 0,
					 .doc = "Product ID. Required",
					 .group = 0 },
				       { .name = "daemon",
					 .key = 'd',
					 .arg = NULL,
					 .flags = OPTION_ARG_OPTIONAL,
					 .doc = "Become a daemon process.",
					 .group = 0 },
				       { 0 } };

const struct argp argp = { .options = options,
			   .parser = parse_opt,
			   .args_doc = NULL,
			   .doc = doc,
			   .children = NULL,
			   .help_filter = NULL,
			   .argp_domain = NULL };

error_t parse_opt(int key, char *arg, struct argp_state *state)
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
	case 'd':
		arguments->become_daemon = true;
		break;
	case ARGP_KEY_ARG:
		fprintf(stderr, "Unrecognized argument: '%s'\n", arg);
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
