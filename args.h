#ifndef ARGS_H
#define ARGS_H

#include <stdlib.h>
#include <argp.h>

error_t parse_opt(int key, char *arg, struct argp_state *state);

struct Args {
	char *device_id;
	char *device_secret;
	char *product_id;
};

extern const char *const program_name;
extern const char *const doc;

extern const struct argp argp;

#endif
