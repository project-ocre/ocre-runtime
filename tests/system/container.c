/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <unistd.h>

#include <unity.h>
#include <ocre/ocre.h>

struct ocre_context *context;
struct ocre_container *hello_world;
struct ocre_container *blinky;

void setUp(void)
{
	const struct ocre_container_args args = {
		.capabilities =
			(const char *[]){
				"ocre:api",
				NULL,
			},
	};

	ocre_initialize(NULL);
	context = ocre_create_context("./ocre/src/ocre/var/lib/ocre");

	hello_world = ocre_context_create_container(context, "hello-world.wasm", "wamr/wasip1", "hello", false, NULL);
	blinky = ocre_context_create_container(context, "blinky.wasm", "wamr/wasip1", NULL, true, &args);
}

void tearDown(void)
{
	ocre_container_kill(hello_world);
	ocre_container_kill(blinky);
	ocre_container_wait(hello_world, NULL);
	ocre_container_wait(blinky, NULL);
	ocre_context_remove_container(context, hello_world);
	ocre_context_remove_container(context, blinky);
	ocre_destroy_context(context);
	ocre_deinitialize();
}

void test_ocre_container_status_null(void)
{
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_UNKNOWN, ocre_container_get_status(NULL));
}

void test_ocre_container_status(void)
{
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(blinky));

	/* Start blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(blinky));
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(blinky));

	/* Kill blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_kill(blinky));

	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(blinky, NULL));

	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_STOPPED, ocre_container_get_status(blinky));
}

void test_ocre_container_restart(void)
{
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_CREATED, ocre_container_get_status(hello_world));

	/* Run hello_world */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(hello_world));
	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(hello_world, NULL));
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_STOPPED, ocre_container_get_status(hello_world));

	/* Run again */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(hello_world));
	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(hello_world, NULL));
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_STOPPED, ocre_container_get_status(hello_world));
}

void test_ocre_container_kill(void)
{
	/* Run blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(blinky));
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(blinky));

	/* Kill blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_kill(blinky));

	/* Wait for blinky to stop */

	int status;
	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(blinky, &status));

	/* Return code should indicate killed */
	// TODO: FIX!!!
	TEST_ASSERT_EQUAL(0, status);

	/* Get status */

	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_STOPPED, ocre_container_get_status(blinky));
}

void test_ocre_container_destroy(void)
{
	/* Start blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(blinky));

	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(blinky));
}

void test_ocre_container_get_image_null(void)
{
	TEST_ASSERT_NULL(ocre_container_get_image(NULL));
}

void test_ocre_container_get_image_blinky(void)
{
	TEST_ASSERT_EQUAL_STRING("blinky.wasm", ocre_container_get_image(blinky));
}

void test_ocre_container_get_id_null(void)
{
	TEST_ASSERT_NULL(ocre_container_get_id(NULL));
}

void test_ocre_container_get_id_hello(void)
{
	TEST_ASSERT_EQUAL_STRING("hello", ocre_container_get_id(hello_world));
}

void test_ocre_container_pause_unpause_null(void)
{
	TEST_ASSERT_EQUAL_INT(-1, ocre_container_pause(NULL));
	TEST_ASSERT_EQUAL_INT(-1, ocre_container_unpause(NULL));
}

void test_ocre_container_pause_unpause_wamr(void)
{
	/* Run blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(blinky));
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(blinky));

	/* Pause blinky. Does not work, will return -1 */

	TEST_ASSERT_EQUAL_INT(-1, ocre_container_pause(blinky));

	/* Should be running because pause does not work */
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(blinky));

	/* Unpause blinky. Does not work, will return -1 */

	TEST_ASSERT_EQUAL_INT(-1, ocre_container_unpause(blinky));

	/* Should be running */
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(blinky));

	/* Kill blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_kill(blinky));

	/* Wait for blinky to stop */

	int status;
	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(blinky, &status));
}

void test_ocre_container_stop_null(void)
{
	TEST_ASSERT_EQUAL_INT(-1, ocre_container_stop(NULL));
}

void test_ocre_container_stop_wamr(void)
{
	/* Run blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(blinky));
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(blinky));

	/* Stop blinky. Does not work, will return -1 */

	TEST_ASSERT_EQUAL_INT(-1, ocre_container_stop(blinky));

	/* Should be running because stop does not work */
	TEST_ASSERT_EQUAL(OCRE_CONTAINER_STATUS_RUNNING, ocre_container_get_status(blinky));

	/* Kill blinky */

	TEST_ASSERT_EQUAL_INT(0, ocre_container_kill(blinky));

	/* Wait for blinky to stop */

	int status;
	TEST_ASSERT_EQUAL_INT(0, ocre_container_wait(blinky, &status));
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_ocre_container_status_null);
	RUN_TEST(test_ocre_container_status);
	RUN_TEST(test_ocre_container_restart);
	RUN_TEST(test_ocre_container_kill);
	RUN_TEST(test_ocre_container_destroy);
	RUN_TEST(test_ocre_container_get_image_null);
	RUN_TEST(test_ocre_container_get_image_blinky);
	RUN_TEST(test_ocre_container_get_id_null);
	RUN_TEST(test_ocre_container_get_id_hello);
	RUN_TEST(test_ocre_container_pause_unpause_null);
	RUN_TEST(test_ocre_container_pause_unpause_wamr);
	RUN_TEST(test_ocre_container_stop_null);
	RUN_TEST(test_ocre_container_stop_wamr);
	return UNITY_END();
}
