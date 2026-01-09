/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <unity.h>
#include <ocre/ocre.h>

struct ocre_context *context;

void setUp(void)
{
	ocre_initialize(NULL);
	context = ocre_create_context("./ocre/src/ocre/var/lib/ocre");
}

void tearDown(void)
{
	ocre_destroy_context(context);
	ocre_deinitialize();
}

void test_ocre_context_initialized(void)
{
	/* Context is initialized */

	TEST_ASSERT_NOT_NULL(context);
}

void test_ocre_context_get_working_directory_ok(void)
{
	/* Try to get working directory with good context */

	TEST_ASSERT_NOT_NULL(ocre_context_get_working_directory(context));
}

void test_ocre_context_get_working_directory_err(void)
{
	/* Try to get working directory with bad context */

	TEST_ASSERT_NULL(ocre_context_get_working_directory(NULL));
}

void test_ocre_context_create_container_null_context(void)
{
	/* Try to create container with bad context */

	TEST_ASSERT_NULL(ocre_context_create_container(NULL, "hello-world.wasm", "wamr/wasip1", NULL, false, NULL));
}

void test_ocre_context_create_container_bad_ids(void)
{
	/* Try to create container with bad ids */

	TEST_ASSERT_NULL(ocre_context_create_container(context, NULL, NULL, NULL, false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "", "", NULL, false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", ".", false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "..", false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "/", false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "/somewhere", false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "some/where", false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, ".", "wamr/wasip1", NULL, false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "..", "wamr/wasip1", NULL, false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "/", "wamr/wasip1", NULL, false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "/somewhere", "wamr/wasip1", NULL, false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "some/where", "wamr/wasip1", NULL, false, NULL));
}

void test_ocre_context_create_container_bad_runtimes(void)
{
	TEST_ASSERT_NULL(
		ocre_context_create_container(context, "hello-world.wasm", "does-not-exit", NULL, false, NULL));
	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "", NULL, false, NULL));
}

void test_ocre_context_create_container_bad_mounts(void)
{
	struct ocre_container_args bad_mounts = {0};

	bad_mounts.mounts = (const char *[]){"no-colon", NULL};

	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, &bad_mounts));

	bad_mounts.mounts = (const char *[]){"no_abs_path:/hello", NULL};

	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, &bad_mounts));

	bad_mounts.mounts = (const char *[]){"/tmp:/", NULL};

	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, &bad_mounts));

	bad_mounts.mounts = (const char *[]){"/tmp:no_abs", NULL};

	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, &bad_mounts));

	bad_mounts.mounts = (const char *[]){"/tmp:", NULL};

	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, &bad_mounts));

	bad_mounts.mounts = (const char *[]){"/tmp", NULL};

	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, &bad_mounts));

	bad_mounts.mounts = (const char *[]){"/tmp:/ok", "bad", NULL};

	TEST_ASSERT_NULL(ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, &bad_mounts));
}

void test_ocre_context_remove_bad_container(void)
{
	/* Try to remove a non-existent container */

	TEST_ASSERT_NOT_EQUAL_INT(0, ocre_context_remove_container(context, NULL));
}

void test_ocre_context_remove_bad_context(void)
{
	/* Try to remove containers from null context */

	TEST_ASSERT_NOT_EQUAL_INT(0, ocre_context_remove_container(NULL, NULL));
}

void test_ocre_context_create_container_ok(void)
{
	/* Create a valid container */

	struct ocre_container *container =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, NULL);
	TEST_ASSERT_NOT_NULL(container);

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(container));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_create_container_null_runtime_ok(void)
{
	/* Create a valid container */

	struct ocre_container *container =
		ocre_context_create_container(context, "hello-world.wasm", NULL, NULL, false, NULL);
	TEST_ASSERT_NOT_NULL(container);

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(container));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_create_container_with_id_ok(void)
{
	/* Create a valid container */

	struct ocre_container *container =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "test-container", false, NULL);
	TEST_ASSERT_NOT_NULL(container);

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(container));

	/* Check container id */

	TEST_ASSERT_EQUAL_STRING("test-container", ocre_container_get_id(container));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_create_start_container_filesystem(void)
{
	const struct ocre_container_args args = {
		.capabilities =
			(const char *[]){
				"filesystem",
				"ocre:api",
				NULL,
			},
	};

	/* Create a valid container */

	struct ocre_container *container =
		ocre_context_create_container(context, "filesystem.wasm", "wamr/wasip1", NULL, false, &args);
	TEST_ASSERT_NOT_NULL(container);

	/* Start the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(container));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_create_container_with_id_twice(void)
{
	/* Create a valid container */

	struct ocre_container *container =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "test-container", false, NULL);
	TEST_ASSERT_NOT_NULL(container);

	/* Another container with the same id should fail */

	TEST_ASSERT_NULL(
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "test-container", false, NULL));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_create_container_and_forget(void)
{
	/* Create a valid container */

	struct ocre_container *container =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "should-not-leak", false, NULL);
	TEST_ASSERT_NOT_NULL(container);

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(container));

	/* Do not remove the container. When the context is destroyed, the container will be removed automatically */
}

void test_ocre_context_create_wait_remove(void)
{
	/* Create a valid container */

	struct ocre_container *container =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, NULL);
	TEST_ASSERT_NOT_NULL(container);

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(container));

	/* Start the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(container));

	/* Wait for the container to finish */

	int status;
	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(container, &status));

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(0, status);

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_create_no_ocre_api(void)
{
	/* Create a valid container but it won't work as we don't have ocre:api */

	struct ocre_container *container =
		ocre_context_create_container(context, "blinky.wasm", "wamr/wasip1", NULL, true, NULL);
	TEST_ASSERT_NOT_NULL(container);

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(container));

	/* Start the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(container));

	/* Wait for the container to finish */

	int status;
	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(container, &status));

	/* Check container status */
	// TODO: Maybe this should be error?

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_STOPPED, ocre_container_get_status(container));

	/* Check return status */

	TEST_ASSERT_EQUAL_INT(-1, status);

	/* Cannot kill stopped container */

	TEST_ASSERT_EQUAL_INT(-1, ocre_container_kill(container));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_create_kill_wait_remove(void)
{
	const struct ocre_container_args args = {
		.capabilities =
			(const char *[]){
				"ocre:api",
				NULL,
			},
	};

	/* Create a valid container and have ocre:api*/

	struct ocre_container *container =
		ocre_context_create_container(context, "blinky.wasm", "wamr/wasip1", NULL, true, &args);
	TEST_ASSERT_NOT_NULL(container);

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(container));

	/* Start the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(container));

	/* Check container status */
	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(container));

	/* Kill the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_kill(container));

	/* Wait for the container to finish */

	int status;
	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(container, &status));

	/* Check container status */

	TEST_ASSERT_EQUAL_INT(OCRE_CONTAINER_STATUS_STOPPED, ocre_container_get_status(container));

	/* Check return status */
	// TODO: Maybe this should not be 0

	TEST_ASSERT_EQUAL_INT(0, status);

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_get_container_count_null(void)
{
	/* There must be zero containers */

	TEST_ASSERT_EQUAL_INT(-1, ocre_context_get_container_count(NULL));
}

void test_ocre_context_get_container_count(void)
{
	/* There must be zero containers */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_get_container_count(context));

	/* Create a valid container */

	struct ocre_container *container1 =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, NULL);
	TEST_ASSERT_NOT_NULL(container1);

	/* There must be one container */

	TEST_ASSERT_EQUAL_INT(1, ocre_context_get_container_count(context));

	/* Create another valid container */

	struct ocre_container *container2 =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, NULL);
	TEST_ASSERT_NOT_NULL(container2);

	/* There must be two containers */

	TEST_ASSERT_EQUAL_INT(2, ocre_context_get_container_count(context));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container1));

	/* There must be one container */

	TEST_ASSERT_EQUAL_INT(1, ocre_context_get_container_count(context));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container2));

	/* There must be zero containers */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_get_container_count(context));
}

void test_ocre_context_get_container_by_id_error(void)
{
	TEST_ASSERT_NULL(ocre_context_get_container_by_id(context, "bad_id"));
	TEST_ASSERT_NULL(ocre_context_get_container_by_id(context, NULL));
	TEST_ASSERT_NULL(ocre_context_get_container_by_id(NULL, "bad_id"));
	TEST_ASSERT_NULL(ocre_context_get_container_by_id(NULL, NULL));
}

void test_ocre_context_get_container_by_id_ok(void)
{
	/* Create a valid container */

	struct ocre_container *container =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "my-id", false, NULL);
	TEST_ASSERT_NOT_NULL(container);

	/* Get container by ID */

	TEST_ASSERT_EQUAL_PTR(container, ocre_context_get_container_by_id(context, "my-id"));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container));
}

void test_ocre_context_get_containers_err(void)
{
	struct ocre_container *containers[1];
	TEST_ASSERT_EQUAL_INT(-1, ocre_context_get_containers(context, NULL, 0));
	TEST_ASSERT_EQUAL_INT(-1, ocre_context_get_containers(context, NULL, 1));
	TEST_ASSERT_EQUAL_INT(-1, ocre_context_get_containers(NULL, NULL, 0));
	TEST_ASSERT_EQUAL_INT(-1, ocre_context_get_containers(NULL, NULL, 1));
	TEST_ASSERT_EQUAL_INT(-1, ocre_context_get_containers(NULL, containers, 0));
	TEST_ASSERT_EQUAL_INT(-1, ocre_context_get_containers(NULL, containers, 1));
}

void test_ocre_context_get_containers_zero(void)
{
	struct ocre_container *containers[1];
	TEST_ASSERT_EQUAL_INT(0, ocre_context_get_containers(context, containers, 0));
	TEST_ASSERT_EQUAL_INT(0, ocre_context_get_containers(context, containers, 1));
}

void test_ocre_context_get_containers_ok(void)
{
	struct ocre_container *containers[2];

	/* Create a valid container */

	struct ocre_container *container1 =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, NULL);
	TEST_ASSERT_NOT_NULL(container1);

	/* There must be one container */

	TEST_ASSERT_EQUAL_INT(1, ocre_context_get_containers(context, containers, 2));

	/* Create another valid container */

	struct ocre_container *container2 =
		ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", NULL, false, NULL);
	TEST_ASSERT_NOT_NULL(container2);

	/* There must be two containers */

	TEST_ASSERT_EQUAL_INT(2, ocre_context_get_containers(context, containers, 2));

	/* We request only one */

	TEST_ASSERT_EQUAL_INT(1, ocre_context_get_containers(context, containers, 1));

	/* We request zero */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_get_containers(context, containers, 0));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container1));

	/* There must be one container */

	TEST_ASSERT_EQUAL_INT(1, ocre_context_get_containers(context, containers, 2));

	/* Remove the container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_remove_container(context, container2));

	/* There must be one container */

	TEST_ASSERT_EQUAL_INT(0, ocre_context_get_containers(context, containers, 2));
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_ocre_context_initialized);
	RUN_TEST(test_ocre_context_get_working_directory_ok);
	RUN_TEST(test_ocre_context_get_working_directory_err);
	RUN_TEST(test_ocre_context_create_container_null_context);
	RUN_TEST(test_ocre_context_create_container_bad_ids);
	RUN_TEST(test_ocre_context_create_container_bad_runtimes);
	RUN_TEST(test_ocre_context_create_container_bad_mounts);
	RUN_TEST(test_ocre_context_remove_bad_container);
	RUN_TEST(test_ocre_context_remove_bad_context);
	RUN_TEST(test_ocre_context_create_container_ok);
	RUN_TEST(test_ocre_context_create_container_null_runtime_ok);
	RUN_TEST(test_ocre_context_create_container_with_id_ok);
	RUN_TEST(test_ocre_context_create_container_with_id_twice);
	RUN_TEST(test_ocre_context_create_container_and_forget);
	RUN_TEST(test_ocre_context_create_wait_remove);
	RUN_TEST(test_ocre_context_create_no_ocre_api);
	RUN_TEST(test_ocre_context_create_kill_wait_remove);
	RUN_TEST(test_ocre_context_create_start_container_filesystem);
	RUN_TEST(test_ocre_context_get_container_count);
	RUN_TEST(test_ocre_context_get_container_count_null);
	RUN_TEST(test_ocre_context_get_container_by_id_error);
	RUN_TEST(test_ocre_context_get_container_by_id_ok);
	RUN_TEST(test_ocre_context_get_containers_err);
	RUN_TEST(test_ocre_context_get_containers_zero);
	RUN_TEST(test_ocre_context_get_containers_ok);
	return UNITY_END();
}
