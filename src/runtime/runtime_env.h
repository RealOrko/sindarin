#ifndef RUNTIME_ENV_H
#define RUNTIME_ENV_H

#include "runtime_arena.h"

/* ============================================================================
 * Environment Variables
 * ============================================================================
 * Sindarin provides the Environment type for accessing process environment
 * variables. All methods are static - there are no Environment instances.
 *
 * Two main usage patterns:
 * 1. get/getInt/etc. without default - throws if variable not set
 * 2. get/getInt/etc. with default - returns default if variable not set
 *
 * Type conversion errors always throw, even with defaults. Defaults only
 * apply when the variable is not set, not when it has an invalid value.
 * ============================================================================ */

/* ============================================================================
 * Basic Get/Set/Remove Functions
 * ============================================================================ */

/* Get environment variable value.
 * Returns NULL if variable is not set.
 * The returned string is copied into the arena. */
char *rt_env_get(RtArena *arena, const char *name);

/* Get environment variable with default value.
 * Returns default_value (copied to arena) if variable is not set.
 * If variable is set, returns its value (copied to arena). */
char *rt_env_get_default(RtArena *arena, const char *name, const char *default_value);

/* Set environment variable.
 * Returns 1 on success, 0 on failure. */
int rt_env_set(const char *name, const char *value);

/* Remove environment variable.
 * Returns 1 if variable existed and was removed, 0 otherwise. */
int rt_env_remove(const char *name);

/* Check if environment variable exists.
 * Returns 1 if set (even if empty), 0 if not set. */
int rt_env_has(const char *name);

/* ============================================================================
 * Typed Getter Functions (with validation)
 * ============================================================================
 * These functions parse the environment variable value as the specified type.
 * On parse failure, they print an error and exit (runtime error behavior).
 * ============================================================================ */

/* Get environment variable as int.
 * Returns 0 and sets *success=0 if not set or invalid.
 * Sets *success=1 on success. */
long rt_env_get_int(const char *name, int *success);

/* Get environment variable as int with default.
 * Returns default_value if not set.
 * Throws (exits) if set but invalid format. */
long rt_env_get_int_default(const char *name, long default_value);

/* Get environment variable as long.
 * Returns 0 and sets *success=0 if not set or invalid.
 * Sets *success=1 on success. */
long long rt_env_get_long(const char *name, int *success);

/* Get environment variable as long with default.
 * Returns default_value if not set.
 * Throws (exits) if set but invalid format. */
long long rt_env_get_long_default(const char *name, long long default_value);

/* Get environment variable as double.
 * Returns 0.0 and sets *success=0 if not set or invalid.
 * Sets *success=1 on success. */
double rt_env_get_double(const char *name, int *success);

/* Get environment variable as double with default.
 * Returns default_value if not set.
 * Throws (exits) if set but invalid format. */
double rt_env_get_double_default(const char *name, double default_value);

/* Get environment variable as bool.
 * Truthy values (case-insensitive): "true", "1", "yes", "on"
 * Falsy values (case-insensitive): "false", "0", "no", "off"
 * Returns 0 and sets *success=0 if not set or invalid.
 * Sets *success=1 on success. */
int rt_env_get_bool(const char *name, int *success);

/* Get environment variable as bool with default.
 * Returns default_value if not set.
 * Throws (exits) if set but invalid format. */
int rt_env_get_bool_default(const char *name, int default_value);

/* ============================================================================
 * Listing Functions
 * ============================================================================ */

/* Get all environment variables as array of [name, value] pairs.
 * Returns a str[][] where each entry is {name, value}.
 * The outer array and inner arrays are allocated in the arena.
 * Returns NULL on error. */
char ***rt_env_list(RtArena *arena);

/* Get all environment variable names.
 * Returns a str[] of all variable names.
 * The array is allocated in the arena.
 * Returns NULL on error. */
char **rt_env_names(RtArena *arena);

#endif /* RUNTIME_ENV_H */
