#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "runtime_env.h"
#include "runtime_array.h"

/* ============================================================================
 * Platform-Specific Includes
 * ============================================================================ */

#ifdef _WIN32
#include <windows.h>
#else
/* POSIX systems */
#include <unistd.h>
extern char **environ;
#endif

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

/*
 * Case-insensitive string comparison.
 * Returns 1 if strings are equal (ignoring case), 0 otherwise.
 */
static int str_equals_ignore_case(const char *a, const char *b) {
    if (a == NULL || b == NULL) {
        return a == b;
    }
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/*
 * Parse a string as a boolean.
 * Truthy: "true", "1", "yes", "on"
 * Falsy: "false", "0", "no", "off"
 * Returns 1 for true, 0 for false, -1 for invalid.
 */
static int parse_bool(const char *value) {
    if (value == NULL) {
        return -1;
    }

    /* Truthy values */
    if (str_equals_ignore_case(value, "true") ||
        str_equals_ignore_case(value, "1") ||
        str_equals_ignore_case(value, "yes") ||
        str_equals_ignore_case(value, "on")) {
        return 1;
    }

    /* Falsy values */
    if (str_equals_ignore_case(value, "false") ||
        str_equals_ignore_case(value, "0") ||
        str_equals_ignore_case(value, "no") ||
        str_equals_ignore_case(value, "off")) {
        return 0;
    }

    return -1;  /* Invalid */
}

/*
 * Parse a string as a long integer.
 * Returns 1 on success (value stored in *result), 0 on failure.
 */
static int parse_long(const char *value, long *result) {
    if (value == NULL || *value == '\0') {
        return 0;
    }

    char *endptr;
    errno = 0;
    long val = strtol(value, &endptr, 10);

    /* Check for errors */
    if (errno == ERANGE || endptr == value || *endptr != '\0') {
        return 0;
    }

    *result = val;
    return 1;
}

/*
 * Parse a string as a long long integer.
 * Returns 1 on success (value stored in *result), 0 on failure.
 */
static int parse_long_long(const char *value, long long *result) {
    if (value == NULL || *value == '\0') {
        return 0;
    }

    char *endptr;
    errno = 0;
    long long val = strtoll(value, &endptr, 10);

    /* Check for errors */
    if (errno == ERANGE || endptr == value || *endptr != '\0') {
        return 0;
    }

    *result = val;
    return 1;
}

/*
 * Parse a string as a double.
 * Returns 1 on success (value stored in *result), 0 on failure.
 */
static int parse_double(const char *value, double *result) {
    if (value == NULL || *value == '\0') {
        return 0;
    }

    char *endptr;
    errno = 0;
    double val = strtod(value, &endptr);

    /* Check for errors */
    if (errno == ERANGE || endptr == value || *endptr != '\0') {
        return 0;
    }

    *result = val;
    return 1;
}

/* ============================================================================
 * Basic Get/Set/Remove Functions
 * ============================================================================ */

#ifdef _WIN32

/*
 * Windows implementation using GetEnvironmentVariable/SetEnvironmentVariable.
 */

char *rt_env_get(RtArena *arena, const char *name) {
    if (arena == NULL || name == NULL) {
        return NULL;
    }

    /* First call to get required buffer size */
    DWORD size = GetEnvironmentVariableA(name, NULL, 0);
    if (size == 0) {
        /* Variable not found or error */
        return NULL;
    }

    /* Allocate buffer in arena and get the value */
    char *buffer = rt_arena_alloc(arena, size);
    DWORD result = GetEnvironmentVariableA(name, buffer, size);

    /* result is the number of characters copied, not including null terminator.
     * For an empty string, result == 0 is valid (size would be 1 for just null).
     * For errors, result == 0 AND GetLastError() != 0. */
    if (result == 0 && size > 1) {
        /* Error - expected some characters but got none */
        return NULL;
    }
    if (result >= size) {
        /* Buffer too small (shouldn't happen since we sized it correctly) */
        return NULL;
    }

    return buffer;
}

int rt_env_set(const char *name, const char *value) {
    if (name == NULL) {
        return 0;
    }
    return SetEnvironmentVariableA(name, value) ? 1 : 0;
}

int rt_env_remove(const char *name) {
    if (name == NULL) {
        return 0;
    }

    /* Check if variable exists first */
    DWORD size = GetEnvironmentVariableA(name, NULL, 0);
    if (size == 0) {
        return 0;  /* Variable doesn't exist */
    }

    /* Remove by setting to NULL */
    return SetEnvironmentVariableA(name, NULL) ? 1 : 0;
}

int rt_env_has(const char *name) {
    if (name == NULL) {
        return 0;
    }
    DWORD size = GetEnvironmentVariableA(name, NULL, 0);
    return size > 0 ? 1 : 0;
}

char ***rt_env_list(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    /* Get the environment block */
    LPCH envStrings = GetEnvironmentStringsA();
    if (envStrings == NULL) {
        return NULL;
    }

    /* First pass: count entries */
    size_t count = 0;
    LPCH ptr = envStrings;
    while (*ptr) {
        /* Skip entries that start with '=' (Windows internal variables) */
        if (*ptr != '=') {
            count++;
        }
        ptr += strlen(ptr) + 1;
    }

    /* Create outer array to hold pairs */
    char ***result = rt_arena_alloc(arena, sizeof(RtArrayMetadata) + count * sizeof(char **));
    if (result == NULL) {
        FreeEnvironmentStringsA(envStrings);
        return NULL;
    }

    /* Set up array metadata */
    RtArrayMetadata *meta = (RtArrayMetadata *)result;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    result = (char ***)(meta + 1);

    /* Second pass: populate array */
    ptr = envStrings;
    size_t idx = 0;
    while (*ptr && idx < count) {
        if (*ptr != '=') {
            /* Find the '=' separator */
            char *eq = strchr(ptr, '=');
            if (eq != NULL) {
                size_t name_len = eq - ptr;

                /* Create pair array [name, value] */
                char **pair = rt_array_create_string(arena, 2, NULL);
                pair[0] = rt_arena_strndup(arena, ptr, name_len);
                pair[1] = rt_arena_strdup(arena, eq + 1);

                result[idx++] = pair;
            }
        }
        ptr += strlen(ptr) + 1;
    }

    FreeEnvironmentStringsA(envStrings);
    return result;
}

char **rt_env_names(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    /* Get the environment block */
    LPCH envStrings = GetEnvironmentStringsA();
    if (envStrings == NULL) {
        return NULL;
    }

    /* First pass: count entries */
    size_t count = 0;
    LPCH ptr = envStrings;
    while (*ptr) {
        if (*ptr != '=') {
            count++;
        }
        ptr += strlen(ptr) + 1;
    }

    /* Create array for names */
    char **result = rt_array_create_string(arena, count, NULL);
    if (result == NULL) {
        FreeEnvironmentStringsA(envStrings);
        return NULL;
    }

    /* Second pass: populate array */
    ptr = envStrings;
    size_t idx = 0;
    while (*ptr && idx < count) {
        if (*ptr != '=') {
            char *eq = strchr(ptr, '=');
            if (eq != NULL) {
                size_t name_len = eq - ptr;
                result[idx++] = rt_arena_strndup(arena, ptr, name_len);
            }
        }
        ptr += strlen(ptr) + 1;
    }

    FreeEnvironmentStringsA(envStrings);
    return result;
}

#else

/*
 * POSIX implementation using getenv/setenv/unsetenv and environ.
 */

char *rt_env_get(RtArena *arena, const char *name) {
    if (arena == NULL || name == NULL) {
        return NULL;
    }

    const char *value = getenv(name);
    if (value == NULL) {
        return NULL;
    }

    /* Copy to arena (getenv returns pointer to static storage) */
    return rt_arena_strdup(arena, value);
}

int rt_env_set(const char *name, const char *value) {
    if (name == NULL) {
        return 0;
    }
    /* setenv returns 0 on success, -1 on failure */
    return setenv(name, value != NULL ? value : "", 1) == 0 ? 1 : 0;
}

int rt_env_remove(const char *name) {
    if (name == NULL) {
        return 0;
    }

    /* Check if variable exists first */
    if (getenv(name) == NULL) {
        return 0;
    }

    /* unsetenv returns 0 on success */
    return unsetenv(name) == 0 ? 1 : 0;
}

int rt_env_has(const char *name) {
    if (name == NULL) {
        return 0;
    }
    return getenv(name) != NULL ? 1 : 0;
}

char ***rt_env_list(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    /* Count entries */
    size_t count = 0;
    for (char **e = environ; *e != NULL; e++) {
        count++;
    }

    /* Create outer array to hold pairs */
    char ***result = rt_arena_alloc(arena, sizeof(RtArrayMetadata) + count * sizeof(char **));
    if (result == NULL) {
        return NULL;
    }

    /* Set up array metadata */
    RtArrayMetadata *meta = (RtArrayMetadata *)result;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    result = (char ***)(meta + 1);

    /* Populate array */
    for (size_t i = 0; i < count; i++) {
        const char *entry = environ[i];
        const char *eq = strchr(entry, '=');

        if (eq != NULL) {
            size_t name_len = eq - entry;

            /* Create pair array [name, value] */
            char **pair = rt_array_create_string(arena, 2, NULL);
            pair[0] = rt_arena_strndup(arena, entry, name_len);
            pair[1] = rt_arena_strdup(arena, eq + 1);

            result[i] = pair;
        } else {
            /* Malformed entry (no '='), use empty value */
            char **pair = rt_array_create_string(arena, 2, NULL);
            pair[0] = rt_arena_strdup(arena, entry);
            pair[1] = rt_arena_strdup(arena, "");

            result[i] = pair;
        }
    }

    return result;
}

char **rt_env_names(RtArena *arena) {
    if (arena == NULL) {
        return NULL;
    }

    /* Count entries */
    size_t count = 0;
    for (char **e = environ; *e != NULL; e++) {
        count++;
    }

    /* Create array for names */
    char **result = rt_array_create_string(arena, count, NULL);
    if (result == NULL) {
        return NULL;
    }

    /* Populate array */
    for (size_t i = 0; i < count; i++) {
        const char *entry = environ[i];
        const char *eq = strchr(entry, '=');

        if (eq != NULL) {
            size_t name_len = eq - entry;
            result[i] = rt_arena_strndup(arena, entry, name_len);
        } else {
            /* Malformed entry (no '='), use whole string as name */
            result[i] = rt_arena_strdup(arena, entry);
        }
    }

    return result;
}

#endif

/* ============================================================================
 * Get with Default (Cross-Platform)
 * ============================================================================ */

char *rt_env_get_default(RtArena *arena, const char *name, const char *default_value) {
    if (arena == NULL || name == NULL) {
        return NULL;
    }

    char *value = rt_env_get(arena, name);
    if (value != NULL) {
        return value;
    }

    /* Variable not set, return default */
    if (default_value != NULL) {
        return rt_arena_strdup(arena, default_value);
    }
    return NULL;
}

/* ============================================================================
 * Typed Getter Functions
 * ============================================================================ */

long rt_env_get_int(const char *name, int *success) {
    *success = 0;

    if (name == NULL) {
        return 0;
    }

#ifdef _WIN32
    char buffer[64];
    DWORD result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (result == 0 || result >= sizeof(buffer)) {
        return 0;  /* Not set or too long */
    }
    const char *value = buffer;
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return 0;
    }
#endif

    long result_val;
    if (!parse_long(value, &result_val)) {
        return 0;
    }

    *success = 1;
    return result_val;
}

long rt_env_get_int_default(const char *name, long default_value) {
    if (name == NULL) {
        return default_value;
    }

#ifdef _WIN32
    char buffer[64];
    DWORD result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (result == 0) {
        return default_value;  /* Not set */
    }
    if (result >= sizeof(buffer)) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' value too long for integer\n", name);
        exit(1);
    }
    const char *value = buffer;
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return default_value;
    }
#endif

    long result_val;
    if (!parse_long(value, &result_val)) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' has invalid integer value: '%s'\n",
                name, value);
        exit(1);
    }

    return result_val;
}

long long rt_env_get_long(const char *name, int *success) {
    *success = 0;

    if (name == NULL) {
        return 0;
    }

#ifdef _WIN32
    char buffer[64];
    DWORD result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (result == 0 || result >= sizeof(buffer)) {
        return 0;
    }
    const char *value = buffer;
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return 0;
    }
#endif

    long long result_val;
    if (!parse_long_long(value, &result_val)) {
        return 0;
    }

    *success = 1;
    return result_val;
}

long long rt_env_get_long_default(const char *name, long long default_value) {
    if (name == NULL) {
        return default_value;
    }

#ifdef _WIN32
    char buffer[64];
    DWORD result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (result == 0) {
        return default_value;
    }
    if (result >= sizeof(buffer)) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' value too long for long\n", name);
        exit(1);
    }
    const char *value = buffer;
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return default_value;
    }
#endif

    long long result_val;
    if (!parse_long_long(value, &result_val)) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' has invalid long value: '%s'\n",
                name, value);
        exit(1);
    }

    return result_val;
}

double rt_env_get_double(const char *name, int *success) {
    *success = 0;

    if (name == NULL) {
        return 0.0;
    }

#ifdef _WIN32
    char buffer[128];
    DWORD result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (result == 0 || result >= sizeof(buffer)) {
        return 0.0;
    }
    const char *value = buffer;
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return 0.0;
    }
#endif

    double result_val;
    if (!parse_double(value, &result_val)) {
        return 0.0;
    }

    *success = 1;
    return result_val;
}

double rt_env_get_double_default(const char *name, double default_value) {
    if (name == NULL) {
        return default_value;
    }

#ifdef _WIN32
    char buffer[128];
    DWORD result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (result == 0) {
        return default_value;
    }
    if (result >= sizeof(buffer)) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' value too long for double\n", name);
        exit(1);
    }
    const char *value = buffer;
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return default_value;
    }
#endif

    double result_val;
    if (!parse_double(value, &result_val)) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' has invalid double value: '%s'\n",
                name, value);
        exit(1);
    }

    return result_val;
}

int rt_env_get_bool(const char *name, int *success) {
    *success = 0;

    if (name == NULL) {
        return 0;
    }

#ifdef _WIN32
    char buffer[64];
    DWORD result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (result == 0 || result >= sizeof(buffer)) {
        return 0;
    }
    const char *value = buffer;
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return 0;
    }
#endif

    int result_val = parse_bool(value);
    if (result_val < 0) {
        return 0;  /* Invalid bool format */
    }

    *success = 1;
    return result_val;
}

int rt_env_get_bool_default(const char *name, int default_value) {
    if (name == NULL) {
        return default_value;
    }

#ifdef _WIN32
    char buffer[64];
    DWORD result = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (result == 0) {
        return default_value;
    }
    if (result >= sizeof(buffer)) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' value too long for bool\n", name);
        exit(1);
    }
    const char *value = buffer;
#else
    const char *value = getenv(name);
    if (value == NULL) {
        return default_value;
    }
#endif

    int result_val = parse_bool(value);
    if (result_val < 0) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' has invalid boolean value: '%s'\n",
                name, value);
        fprintf(stderr, "Valid values: true, false, 1, 0, yes, no, on, off\n");
        exit(1);
    }

    return result_val;
}
