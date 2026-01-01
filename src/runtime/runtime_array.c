#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "runtime_array.h"
#include "runtime_arena.h"

/* ============================================================================
 * Array Operations Implementation
 * ============================================================================
 * This module provides array manipulation functions for the Sn runtime:
 * - Array creation and allocation
 * - Push/pop operations
 * - Array utilities (clear, length, etc.)
 * ============================================================================ */

/* Typedef for backward compatibility with existing code */
typedef RtArrayMetadata ArrayMetadata;

/* ============================================================================
 * Array Clear
 * ============================================================================ */

/* Clear all elements from an array (sets size to 0, keeps capacity) */
void rt_array_clear(void *arr) {
    if (arr == NULL) return;
    RtArrayMetadata *meta = &((RtArrayMetadata *)arr)[-1];
    meta->size = 0;
}

/* ============================================================================
 * Array Push Operations
 * ============================================================================
 * Push element to end of array, growing capacity if needed.
 * Uses arena allocation - when capacity is exceeded, allocates new array and copies.
 * The arena parameter is used only for NEW arrays; existing arrays use their stored arena.
 */
#define DEFINE_ARRAY_PUSH(suffix, elem_type, assign_expr)                      \
elem_type *rt_array_push_##suffix(RtArena *arena, elem_type *arr, elem_type element) { \
    ArrayMetadata *meta;                                                       \
    elem_type *new_arr;                                                        \
    size_t new_capacity;                                                       \
    RtArena *alloc_arena;                                                      \
                                                                               \
    if (arr == NULL) {                                                         \
        meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + 4 * sizeof(elem_type)); \
        if (meta == NULL) {                                                    \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
        meta->arena = arena;                                                   \
        meta->size = 1;                                                        \
        meta->capacity = 4;                                                    \
        new_arr = (elem_type *)(meta + 1);                                     \
        new_arr[0] = assign_expr;                                              \
        return new_arr;                                                        \
    }                                                                          \
                                                                               \
    meta = ((ArrayMetadata *)arr) - 1;                                         \
    alloc_arena = meta->arena ? meta->arena : arena;                           \
                                                                               \
    if (meta->size >= meta->capacity) {                                        \
        new_capacity = meta->capacity == 0 ? 4 : meta->capacity * 2;           \
        if (new_capacity < meta->capacity) {                                   \
            fprintf(stderr, "rt_array_push_" #suffix ": capacity overflow\n"); \
            exit(1);                                                           \
        }                                                                      \
        ArrayMetadata *new_meta = rt_arena_alloc(alloc_arena, sizeof(ArrayMetadata) + new_capacity * sizeof(elem_type)); \
        if (new_meta == NULL) {                                                \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
        new_meta->arena = alloc_arena;                                         \
        new_meta->size = meta->size;                                           \
        new_meta->capacity = new_capacity;                                     \
        new_arr = (elem_type *)(new_meta + 1);                                 \
        memcpy(new_arr, arr, meta->size * sizeof(elem_type));                  \
        meta = new_meta;                                                       \
    } else {                                                                   \
        new_arr = arr;                                                         \
    }                                                                          \
                                                                               \
    new_arr[meta->size] = assign_expr;                                         \
    meta->size++;                                                              \
    return new_arr;                                                            \
}

/* Generate array push functions for each type */
DEFINE_ARRAY_PUSH(long, long, element)
DEFINE_ARRAY_PUSH(double, double, element)
DEFINE_ARRAY_PUSH(char, char, element)
DEFINE_ARRAY_PUSH(bool, int, element)
DEFINE_ARRAY_PUSH(byte, unsigned char, element)
DEFINE_ARRAY_PUSH(ptr, void *, element)  /* For closures/function pointers and other pointer types */

/* String arrays need special handling for strdup */
char **rt_array_push_string(RtArena *arena, char **arr, const char *element) {
    ArrayMetadata *meta;
    char **new_arr;
    size_t new_capacity;
    RtArena *alloc_arena;

    if (arr == NULL) {
        meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + 4 * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = 1;
        meta->capacity = 4;
        new_arr = (char **)(meta + 1);
        new_arr[0] = element ? rt_arena_strdup(arena, element) : NULL;
        return new_arr;
    }

    meta = ((ArrayMetadata *)arr) - 1;
    alloc_arena = meta->arena ? meta->arena : arena;

    if (meta->size >= meta->capacity) {
        new_capacity = meta->capacity == 0 ? 4 : meta->capacity * 2;
        if (new_capacity < meta->capacity) {
            fprintf(stderr, "rt_array_push_string: capacity overflow\n");
            exit(1);
        }
        ArrayMetadata *new_meta = rt_arena_alloc(alloc_arena, sizeof(ArrayMetadata) + new_capacity * sizeof(char *));
        if (new_meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        new_meta->arena = alloc_arena;
        new_meta->size = meta->size;
        new_meta->capacity = new_capacity;
        new_arr = (char **)(new_meta + 1);
        memcpy(new_arr, arr, meta->size * sizeof(char *));
        meta = new_meta;
    } else {
        new_arr = arr;
    }

    new_arr[meta->size] = element ? rt_arena_strdup(alloc_arena, element) : NULL;
    meta->size++;
    return new_arr;
}

/* ============================================================================
 * Array Pop Operations
 * ============================================================================
 * Remove and return the last element of an array.
 * Decrements size but does not free memory.
 */
#define DEFINE_ARRAY_POP(suffix, elem_type, default_val)                       \
elem_type rt_array_pop_##suffix(elem_type *arr) {                              \
    if (arr == NULL) {                                                         \
        fprintf(stderr, "rt_array_pop_" #suffix ": NULL array\n");             \
        exit(1);                                                               \
    }                                                                          \
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;                          \
    if (meta->size == 0) {                                                     \
        fprintf(stderr, "rt_array_pop_" #suffix ": empty array\n");            \
        exit(1);                                                               \
    }                                                                          \
    meta->size--;                                                              \
    return arr[meta->size];                                                    \
}

/* Generate array pop functions for each type */
DEFINE_ARRAY_POP(long, long, 0)
DEFINE_ARRAY_POP(double, double, 0.0)
DEFINE_ARRAY_POP(char, char, '\0')
DEFINE_ARRAY_POP(bool, int, 0)
DEFINE_ARRAY_POP(byte, unsigned char, 0)
DEFINE_ARRAY_POP(ptr, void *, NULL)  /* For closures/function pointers and other pointer types */

/* String arrays - same implementation, no special handling needed for pop */
char *rt_array_pop_string(char **arr) {
    if (arr == NULL) {
        fprintf(stderr, "rt_array_pop_string: NULL array\n");
        exit(1);
    }
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    if (meta->size == 0) {
        fprintf(stderr, "rt_array_pop_string: empty array\n");
        exit(1);
    }
    meta->size--;
    return arr[meta->size];
}

/* ============================================================================
 * Array Concat Operations
 * ============================================================================
 * Return a NEW array containing elements from both arrays (non-mutating).
 * Both source arrays remain unchanged.
 */
#define DEFINE_ARRAY_CONCAT(suffix, elem_type)                                 \
elem_type *rt_array_concat_##suffix(RtArena *arena, elem_type *arr1, elem_type *arr2) { \
    size_t len1 = arr1 ? rt_array_length(arr1) : 0;                            \
    size_t len2 = arr2 ? rt_array_length(arr2) : 0;                            \
    size_t total = len1 + len2;                                                \
    size_t capacity = total > 4 ? total : 4;                                   \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                        \
        fprintf(stderr, "rt_array_concat_" #suffix ": allocation failed\n");   \
        exit(1);                                                               \
    }                                                                          \
    meta->arena = arena;                                                       \
    meta->size = total;                                                        \
    meta->capacity = capacity;                                                 \
    elem_type *result = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len1; i++) {                                        \
        result[i] = arr1[i];                                                   \
    }                                                                          \
    for (size_t i = 0; i < len2; i++) {                                        \
        result[len1 + i] = arr2[i];                                            \
    }                                                                          \
    return result;                                                             \
}

/* Generate array concat functions for each type */
DEFINE_ARRAY_CONCAT(long, long)
DEFINE_ARRAY_CONCAT(double, double)
DEFINE_ARRAY_CONCAT(char, char)
DEFINE_ARRAY_CONCAT(bool, int)
DEFINE_ARRAY_CONCAT(byte, unsigned char)
DEFINE_ARRAY_CONCAT(ptr, void *)  /* For closures/function pointers and other pointer types */

/* String arrays need special handling for strdup */
char **rt_array_concat_string(RtArena *arena, char **arr1, char **arr2) {
    size_t len1 = arr1 ? rt_array_length(arr1) : 0;
    size_t len2 = arr2 ? rt_array_length(arr2) : 0;
    size_t total = len1 + len2;
    size_t capacity = total > 4 ? total : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_concat_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = total;
    meta->capacity = capacity;
    char **result = (char **)(meta + 1);
    for (size_t i = 0; i < len1; i++) {
        result[i] = arr1[i] ? rt_arena_strdup(arena, arr1[i]) : NULL;
    }
    for (size_t i = 0; i < len2; i++) {
        result[len1 + i] = arr2[i] ? rt_arena_strdup(arena, arr2[i]) : NULL;
    }
    return result;
}

/* ============================================================================
 * Array Slice Operations
 * ============================================================================
 * Create a new array from a portion of the source array.
 * start: starting index (inclusive), use LONG_MIN for beginning
 * end: ending index (exclusive), use LONG_MIN for end of array
 * step: step size, use LONG_MIN for default step of 1
 */
#define DEFINE_ARRAY_SLICE(suffix, elem_type)                                   \
elem_type *rt_array_slice_##suffix(RtArena *arena, elem_type *arr, long start, long end, long step) { \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    /* Handle step: LONG_MIN means step of 1 */                                 \
    long actual_step = (step == LONG_MIN) ? 1 : step;                           \
    if (actual_step <= 0) {                                                     \
        fprintf(stderr, "rt_array_slice_" #suffix ": step must be positive\n"); \
        return NULL;                                                            \
    }                                                                           \
    /* Handle start: LONG_MIN means "from beginning", negative means from end */\
    long actual_start;                                                          \
    if (start == LONG_MIN) {                                                    \
        actual_start = 0;                                                       \
    } else if (start < 0) {                                                     \
        actual_start = (long)len + start;                                       \
        if (actual_start < 0) actual_start = 0;                                 \
    } else {                                                                    \
        actual_start = start;                                                   \
    }                                                                           \
    /* Handle end: LONG_MIN means "to end", negative means from end */          \
    long actual_end;                                                            \
    if (end == LONG_MIN) {                                                      \
        actual_end = (long)len;                                                 \
    } else if (end < 0) {                                                       \
        actual_end = (long)len + end;                                           \
        if (actual_end < 0) actual_end = 0;                                     \
    } else {                                                                    \
        actual_end = end;                                                       \
    }                                                                           \
    if (actual_start > (long)len) actual_start = (long)len;                     \
    if (actual_end > (long)len) actual_end = (long)len;                         \
    if (actual_start >= actual_end) {                                           \
        return NULL;                                                            \
    }                                                                           \
    /* Calculate slice length with step */                                      \
    size_t range = (size_t)(actual_end - actual_start);                         \
    size_t slice_len = (range + (size_t)actual_step - 1) / (size_t)actual_step; \
    size_t capacity = slice_len > 4 ? slice_len : 4;                            \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_slice_" #suffix ": allocation failed\n");     \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = slice_len;                                                     \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < slice_len; i++) {                                    \
        new_arr[i] = arr[actual_start + i * (size_t)actual_step];               \
    }                                                                           \
    return new_arr;                                                             \
}

/* Generate array slice functions for each type */
DEFINE_ARRAY_SLICE(long, long)
DEFINE_ARRAY_SLICE(double, double)
DEFINE_ARRAY_SLICE(char, char)
DEFINE_ARRAY_SLICE(bool, int)
DEFINE_ARRAY_SLICE(byte, unsigned char)

/* String slice needs special handling to strdup elements */
char **rt_array_slice_string(RtArena *arena, char **arr, long start, long end, long step) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    /* Handle step: LONG_MIN means step of 1 */
    long actual_step = (step == LONG_MIN) ? 1 : step;
    if (actual_step <= 0) {
        fprintf(stderr, "rt_array_slice_string: step must be positive\n");
        return NULL;
    }
    /* Handle start: LONG_MIN means "from beginning", negative means from end */
    long actual_start;
    if (start == LONG_MIN) {
        actual_start = 0;
    } else if (start < 0) {
        actual_start = (long)len + start;
        if (actual_start < 0) actual_start = 0;
    } else {
        actual_start = start;
    }
    /* Handle end: LONG_MIN means "to end", negative means from end */
    long actual_end;
    if (end == LONG_MIN) {
        actual_end = (long)len;
    } else if (end < 0) {
        actual_end = (long)len + end;
        if (actual_end < 0) actual_end = 0;
    } else {
        actual_end = end;
    }
    if (actual_start > (long)len) actual_start = (long)len;
    if (actual_end > (long)len) actual_end = (long)len;
    if (actual_start >= actual_end) {
        return NULL;
    }
    /* Calculate slice length with step */
    size_t range = (size_t)(actual_end - actual_start);
    size_t slice_len = (range + (size_t)actual_step - 1) / (size_t)actual_step;
    size_t capacity = slice_len > 4 ? slice_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_slice_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = slice_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < slice_len; i++) {
        size_t src_idx = actual_start + i * (size_t)actual_step;
        new_arr[i] = arr[src_idx] ? rt_arena_strdup(arena, arr[src_idx]) : NULL;
    }
    return new_arr;
}

/* ============================================================================
 * Array Reverse Functions
 * ============================================================================
 * Return a new reversed array - the original array is not modified.
 */
#define DEFINE_ARRAY_REV(suffix, elem_type)                                     \
elem_type *rt_array_rev_##suffix(RtArena *arena, elem_type *arr) {              \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (len == 0) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = len > 4 ? len : 4;                                        \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_rev_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = len;                                                           \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len; i++) {                                          \
        new_arr[i] = arr[len - 1 - i];                                          \
    }                                                                           \
    return new_arr;                                                             \
}

/* Generate array reverse functions for each type */
DEFINE_ARRAY_REV(long, long)
DEFINE_ARRAY_REV(double, double)
DEFINE_ARRAY_REV(char, char)
DEFINE_ARRAY_REV(bool, int)
DEFINE_ARRAY_REV(byte, unsigned char)

/* String reverse needs special handling to strdup elements */
char **rt_array_rev_string(RtArena *arena, char **arr) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (len == 0) {
        return NULL;
    }
    size_t capacity = len > 4 ? len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_rev_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[len - 1 - i] ? rt_arena_strdup(arena, arr[len - 1 - i]) : NULL;
    }
    return new_arr;
}

/* ============================================================================
 * Array Remove At Index Functions
 * ============================================================================
 * Return a new array without the element at the specified index.
 */
#define DEFINE_ARRAY_REM(suffix, elem_type)                                     \
elem_type *rt_array_rem_##suffix(RtArena *arena, elem_type *arr, long index) {  \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (index < 0 || (size_t)index >= len) {                                    \
        fprintf(stderr, "rt_array_rem_" #suffix ": index out of bounds\n");     \
        exit(1);                                                                \
    }                                                                           \
    if (len == 1) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t new_len = len - 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_rem_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < (size_t)index; i++) {                                \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    for (size_t i = (size_t)index; i < new_len; i++) {                          \
        new_arr[i] = arr[i + 1];                                                \
    }                                                                           \
    return new_arr;                                                             \
}

/* Generate array remove functions for each type */
DEFINE_ARRAY_REM(long, long)
DEFINE_ARRAY_REM(double, double)
DEFINE_ARRAY_REM(char, char)
DEFINE_ARRAY_REM(bool, int)
DEFINE_ARRAY_REM(byte, unsigned char)

/* String remove needs special handling to strdup elements */
char **rt_array_rem_string(RtArena *arena, char **arr, long index) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (index < 0 || (size_t)index >= len) {
        fprintf(stderr, "rt_array_rem_string: index out of bounds\n");
        exit(1);
    }
    if (len == 1) {
        return NULL;
    }
    size_t new_len = len - 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_rem_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < (size_t)index; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    for (size_t i = (size_t)index; i < new_len; i++) {
        new_arr[i] = arr[i + 1] ? rt_arena_strdup(arena, arr[i + 1]) : NULL;
    }
    return new_arr;
}

/* ============================================================================
 * Array Insert At Index Functions
 * ============================================================================
 * Return a new array with the element inserted at the specified index.
 */
#define DEFINE_ARRAY_INS(suffix, elem_type)                                     \
elem_type *rt_array_ins_##suffix(RtArena *arena, elem_type *arr, elem_type elem, long index) { \
    size_t len = arr ? rt_array_length(arr) : 0;                                \
    if (index < 0) index = 0;                                                   \
    if ((size_t)index > len) index = (long)len;                                 \
    size_t new_len = len + 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_ins_" #suffix ": allocation failed\n");       \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < (size_t)index; i++) {                                \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    new_arr[index] = elem;                                                      \
    for (size_t i = (size_t)index + 1; i < new_len; i++) {                      \
        new_arr[i] = arr[i - 1];                                                \
    }                                                                           \
    return new_arr;                                                             \
}

/* Generate array insert functions for each type */
DEFINE_ARRAY_INS(long, long)
DEFINE_ARRAY_INS(double, double)
DEFINE_ARRAY_INS(char, char)
DEFINE_ARRAY_INS(bool, int)
DEFINE_ARRAY_INS(byte, unsigned char)

/* String insert needs special handling to strdup elements */
char **rt_array_ins_string(RtArena *arena, char **arr, const char *elem, long index) {
    size_t len = arr ? rt_array_length(arr) : 0;
    if (index < 0) index = 0;
    if ((size_t)index > len) index = (long)len;
    size_t new_len = len + 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_ins_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < (size_t)index; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    new_arr[index] = elem ? rt_arena_strdup(arena, elem) : NULL;
    for (size_t i = (size_t)index + 1; i < new_len; i++) {
        new_arr[i] = arr[i - 1] ? rt_arena_strdup(arena, arr[i - 1]) : NULL;
    }
    return new_arr;
}

/* ============================================================================
 * Array IndexOf Functions
 * ============================================================================
 * Find first index of element, returns -1 if not found.
 */
#define DEFINE_ARRAY_INDEXOF(suffix, elem_type, compare_expr)                   \
long rt_array_indexOf_##suffix(elem_type *arr, elem_type elem) {                \
    if (arr == NULL) {                                                          \
        return -1L;                                                             \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    for (size_t i = 0; i < len; i++) {                                          \
        if (compare_expr) {                                                     \
            return (long)i;                                                     \
        }                                                                       \
    }                                                                           \
    return -1L;                                                                 \
}

/* Generate array indexOf functions for each type */
DEFINE_ARRAY_INDEXOF(long, long, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(double, double, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(char, char, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(bool, int, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(byte, unsigned char, arr[i] == elem)

/* String indexOf needs special comparison */
long rt_array_indexOf_string(char **arr, const char *elem) {
    if (arr == NULL) {
        return -1L;
    }
    size_t len = rt_array_length(arr);
    for (size_t i = 0; i < len; i++) {
        if (arr[i] == NULL && elem == NULL) {
            return (long)i;
        }
        if (arr[i] != NULL && elem != NULL && strcmp(arr[i], elem) == 0) {
            return (long)i;
        }
    }
    return -1L;
}

/* ============================================================================
 * Array Contains Functions
 * ============================================================================
 * Check if element exists in array.
 */
#define DEFINE_ARRAY_CONTAINS(suffix, elem_type)                                \
int rt_array_contains_##suffix(elem_type *arr, elem_type elem) {                \
    return rt_array_indexOf_##suffix(arr, elem) >= 0;                           \
}

/* Generate array contains functions for each type */
DEFINE_ARRAY_CONTAINS(long, long)
DEFINE_ARRAY_CONTAINS(double, double)
DEFINE_ARRAY_CONTAINS(char, char)
DEFINE_ARRAY_CONTAINS(bool, int)
DEFINE_ARRAY_CONTAINS(byte, unsigned char)

/* String contains uses string indexOf */
int rt_array_contains_string(char **arr, const char *elem) {
    return rt_array_indexOf_string(arr, elem) >= 0;
}

/* ============================================================================
 * Array Clone Functions
 * ============================================================================
 * Create a deep copy of the array.
 */
#define DEFINE_ARRAY_CLONE(suffix, elem_type)                                   \
elem_type *rt_array_clone_##suffix(RtArena *arena, elem_type *arr) {            \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (len == 0) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = len > 4 ? len : 4;                                        \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_clone_" #suffix ": allocation failed\n");     \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = len;                                                           \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    memcpy(new_arr, arr, len * sizeof(elem_type));                              \
    return new_arr;                                                             \
}

/* Generate array clone functions for each type */
DEFINE_ARRAY_CLONE(long, long)
DEFINE_ARRAY_CLONE(double, double)
DEFINE_ARRAY_CLONE(char, char)
DEFINE_ARRAY_CLONE(bool, int)
DEFINE_ARRAY_CLONE(byte, unsigned char)

/* String clone needs special handling to strdup elements */
char **rt_array_clone_string(RtArena *arena, char **arr) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (len == 0) {
        return NULL;
    }
    size_t capacity = len > 4 ? len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_clone_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    return new_arr;
}

/* ============================================================================
 * Array Join Functions
 * ============================================================================
 * Join array elements into a string with separator.
 */
char *rt_array_join_long(RtArena *arena, long *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each long can be up to 20 chars + separators */
    size_t buf_size = len * 24 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_long: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%ld", arr[i]);
    }
    return result;
}

char *rt_array_join_double(RtArena *arena, double *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_double: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%.5f", arr[i]);
    }
    return result;
}

char *rt_array_join_char(RtArena *arena, char *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_char: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        *ptr++ = arr[i];
    }
    *ptr = '\0';
    return result;
}

char *rt_array_join_bool(RtArena *arena, int *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "true" or "false" + separators */
    size_t buf_size = len * 6 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_bool: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "%s", arr[i] ? "true" : "false");
    }
    return result;
}

char *rt_array_join_byte(RtArena *arena, unsigned char *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "0xXX" (4 chars) + separators */
    size_t buf_size = len * 4 + (len - 1) * sep_len + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_byte: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            ptr += sprintf(ptr, "%s", separator);
        }
        ptr += sprintf(ptr, "0x%02X", arr[i]);
    }
    return result;
}

char *rt_array_join_string(RtArena *arena, char **arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return rt_arena_strdup(arena, "");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Calculate total length */
    size_t total_len = 0;
    for (size_t i = 0; i < len; i++) {
        if (arr[i]) {
            total_len += strlen(arr[i]);
        }
    }
    total_len += (len - 1) * sep_len + 1;

    char *result = rt_arena_alloc(arena, total_len);
    if (result == NULL) {
        fprintf(stderr, "rt_array_join_string: allocation failed\n");
        exit(1);
    }

    char *ptr = result;
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && separator) {
            size_t l = strlen(separator);
            memcpy(ptr, separator, l);
            ptr += l;
        }
        if (arr[i]) {
            size_t l = strlen(arr[i]);
            memcpy(ptr, arr[i], l);
            ptr += l;
        }
    }
    *ptr = '\0';
    return result;
}

/* ============================================================================
 * Array Print Functions
 * ============================================================================
 * Print array contents to stdout for debugging.
 */
void rt_print_array_long(long *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("%ld", arr[i]);
        }
    }
    printf("]");
}

void rt_print_array_double(double *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("%.5f", arr[i]);
        }
    }
    printf("]");
}

void rt_print_array_char(char *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("'%c'", arr[i]);
        }
    }
    printf("]");
}

void rt_print_array_bool(int *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("%s", arr[i] ? "true" : "false");
        }
    }
    printf("]");
}

void rt_print_array_byte(unsigned char *arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("0x%02X", arr[i]);
        }
    }
    printf("]");
}

void rt_print_array_string(char **arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            if (arr[i]) {
                printf("\"%s\"", arr[i]);
            } else {
                printf("null");
            }
        }
    }
    printf("]");
}

/* ============================================================================
 * Array Create Functions
 * ============================================================================
 * Create runtime array from static C array.
 */
#define DEFINE_ARRAY_CREATE(suffix, elem_type)                                  \
elem_type *rt_array_create_##suffix(RtArena *arena, size_t count, const elem_type *data) { \
    /* Always create array with metadata to track arena, even for empty arrays */ \
    size_t capacity = count > 4 ? count : 4;                                    \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_create_" #suffix ": allocation failed\n");    \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = count;                                                         \
    meta->capacity = capacity;                                                  \
    elem_type *arr = (elem_type *)(meta + 1);                                   \
    for (size_t i = 0; i < count; i++) {                                        \
        if (data != NULL) arr[i] = data[i];                                     \
    }                                                                           \
    return arr;                                                                 \
}

/* Generate array create functions for each type */
DEFINE_ARRAY_CREATE(long, long)
DEFINE_ARRAY_CREATE(double, double)
DEFINE_ARRAY_CREATE(char, char)
DEFINE_ARRAY_CREATE(bool, int)
DEFINE_ARRAY_CREATE(byte, unsigned char)

/* Create an uninitialized byte array for filling in later (e.g., file reads) */
unsigned char *rt_array_create_byte_uninit(RtArena *arena, size_t count) {
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(unsigned char));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_byte_uninit: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    unsigned char *arr = (unsigned char *)(meta + 1);
    /* Zero-initialize for safety */
    memset(arr, 0, count);
    return arr;
}

/* String array create needs special handling for strdup */
char **rt_array_create_string(RtArena *arena, size_t count, const char **data) {
    /* Always create array with metadata to track arena, even for empty arrays */
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = capacity;
    char **arr = (char **)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        arr[i] = (data && data[i]) ? rt_arena_strdup(arena, data[i]) : NULL;
    }
    return arr;
}

/* ============================================================================
 * Array Equality Functions
 * ============================================================================
 * Compare arrays element by element.
 */
#define DEFINE_ARRAY_EQ(suffix, elem_type, compare_expr)                        \
int rt_array_eq_##suffix(elem_type *a, elem_type *b) {                          \
    /* Both NULL means equal */                                                 \
    if (a == NULL && b == NULL) return 1;                                       \
    /* One NULL, one not means not equal */                                     \
    if (a == NULL || b == NULL) return 0;                                       \
    size_t len_a = rt_array_length(a);                                          \
    size_t len_b = rt_array_length(b);                                          \
    /* Different lengths means not equal */                                     \
    if (len_a != len_b) return 0;                                               \
    /* Compare element by element */                                            \
    for (size_t i = 0; i < len_a; i++) {                                        \
        if (!(compare_expr)) return 0;                                          \
    }                                                                           \
    return 1;                                                                   \
}

/* Generate array equality functions for each type */
DEFINE_ARRAY_EQ(long, long, a[i] == b[i])
DEFINE_ARRAY_EQ(double, double, a[i] == b[i])
DEFINE_ARRAY_EQ(char, char, a[i] == b[i])
DEFINE_ARRAY_EQ(bool, int, a[i] == b[i])
DEFINE_ARRAY_EQ(byte, unsigned char, a[i] == b[i])

/* String array equality needs strcmp */
int rt_array_eq_string(char **a, char **b) {
    /* Both NULL means equal */
    if (a == NULL && b == NULL) return 1;
    /* One NULL, one not means not equal */
    if (a == NULL || b == NULL) return 0;
    size_t len_a = rt_array_length(a);
    size_t len_b = rt_array_length(b);
    /* Different lengths means not equal */
    if (len_a != len_b) return 0;
    /* Compare element by element */
    for (size_t i = 0; i < len_a; i++) {
        /* Handle NULL strings */
        if (a[i] == NULL && b[i] == NULL) continue;
        if (a[i] == NULL || b[i] == NULL) return 0;
        if (strcmp(a[i], b[i]) != 0) return 0;
    }
    return 1;
}

/* ============================================================================
 * Array Range Function
 * ============================================================================
 * Creates long[] from start to end (exclusive).
 */
long *rt_array_range(RtArena *arena, long start, long end) {
    /* Calculate count, handle both ascending and descending ranges */
    long count;
    if (end >= start) {
        count = end - start;
    } else {
        /* Descending range: empty for now (future: could support negative step) */
        count = 0;
    }

    if (count <= 0) {
        /* Empty range */
        size_t capacity = 4;
        ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(long));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_range: allocation failed\n");
            exit(1);
        }
        meta->arena = arena;
        meta->size = 0;
        meta->capacity = capacity;
        return (long *)(meta + 1);
    }

    size_t capacity = (size_t)count > 4 ? (size_t)count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(long));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_range: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = (size_t)count;
    meta->capacity = capacity;
    long *arr = (long *)(meta + 1);

    for (long i = 0; i < count; i++) {
        arr[i] = start + i;
    }

    return arr;
}

/* ============================================================================
 * Array Alloc Functions
 * ============================================================================
 * Creates arrays of specified size filled with default value.
 */
long *rt_array_alloc_long(RtArena *arena, size_t count, long default_value) {
    size_t data_size = count * sizeof(long);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_long: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    long *data = (long *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

double *rt_array_alloc_double(RtArena *arena, size_t count, double default_value) {
    size_t data_size = count * sizeof(double);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_double: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    double *data = (double *)(meta + 1);
    if (default_value == 0.0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

char *rt_array_alloc_char(RtArena *arena, size_t count, char default_value) {
    size_t data_size = count * sizeof(char);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_char: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    char *data = (char *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

int *rt_array_alloc_bool(RtArena *arena, size_t count, int default_value) {
    size_t data_size = count * sizeof(int);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_bool: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    int *data = (int *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        for (size_t i = 0; i < count; i++) data[i] = default_value;
    }
    return data;
}

unsigned char *rt_array_alloc_byte(RtArena *arena, size_t count, unsigned char default_value) {
    size_t data_size = count * sizeof(unsigned char);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_byte: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    unsigned char *data = (unsigned char *)(meta + 1);
    if (default_value == 0) {
        memset(data, 0, data_size);
    } else {
        memset(data, default_value, data_size);  /* memset works perfectly for bytes */
    }
    return data;
}

char **rt_array_alloc_string(RtArena *arena, size_t count, const char *default_value) {
    size_t data_size = count * sizeof(char *);
    size_t total = sizeof(ArrayMetadata) + data_size;

    ArrayMetadata *meta = rt_arena_alloc(arena, total);
    if (meta == NULL) {
        fprintf(stderr, "rt_array_alloc_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    char **data = (char **)(meta + 1);
    if (default_value == NULL) {
        memset(data, 0, data_size);  /* Set all pointers to NULL */
    } else {
        for (size_t i = 0; i < count; i++) {
            data[i] = rt_arena_strdup(arena, default_value);
        }
    }
    return data;
}

/* ============================================================================
 * Array Push Copy Functions (Non-mutating)
 * ============================================================================
 * Create a NEW array with element appended - the original is not modified.
 */
#define DEFINE_ARRAY_PUSH_COPY(suffix, elem_type)                               \
elem_type *rt_array_push_copy_##suffix(RtArena *arena, elem_type *arr, elem_type elem) { \
    size_t len = arr ? rt_array_length(arr) : 0;                                \
    size_t new_len = len + 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_push_copy_" #suffix ": allocation failed\n"); \
        exit(1);                                                                \
    }                                                                           \
    meta->arena = arena;                                                        \
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len; i++) {                                          \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    new_arr[len] = elem;                                                        \
    return new_arr;                                                             \
}

/* Generate array push copy functions for each type */
DEFINE_ARRAY_PUSH_COPY(long, long)
DEFINE_ARRAY_PUSH_COPY(double, double)
DEFINE_ARRAY_PUSH_COPY(char, char)
DEFINE_ARRAY_PUSH_COPY(bool, int)
DEFINE_ARRAY_PUSH_COPY(byte, unsigned char)

/* String push copy needs special handling to strdup elements */
char **rt_array_push_copy_string(RtArena *arena, char **arr, const char *elem) {
    size_t len = arr ? rt_array_length(arr) : 0;
    size_t new_len = len + 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_push_copy_string: allocation failed\n");
        exit(1);
    }
    meta->arena = arena;
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    new_arr[len] = elem ? rt_arena_strdup(arena, elem) : NULL;
    return new_arr;
}
