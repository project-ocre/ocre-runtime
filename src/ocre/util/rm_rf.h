#ifndef RM_RF_H
#define RM_RF_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Remove a file or directory recursively.
 * 
 * This function removes the specified path and all its contents recursively.
 * It uses a stack-based approach to traverse directories without function recursion.
 * 
 * @param path The path to the file or directory to remove
 * @return 0 on success, -1 on error (errno is set appropriately)
 * 
 * Note: This function will attempt to remove as many files/directories as possible
 * even if some operations fail. The return value indicates if any errors occurred.
 */
int rm_rf(const char *path);

/**
 * Remove a file or directory recursively with error callback.
 * 
 * This function is similar to rm_rf() but provides detailed error reporting
 * through a callback function that is called for each error encountered.
 * 
 * @param path The path to the file or directory to remove
 * @param error_callback Callback function called for each error. Should return 0
 *                      to continue processing, non-zero to abort. Can be NULL.
 *                      Parameters: (const char *failed_path, int error_code)
 * @return 0 on success, -1 on error (errno is set appropriately)
 * 
 * The error_callback receives:
 * - failed_path: The path that caused the error (or descriptive string for memory errors)
 * - error_code: The errno value associated with the error
 */
int rm_rf_verbose(const char *path, int (*error_callback)(const char *path, int error_code));

#ifdef __cplusplus
}
#endif

#endif /* RM_RF_H */