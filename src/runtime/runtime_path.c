#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

#ifdef _WIN32
#include "../platform/platform.h"
#else
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#endif

#include "runtime_path.h"
#include "runtime_arena.h"
#include "runtime_array.h"

/* ============================================================================
 * Path Utilities
 * ============================================================================
 * Implementation of cross-platform path manipulation functions.
 * ============================================================================ */

/* Platform-specific path separator */
#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

/* Helper: Check if character is a path separator */
static int is_path_separator(char c) {
#ifdef _WIN32
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

/* Helper: Find the last path separator in a string */
static const char *find_last_separator(const char *path) {
    const char *last = NULL;
    for (const char *p = path; *p; p++) {
        if (is_path_separator(*p)) {
            last = p;
        }
    }
    return last;
}

/* Extract directory portion of a path */
char *rt_path_directory(RtArena *arena, const char *path) {
    if (path == NULL || *path == '\0') {
        return rt_arena_strdup(arena, ".");
    }

    const char *last_sep = find_last_separator(path);
    if (last_sep == NULL) {
        /* No separator found - return current directory */
        return rt_arena_strdup(arena, ".");
    }

    /* Handle root path (/ or C:\) - always return forward slash for consistency */
    if (last_sep == path) {
        return rt_arena_strdup(arena, "/");
    }

#ifdef _WIN32
    /* Handle Windows drive letter like C:\ */
    if (last_sep == path + 2 && path[1] == ':') {
        char *result = rt_arena_alloc(arena, 4);
        result[0] = path[0];
        result[1] = ':';
        result[2] = '/';  /* Use forward slash for consistency */
        result[3] = '\0';
        return result;
    }
#endif

    /* Return everything up to (not including) the last separator */
    size_t dir_len = last_sep - path;
    char *result = rt_arena_alloc(arena, dir_len + 1);
    memcpy(result, path, dir_len);
    result[dir_len] = '\0';
    return result;
}

/* Extract filename (with extension) from a path */
char *rt_path_filename(RtArena *arena, const char *path) {
    if (path == NULL || *path == '\0') {
        return rt_arena_strdup(arena, "");
    }

    const char *last_sep = find_last_separator(path);
    if (last_sep == NULL) {
        /* No separator - the whole thing is the filename */
        return rt_arena_strdup(arena, path);
    }

    /* Return everything after the last separator */
    return rt_arena_strdup(arena, last_sep + 1);
}

/* Extract file extension (without dot) from a path */
char *rt_path_extension(RtArena *arena, const char *path) {
    if (path == NULL || *path == '\0') {
        return rt_arena_strdup(arena, "");
    }

    /* Get just the filename part first */
    const char *last_sep = find_last_separator(path);
    const char *filename = last_sep ? last_sep + 1 : path;

    /* Find the last dot in the filename */
    const char *last_dot = NULL;
    for (const char *p = filename; *p; p++) {
        if (*p == '.') {
            last_dot = p;
        }
    }

    /* No dot, or dot is at start (hidden file like .bashrc) */
    if (last_dot == NULL || last_dot == filename) {
        return rt_arena_strdup(arena, "");
    }

    /* Return extension without the dot */
    return rt_arena_strdup(arena, last_dot + 1);
}

/* Join two path components */
char *rt_path_join2(RtArena *arena, const char *path1, const char *path2) {
    if (path1 == NULL) path1 = "";
    if (path2 == NULL) path2 = "";

    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);

    /* If path2 is absolute, return it directly */
    if (len2 > 0 && is_path_separator(path2[0])) {
        return rt_arena_strdup(arena, path2);
    }
#ifdef _WIN32
    /* Check for Windows absolute path like C:\ */
    if (len2 > 2 && path2[1] == ':' && is_path_separator(path2[2])) {
        return rt_arena_strdup(arena, path2);
    }
#endif

    /* If path1 is empty, return path2 */
    if (len1 == 0) {
        return rt_arena_strdup(arena, path2);
    }

    /* Check if path1 already ends with separator */
    int has_trailing_sep = is_path_separator(path1[len1 - 1]);

    /* Allocate: path1 + optional separator + path2 + null */
    size_t result_len = len1 + (has_trailing_sep ? 0 : 1) + len2 + 1;
    char *result = rt_arena_alloc(arena, result_len);

    memcpy(result, path1, len1);
    size_t pos = len1;
    if (!has_trailing_sep) {
        result[pos++] = '/';  /* Always use forward slash for consistency */
    }
    memcpy(result + pos, path2, len2);
    result[pos + len2] = '\0';

    return result;
}

/* Join three path components */
char *rt_path_join3(RtArena *arena, const char *path1, const char *path2, const char *path3) {
    char *temp = rt_path_join2(arena, path1, path2);
    return rt_path_join2(arena, temp, path3);
}

/* Helper: Check if a path is absolute */
static int is_absolute_path(const char *path) {
    if (path == NULL || *path == '\0') return 0;

#ifdef _WIN32
    /* Windows: absolute if starts with drive letter (e.g., C:\) or UNC path (\\server) */
    if (strlen(path) >= 3 && path[1] == ':' && is_path_separator(path[2])) {
        return 1;  /* Drive letter path */
    }
    if (strlen(path) >= 2 && is_path_separator(path[0]) && is_path_separator(path[1])) {
        return 1;  /* UNC path */
    }
#endif
    /* Unix style: absolute if starts with / */
    return is_path_separator(path[0]);
}

/* Resolve a path to its absolute form */
char *rt_path_absolute(RtArena *arena, const char *path) {
    if (path == NULL || *path == '\0') {
        /* Empty path - return current working directory */
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            return rt_arena_strdup(arena, cwd);
        }
        /* Fallback if getcwd fails */
        return rt_arena_strdup(arena, ".");
    }

#ifdef _WIN32
    char resolved[_MAX_PATH];
    if (_fullpath(resolved, path, _MAX_PATH) != NULL) {
        return rt_arena_strdup(arena, resolved);
    }
#else
    char resolved[PATH_MAX];
    if (realpath(path, resolved) != NULL) {
        return rt_arena_strdup(arena, resolved);
    }
#endif

    /* realpath/_fullpath fails if path doesn't exist - try to resolve manually */
    if (is_absolute_path(path)) {
        /* Already absolute */
        return rt_arena_strdup(arena, path);
    }

    /* Prepend current working directory */
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return rt_path_join2(arena, cwd, path);
    }

    /* Fallback - return as-is */
    return rt_arena_strdup(arena, path);
}

/* Check if a path exists */
int rt_path_exists(const char *path) {
    if (path == NULL) return 0;
    struct stat st;
    return stat(path, &st) == 0;
}

/* Check if a path points to a regular file */
int rt_path_is_file(const char *path) {
    if (path == NULL) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

/* Check if a path points to a directory */
int rt_path_is_directory(const char *path) {
    if (path == NULL) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

/* ============================================================================
 * Directory Operations - Helper Functions
 * ============================================================================ */

/* Create a string array with metadata (similar to other array types) */
char **rt_create_string_array(RtArena *arena, size_t initial_capacity) {
    size_t capacity = initial_capacity > 4 ? initial_capacity : 4;
    RtArrayMetadata *meta = rt_arena_alloc(arena, sizeof(RtArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_create_string_array: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = 0;
    meta->capacity = capacity;
    return (char **)(meta + 1);
}

/* Push a string onto a string array */
char **rt_push_string_to_array(RtArena *arena, char **arr, const char *str) {
    RtArrayMetadata *meta = ((RtArrayMetadata *)arr) - 1;
    RtArena *alloc_arena = meta->arena ? meta->arena : arena;

    if ((size_t)meta->size >= meta->capacity) {
        /* Need to grow the array */
        size_t new_capacity = meta->capacity * 2;
        RtArrayMetadata *new_meta = rt_arena_alloc(alloc_arena, sizeof(RtArrayMetadata) + new_capacity * sizeof(char *));
        if (new_meta == NULL) {
            fprintf(stderr, "rt_push_string_to_array: allocation failed\n");
            exit(1);
        }
        new_meta->arena = alloc_arena;
        new_meta->size = meta->size;
        new_meta->capacity = new_capacity;
        char **new_arr = (char **)(new_meta + 1);
        memcpy(new_arr, arr, meta->size * sizeof(char *));
        arr = new_arr;
        meta = new_meta;
    }

    arr[meta->size] = rt_arena_strdup(alloc_arena, str);
    meta->size++;
    return arr;
}

/* ============================================================================
 * Directory Operations
 * ============================================================================ */

/* List files in a directory (non-recursive) */
char **rt_directory_list(RtArena *arena, const char *path) {
    if (path == NULL) {
        return rt_create_string_array(arena, 4);  /* Return empty array */
    }

    DIR *dir = opendir(path);
    if (dir == NULL) {
        /* Directory doesn't exist or can't be opened - return empty array */
        return rt_create_string_array(arena, 4);
    }

    char **result = rt_create_string_array(arena, 16);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        result = rt_push_string_to_array(arena, result, entry->d_name);
    }

    closedir(dir);
    return result;
}

/* Helper: join two paths with forward slash (for consistent cross-platform relative paths) */
static char *join_with_forward_slash(RtArena *arena, const char *prefix, const char *name) {
    size_t prefix_len = strlen(prefix);
    size_t name_len = strlen(name);
    char *result = rt_arena_alloc(arena, prefix_len + 1 + name_len + 1);
    memcpy(result, prefix, prefix_len);
    result[prefix_len] = '/';
    memcpy(result + prefix_len + 1, name, name_len);
    result[prefix_len + 1 + name_len] = '\0';
    return result;
}

/* Helper for recursive directory listing */
static char **list_recursive_helper(RtArena *arena, char **result, const char *base_path, const char *rel_prefix) {
    DIR *dir = opendir(base_path);
    if (dir == NULL) {
        return result;  /* Skip directories we can't open */
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path for stat check (use native separator) */
        char *full_path = rt_path_join2(arena, base_path, entry->d_name);

        /* Build relative path for result (always use '/' for cross-platform consistency) */
        char *rel_path;
        if (rel_prefix[0] == '\0') {
            rel_path = rt_arena_strdup(arena, entry->d_name);
        } else {
            rel_path = join_with_forward_slash(arena, rel_prefix, entry->d_name);
        }

        /* Add this entry */
        result = rt_push_string_to_array(arena, result, rel_path);

        /* If it's a directory, recurse */
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            result = list_recursive_helper(arena, result, full_path, rel_path);
        }
    }

    closedir(dir);
    return result;
}

/* List files in a directory recursively */
char **rt_directory_list_recursive(RtArena *arena, const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.listRecursive: path cannot be null\n");
        exit(1);
    }

    if (!rt_path_is_directory(path)) {
        fprintf(stderr, "Directory.listRecursive: '%s' is not a directory\n", path);
        exit(1);
    }

    char **result = rt_create_string_array(arena, 64);
    return list_recursive_helper(arena, result, path, "");
}

/* Helper: Create directory and all parents */
static int create_directory_recursive(const char *path) {
    if (path == NULL || *path == '\0') return 0;

    /* Check if it already exists */
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;  /* Already exists and is a directory */
        }
        return -1;  /* Exists but is not a directory */
    }

    /* Make a copy we can modify */
    size_t len = strlen(path);
    char *path_copy = malloc(len + 1);
    if (path_copy == NULL) return -1;
    strcpy(path_copy, path);

    /* Create parent directories first */
    char *p = path_copy;

#ifdef _WIN32
    /* Skip Windows drive letter (e.g., C:\) */
    if (len >= 3 && path_copy[1] == ':' && is_path_separator(path_copy[2])) {
        p = path_copy + 3;
    }
#endif

    /* Skip leading path separators for absolute paths */
    while (is_path_separator(*p)) p++;

    while (*p) {
        /* Find next path separator */
        while (*p && !is_path_separator(*p)) p++;

        if (is_path_separator(*p)) {
            char saved = *p;
            *p = '\0';
            if (path_copy[0] != '\0') {
                if (stat(path_copy, &st) != 0) {
                    if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
                        free(path_copy);
                        return -1;
                    }
                }
            }
            *p = saved;
            p++;
        }
    }

    /* Create final directory */
    int result = mkdir(path_copy, 0755);
    free(path_copy);

    if (result != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

/* Create a directory (including parents if needed) */
void rt_directory_create(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.create: path cannot be null\n");
        exit(1);
    }

    if (create_directory_recursive(path) != 0) {
        fprintf(stderr, "Directory.create: failed to create directory '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }
}

/* Delete an empty directory */
void rt_directory_delete(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.delete: path cannot be null\n");
        exit(1);
    }

    if (rmdir(path) != 0) {
        if (errno == ENOTEMPTY) {
            fprintf(stderr, "Directory.delete: directory '%s' is not empty\n", path);
        } else {
            fprintf(stderr, "Directory.delete: failed to delete directory '%s': %s\n",
                    path, strerror(errno));
        }
        exit(1);
    }
}

/* Helper: Recursively delete directory contents */
static int delete_recursive_helper(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    struct dirent *entry;
    int result = 0;

    while ((entry = readdir(dir)) != NULL && result == 0) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path using cross-platform separator */
        size_t path_len = strlen(path);
        size_t name_len = strlen(entry->d_name);
        /* Check if path already ends with a separator */
        int has_sep = (path_len > 0 && is_path_separator(path[path_len - 1]));
        char *full_path = malloc(path_len + (has_sep ? 0 : 1) + name_len + 1);
        if (full_path == NULL) {
            result = -1;
            break;
        }

        strcpy(full_path, path);
        if (!has_sep) {
            full_path[path_len] = PATH_SEPARATOR;
            strcpy(full_path + path_len + 1, entry->d_name);
        } else {
            strcpy(full_path + path_len, entry->d_name);
        }

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                /* Recursively delete subdirectory */
                result = delete_recursive_helper(full_path);
                if (result == 0) {
                    result = rmdir(full_path);
                }
            } else {
                /* Delete file */
                result = unlink(full_path);
            }
        }

        free(full_path);
    }

    closedir(dir);
    return result;
}

/* Delete a directory and all its contents recursively */
void rt_directory_delete_recursive(const char *path) {
    if (path == NULL) {
        fprintf(stderr, "Directory.deleteRecursive: path cannot be null\n");
        exit(1);
    }

    if (!rt_path_is_directory(path)) {
        fprintf(stderr, "Directory.deleteRecursive: '%s' is not a directory\n", path);
        exit(1);
    }

    /* First delete contents recursively */
    if (delete_recursive_helper(path) != 0) {
        fprintf(stderr, "Directory.deleteRecursive: failed to delete contents of '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }

    /* Then delete the directory itself */
    if (rmdir(path) != 0) {
        fprintf(stderr, "Directory.deleteRecursive: failed to delete directory '%s': %s\n",
                path, strerror(errno));
        exit(1);
    }
}
