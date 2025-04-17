#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>

#include "../include/path_resolution.h"
#include "../include/error_handling.h"

/* Maximum path length for safety */
#define MAX_PATH_LENGTH PATH_MAX

/**
 * Check if a file exists
 */
static int file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

/**
 * @brief Get the directory containing the current script
 * @return The directory path or NULL on failure
 */
char* get_script_dir(void) {
    char* script_path = NULL;
    char* script_dir = NULL;
    char buffer[MAX_PATH_LENGTH];
    ssize_t len;
    
    /* Use /proc/self/exe to get the path of the current executable */
    if ((len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1)) == -1) {
        set_error(ERROR_PATH_RESOLUTION);
        return NULL;
    }
    
    buffer[len] = '\0';
    
    /* Use dirname to get the directory */
    script_path = strdup(buffer);
    if (script_path == NULL) {
        set_error(ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    script_dir = strdup(dirname(script_path));
    free(script_path);
    
    if (script_dir == NULL) {
        set_error(ERROR_MEMORY_ALLOCATION);
        return NULL;
    }
    
    return script_dir;
}

/**
 * @brief Get the project root directory
 * @return The project root directory path or NULL on failure
 */
char* get_project_root(void) {
    char* script_dir = get_script_dir();
    char* project_root = NULL;
    char path[MAX_PATH_LENGTH];
    
    if (script_dir == NULL) {
        /* Error already set by get_script_dir */
        return NULL;
    }
    
    /* Try different paths to find project root */
    
    /* First try - script_dir is in the unified/scripts directory */
    snprintf(path, sizeof(path), "%s/../..", script_dir);
    if (file_exists(path)) {
        project_root = realpath(path, NULL);
        free(script_dir);
        return project_root;
    }
    
    /* Second try - script_dir is in the project root */
    snprintf(path, sizeof(path), "%s", script_dir);
    if (file_exists(path)) {
        project_root = realpath(path, NULL);
        free(script_dir);
        return project_root;
    }
    
    free(script_dir);
    set_error(ERROR_PATH_RESOLUTION);
    return NULL;
}

/**
 * @brief Resolve the path to a unified system binary
 * @param binary_name The name of the binary to resolve
 * @return The full path to the binary or NULL on failure
 */
char* resolve_binary_path(const char* binary_name) {
    char* project_root = get_project_root();
    char* binary_path = NULL;
    char path[MAX_PATH_LENGTH];
    
    if (project_root == NULL) {
        /* Error already set by get_project_root */
        return NULL;
    }
    
    /* Construct the expected path to the binary */
    snprintf(path, sizeof(path), "%s/unified/bin/%s", project_root, binary_name);
    
    /* Check if the binary exists at the expected path */
    if (file_exists(path)) {
        binary_path = strdup(path);
        free(project_root);
        return binary_path;
    }
    
    /* If not found, try in the current project directory */
    snprintf(path, sizeof(path), "%s/%s", project_root, binary_name);
    
    if (file_exists(path)) {
        binary_path = strdup(path);
        free(project_root);
        return binary_path;
    }
    
    free(project_root);
    set_error(ERROR_FILE_NOT_FOUND);
    return NULL;
}

/**
 * @brief Resolve the path to a legacy system binary
 * @param version Version of the legacy binary to resolve (e.g., "v2", "v3")
 * @param binary_name The name of the binary to resolve
 * @return The full path to the binary or NULL on failure
 */
char* resolve_legacy_binary_path(const char* version, const char* binary_name) {
    char* project_root = get_project_root();
    char* binary_path = NULL;
    char path[MAX_PATH_LENGTH];
    
    if (project_root == NULL) {
        /* Error already set by get_project_root */
        return NULL;
    }
    
    /* Construct the expected path to the legacy binary */
    if (version && *version) {
        snprintf(path, sizeof(path), "%s/%s_%s", project_root, binary_name, version);
    } else {
        snprintf(path, sizeof(path), "%s/%s", project_root, binary_name);
    }
    
    /* Check if the binary exists at the expected path */
    if (file_exists(path)) {
        binary_path = strdup(path);
        free(project_root);
        return binary_path;
    }
    
    free(project_root);
    set_error(ERROR_FILE_NOT_FOUND);
    return NULL;
}

/**
 * @brief Clean up memory allocated by path resolution functions
 * @param path The path to free
 */
void free_resolved_path(char* path) {
    free(path);
} 