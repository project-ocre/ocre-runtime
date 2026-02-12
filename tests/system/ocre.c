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
	ocre_initialize(NULL);
}

void tearDown(void)
{
	ocre_deinitialize();
}

void test_ocre_create_default_context(void)
{
	/* Creating the main context should work */

	struct ocre_context *main_ctx = ocre_create_context(NULL);
	TEST_ASSERT_NOT_NULL(main_ctx);

	/* We can also destroy the main_ctx */

	TEST_ASSERT_EQUAL_INT(0, ocre_destroy_context(main_ctx));
}

void test_ocre_create_provided_context(void)
{
	/* Creating the other context should work */

	struct ocre_context *other_ctx = ocre_create_context(OCRE_TEST_OTHER_CONTEXT_PATH);
	TEST_ASSERT_NOT_NULL(other_ctx);

	/* We can also destroy the other context */

	TEST_ASSERT_EQUAL_INT(0, ocre_destroy_context(other_ctx));
}

void test_ocre_create_two_contexts(void)
{
	/* Creating the main context should work */

	struct ocre_context *main_ctx = ocre_create_context(NULL);
	TEST_ASSERT_NOT_NULL(main_ctx);

	/* Creating the other context should work */

	struct ocre_context *other_ctx = ocre_create_context(OCRE_TEST_OTHER_CONTEXT_PATH);
	TEST_ASSERT_NOT_NULL(other_ctx);

	/* We can also destroy the main_ctx */

	TEST_ASSERT_EQUAL_INT(0, ocre_destroy_context(main_ctx));

	/* We can also destroy the other context */

	TEST_ASSERT_EQUAL_INT(0, ocre_destroy_context(other_ctx));
}

void test_ocre_create_context_twice(void)
{
	/* Creating the context once should work */

	struct ocre_context *main_ctx = ocre_create_context(NULL);
	TEST_ASSERT_NOT_NULL(main_ctx);

	/* Trying to create another context with the same workdir should fail */

	TEST_ASSERT_NULL(ocre_create_context(NULL));

	/* We can also destroy the main_ctx */

	TEST_ASSERT_EQUAL_INT(0, ocre_destroy_context(main_ctx));
}

void test_ocre_create_context_no_cleanup(void)
{
	/* Creating the context once should work */

	struct ocre_context *main_ctx = ocre_create_context(NULL);
	TEST_ASSERT_NOT_NULL(main_ctx);

	/* Do not cleanup the context. ocre_deinitialize() should do it for us */
}

void test_ocre_destroy_null_context(void)
{
	TEST_ASSERT_NOT_EQUAL_INT(0, ocre_destroy_context(NULL));
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_ocre_create_default_context);
	RUN_TEST(test_ocre_create_provided_context);
	RUN_TEST(test_ocre_create_two_contexts);
	RUN_TEST(test_ocre_create_context_twice);
	RUN_TEST(test_ocre_create_context_no_cleanup);
	RUN_TEST(test_ocre_destroy_null_context);
	return UNITY_END();
}
