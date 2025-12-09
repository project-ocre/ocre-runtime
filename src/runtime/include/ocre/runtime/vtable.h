#ifndef OCRE_RUNTIME_VTABLE_H
#define OCRE_RUNTIME_VTABLE_H

#include <stddef.h>

struct ocre_runtime_vtable {
	const char *const runtime_name;
	int (*init)(void);
	int (*deinit)(void);

	void *(*create)(const char *img_path, const char *workdir, size_t stack_size, size_t heap_size,
			const char **capabilities, const char **argv, const char **envp, const char **mounts);
	int (*destroy)(void *runtime_context);
	int (*thread_execute)(void *arg);
	int (*kill)(void *runtime_context);
	int (*pause)(void *runtime_context);
	int (*unpause)(void *runtime_context);
};

#endif /* OCRE_RUNTIME_VTABLE_H */
