/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <unity.h>
#include <ocre/ocre.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_ocre_build_config(void)
{
	TEST_ASSERT_NOT_NULL(ocre_build_configuration.version);
	TEST_ASSERT_NOT_NULL(ocre_build_configuration.commit_id);
	TEST_ASSERT_NOT_NULL(ocre_build_configuration.build_info);
	TEST_ASSERT_NOT_NULL(ocre_build_configuration.build_date);
}

void test_ocre_initialize_null(void)
{
	/* Initialize ocre should work */

	TEST_ASSERT_EQUAL_INT(0, ocre_initialize(NULL));

	ocre_deinitialize();
}

void test_ocre_initialize_arr_null(void)
{
	/* Initialize ocre with a NULL array should work */

	TEST_ASSERT_EQUAL_INT(0, ocre_initialize((const struct ocre_runtime_vtable *const[]){NULL}));

	ocre_deinitialize();
}

static int dummy_runtime_init_ok(void)
{
	return 0;
}

const struct ocre_runtime_vtable test_runtime_ok = {
	.runtime_name = "test",
	.init = dummy_runtime_init_ok,
};

void test_ocre_initialize_vtable_init_ok(void)
{
	/* Initialize ocre with a good vtable should call init on the runtime */

	TEST_ASSERT_EQUAL_INT(0, ocre_initialize((const struct ocre_runtime_vtable *const[]){
					 &test_runtime_ok,
					 NULL,
				 }));

	ocre_deinitialize();
}

static int dummy_runtime_init_err(void)
{
	return -1;
}

const struct ocre_runtime_vtable test_runtime_err = {
	.runtime_name = "test",
	.init = dummy_runtime_init_err,
};

void test_ocre_initialize_vtable_init_err(void)
{
	/* Initialize ocre with a good vtable should call init on the runtime but it fails */

	TEST_ASSERT_NOT_EQUAL_INT(0, ocre_initialize((const struct ocre_runtime_vtable *const[]){
					     &test_runtime_err,
					     NULL,
				     }));
}

void test_ocre_initialize_duplicate(void)
{
	/* Try to initialize ocre with two runtimes with the same name should fail */

	TEST_ASSERT_NOT_EQUAL_INT(0, ocre_initialize((const struct ocre_runtime_vtable *const[]){
					     &test_runtime_ok,
					     &test_runtime_ok,
					     NULL,
				     }));
}

static int dummy_runtime_init_func(void)
{
	/* We just deinit here and pass the test */

	ocre_deinitialize();

	TEST_PASS();

	return 0;
}

const struct ocre_runtime_vtable dummy_runtime = {
	.runtime_name = "dummy",
	.init = dummy_runtime_init_func,
};

void test_ocre_initialize_init_called(void)
{
	/* Initialize ocre with two runtimes with the same name should fail */

	ocre_initialize((const struct ocre_runtime_vtable *const[]){
		&dummy_runtime,
		NULL,
	});

	/* If we reach here, we failed to run the init function */

	TEST_FAIL();
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_ocre_build_config);
	RUN_TEST(test_ocre_initialize_null);
	RUN_TEST(test_ocre_initialize_arr_null);
	RUN_TEST(test_ocre_initialize_vtable_init_ok);
	RUN_TEST(test_ocre_initialize_vtable_init_err);
	RUN_TEST(test_ocre_initialize_duplicate);
	RUN_TEST(test_ocre_initialize_init_called);
	return UNITY_END();
}
