struct ocre_context;

// struct ocre_container *ocre_container_create(const char *path, const char *const runtime, const char *container_id,
//                                              const void *arguments);
// void ocre_container_destroy(struct ocre_container *container);
struct ocre_container *ocre_context_get_container_by_id_locked(const struct ocre_context *context, const char *id);
