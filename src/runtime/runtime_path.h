#ifndef RUNTIME_PATH_H
#define RUNTIME_PATH_H

#include "runtime_arena.h"

/* ============================================================================
 * Path Utilities
 * ============================================================================
 * Functions for manipulating file system paths in a cross-platform manner.
 * ============================================================================ */

/* Extract directory portion of a path */
char *rt_path_directory(RtArena *arena, const char *path);

/* Extract filename (with extension) from a path */
char *rt_path_filename(RtArena *arena, const char *path);

/* Extract file extension (without dot) from a path */
char *rt_path_extension(RtArena *arena, const char *path);

/* Join two path components */
char *rt_path_join2(RtArena *arena, const char *path1, const char *path2);

/* Join three path components */
char *rt_path_join3(RtArena *arena, const char *path1, const char *path2, const char *path3);

/* Resolve a path to its absolute form */
char *rt_path_absolute(RtArena *arena, const char *path);

/* Check if a path exists */
int rt_path_exists(const char *path);

/* Check if a path points to a regular file */
int rt_path_is_file(const char *path);

/* Check if a path points to a directory */
int rt_path_is_directory(const char *path);

/* ============================================================================
 * Directory Operations
 * ============================================================================
 * Functions for directory manipulation.
 * ============================================================================ */

/* List files in a directory (non-recursive) */
char **rt_directory_list(RtArena *arena, const char *path);

/* List files in a directory recursively */
char **rt_directory_list_recursive(RtArena *arena, const char *path);

/* Create a directory */
void rt_directory_create(const char *path);

/* Delete an empty directory */
void rt_directory_delete(const char *path);

/* Delete a directory and all its contents */
void rt_directory_delete_recursive(const char *path);

/* ============================================================================
 * Directory Helper Functions
 * ============================================================================
 * Used internally for building string arrays during directory operations.
 * ============================================================================ */

/* Create a string array with metadata */
char **rt_create_string_array(RtArena *arena, size_t initial_capacity);

/* Push a string onto a string array */
char **rt_push_string_to_array(RtArena *arena, char **arr, const char *str);

#endif /* RUNTIME_PATH_H */
