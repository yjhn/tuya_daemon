#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>

#include <uci.h>

#include "args.h"

static bool uci_lookup_succeeded(int ret, const struct uci_ptr *ptr)
{
	if (ret == UCI_ERR_NOTFOUND) {
		return false;
	} else if ((ptr->flags & UCI_LOOKUP_COMPLETE) == 0) {
		return false;
	}
	return true;
}

// option must not be const as it will be modified by uci_lookup_ptr
// Returs NULL on failure.
char *uci_get_option(struct uci_context *ctx, struct uci_ptr *ptr, char *option,
		     const char *const_option)
{
	int ret = uci_lookup_ptr(ctx, ptr, option, false);
	if (!uci_lookup_succeeded(ret, ptr)) {
		char *error;
		uci_get_errorstr(ctx, &error, "");
		syslog(LOG_ERR, "Option '%s' not found%s", const_option, error);
		free(error);
		return NULL;
	}
	if (ptr->o->type != UCI_TYPE_STRING) {
		syslog(LOG_ERR,
		       "Option '%s' value is of unexpected type: expected UCI_TYPE_STRING, got %u",
		       const_option, ptr->o->type);
		return NULL;
	}

	return ptr->o->v.string;
}

bool str_to_bool(const char *str, bool *result)
{
	if (strcmp(str, "1") == 0) {
		*result = true;
		return true;
	} else if (strcmp(str, "0") == 0) {
		*result = false;
		return true;
	} else {
		return false;
	}
}
