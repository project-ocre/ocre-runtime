#include <stddef.h>

/* returns the size of a string array including the NULL termination
 * returns 0 if pointer is null
 */
size_t string_array_size(const char **const array);

/* duplicates an array of char *
 * returns NULL on error
 */
char **string_array_dup(char **src);

/* makes a deep copy of an array of char *
 * returns NULL on error
 */
char **string_array_deep_dup(const char **const src);

/* frees an array of char *
 * does nothing if pointer is null
 */
void string_array_free(char **array);

/* copies an array of char *
 * returns the number of elements copied
 */
size_t string_array_copy(char **dest, char **src);

/* looks up a key in an array of char *
 * returns NULL if not found
 */
const char *string_array_lookup(const char **array, const char *key);
