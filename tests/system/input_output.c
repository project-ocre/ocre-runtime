/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <unistd.h>

#include <unity.h>
#include <ocre/ocre.h>

#define ARG_TEST_STRING "Zirigdum!"

struct ocre_context *context;

void setUp(void)
{
	ocre_initialize(NULL);
	context = ocre_create_context(NULL);
}

void tearDown(void)
{
	ocre_destroy_context(context);
	ocre_deinitialize();
}

void test_ocre_container_output_stdout(void)
{
	const struct ocre_container_args args = {
		.argv =
			(const char *[]){
				ARG_TEST_STRING,
				NULL,
			},
	};

	int stdout_pair[2];
	TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, stdout_pair));

	struct ocre_container *container =
		ocre_context_create_container(context, "print_args.wasm", "wamr/wasip1", NULL, true, &args,
					      STDIN_FILENO, stdout_pair[1], STDERR_FILENO);

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(container));

	ocre_container_wait(container, NULL);

	/* Skip to newline */

	char c;
	do {
		size_t n = read(stdout_pair[0], &c, 1);
		TEST_ASSERT_EQUAL_size_t(1, n);
	} while (c != '\n');

	char buf[19]; /* "argv[1]=Zirigdum!\n" */
	memset(buf, 0, sizeof(buf));

	ssize_t n = read(stdout_pair[0], buf, sizeof(buf) - 1);

	TEST_ASSERT_EQUAL_size_t(sizeof(buf) - 1, n);

	TEST_ASSERT_EQUAL_STRING("argv[1]=" ARG_TEST_STRING "\n", buf);

	ocre_container_kill(container);
	close(stdout_pair[0]);
	ocre_container_wait(container, NULL);

	ocre_context_remove_container(context, container);
}

void test_ocre_container_input_stdin_output_stdout(void)
{
	int stdin_pair[2];
	int stdout_pair[2];
	TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, stdin_pair));
	TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, stdout_pair));

	struct ocre_container *container = ocre_context_create_container(
		context, "cat.wasm", "wamr/wasip1", NULL, true, NULL, stdin_pair[1], stdout_pair[1], STDERR_FILENO);

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(container));

	char buf[1000];
	memset(buf, 0, sizeof(buf));

	ssize_t nwrite = write(stdin_pair[0], ARG_TEST_STRING "\n", strlen(ARG_TEST_STRING) + 1);

	ssize_t nread = read(stdout_pair[0], buf, nwrite);

	TEST_ASSERT_EQUAL(nwrite, nread);

	TEST_ASSERT_EQUAL_STRING(ARG_TEST_STRING "\n", buf);

	ocre_container_kill(container);
	close(stdin_pair[0]);
	close(stdout_pair[0]);
	ocre_container_wait(container, NULL);

	ocre_context_remove_container(context, container);
}

void test_ocre_container_input_stdin_output_stderr(void)
{
	const struct ocre_container_args args = {
		.argv =
			(const char *[]){
				"-e",
				NULL,
			},
	};

	int stdin_pair[2];
	int stderr_pair[2];
	TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, stdin_pair));
	TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, stderr_pair));

	struct ocre_container *container = ocre_context_create_container(
		context, "cat.wasm", "wamr/wasip1", NULL, true, &args, stdin_pair[1], STDOUT_FILENO, stderr_pair[1]);

	TEST_ASSERT_EQUAL_INT(0, ocre_container_start(container));

	char buf[1000];
	memset(buf, 0, sizeof(buf));

	ssize_t nwrite = write(stdin_pair[0], ARG_TEST_STRING "\n", strlen(ARG_TEST_STRING) + 1);

	ssize_t nread = read(stderr_pair[0], buf, nwrite);

	TEST_ASSERT_EQUAL(nwrite, nread);

	TEST_ASSERT_EQUAL_STRING(ARG_TEST_STRING "\n", buf);

	ocre_container_kill(container);
	close(stdin_pair[0]);
	close(stderr_pair[0]);
	ocre_container_wait(container, NULL);

	ocre_context_remove_container(context, container);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_ocre_container_output_stdout);
	RUN_TEST(test_ocre_container_input_stdin_output_stdout);
	RUN_TEST(test_ocre_container_input_stdin_output_stderr);
	return UNITY_END();
}
