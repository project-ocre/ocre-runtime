#include <stdio.h>
#include <unistd.h>

#include <ocre/ocre.h>

const struct ocre_container_args args = {
	.capabilities = (const char *[]){"ocre:api", NULL},
};

int main(int argc, char *argv[])
{
	int rc;
	int status;

	rc = ocre_initialize(NULL);
	if (rc) {
		fprintf(stderr, "Failed to initialize runtimes\n");
		return 1;
	}

	struct ocre_context *ocre = ocre_create_context(NULL);
	if (!ocre) {
		fprintf(stderr, "Failed to create ocre context\n");
		return 1;
	}

	struct ocre_container *hello_world =
		ocre_context_create_container(ocre, "hello-world.wasm", "wamr", NULL, false, &args);

	if (!hello_world) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	rc = ocre_container_start(hello_world);
	if (rc) {
		fprintf(stderr, "Failed to start container\n");
		return 1;
	}

	rc = ocre_context_remove_container(ocre, hello_world);
	if (rc) {
		fprintf(stderr, "Failed to remove container\n");
		return 1;
	}

	struct ocre_container *blinky = ocre_context_create_container(ocre, "blinky.wasm", "wamr", NULL, true, &args);

	if (!blinky) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	rc = ocre_container_start(blinky);
	if (rc) {
		fprintf(stderr, "Failed to start container\n");
		return 1;
	}

	sleep(2);

	rc = ocre_container_kill(blinky);
	if (rc) {
		fprintf(stderr, "Failed to kill container\n");
		return 1;
	}

	rc = ocre_container_wait(blinky, &status);
	if (rc) {
		fprintf(stderr, "Failed to wait for container\n");
		return 1;
	}

	fprintf(stderr, "Container exited with status %d\n", status);

	rc = ocre_context_remove_container(ocre, blinky);
	if (rc) {
		fprintf(stderr, "Failed to remove container\n");
		return 1;
	}

	struct ocre_container *subscriber =
		ocre_context_create_container(ocre, "subscriber.wasm", "wamr", NULL, true, &args);

	if (!subscriber) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	struct ocre_container *publisher =
		ocre_context_create_container(ocre, "publisher.wasm", "wamr", NULL, true, &args);

	if (!publisher) {
		fprintf(stderr, "Failed to create container\n");
		return 1;
	}

	rc = ocre_container_start(subscriber);
	if (rc) {
		fprintf(stderr, "Failed to start container\n");
		return 1;
	}


	rc = ocre_container_start(publisher);
	if (rc) {
		fprintf(stderr, "Failed to start container\n");
		return 1;
	}

	sleep(8);

	rc = ocre_container_kill(subscriber);
	if (rc) {
		fprintf(stderr, "Failed to kill subscriber container\n");
		return 1;
	}

	rc = ocre_container_kill(publisher);
	if (rc) {
		fprintf(stderr, "Failed to kill publisher container\n");
		return 1;
	}

	rc = ocre_container_wait(subscriber, &status);
	if (rc) {
		fprintf(stderr, "Failed to wait for subscriber container\n");
		return 1;
	}

	fprintf(stderr, "Subscriber exited with status %d\n", status);

	rc = ocre_container_wait(publisher, &status);
	if (rc) {
		fprintf(stderr, "Failed to wait for publisher container\n");
		return 1;
	}

	fprintf(stderr, "Publisher exited with status %d\n", status);

	rc = ocre_context_remove_container(ocre, subscriber);
	if (rc) {
		fprintf(stderr, "Failed to remove subscriber\n");
		return 1;
	}

	rc = ocre_context_remove_container(ocre, publisher);
	if (rc) {
		fprintf(stderr, "Failed to remove publisher\n");
		return 1;
	}

	ocre_context_destroy(ocre);

	ocre_deinitialize();

	fprintf(stdout, "Demo completed successfully\n");

	return 0;
}
