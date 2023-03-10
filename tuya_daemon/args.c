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

void print_help_msg(void);

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
		// We don't take any positional arguments.
		// Cannot call argp_usage(state) or argp_error(), because they leak memory.
		fprintf(stderr,
			"Unrecognized argument: '%s'\nYou can see available options by passing --help\n",
			arg);
		return ARGP_ERR_UNKNOWN;
	case ARGP_KEY_END:
		if (arguments->device_id == NULL) {
			fprintf(stderr,
				"Missing required program option --dev-id\n");
			print_help_msg();
			return ARGP_ERR_UNKNOWN;
		} else if (arguments->device_secret == NULL) {
			fprintf(stderr,
				"Missing required program option --dev-secret");
			print_help_msg();
			return ARGP_ERR_UNKNOWN;
		} else if (arguments->product_id == NULL) {
			fprintf(stderr,
				"Missing required program option --product-id");
			print_help_msg();
			return ARGP_ERR_UNKNOWN;
		}
	case ARGP_KEY_NO_ARGS:
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

void print_help_msg()
{
	fprintf(stderr, "Try `%s --help' or `%s --usage' for more information.",
		program_name, program_name);
}
