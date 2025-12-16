#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

static const char *null_str = "(null)";

char *rt_str_concat(const char *left, const char *right) {
    const char *l = left ? left : "";
    const char *r = right ? right : "";
    size_t left_len = strlen(l);
    size_t right_len = strlen(r);
    size_t new_len = left_len + right_len;
    if (new_len > (1UL << 30) - 1) {
        return NULL;
    }
    char *new_str = malloc(new_len + 1);
    if (new_str == NULL) {
        return NULL;
    }
    memcpy(new_str, l, left_len);
    memcpy(new_str + left_len, r, right_len + 1);
    return new_str;
}

char *rt_to_string_long(long val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", val);
    return strdup(buf);
}

char *rt_to_string_double(double val)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.5f", val);
    return strdup(buf);
}

char *rt_to_string_char(char val)
{
    char buf[2] = {val, '\0'};
    return strdup(buf);
}

char *rt_to_string_bool(int val)
{
    return strdup(val ? "true" : "false");
}

char *rt_to_string_string(const char *val)
{
    if (val == NULL) {
        return (char *)null_str;
    }
    return strdup(val);
}

char *rt_to_string_void(void)
{
    return strdup("void");
}

char *rt_to_string_pointer(void *p)
{
    if (p == NULL) {
        return strdup("nil");
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", p);
    return strdup(buf);
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
 * Reduces code duplication across different element types.
 */
#define DEFINE_ARRAY_PUSH(suffix, elem_type, assign_expr)                      \
elem_type *rt_array_push_##suffix(elem_type *arr, elem_type element) {         \
    ArrayMetadata *meta;                                                       \
    elem_type *new_arr;                                                        \
    size_t new_capacity;                                                       \
                                                                               \
    if (arr == NULL) {                                                         \
        meta = malloc(sizeof(ArrayMetadata) + 4 * sizeof(elem_type));          \
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
        meta = realloc(meta, sizeof(ArrayMetadata) + new_capacity * sizeof(elem_type)); \
        if (meta == NULL) {                                                    \
            fprintf(stderr, "rt_array_push_" #suffix ": allocation failed\n"); \
            exit(1);                                                           \
        }                                                                      \
        meta->capacity = new_capacity;                                         \
        new_arr = (elem_type *)(meta + 1);                                     \
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
char **rt_array_push_string(char **arr, const char *element) {
    ArrayMetadata *meta;
    char **new_arr;
    size_t new_capacity;

    if (arr == NULL) {
        meta = malloc(sizeof(ArrayMetadata) + 4 * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        meta->size = 1;
        meta->capacity = 4;
        new_arr = (char **)(meta + 1);
        new_arr[0] = element ? strdup(element) : NULL;
        return new_arr;
    }

    meta = ((ArrayMetadata *)arr) - 1;

    if (meta->size >= meta->capacity) {
        new_capacity = meta->capacity * 2;
        if (new_capacity < meta->capacity) {
            fprintf(stderr, "rt_array_push_string: capacity overflow\n");
            exit(1);
        }
        meta = realloc(meta, sizeof(ArrayMetadata) + new_capacity * sizeof(char *));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push_string: allocation failed\n");
            exit(1);
        }
        meta->capacity = new_capacity;
        new_arr = (char **)(meta + 1);
    } else {
        new_arr = arr;
    }

    new_arr[meta->size] = element ? strdup(element) : NULL;
    meta->size++;
    return new_arr;
}

void rt_free_string(char *s) {
    if (s == NULL || s == null_str) {
        return;
    }
    free(s);
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

/* Concat functions - append all elements from src to dest, returns dest */
#define DEFINE_ARRAY_CONCAT(suffix, elem_type)                                 \
elem_type *rt_array_concat_##suffix(elem_type *dest, elem_type *src) {         \
    if (src == NULL) {                                                         \
        return dest;                                                           \
    }                                                                          \
    size_t src_len = rt_array_length(src);                                     \
    for (size_t i = 0; i < src_len; i++) {                                     \
        dest = rt_array_push_##suffix(dest, src[i]);                           \
    }                                                                          \
    return dest;                                                               \
}

DEFINE_ARRAY_CONCAT(long, long)
DEFINE_ARRAY_CONCAT(double, double)
DEFINE_ARRAY_CONCAT(char, char)
DEFINE_ARRAY_CONCAT(bool, int)

char **rt_array_concat_string(char **dest, char **src) {
    if (src == NULL) {
        return dest;
    }
    size_t src_len = rt_array_length(src);
    for (size_t i = 0; i < src_len; i++) {
        dest = rt_array_push_string(dest, src[i]);
    }
    return dest;
}

/* Free array functions */
void rt_array_free(void *arr) {
    if (arr == NULL) {
        return;
    }
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    free(meta);
}

void rt_array_free_string(char **arr) {
    if (arr == NULL) {
        return;
    }
    ArrayMetadata *meta = ((ArrayMetadata *)arr) - 1;
    /* Free each string element first */
    for (size_t i = 0; i < meta->size; i++) {
        if (arr[i] != NULL) {
            free(arr[i]);
        }
    }
    free(meta);
}

/* Array slice functions - create a new array from a portion of the source
 * start: starting index (inclusive), use LONG_MIN for beginning
 * end: ending index (exclusive), use LONG_MIN for end of array
 * step: step size, use LONG_MIN for default step of 1
 */
#define DEFINE_ARRAY_SLICE(suffix, elem_type)                                   \
elem_type *rt_array_slice_##suffix(elem_type *arr, long start, long end, long step) { \
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
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
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
char **rt_array_slice_string(char **arr, long start, long end, long step) {
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
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_slice_string: allocation failed\n");
        exit(1);
    }
    meta->size = slice_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < slice_len; i++) {
        size_t src_idx = actual_start + i * (size_t)actual_step;
        new_arr[i] = arr[src_idx] ? strdup(arr[src_idx]) : NULL;
    }
    return new_arr;
}

/* Array reverse functions - return a new reversed array */
#define DEFINE_ARRAY_REV(suffix, elem_type)                                     \
elem_type *rt_array_rev_##suffix(elem_type *arr) {                              \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (len == 0) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = len > 4 ? len : 4;                                        \
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
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
char **rt_array_rev_string(char **arr) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (len == 0) {
        return NULL;
    }
    size_t capacity = len > 4 ? len : 4;
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_rev_string: allocation failed\n");
        exit(1);
    }
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[len - 1 - i] ? strdup(arr[len - 1 - i]) : NULL;
    }
    return new_arr;
}

/* Array remove at index functions - return a new array without the element */
#define DEFINE_ARRAY_REM(suffix, elem_type)                                     \
elem_type *rt_array_rem_##suffix(elem_type *arr, long index) {                  \
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
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
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
char **rt_array_rem_string(char **arr, long index) {
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
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_rem_string: allocation failed\n");
        exit(1);
    }
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < (size_t)index; i++) {
        new_arr[i] = arr[i] ? strdup(arr[i]) : NULL;
    }
    for (size_t i = (size_t)index; i < new_len; i++) {
        new_arr[i] = arr[i + 1] ? strdup(arr[i + 1]) : NULL;
    }
    return new_arr;
}

/* Array insert at index functions - return a new array with the element inserted */
#define DEFINE_ARRAY_INS(suffix, elem_type)                                     \
elem_type *rt_array_ins_##suffix(elem_type *arr, elem_type elem, long index) {  \
    size_t len = arr ? rt_array_length(arr) : 0;                                \
    if (index < 0) index = 0;                                                   \
    if ((size_t)index > len) index = (long)len;                                 \
    size_t new_len = len + 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
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
char **rt_array_ins_string(char **arr, const char *elem, long index) {
    size_t len = arr ? rt_array_length(arr) : 0;
    if (index < 0) index = 0;
    if ((size_t)index > len) index = (long)len;
    size_t new_len = len + 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_ins_string: allocation failed\n");
        exit(1);
    }
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < (size_t)index; i++) {
        new_arr[i] = arr[i] ? strdup(arr[i]) : NULL;
    }
    new_arr[index] = elem ? strdup(elem) : NULL;
    for (size_t i = (size_t)index + 1; i < new_len; i++) {
        new_arr[i] = arr[i - 1] ? strdup(arr[i - 1]) : NULL;
    }
    return new_arr;
}

/* Array push (copy) functions - create a NEW array with element appended */
#define DEFINE_ARRAY_PUSH_COPY(suffix, elem_type)                               \
elem_type *rt_array_push_copy_##suffix(elem_type *arr, elem_type elem) {        \
    size_t len = arr ? rt_array_length(arr) : 0;                                \
    size_t new_len = len + 1;                                                   \
    size_t capacity = new_len > 4 ? new_len : 4;                                \
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
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
char **rt_array_push_copy_string(char **arr, const char *elem) {
    size_t len = arr ? rt_array_length(arr) : 0;
    size_t new_len = len + 1;
    size_t capacity = new_len > 4 ? new_len : 4;
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_push_copy_string: allocation failed\n");
        exit(1);
    }
    meta->size = new_len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? strdup(arr[i]) : NULL;
    }
    new_arr[len] = elem ? strdup(elem) : NULL;
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
elem_type *rt_array_clone_##suffix(elem_type *arr) {                            \
    if (arr == NULL) {                                                          \
        return NULL;                                                            \
    }                                                                           \
    size_t len = rt_array_length(arr);                                          \
    if (len == 0) {                                                             \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = len > 4 ? len : 4;                                        \
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
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
char **rt_array_clone_string(char **arr) {
    if (arr == NULL) {
        return NULL;
    }
    size_t len = rt_array_length(arr);
    if (len == 0) {
        return NULL;
    }
    size_t capacity = len > 4 ? len : 4;
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_clone_string: allocation failed\n");
        exit(1);
    }
    meta->size = len;
    meta->capacity = capacity;
    char **new_arr = (char **)(meta + 1);
    for (size_t i = 0; i < len; i++) {
        new_arr[i] = arr[i] ? strdup(arr[i]) : NULL;
    }
    return new_arr;
}

/* Array join function - join elements into a string with separator */
char *rt_array_join_long(long *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return strdup("");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* Estimate buffer size: each long can be up to 20 chars + separators */
    size_t buf_size = len * 24 + (len - 1) * sep_len + 1;
    char *result = malloc(buf_size);
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

char *rt_array_join_double(double *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return strdup("");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len * 32 + (len - 1) * sep_len + 1;
    char *result = malloc(buf_size);
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

char *rt_array_join_char(char *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return strdup("");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    size_t buf_size = len + (len - 1) * sep_len + 1;
    char *result = malloc(buf_size);
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

char *rt_array_join_bool(int *arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return strdup("");
    }
    size_t len = rt_array_length(arr);
    size_t sep_len = separator ? strlen(separator) : 0;

    /* "true" or "false" + separators */
    size_t buf_size = len * 6 + (len - 1) * sep_len + 1;
    char *result = malloc(buf_size);
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

char *rt_array_join_string(char **arr, const char *separator) {
    if (arr == NULL || rt_array_length(arr) == 0) {
        return strdup("");
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

    char *result = malloc(total_len);
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
elem_type *rt_array_create_##suffix(size_t count, const elem_type *data) {      \
    if (count == 0 || data == NULL) {                                           \
        return NULL;                                                            \
    }                                                                           \
    size_t capacity = count > 4 ? count : 4;                                    \
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(elem_type)); \
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
char **rt_array_create_string(size_t count, const char **data) {
    if (count == 0 || data == NULL) {
        return NULL;
    }
    size_t capacity = count > 4 ? count : 4;
    ArrayMetadata *meta = malloc(sizeof(ArrayMetadata) + capacity * sizeof(char *));
    if (meta == NULL) {
        fprintf(stderr, "rt_array_create_string: allocation failed\n");
        exit(1);
    }
    meta->size = count;
    meta->capacity = capacity;
    char **arr = (char **)(meta + 1);
    for (size_t i = 0; i < count; i++) {
        arr[i] = data[i] ? strdup(data[i]) : NULL;
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