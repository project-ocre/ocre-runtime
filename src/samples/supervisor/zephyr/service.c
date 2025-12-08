#include <stdio.h>
#include <ocre/ocre.h>

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
	ocre_global_context = ocre_create_context("/lfs/ocre");
	if (!ocre_global_context) {
		fprintf(stderr, "Failed to create Ocre context\n");
		ocre_deinitialize();
		return -1;
	}

	fprintf(stderr, "Created Ocre context: %p\n", ocre_global_context);

	return 0;
}
