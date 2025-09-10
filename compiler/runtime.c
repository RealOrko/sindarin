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

long *rt_array_push(long *arr, long element) {
    // Structure to store array metadata (size and capacity) before the array data
    struct array_metadata {
        size_t size;     // Number of elements currently in the array
        size_t capacity; // Total allocated space for elements
    };

    struct array_metadata *meta;
    long *new_arr;
    size_t new_capacity;

    if (arr == NULL) {
        // Initial case: create a new array with capacity 4
        meta = malloc(sizeof(struct array_metadata) + 4 * sizeof(long));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push: memory allocation failed\n");
            exit(1);
        }
        meta->size = 1;
        meta->capacity = 4;
        new_arr = (long *)(meta + 1);
        new_arr[0] = element;
        return new_arr;
    }

    // Get metadata (stored just before the array)
    meta = ((struct array_metadata *)arr) - 1;

    if (meta->size >= meta->capacity) {
        // Resize array: double the capacity
        new_capacity = meta->capacity * 2;
        if (new_capacity < meta->capacity) { // Check for overflow
            fprintf(stderr, "rt_array_push: capacity overflow\n");
            exit(1);
        }
        meta = realloc(meta, sizeof(struct array_metadata) + new_capacity * sizeof(long));
        if (meta == NULL) {
            fprintf(stderr, "rt_array_push: memory allocation failed\n");
            exit(1);
        }
        meta->capacity = new_capacity;
        new_arr = (long *)(meta + 1);
    } else {
        new_arr = arr;
    }

    // Add the new element
    new_arr[meta->size] = element;
    meta->size++;

    return new_arr;
}

void rt_free_string(char *s) {
    if (s == NULL || s == null_str) {
        return;
    }
    free(s);
}