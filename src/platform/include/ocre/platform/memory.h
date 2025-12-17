#include <stddef.h>

void *user_malloc(size_t size);
void user_free(void *p);
void *user_realloc(void *p, size_t size);
