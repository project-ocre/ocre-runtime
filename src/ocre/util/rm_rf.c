/**
 * @copyright Copyright (c) contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <ocre/platform/lstat.h>

/* Stack node for directory traversal */

struct dir_stack_node {
	char *path;
	DIR *dir;
	int is_removal_phase;
	struct dir_stack_node *next;
};

/* Stack operations */

static struct dir_stack_node *push_dir(struct dir_stack_node *stack, const char *path, int is_removal_phase)
{
	struct dir_stack_node *node = malloc(sizeof(struct dir_stack_node));
	if (!node) {
		return NULL;
	}

	node->path = strdup(path);
	if (!node->path) {
		free(node);
		return NULL;
	}

	node->dir = NULL;
	node->is_removal_phase = is_removal_phase;
	node->next = stack;

	return node;
}

static struct dir_stack_node *pop_dir(struct dir_stack_node *stack)
{
	if (!stack) {
		return NULL;
	}

	struct dir_stack_node *next = stack->next;

	if (stack->dir) {
		closedir(stack->dir);
	}
	free(stack->path);
	free(stack);

	return next;
}

/* Build full path by concatenating directory and filename */

static char *build_path(const char *dir, const char *name)
{
	size_t dir_len = strlen(dir);
	size_t name_len = strlen(name);
	size_t total_len = dir_len + name_len + 2; /* +1 for '/', +1 for '\0' */

	char *path = malloc(total_len);
	if (!path) {
		return NULL;
	}

	snprintf(path, total_len, "%s/%s", dir, name);
	return path;
}

/* Remove directory recursively using stack-based traversal (no function recursion) */

int rm_rf(const char *path)
{
	if (!path) {
		errno = EINVAL;
		return -1;
	}

	struct stat st;
	if (ocre_lstat(path, &st) == -1) {
		return -1; /* errno already set by stat */
	}

	/* If it's a regular file, just remove it */

	if (!S_ISDIR(st.st_mode)) {
		return unlink(path);
	}

	/* Stack for directory traversal */

	struct dir_stack_node *stack = NULL;
	int result = 0;

	/* Push initial directory onto stack */

	stack = push_dir(stack, path, 0);
	if (!stack) {
		errno = ENOMEM;
		return -1;
	}

	/* Process stack until empty */

	while (stack) {
		struct dir_stack_node *current = stack;

		if (current->is_removal_phase) {
			/* This directory has been processed, now remove it */

			if (rmdir(current->path) == -1) {
				result = -1;

				/* Continue processing to clean up stack */
			}
			stack = pop_dir(stack);
			continue;
		}

		/* First time processing this directory */

		if (!current->dir) {
			current->dir = opendir(current->path);
			if (!current->dir) {
				result = -1;
				stack = pop_dir(stack);
				continue;
			}
		}

		/* Read next directory entry */

		const struct dirent *entry = readdir(current->dir);
		if (!entry) {
			/* No more entries, mark for removal and continue */
			current->is_removal_phase = 1;
			continue;
		}

		/* Skip . and .. entries */

		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		/* Build full path for this entry */

		char *entry_path = build_path(current->path, entry->d_name);
		if (!entry_path) {
			result = -1;
			errno = ENOMEM;
			break;
		}

		/* Get entry information */

		if (ocre_lstat(entry_path, &st) == -1) {
			result = -1;
			free(entry_path);
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			/* It's a directory - push it onto stack for processing */

			struct dir_stack_node *new_node = push_dir(stack, entry_path, 0);
			if (!new_node) {
				result = -1;
				errno = ENOMEM;
				free(entry_path);
				break;
			}
			stack = new_node;
		} else {
			/* It's a file - remove it directly */

			if (unlink(entry_path) == -1) {
				result = -1;

				/* Continue processing other entries */
			}
		}

		free(entry_path);
	}

	/* Clean up remaining stack in case of error */

	while (stack) {
		stack = pop_dir(stack);
	}

	return result;
}
