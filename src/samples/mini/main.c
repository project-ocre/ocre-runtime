#include <stdio.h>
#include <unistd.h>

#include <ocre/ocre.h>

int main(int argc, char *argv[])
{
	int rc;

	rc = ocre_initialize();
	if (rc) {
		fprintf(stderr, "Failed to initialize runtimes\n");
		return 1;
	}

	struct ocre_context *ocre = ocre_create_context(NULL);
	if (!ocre) {
		fprintf(stderr, "Failed to create ocre context\n");
		return 1;
	}

	struct ocre_container *container =
		ocre_context_create_container(ocre, "hello-world.wasm", "wamr", NULL, false, NULL);

	if (!container) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	rc = ocre_container_start(container);
	if (rc) {
		fprintf(stderr, "Failed to start container\n");
		return 1;
	}

	rc = ocre_context_remove_container(ocre, container);
	if (rc) {
		fprintf(stderr, "Failed to remove container\n");
		return 1;
	}

	ocre_destroy_context(ocre);

	ocre_deinitialize();

	return 0;
}
