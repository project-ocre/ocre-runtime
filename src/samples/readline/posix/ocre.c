#include <stdio.h>
#include <unistd.h>
#include <ocre/ocre.h>

#include <ocre/shell/shell.h>

#include <stdbool.h> // remove

// keep a reference to the single instance of the runtime
struct ocre_context *ocre_global_context = NULL;

int main(int argc, char *argv[])
{
	// Initialize OCRE
	if (ocre_initialize() != 0) {
		return -1;
	}

	fprintf(stderr, "Initialized Ocre\n");

	// Create a context
	ocre_global_context = ocre_create_context("var/lib/ocre");
	if (!ocre_global_context) {
		fprintf(stderr, "Failed to create Ocre context\n");
		ocre_deinitialize();
		return -1;
	}

	fprintf(stderr, "Created Ocre context: %p\n", ocre_global_context);

	struct ocre_container *hello =
		ocre_context_create_container(ocre_global_context, "hello-world.wasm", "wamr", "marco", true, NULL);

		if (!hello) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	struct ocre_container *subscriber =
		ocre_context_create_container(ocre_global_context, "subscriber.wasm", "wamr", NULL, true, NULL);

	if (!subscriber) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	struct ocre_container *publisher =
		ocre_context_create_container(ocre_global_context, "publisher.wasm", "wamr", NULL, true, NULL);

	if (!publisher) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	return ocre_shell(ocre_global_context, argc, argv);
}
