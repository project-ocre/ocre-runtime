#include <ocre/platform/memory.h>
#include <stdlib.h>

void *user_malloc(size_t size)
{
	return malloc(size);
}

void user_free(void *p)
{
	free(p);
}

void *user_calloc(size_t n, size_t size)
{
	return calloc(n, size);
}

void *user_realloc(void *p, size_t size)
{
	return realloc(p, size);
}
