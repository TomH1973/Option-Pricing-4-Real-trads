#ifndef PATH_RESOLUTION_H
#define PATH_RESOLUTION_H

/**
 * @file path_resolution.h
 * @brief Path resolution utilities for the unified option pricing system
 */

/**
 * @brief Get the directory containing the current script
 * @return The directory path or NULL on failure
 */
char* get_script_dir(void);

/**
 * @brief Get the project root directory
 * @return The project root directory path or NULL on failure
 */
char* get_project_root(void);

/**
 * @brief Resolve the path to a unified system binary
 * @param binary_name The name of the binary to resolve
 * @return The full path to the binary or NULL on failure
 */
char* resolve_binary_path(const char* binary_name);

/**
 * @brief Resolve the path to a legacy system binary
 * @param version Version of the legacy binary to resolve (e.g., "v2", "v3")
 * @param binary_name The name of the binary to resolve
 * @return The full path to the binary or NULL on failure
 */
char* resolve_legacy_binary_path(const char* version, const char* binary_name);

/**
 * @brief Clean up memory allocated by path resolution functions
 * @param path The path to free
 */
void free_resolved_path(char* path);

#endif /* PATH_RESOLUTION_H */ 