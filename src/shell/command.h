#include <ocre/ocre.h>

struct ocre_command {
	char *name;
	int (*func)(struct ocre_context *ctx, const char *argv0, int argc, char **argv);
};
