#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern char *rt_str_concat(char *, char *);
extern void rt_print_long(long);
extern void rt_print_double(double);
extern void rt_print_char(long);
extern void rt_print_string(char *);
extern void rt_print_bool(long);
extern long rt_add_long(long, long);
extern long rt_sub_long(long, long);
extern long rt_mul_long(long, long);
extern long rt_div_long(long, long);
extern long rt_mod_long(long, long);
extern long rt_eq_long(long, long);
extern long rt_ne_long(long, long);
extern long rt_lt_long(long, long);
extern long rt_le_long(long, long);
extern long rt_gt_long(long, long);
extern long rt_ge_long(long, long);
extern double rt_add_double(double, double);
extern double rt_sub_double(double, double);
extern double rt_mul_double(double, double);
extern double rt_div_double(double, double);
extern long rt_eq_double(double, double);
extern long rt_ne_double(double, double);
extern long rt_lt_double(double, double);
extern long rt_le_double(double, double);
extern long rt_gt_double(double, double);
extern long rt_ge_double(double, double);
extern long rt_neg_long(long);
extern double rt_neg_double(double);
extern long rt_not_bool(long);
extern long rt_post_inc_long(long *);
extern long rt_post_dec_long(long *);
extern char *rt_to_string_long(long);
extern char *rt_to_string_double(double);
extern char *rt_to_string_char(long);
extern char *rt_to_string_bool(long);
extern char *rt_to_string_string(char *);
extern long rt_eq_string(char *, char *);
extern long rt_ne_string(char *, char *);
extern long rt_lt_string(char *, char *);
extern long rt_le_string(char *, char *);
extern long rt_gt_string(char *, char *);
extern long rt_ge_string(char *, char *);
extern void rt_free_string(char *);

long x = 0;
x;
int main() {
    return 0;
}
