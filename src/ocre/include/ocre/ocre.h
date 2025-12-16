#ifndef OCRE_H
#define OCRE_H

#include <stdbool.h>
#include <stddef.h>

#include <ocre/runtime/vtable.h>

/**
 * @brief Enum representing the possible status of a container.
 */
typedef enum {
	OCRE_CONTAINER_STATUS_UNKNOWN = 0, ///< Status is unknown.
	OCRE_CONTAINER_STATUS_CREATED,	   ///< Container has been created.
	OCRE_CONTAINER_STATUS_RUNNING,	   ///< Container is currently running.
	OCRE_CONTAINER_STATUS_PAUSED,	   ///< Container is currently paused.
	OCRE_CONTAINER_STATUS_EXITED,	   ///< Container has exited but we did not get the exit code yet.
	OCRE_CONTAINER_STATUS_STOPPED,	   ///< Container has been stopped.
	OCRE_CONTAINER_STATUS_ERROR,	   ///< An error occurred with the container.
} ocre_container_status_t;

struct ocre_config {
	const char *version;
	const char *commit_id;
	const char *build_info;
};

extern const struct ocre_config ocre_build_configuration;

struct ocre_context;

struct ocre_container_args {
	const char **argv;
	const char **envp;
	const char **capabilities;
	const char **mounts;
};

struct ocre_container;

int ocre_initialize(void);
int ocre_initialize_with_runtimes(const struct ocre_runtime_vtable *const vtable[]);
struct ocre_context *ocre_create_context(const char *workdir);
void ocre_context_destroy(struct ocre_context *context);
void ocre_deinitialize(void);

struct ocre_container *ocre_context_create_container(struct ocre_context *context, const char *image,
						     const char *const runtime, const char *container_id, bool detached,
						     const struct ocre_container_args *arguments);
struct ocre_container *ocre_context_get_container_by_id(struct ocre_context *context, const char *id);
int ocre_context_remove_container(struct ocre_context *context, struct ocre_container *container);
int ocre_context_get_num_containers(struct ocre_context *context);
int ocre_context_list_containers(struct ocre_context *context, struct ocre_container **containers, int max_size);
char *ocre_context_get_working_directory(struct ocre_context *context);

int ocre_container_start(struct ocre_container *container);
ocre_container_status_t ocre_container_get_status(struct ocre_container *container);
char *ocre_container_get_id(const struct ocre_container *container);
char *ocre_container_get_image(const struct ocre_container *container);

int ocre_container_pause(struct ocre_container *container);
int ocre_container_unpause(struct ocre_container *container);
int ocre_container_stop(struct ocre_container *container);
int ocre_container_kill(struct ocre_container *container);
int ocre_container_wait(struct ocre_container *container, int *status);

#endif /* OCRE_H */
