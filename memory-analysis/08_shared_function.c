#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

/* Runtime arena operations */
typedef struct RtArena RtArena;
extern RtArena *rt_arena_create(RtArena *parent);
extern void rt_arena_destroy(RtArena *arena);
extern void *rt_arena_alloc(RtArena *arena, size_t size);

/* Runtime string operations */
extern char *rt_str_concat(RtArena *, const char *, const char *);
extern long rt_str_length(const char *);
extern char *rt_str_substring(RtArena *, const char *, long, long);
extern long rt_str_indexOf(const char *, const char *);
extern char **rt_str_split(RtArena *, const char *, const char *);
extern char *rt_str_trim(RtArena *, const char *);
extern char *rt_str_toUpper(RtArena *, const char *);
extern char *rt_str_toLower(RtArena *, const char *);
extern int rt_str_startsWith(const char *, const char *);
extern int rt_str_endsWith(const char *, const char *);
extern int rt_str_contains(const char *, const char *);
extern char *rt_str_replace(RtArena *, const char *, const char *, const char *);
extern long rt_str_charAt(const char *, long);

/* Runtime print functions */
extern void rt_print_long(long);
extern void rt_print_double(double);
extern void rt_print_char(long);
extern void rt_print_string(const char *);
extern void rt_print_bool(long);

/* Runtime type conversions */
extern char *rt_to_string_long(RtArena *, long);
extern char *rt_to_string_double(RtArena *, double);
extern char *rt_to_string_char(RtArena *, char);
extern char *rt_to_string_bool(RtArena *, int);
extern char *rt_to_string_string(RtArena *, const char *);
extern char *rt_to_string_void(RtArena *);
extern char *rt_to_string_pointer(RtArena *, void *);

/* Runtime long arithmetic */
extern long rt_add_long(long, long);
extern long rt_sub_long(long, long);
extern long rt_mul_long(long, long);
extern long rt_div_long(long, long);
extern long rt_mod_long(long, long);
extern long rt_neg_long(long);
extern long rt_eq_long(long, long);
extern long rt_ne_long(long, long);
extern long rt_lt_long(long, long);
extern long rt_le_long(long, long);
extern long rt_gt_long(long, long);
extern long rt_ge_long(long, long);
extern long rt_post_inc_long(long *);
extern long rt_post_dec_long(long *);

/* Runtime double arithmetic */
extern double rt_add_double(double, double);
extern double rt_sub_double(double, double);
extern double rt_mul_double(double, double);
extern double rt_div_double(double, double);
extern double rt_neg_double(double);
extern long rt_eq_double(double, double);
extern long rt_ne_double(double, double);
extern long rt_lt_double(double, double);
extern long rt_le_double(double, double);
extern long rt_gt_double(double, double);
extern long rt_ge_double(double, double);

/* Runtime boolean and string comparisons */
extern long rt_not_bool(long);
extern long rt_eq_string(const char *, const char *);
extern long rt_ne_string(const char *, const char *);
extern long rt_lt_string(const char *, const char *);
extern long rt_le_string(const char *, const char *);
extern long rt_gt_string(const char *, const char *);
extern long rt_ge_string(const char *, const char *);

/* Runtime array operations */
extern long *rt_array_push_long(RtArena *, long *, long);
extern double *rt_array_push_double(RtArena *, double *, double);
extern char *rt_array_push_char(RtArena *, char *, char);
extern char **rt_array_push_string(RtArena *, char **, const char *);
extern int *rt_array_push_bool(RtArena *, int *, int);
extern long rt_array_length(void *);

/* Runtime array print functions */
extern void rt_print_array_long(long *);
extern void rt_print_array_double(double *);
extern void rt_print_array_char(char *);
extern void rt_print_array_bool(int *);
extern void rt_print_array_string(char **);

/* Runtime array clear */
extern void rt_array_clear(void *);

/* Runtime array pop functions */
extern long rt_array_pop_long(long *);
extern double rt_array_pop_double(double *);
extern char rt_array_pop_char(char *);
extern int rt_array_pop_bool(int *);
extern char *rt_array_pop_string(char **);

/* Runtime array concat functions */
extern long *rt_array_concat_long(RtArena *, long *, long *);
extern double *rt_array_concat_double(RtArena *, double *, double *);
extern char *rt_array_concat_char(RtArena *, char *, char *);
extern int *rt_array_concat_bool(RtArena *, int *, int *);
extern char **rt_array_concat_string(RtArena *, char **, char **);

/* Runtime array slice functions (start, end, step) */
extern long *rt_array_slice_long(RtArena *, long *, long, long, long);
extern double *rt_array_slice_double(RtArena *, double *, long, long, long);
extern char *rt_array_slice_char(RtArena *, char *, long, long, long);
extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);
extern char **rt_array_slice_string(RtArena *, char **, long, long, long);

/* Runtime array reverse functions */
extern long *rt_array_rev_long(RtArena *, long *);
extern double *rt_array_rev_double(RtArena *, double *);
extern char *rt_array_rev_char(RtArena *, char *);
extern int *rt_array_rev_bool(RtArena *, int *);
extern char **rt_array_rev_string(RtArena *, char **);

/* Runtime array remove functions */
extern long *rt_array_rem_long(RtArena *, long *, long);
extern double *rt_array_rem_double(RtArena *, double *, long);
extern char *rt_array_rem_char(RtArena *, char *, long);
extern int *rt_array_rem_bool(RtArena *, int *, long);
extern char **rt_array_rem_string(RtArena *, char **, long);

/* Runtime array insert functions */
extern long *rt_array_ins_long(RtArena *, long *, long, long);
extern double *rt_array_ins_double(RtArena *, double *, double, long);
extern char *rt_array_ins_char(RtArena *, char *, char, long);
extern int *rt_array_ins_bool(RtArena *, int *, int, long);
extern char **rt_array_ins_string(RtArena *, char **, const char *, long);

/* Runtime array push (copy) functions */
extern long *rt_array_push_copy_long(RtArena *, long *, long);
extern double *rt_array_push_copy_double(RtArena *, double *, double);
extern char *rt_array_push_copy_char(RtArena *, char *, char);
extern int *rt_array_push_copy_bool(RtArena *, int *, int);
extern char **rt_array_push_copy_string(RtArena *, char **, const char *);

/* Runtime array indexOf functions */
extern long rt_array_indexOf_long(long *, long);
extern long rt_array_indexOf_double(double *, double);
extern long rt_array_indexOf_char(char *, char);
extern long rt_array_indexOf_bool(int *, int);
extern long rt_array_indexOf_string(char **, const char *);

/* Runtime array contains functions */
extern int rt_array_contains_long(long *, long);
extern int rt_array_contains_double(double *, double);
extern int rt_array_contains_char(char *, char);
extern int rt_array_contains_bool(int *, int);
extern int rt_array_contains_string(char **, const char *);

/* Runtime array clone functions */
extern long *rt_array_clone_long(RtArena *, long *);
extern double *rt_array_clone_double(RtArena *, double *);
extern char *rt_array_clone_char(RtArena *, char *);
extern int *rt_array_clone_bool(RtArena *, int *);
extern char **rt_array_clone_string(RtArena *, char **);

/* Runtime array join functions */
extern char *rt_array_join_long(RtArena *, long *, const char *);
extern char *rt_array_join_double(RtArena *, double *, const char *);
extern char *rt_array_join_char(RtArena *, char *, const char *);
extern char *rt_array_join_bool(RtArena *, int *, const char *);
extern char *rt_array_join_string(RtArena *, char **, const char *);

/* Runtime array create from static data */
extern long *rt_array_create_long(RtArena *, size_t, const long *);
extern double *rt_array_create_double(RtArena *, size_t, const double *);
extern char *rt_array_create_char(RtArena *, size_t, const char *);
extern int *rt_array_create_bool(RtArena *, size_t, const int *);
extern char **rt_array_create_string(RtArena *, size_t, const char **);

/* Runtime array equality functions */
extern int rt_array_eq_long(long *, long *);
extern int rt_array_eq_double(double *, double *);
extern int rt_array_eq_char(char *, char *);
extern int rt_array_eq_bool(int *, int *);
extern int rt_array_eq_string(char **, char **);

/* Runtime range creation */
extern long *rt_array_range(RtArena *, long, long);

/* Forward declarations */
long add_numbers(RtArena *, long, long);
long compute_sum(RtArena *, long, long);

long add_numbers(RtArena *__caller_arena__, long a, long b) {
    long _return_value = 0;
    _return_value = rt_add_long(a, b);
    goto add_numbers_return;
add_numbers_return:
    return _return_value;
}

long compute_sum(RtArena *__caller_arena__, long start, long end) {
    long _return_value = 0;
    long sum = 0L;
    {
        long i = start;
        while (rt_le_long(i, end)) {
            {
                (sum = rt_add_long(sum, i));
            }
        __for_continue_0__:;
            rt_post_inc_long(&i);
        }
    }
    _return_value = sum;
    goto compute_sum_return;
compute_sum_return:
    return _return_value;
}

int main() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    int _return_value = 0;
    long result1 = add_numbers(__arena_1__, 10L, 20L);
    ({
        char *_str_arg0 = ({
        char *_str_part0, *_str_part1, *_str_part2;
        _str_part0 = rt_to_string_string(__arena_1__, "add_numbers(10, 20) = ");
        _str_part1 = rt_to_string_long(__arena_1__, result1);
        _str_part2 = rt_to_string_string(__arena_1__, "\n");
        char *_concat_tmp0;
        _concat_tmp0 = rt_str_concat(__arena_1__, _str_part0, _str_part1);
        char *_interpol_result = rt_str_concat(__arena_1__, _concat_tmp0, _str_part2);
        _interpol_result;
    });
        rt_print_string(_str_arg0);
    });
    long result2 = compute_sum(__arena_1__, 1L, 10L);
    ({
        char *_str_arg0 = ({
        char *_str_part0, *_str_part1, *_str_part2;
        _str_part0 = rt_to_string_string(__arena_1__, "compute_sum(1, 10) = ");
        _str_part1 = rt_to_string_long(__arena_1__, result2);
        _str_part2 = rt_to_string_string(__arena_1__, "\n");
        char *_concat_tmp0;
        _concat_tmp0 = rt_str_concat(__arena_1__, _str_part0, _str_part1);
        char *_interpol_result = rt_str_concat(__arena_1__, _concat_tmp0, _str_part2);
        _interpol_result;
    });
        rt_print_string(_str_arg0);
    });
    long total = 0L;
    {
        long i = 1L;
        while (rt_le_long(i, 5L)) {
            RtArena *__loop_arena_1__ = rt_arena_create(__arena_1__);
            {
                (total = rt_add_long(total, add_numbers(__loop_arena_1__, i, rt_mul_long(i, 2L))));
            }
        __loop_cleanup_1__:
            rt_arena_destroy(__loop_arena_1__);
        __for_continue_2__:;
            rt_post_inc_long(&i);
        }
    }
    ({
        char *_str_arg0 = ({
        char *_str_part0, *_str_part1, *_str_part2;
        _str_part0 = rt_to_string_string(__arena_1__, "Total from loop: ");
        _str_part1 = rt_to_string_long(__arena_1__, total);
        _str_part2 = rt_to_string_string(__arena_1__, "\n");
        char *_concat_tmp0;
        _concat_tmp0 = rt_str_concat(__arena_1__, _str_part0, _str_part1);
        char *_interpol_result = rt_str_concat(__arena_1__, _concat_tmp0, _str_part2);
        _interpol_result;
    });
        rt_print_string(_str_arg0);
    });
    goto main_return;
main_return:
    rt_arena_destroy(__arena_1__);
    return _return_value;
}

