#include <stddef.h>

void *ocre_load_file(const char *path, size_t *size);
int ocre_unload_file(void *buffer, size_t size);
