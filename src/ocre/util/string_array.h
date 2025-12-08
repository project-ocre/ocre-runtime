#include <stddef.h>
size_t string_array_size(const char **const array);
char **string_array_dup(char **src);
char **string_array_deep_dup(const char **const src);
void string_array_free(char **array);
size_t string_array_copy(char **dest, char **src);
