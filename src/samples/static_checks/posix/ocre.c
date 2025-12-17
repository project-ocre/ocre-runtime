#include <stdio.h>
#include <unistd.h>
#include <ocre/ocre.h>

#include <ocre/shell/shell.h>

/* keep a reference to the single instance of the runtime */

struct ocre_context *ocre_global_context = NULL;

int main(int argc, char *argv[])
{
	/* Initialize Ocre */

	if (ocre_initialize(NULL)) {
		return -1;
	}

	fprintf(stderr, "Initialized Ocre\n");

	/* Create a context */

	ocre_global_context = ocre_create_context(NULL);
	if (!ocre_global_context) {
		fprintf(stderr, "Failed to create Ocre context\n");
		ocre_deinitialize();
		return -1;
	}

	fprintf(stderr, "Created Ocre context: %p\n", ocre_global_context);

	return ocre_shell(ocre_global_context, argc, argv);
}
