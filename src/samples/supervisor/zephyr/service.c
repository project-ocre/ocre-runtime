#include <stdio.h>
#include <ocre/ocre.h>

#include <zephyr/init.h>

/* Keep a reference to the single instance of the runtime */

struct ocre_context *ocre_global_context = NULL;

static int ocre_service_init()
{
	/* Initialize Ocre Library */

	if (ocre_initialize(NULL)) {
		return -1;
	}

	fprintf(stderr, "Initialized Ocre\n");

	/* Create a context */

	ocre_global_context = ocre_create_context("/lfs/ocre");
	if (!ocre_global_context) {
		fprintf(stderr, "Failed to create Ocre context\n");
		ocre_deinitialize();
		return -1;
	}

	fprintf(stderr, "Created Ocre context: %p\n", ocre_global_context);

	return 0;
}

SYS_INIT(ocre_service_init, APPLICATION, 0);
