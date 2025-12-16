#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "runtime.h"

static const char *null_str = "(null)";

/* ============================================================================
 * Arena Memory Management Implementation
 * ============================================================================ */

/* Allocate a new arena block */
static RtArenaBlock *rt_arena_new_block(size_t size)
{
    RtArenaBlock *block = malloc(sizeof(RtArenaBlock) + size);
    if (block == NULL) {
        fprintf(stderr, "rt_arena_new_block: allocation failed\n");
        exit(1);
    }
    block->next = NULL;
    block->size = size;
    block->used = 0;
    return block;
}

/* Create a new arena with custom block size */
RtArena *rt_arena_create_sized(RtArena *parent, size_t block_size)
{
    if (block_size == 0) {
        block_size = RT_ARENA_DEFAULT_BLOCK_SIZE;
    }

    RtArena *arena = malloc(sizeof(RtArena));
    if (arena == NULL) {
        fprintf(stderr, "rt_arena_create_sized: allocation failed\n");
        exit(1);
    }

    arena->parent = parent;
    arena->default_block_size = block_size;
    arena->total_allocated = 0;

    /* Create initial block */
    arena->first = rt_arena_new_block(block_size);
    arena->current = arena->first;
    arena->total_allocated += sizeof(RtArenaBlock) + block_size;

    return arena;
}

/* Create a new arena with default block size */
RtArena *rt_arena_create(RtArena *parent)
{
    return rt_arena_create_sized(parent, RT_ARENA_DEFAULT_BLOCK_SIZE);
}

/* Align a pointer up to the given alignment */
static inline size_t rt_align_up(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/* Allocate aligned memory from arena */
void *rt_arena_alloc_aligned(RtArena *arena, size_t size, size_t alignment)
{
    if (arena == NULL || size == 0) {
        return NULL;
    }

    /* Ensure alignment is at least sizeof(void*) and a power of 2 */
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    RtArenaBlock *block = arena->current;

    /* Calculate the actual address where we'd allocate next */
    char *current_ptr = block->data + block->used;

    /* Calculate alignment padding needed for the actual memory address */
    size_t addr = (size_t)current_ptr;
    size_t aligned_addr = rt_align_up(addr, alignment);
    size_t padding = aligned_addr - addr;

    /* Calculate total space needed */
    size_t end_offset = block->used + padding + size;

    /* Check if allocation fits in current block */
    if (end_offset <= block->size) {
        block->used = end_offset;
        return (char *)aligned_addr;
    }

    /* Need a new block - calculate size needed */
    size_t needed = size + alignment;  /* Extra for alignment */
    size_t new_block_size = arena->default_block_size;
    if (needed > new_block_size) {
        new_block_size = needed;
    }

    /* Allocate new block */
    RtArenaBlock *new_block = rt_arena_new_block(new_block_size);
    arena->total_allocated += sizeof(RtArenaBlock) + new_block_size;

    /* Link new block */
    block->next = new_block;
    arena->current = new_block;

    /* Allocate from new block with proper alignment */
    current_ptr = new_block->data;
    addr = (size_t)current_ptr;
    aligned_addr = rt_align_up(addr, alignment);
    padding = aligned_addr - addr;

    new_block->used = padding + size;
    return (char *)aligned_addr;
}

/* Allocate memory from arena (uninitialized) */
void *rt_arena_alloc(RtArena *arena, size_t size)
{
    return rt_arena_alloc_aligned(arena, size, sizeof(void *));
}

/* Allocate zeroed memory from arena */
void *rt_arena_calloc(RtArena *arena, size_t count, size_t size)
{
    size_t total = count * size;
    void *ptr = rt_arena_alloc(arena, total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

/* Duplicate a string into arena */
char *rt_arena_strdup(RtArena *arena, const char *str)
{
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    char *copy = rt_arena_alloc(arena, len + 1);
    if (copy != NULL) {
        memcpy(copy, str, len + 1);
    }
    return copy;
}

/* Duplicate n bytes of a string into arena */
char *rt_arena_strndup(RtArena *arena, const char *str, size_t n)
{
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str);
    if (n < len) {
        len = n;
    }
    char *copy = rt_arena_alloc(arena, len + 1);
    if (copy != NULL) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

/* Destroy arena and free all memory */
void rt_arena_destroy(RtArena *arena)
{
    if (arena == NULL) {
        return;
    }

    /* Free all blocks */
    RtArenaBlock *block = arena->first;
    while (block != NULL) {
        RtArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    free(arena);
}

/* Reset arena for reuse (keeps first block, frees rest) */
void rt_arena_reset(RtArena *arena)
{
    if (arena == NULL) {
        return;
    }

    /* Free all blocks except the first */
    RtArenaBlock *block = arena->first->next;
    while (block != NULL) {
        RtArenaBlock *next = block->next;
        free(block);
        block = next;
    }

    /* Reset first block */
    arena->first->next = NULL;
    arena->first->used = 0;
    arena->current = arena->first;
    arena->total_allocated = sizeof(RtArenaBlock) + arena->first->size;
}

/* Copy data from one arena to another (for promotion) */
void *rt_arena_promote(RtArena *dest, const void *src, size_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return NULL;
    }
    void *copy = rt_arena_alloc(dest, size);
    if (copy != NULL) {
        memcpy(copy, src, size);
    }
    return copy;
}

/* Copy string from one arena to another (for promotion) */
char *rt_arena_promote_string(RtArena *dest, const char *src)
{
    return rt_arena_strdup(dest, src);
}

/* Get total bytes allocated by arena */
size_t rt_arena_total_allocated(RtArena *arena)
{
    if (arena == NULL) {
        return 0;
    }
    return arena->total_allocated;
}

char *rt_str_concat(RtArena *arena, const char *left, const char *right) {
    const char *l = left ? left : "";
    const char *r = right ? right : "";
    size_t left_len = strlen(l);
    size_t right_len = strlen(r);
    size_t new_len = left_len + right_len;
    if (new_len > (1UL << 30) - 1) {
        return NULL;
    }
    char *new_str = rt_arena_alloc(arena, new_len + 1);
    if (new_str == NULL) {
        return NULL;
    }
    memcpy(new_str, l, left_len);
    memcpy(new_str + left_len, r, right_len + 1);
    return new_str;
}

char *rt_to_string_long(RtArena *arena, long val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_double(RtArena *arena, double val)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.5f", val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_char(RtArena *arena, char val)
{
    char buf[2] = {val, '\0'};
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_bool(RtArena *arena, int val)
{
    return rt_arena_strdup(arena, val ? "true" : "false");
}

char *rt_to_string_string(RtArena *arena, const char *val)
{
    if (val == NULL) {
        return (char *)null_str;
    }
    return rt_arena_strdup(arena, val);
}

char *rt_to_string_void(RtArena *arena)
{
    return rt_arena_strdup(arena, "void");
}

char *rt_to_string_pointer(RtArena *arena, void *p)
{
    if (p == NULL) {
        return rt_arena_strdup(arena, "nil");
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", p);
    return rt_arena_strdup(arena, buf);
}

void rt_print_long(long val)
{
    printf("%ld", val);
}

void rt_print_double(double val)
{
    if (isnan(val))
    {
        printf("NaN");
    }
    else if (isinf(val))
    {
        if (val > 0)
        {
            printf("Inf");
        }
        else
        {
            printf("-Inf");
        }
    }
    else
    {
        printf("%.5f", val);
    }
}

void rt_print_char(long c)
{
    if (c < 0 || c > 255)
    {
        fprintf(stderr, "rt_print_char: invalid char value %ld (must be 0-255)\n", c);
        printf("?");
    }
    else
    {
        printf("%c", (int)c);
    }
}

void rt_print_string(const char *s)
{
    if (s == NULL)
    {
        printf("(null)");
    }
    else
    {
        printf("%s", s);
    }
}

void rt_print_bool(long b)
{
    printf("%s", b ? "true" : "false");
}

long rt_add_long(long a, long b)
{
    if ((b > 0 && a > LONG_MAX - b) || (b < 0 && a < LONG_MIN - b))
    {
        fprintf(stderr, "rt_add_long: overflow detected\n");
        exit(1);
    }
    return a + b;
}

long rt_sub_long(long a, long b)
{
    if ((b > 0 && a < LONG_MIN + b) || (b < 0 && a > LONG_MAX + b))
    {
        fprintf(stderr, "rt_sub_long: overflow detected\n");
        exit(1);
    }
    return a - b;
}

long rt_mul_long(long a, long b)
{
    if (a != 0 && b != 0 && ((a > LONG_MAX / b) || (a < LONG_MIN / b)))
    {
        fprintf(stderr, "rt_mul_long: overflow detected\n");
        exit(1);
    }
    return a * b;
}

long rt_div_long(long a, long b)
{
    if (b == 0)
    {
        fprintf(stderr, "Division by zero\n");
        exit(1);
    }
    if (a == LONG_MIN && b == -1)
    {
        fprintf(stderr, "rt_div_long: overflow detected (LONG_MIN / -1)\n");
        exit(1);
    }
    return a / b;
}

long rt_mod_long(long a, long b)
{
    if (b == 0)
    {
        fprintf(stderr, "Modulo by zero\n");
        exit(1);
    }
    if (a == LONG_MIN && b == -1)
    {
        fprintf(stderr, "rt_mod_long: overflow detected (LONG_MIN %% -1)\n");
        exit(1);
    }
    return a % b;
}

int rt_eq_long(long a, long b) { return a == b; }
int rt_ne_long(long a, long b) { return a != b; }
int rt_lt_long(long a, long b) { return a < b; }
int rt_le_long(long a, long b) { return a <= b; }
int rt_gt_long(long a, long b) { return a > b; }
int rt_ge_long(long a, long b) { return a >= b; }

double rt_add_double(double a, double b)
{
    double result = a + b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_add_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_sub_double(double a, double b)
{
    double result = a - b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_sub_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_mul_double(double a, double b)
{
    double result = a * b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_mul_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_div_double(double a, double b)
{
    if (b == 0.0)
    {
        fprintf(stderr, "Division by zero\n");
        exit(1);
    }
    double result = a / b;
    if (isinf(result) && !isinf(a) && b != 0.0)
    {
        fprintf(stderr, "rt_div_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

int rt_eq_double(double a, double b) { return a == b; }
int rt_ne_double(double a, double b) { return a != b; }
int rt_lt_double(double a, double b) { return a < b; }
int rt_le_double(double a, double b) { return a <= b; }
int rt_gt_double(double a, double b) { return a > b; }
int rt_ge_double(double a, double b) { return a >= b; }

long rt_neg_long(long a)
{
    if (a == LONG_MIN)
    {
        fprintf(stderr, "rt_neg_long: overflow detected (-LONG_MIN)\n");
        exit(1);
    }
    return -a;
}

double rt_neg_double(double a) { return -a; }

int rt_not_bool(int a)
{
    return !a;
}

long rt_post_inc_long(long *p)
{
    if (p == NULL)
    {
        fprintf(stderr, "rt_post_inc_long: NULL pointer\n");
        exit(1);
    }
    if (*p == LONG_MAX)
    {
        fprintf(stderr, "rt_post_inc_long: overflow detected\n");
        exit(1);
    }
    return (*p)++;
}

long rt_post_dec_long(long *p)
{
    if (p == NULL)
    {
        fprintf(stderr, "rt_post_dec_long: NULL pointer\n");
        exit(1);
    }
    if (*p == LONG_MIN)
    {
        fprintf(stderr, "rt_post_dec_long: overflow detected\n");
        exit(1);
    }
    return (*p)--;
}

int rt_eq_string(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

int rt_ne_string(const char *a, const char *b) {
    return strcmp(a, b) != 0;
}

int rt_lt_string(const char *a, const char *b) {
    return strcmp(a, b) < 0;
}

int rt_le_string(const char *a, const char *b) {
    return strcmp(a, b) <= 0;
}

int rt_gt_string(const char *a, const char *b) {
    return strcmp(a, b) > 0;
}

int rt_ge_string(const char *a, const char *b) {
    return strcmp(a, b) >= 0;
}

/* Array metadata structure - stored before array data */
typedef struct {
    size_t size;     /* Number of elements currently in the array */
    size_t capacity; /* Total allocated space for elements */
} ArrayMetadata;

/*
 * Macro to generate type-safe array push functions.
 * Uses arena allocation - when capacity is exceeded, allocates new array and copies.
 */
#define DEFINE_ARRAY_PUSH(suffix, elem_type, assign_expr)                      \
elem_type *rt_array_push_##suffix(RtArena *arena, elem_type *arr, elem_type element) { \
    ArrayMetadata *meta;                                                       \
    elem_type *new_arr;                                                        \
    size_t new_capacity;                                                       \
                                                                               \
    if (arr == NULL) {                                                         \
        meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + 4 * sizeof(elem_type)); \
        if (meta == NULL) {                                                    \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
        meta->size = 1;                                                        \
        meta->capacity = 4;                                                    \
        new_arr = (elem_type *)(meta + 1);                                     \
        new_arr[0] = assign_expr;                                              \
        return new_arr;                                                        \
    }                                                                          \
                                                                               \
    meta = ((ArrayMetadata *)arr) - 1;                                         \
                                                                               \
    if (meta->size >= meta->capacity) {                                        \
        new_capacity = meta->capacity * 2;                                     \
        if (new_capacity < meta->capacity) {                                   \
            fprintf(stderr, "rt_array_push_" #suffix ": capacity overflow\n"); \
            exit(1);                                                           \
        }                                                                      \
        ArrayMetadata *new_meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + new_capacity * sizeof(elem_type)); \
        if (new_meta == NULL) {                                                \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
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

/* String arrays need special handling for strdup */
char **rt_array_push_string(RtArena *arena, char **arr, const char *element) {
    ArrayMetadata *meta;
    char **new_arr;
    size_t new_capacity;

    if (arr == NULL) {
        meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + 4 * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        meta->size = 1;
        meta->capacity = 4;
        new_arr = (char **)(meta + 1);
        new_arr[0] = element ? rt_arena_strdup(arena, element) : NULL;
        return new_arr;
    }

    meta = ((ArrayMetadata *)arr) - 1;

    if (meta->size >= meta->capacity) {
        new_capacity = meta->capacity * 2;
        if (new_capacity < meta->capacity) {
            fprintf(stderr, "rt_array_push_string: capacity overflow\n");
            exit(1);
        }
        ArrayMetadata *new_meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + new_capacity * sizeof(char *));
        if (new_meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        new_meta->size = meta->size;
        new_meta->capacity = new_capacity;
        new_arr = (char **)(new_meta + 1);
        memcpy(new_arr, arr, meta->size * sizeof(char *));
        meta = new_meta;
    } else {
        new_arr = arr;
    }

    new_arr[meta->size] = element ? rt_arena_strdup(arena, element) : NULL;
    meta->size++;
    return new_arr;
}

/* Get array length - returns 0 for NULL arrays */
size_t rt_array_length(void *arr) {
    if (arr == NULL) {
        return 0;
    }
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    return meta->size;
}

/* Print array functions */
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

void rt_print_array_string(char **arr) {
    printf("[");
    if (arr != NULL) {
        size_t len = rt_array_length(arr);
        for (size_t i = 0; i < len; i++) {
            if (i > 0) printf(", ");
            printf("\"%s\"", arr[i] ? arr[i] : "(null)");
        }
    }
    printf("]");
}

/* Clear array - sets size to 0 but keeps capacity */
void rt_array_clear(void *arr) {
    if (arr == NULL) {
        return;
    }
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    meta->size = 0;
}

/* Pop functions - remove and return last element */
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

DEFINE_ARRAY_POP(long, long, 0)
DEFINE_ARRAY_POP(double, double, 0.0)
DEFINE_ARRAY_POP(char, char, '\0')
DEFINE_ARRAY_POP(bool, int, 0)

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

/* Concat functions - return a NEW array containing elements from both arrays (non-mutating) */
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

DEFINE_ARRAY_CONCAT(long, long)
DEFINE_ARRAY_CONCAT(double, double)
DEFINE_ARRAY_CONCAT(char, char)
DEFINE_ARRAY_CONCAT(bool, int)

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

/* Array slice functions - create a new array from a portion of the source
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
    meta->size = slice_len;                                                     \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < slice_len; i++) {                                    \
        new_arr[i] = arr[actual_start + i * (size_t)actual_step];               \
    }                                                                           \
    return new_arr;                                                             \
}

DEFINE_ARRAY_SLICE(long, long)
DEFINE_ARRAY_SLICE(double, double)
DEFINE_ARRAY_SLICE(char, char)
DEFINE_ARRAY_SLICE(bool, int)

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
    meta->size = slice_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < slice_len; i++) {
        size_t src_idx = actual_start + i * (size_t)actual_step;
        new_arr[i] = arr[src_idx] ? rt_arena_strdup(arena, arr[src_idx]) : NULL;
    }
    return new_arr;
}

/* Array reverse functions - return a new reversed array */
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
    meta->size = len;                                                           \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len; i++) {                                          \
        new_arr[i] = arr[len - 1 - i];                                          \
    }                                                                           \
    return new_arr;                                                             \
}

DEFINE_ARRAY_REV(long, long)
DEFINE_ARRAY_REV(double, double)
DEFINE_ARRAY_REV(char, char)
DEFINE_ARRAY_REV(bool, int)

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
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[len - 1 - i] ? rt_arena_strdup(arena, arr[len - 1 - i]) : NULL;
    }
    return new_arr;
}

/* Array remove at index functions - return a new array without the element */
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

DEFINE_ARRAY_REM(long, long)
DEFINE_ARRAY_REM(double, double)
DEFINE_ARRAY_REM(char, char)
DEFINE_ARRAY_REM(bool, int)

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

/* Array insert at index functions - return a new array with the element inserted */
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

DEFINE_ARRAY_INS(long, long)
DEFINE_ARRAY_INS(double, double)
DEFINE_ARRAY_INS(char, char)
DEFINE_ARRAY_INS(bool, int)

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

/* Array push (copy) functions - create a NEW array with element appended */
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
    meta->size = new_len;                                                       \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    for (size_t i = 0; i < len; i++) {                                          \
        new_arr[i] = arr[i];                                                    \
    }                                                                           \
    new_arr[len] = elem;                                                        \
    return new_arr;                                                             \
}

DEFINE_ARRAY_PUSH_COPY(long, long)
DEFINE_ARRAY_PUSH_COPY(double, double)
DEFINE_ARRAY_PUSH_COPY(char, char)
DEFINE_ARRAY_PUSH_COPY(bool, int)

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
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    new_arr[len] = elem ? rt_arena_strdup(arena, elem) : NULL;
    return new_arr;
}

/* Array indexOf functions - find first index of element, returns -1 if not found */
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

DEFINE_ARRAY_INDEXOF(long, long, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(double, double, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(char, char, arr[i] == elem)
DEFINE_ARRAY_INDEXOF(bool, int, arr[i] == elem)

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

/* Array contains functions - check if element exists */
#define DEFINE_ARRAY_CONTAINS(suffix, elem_type)                                \
int rt_array_contains_##suffix(elem_type *arr, elem_type elem) {                \
    return rt_array_indexOf_##suffix(arr, elem) >= 0;                           \
}

DEFINE_ARRAY_CONTAINS(long, long)
DEFINE_ARRAY_CONTAINS(double, double)
DEFINE_ARRAY_CONTAINS(char, char)
DEFINE_ARRAY_CONTAINS(bool, int)

int rt_array_contains_string(char **arr, const char *elem) {
    return rt_array_indexOf_string(arr, elem) >= 0;
}

/* Array clone functions - create a deep copy of the array */
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
    meta->size = len;                                                           \
    meta->capacity = capacity;                                                  \
    elem_type *new_arr = (elem_type *)(meta + 1);                               \
    memcpy(new_arr, arr, len * sizeof(elem_type));                              \
    return new_arr;                                                             \
}

DEFINE_ARRAY_CLONE(long, long)
DEFINE_ARRAY_CLONE(double, double)
DEFINE_ARRAY_CLONE(char, char)
DEFINE_ARRAY_CLONE(bool, int)

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
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? rt_arena_strdup(arena, arr[i]) : NULL;
    }
    return new_arr;
}

/* Array join function - join elements into a string with separator */
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

/* Array create functions - create runtime array from static C array */
#define DEFINE_ARRAY_CREATE(suffix, elem_type)                                  \
elem_type *rt_array_create_##suffix(RtArena *arena, size_t count, const elem_type *data) { \
    if (count == 0 || data == NULL) {                                           \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = count > 4 ? count : 4;                                    \
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
    if (meta == NULL) {                                                         \
        fprintf(stderr, "rt_array_create_" #suffix ": allocation failed\n");    \
        exit(1);                                                                \
    }                                                                           \
    meta->size = count;                                                         \
    meta->capacity = capacity;                                                  \
    elem_type *arr = (elem_type *)(meta + 1);                                   \
    for (size_t i = 0; i < count; i++) {                                        \
        arr[i] = data[i];                                                       \
    }                                                                           \
    return arr;                                                                 \
}

DEFINE_ARRAY_CREATE(long, long)
DEFINE_ARRAY_CREATE(double, double)
DEFINE_ARRAY_CREATE(char, char)
DEFINE_ARRAY_CREATE(bool, int)

/* String array create needs special handling for strdup */
char **rt_array_create_string(RtArena *arena, size_t count, const char **data) {
    if (count == 0 || data == NULL) {
        return NULL;
    }
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_string: allocation failed\n");
        exit(1);
    }
    meta->size = count;
    meta->capacity = capacity;
    char **arr = (char **)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        arr[i] = data[i] ? rt_arena_strdup(arena, data[i]) : NULL;
    }
    return arr;
}

/* Array equality functions - compare arrays element by element */
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

DEFINE_ARRAY_EQ(long, long, a[i] == b[i])
DEFINE_ARRAY_EQ(double, double, a[i] == b[i])
DEFINE_ARRAY_EQ(char, char, a[i] == b[i])
DEFINE_ARRAY_EQ(bool, int, a[i] == b[i])

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

/* Range creation - creates int[] from start to end (exclusive) */
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
    meta->size = (size_t)count;
    meta->capacity = capacity;
    long *arr = (long *)(meta + 1);

    for (long i = 0; i < count; i++) {
        arr[i] = start + i;
    }

    return arr;
}

/* ============================================================
   String Manipulation Functions
   ============================================================ */

/* Get length of string */
long rt_str_length(const char *str) {
    if (str == NULL) return 0;
    return (long)strlen(str);
}

/* Get substring from start to end (exclusive) */
char *rt_str_substring(RtArena *arena, const char *str, long start, long end) {
    if (str == NULL) return rt_arena_strdup(arena, "");
    long len = (long)strlen(str);

    /* Handle negative indices */
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;

    /* Clamp to valid range */
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end || start >= len) return rt_arena_strdup(arena, "");

    long sub_len = end - start;
    char *result = rt_arena_alloc(arena, sub_len + 1);
    if (result == NULL) return rt_arena_strdup(arena, "");

    memcpy(result, str + start, sub_len);
    result[sub_len] = '\0';
    return result;
}

/* Find index of substring, returns -1 if not found */
long rt_str_indexOf(const char *str, const char *search) {
    if (str == NULL || search == NULL) return -1;
    const char *pos = strstr(str, search);
    if (pos == NULL) return -1;
    return (long)(pos - str);
}

/* Split string by delimiter, returns array of strings */
char **rt_str_split(RtArena *arena, const char *str, const char *delimiter) {
    if (str == NULL || delimiter == NULL) {
        return NULL;
    }

    size_t delim_len = strlen(delimiter);
    if (delim_len == 0) {
        /* Empty delimiter: split into individual characters */
        size_t len = strlen(str);
        if (len == 0) return NULL;

        /* Allocate the result array directly */
        size_t capacity = len > 4 ? len : 4;
        ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_str_split: allocation failed\n");
            exit(1);
        }
        meta->size = len;
        meta->capacity = capacity;
        char **result = (char **)(meta + 1);

        for (size_t i = 0; i < len; i++) {
            char *ch = rt_arena_alloc(arena, 2);
            ch[0] = str[i];
            ch[1] = '\0';
            result[i] = ch;
        }
        return result;
    }

    /* Count the number of parts */
    size_t count = 1;
    const char *p = str;
    while ((p = strstr(p, delimiter)) != NULL) {
        count++;
        p += delim_len;
    }

    /* Allocate the result array */
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = rt_arena_alloc(arena, sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_str_split: allocation failed\n");
        exit(1);
    }
    meta->size = count;
    meta->capacity = capacity;
    char **result = (char **)(meta + 1);

    /* Split the string */
    const char *start = str;
    size_t idx = 0;
    p = str;
    while ((p = strstr(p, delimiter)) != NULL) {
        size_t part_len = p - start;
        char *part = rt_arena_alloc(arena, part_len + 1);
        memcpy(part, start, part_len);
        part[part_len] = '\0';
        result[idx++] = part;
        p += delim_len;
        start = p;
    }
    /* Add final part */
    result[idx] = rt_arena_strdup(arena, start);

    return result;
}

/* Trim whitespace from both ends */
char *rt_str_trim(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    /* Skip leading whitespace */
    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) {
        str++;
    }

    if (*str == '\0') return rt_arena_strdup(arena, "");

    /* Find end, skipping trailing whitespace */
    const char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }

    size_t len = end - str + 1;
    char *result = rt_arena_alloc(arena, len + 1);
    if (result == NULL) return rt_arena_strdup(arena, "");

    memcpy(result, str, len);
    result[len] = '\0';
    return result;
}

/* Convert to uppercase */
char *rt_str_toUpper(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    char *result = rt_arena_strdup(arena, str);
    if (result == NULL) return rt_arena_strdup(arena, "");

    for (char *p = result; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 'a' + 'A';
        }
    }
    return result;
}

/* Convert to lowercase */
char *rt_str_toLower(RtArena *arena, const char *str) {
    if (str == NULL) return rt_arena_strdup(arena, "");

    char *result = rt_arena_strdup(arena, str);
    if (result == NULL) return rt_arena_strdup(arena, "");

    for (char *p = result; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            *p = *p - 'A' + 'a';
        }
    }
    return result;
}

/* Check if string starts with prefix */
int rt_str_startsWith(const char *str, const char *prefix) {
    if (str == NULL || prefix == NULL) return 0;
    size_t prefix_len = strlen(prefix);
    if (strlen(str) < prefix_len) return 0;
    return strncmp(str, prefix, prefix_len) == 0;
}

/* Check if string ends with suffix */
int rt_str_endsWith(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) return 0;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/* Check if string contains substring */
int rt_str_contains(const char *str, const char *search) {
    if (str == NULL || search == NULL) return 0;
    return strstr(str, search) != NULL;
}

/* Replace all occurrences of old with new */
char *rt_str_replace(RtArena *arena, const char *str, const char *old, const char *new_str) {
    if (str == NULL || old == NULL || new_str == NULL) return rt_arena_strdup(arena, str ? str : "");

    size_t old_len = strlen(old);
    if (old_len == 0) return rt_arena_strdup(arena, str);

    size_t new_len = strlen(new_str);

    /* Count occurrences */
    size_t count = 0;
    const char *p = str;
    while ((p = strstr(p, old)) != NULL) {
        count++;
        p += old_len;
    }

    if (count == 0) return rt_arena_strdup(arena, str);

    /* Calculate new length */
    size_t str_len = strlen(str);
    size_t result_len = str_len + count * (new_len - old_len);

    char *result = rt_arena_alloc(arena, result_len + 1);
    if (result == NULL) return rt_arena_strdup(arena, str);

    /* Build result */
    char *dst = result;
    p = str;
    const char *found;
    while ((found = strstr(p, old)) != NULL) {
        size_t prefix_len = found - p;
        memcpy(dst, p, prefix_len);
        dst += prefix_len;
        memcpy(dst, new_str, new_len);
        dst += new_len;
        p = found + old_len;
    }
    /* Copy remainder */
    strcpy(dst, p);

    return result;
}

/* Get character at index (returns char cast to long for consistency) */
long rt_str_charAt(const char *str, long index) {
    if (str == NULL) return 0;
    long len = (long)strlen(str);

    /* Handle negative index */
    if (index < 0) index = len + index;

    if (index < 0 || index >= len) return 0;
    return (long)(unsigned char)str[index];
}