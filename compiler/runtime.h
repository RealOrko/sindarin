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

/* Array slice functions - step defaults to 1 if LONG_MIN */
long *rt_array_slice_long(long *arr, long start, long end, long step);
double *rt_array_slice_double(double *arr, long start, long end, long step);
char *rt_array_slice_char(char *arr, long start, long end, long step);
int *rt_array_slice_bool(int *arr, long start, long end, long step);
char **rt_array_slice_string(char **arr, long start, long end, long step);

/* Array reverse functions */
long *rt_array_rev_long(long *arr);
double *rt_array_rev_double(double *arr);
char *rt_array_rev_char(char *arr);
int *rt_array_rev_bool(int *arr);
char **rt_array_rev_string(char **arr);

/* Array remove at index functions */
long *rt_array_rem_long(long *arr, long index);
double *rt_array_rem_double(double *arr, long index);
char *rt_array_rem_char(char *arr, long index);
int *rt_array_rem_bool(int *arr, long index);
char **rt_array_rem_string(char **arr, long index);

/* Array insert at index functions */
long *rt_array_ins_long(long *arr, long elem, long index);
double *rt_array_ins_double(double *arr, double elem, long index);
char *rt_array_ins_char(char *arr, char elem, long index);
int *rt_array_ins_bool(int *arr, int elem, long index);
char **rt_array_ins_string(char **arr, const char *elem, long index);

/* Array push (copy) functions - return NEW array with element appended */
long *rt_array_push_copy_long(long *arr, long elem);
double *rt_array_push_copy_double(double *arr, double elem);
char *rt_array_push_copy_char(char *arr, char elem);
int *rt_array_push_copy_bool(int *arr, int elem);
char **rt_array_push_copy_string(char **arr, const char *elem);

/* Array indexOf functions - find first index of element, returns -1 if not found */
long rt_array_indexOf_long(long *arr, long elem);
long rt_array_indexOf_double(double *arr, double elem);
long rt_array_indexOf_char(char *arr, char elem);
long rt_array_indexOf_bool(int *arr, int elem);
long rt_array_indexOf_string(char **arr, const char *elem);

/* Array contains functions - check if element exists */
int rt_array_contains_long(long *arr, long elem);
int rt_array_contains_double(double *arr, double elem);
int rt_array_contains_char(char *arr, char elem);
int rt_array_contains_bool(int *arr, int elem);
int rt_array_contains_string(char **arr, const char *elem);

/* Array clone functions - create deep copy */
long *rt_array_clone_long(long *arr);
double *rt_array_clone_double(double *arr);
char *rt_array_clone_char(char *arr);
int *rt_array_clone_bool(int *arr);
char **rt_array_clone_string(char **arr);

/* Array join functions - join elements into string with separator */
char *rt_array_join_long(long *arr, const char *separator);
char *rt_array_join_double(double *arr, const char *separator);
char *rt_array_join_char(char *arr, const char *separator);
char *rt_array_join_bool(int *arr, const char *separator);
char *rt_array_join_string(char **arr, const char *separator);

/* Array create from static data - creates runtime array with metadata */
long *rt_array_create_long(size_t count, const long *data);
double *rt_array_create_double(size_t count, const double *data);
char *rt_array_create_char(size_t count, const char *data);
int *rt_array_create_bool(size_t count, const int *data);
char **rt_array_create_string(size_t count, const char **data);

/* Array equality functions - compare arrays element by element */
int rt_array_eq_long(long *a, long *b);
int rt_array_eq_double(double *a, double *b);
int rt_array_eq_char(char *a, char *b);
int rt_array_eq_bool(int *a, int *b);
int rt_array_eq_string(char **a, char **b);

#endif /* RUNTIME_H */
