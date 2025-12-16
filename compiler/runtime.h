#ifndef RUNTIME_H
#define RUNTIME_H

#include <stddef.h>

/* String operations */
char *rt_str_concat(const char *left, const char *right);
void rt_free_string(char *s);

/* Print functions */
void rt_print_long(long val);
void rt_print_double(double val);
void rt_print_char(long c);
void rt_print_string(const char *s);
void rt_print_bool(long b);

/* Type conversion to string */
char *rt_to_string_long(long val);
char *rt_to_string_double(double val);
char *rt_to_string_char(char val);
char *rt_to_string_bool(int val);
char *rt_to_string_string(const char *val);
char *rt_to_string_void(void);
char *rt_to_string_pointer(void *p);

/* Long arithmetic (with overflow checking) */
long rt_add_long(long a, long b);
long rt_sub_long(long a, long b);
long rt_mul_long(long a, long b);
long rt_div_long(long a, long b);
long rt_mod_long(long a, long b);
long rt_neg_long(long a);

/* Long comparisons */
int rt_eq_long(long a, long b);
int rt_ne_long(long a, long b);
int rt_lt_long(long a, long b);
int rt_le_long(long a, long b);
int rt_gt_long(long a, long b);
int rt_ge_long(long a, long b);

/* Double arithmetic (with overflow checking) */
double rt_add_double(double a, double b);
double rt_sub_double(double a, double b);
double rt_mul_double(double a, double b);
double rt_div_double(double a, double b);
double rt_neg_double(double a);

/* Double comparisons */
int rt_eq_double(double a, double b);
int rt_ne_double(double a, double b);
int rt_lt_double(double a, double b);
int rt_le_double(double a, double b);
int rt_gt_double(double a, double b);
int rt_ge_double(double a, double b);

/* Boolean operations */
int rt_not_bool(int a);

/* Increment/decrement */
long rt_post_inc_long(long *p);
long rt_post_dec_long(long *p);

/* String comparisons */
int rt_eq_string(const char *a, const char *b);
int rt_ne_string(const char *a, const char *b);
int rt_lt_string(const char *a, const char *b);
int rt_le_string(const char *a, const char *b);
int rt_gt_string(const char *a, const char *b);
int rt_ge_string(const char *a, const char *b);

/* Array operations */
long *rt_array_push_long(long *arr, long element);
double *rt_array_push_double(double *arr, double element);
char *rt_array_push_char(char *arr, char element);
char **rt_array_push_string(char **arr, const char *element);
int *rt_array_push_bool(int *arr, int element);

/* Array length */
size_t rt_array_length(void *arr);

/* Array print functions */
void rt_print_array_long(long *arr);
void rt_print_array_double(double *arr);
void rt_print_array_char(char *arr);
void rt_print_array_bool(int *arr);
void rt_print_array_string(char **arr);

/* Array clear */
void rt_array_clear(void *arr);

/* Array pop functions */
long rt_array_pop_long(long *arr);
double rt_array_pop_double(double *arr);
char rt_array_pop_char(char *arr);
int rt_array_pop_bool(int *arr);
char *rt_array_pop_string(char **arr);

/* Array concat functions */
long *rt_array_concat_long(long *dest, long *src);
double *rt_array_concat_double(double *dest, double *src);
char *rt_array_concat_char(char *dest, char *src);
int *rt_array_concat_bool(int *dest, int *src);
char **rt_array_concat_string(char **dest, char **src);

#endif /* RUNTIME_H */
