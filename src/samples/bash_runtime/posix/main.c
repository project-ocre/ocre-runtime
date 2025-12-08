#include <stdio.h>
#include <unistd.h>

#include <ocre/ocre.h>

#include <ocre/runtime/vtable.h>
#include <ocre/runtime/bash/bash.h>

#include "../../ocre/container.h"

#define OCRE_DEFAULT_WORKING_DIRECTORY "./var/lib/ocre"

int main(int argc, char *argv[])
{
	int rc;

	rc = ocre_initialize();
	if (rc) {
		fprintf(stderr, "Failed to initialize OCRE\n");
		return 1;
	}

	rc = ocre_initialize_with_runtimes((const struct ocre_runtime_vtable *const[]){&bash_vtable, NULL});
	if (rc) {
		fprintf(stderr, "Failed to initialize runtimes\n");
		return 1;
	}

	struct ocre_context *ocre = ocre_create_context(OCRE_DEFAULT_WORKING_DIRECTORY);
	if (!ocre) {
		fprintf(stderr, "Failed to create ocre context\n");
		return 1;
	}

	struct ocre_container_args args = {
		.argv = (const char *[]){"teste", "dois", NULL},
		.envp = (const char *[]){"VAR=variable", "VAR2=variable2", NULL},
	};

	struct ocre_container *wasm_container =
		ocre_context_create_container(ocre, "blinky", "wamr", NULL, false, &args);
	if (!wasm_container) {
		fprintf(stderr, "Failed to create WASM container\n");
		return 1;
	}

	struct ocre_container *bash_container =
		ocre_context_create_container(ocre, "atym.sh", "bash", "my_bash", true, &args);
	if (!bash_container) {
		fprintf(stderr, "Failed to create bash container\n");
		return 1;
	}

	fprintf(stderr, "Will start WAMR container\n");

	rc = ocre_container_start(wasm_container);
	if (rc) {
		fprintf(stderr, "Failed to start WASM container\n");
		return 1;
	}

	fprintf(stderr, "Will start bash container\n");

	rc = ocre_container_start(bash_container);
	if (rc) {
		fprintf(stderr, "Failed to start bash container\n");
		return 1;
	}

	int exit_code;

	if (ocre_container_wait(bash_container, &exit_code)) {
		fprintf(stderr, "Failed to wait for bash container\n");
		return 1;
	}

	fprintf(stderr, "Bash container exited with status %d\n", exit_code);

	if (ocre_container_wait(wasm_container, &exit_code)) {
		fprintf(stderr, "Failed to wait for WASM container\n");
		return 1;
	}

	fprintf(stderr, "WASM container exited with status %d\n", exit_code);

	return 0;
}
