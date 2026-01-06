#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <setjmp.h>
#include <pthread.h>
#include "runtime.h"

/* Runtime arena operations */
typedef struct RtArena RtArena;
extern RtArena *rt_arena_create(RtArena *parent);
extern void rt_arena_destroy(RtArena *arena);
extern void *rt_arena_alloc(RtArena *arena, size_t size);

/* Closure type for lambdas */
typedef struct __Closure__ { void *fn; RtArena *arena; } __Closure__;

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
extern void rt_print_byte(unsigned char);

/* Runtime type conversions */
extern char *rt_to_string_long(RtArena *, long);
extern char *rt_to_string_double(RtArena *, double);
extern char *rt_to_string_char(RtArena *, char);
extern char *rt_to_string_bool(RtArena *, int);
extern char *rt_to_string_byte(RtArena *, unsigned char);
extern char *rt_to_string_string(RtArena *, const char *);
extern char *rt_to_string_void(RtArena *);
extern char *rt_to_string_pointer(RtArena *, void *);

/* Runtime format specifier functions */
extern char *rt_format_long(RtArena *, long, const char *);
extern char *rt_format_double(RtArena *, double, const char *);
extern char *rt_format_string(RtArena *, const char *, const char *);

/* Runtime long arithmetic (comparisons are static inline in runtime.h) */
extern long rt_add_long(long, long);
extern long rt_sub_long(long, long);
extern long rt_mul_long(long, long);
extern long rt_div_long(long, long);
extern long rt_mod_long(long, long);
extern long rt_neg_long(long);
extern long rt_post_inc_long(long *);
extern long rt_post_dec_long(long *);

/* Runtime double arithmetic (comparisons are static inline in runtime.h) */
extern double rt_add_double(double, double);
extern double rt_sub_double(double, double);
extern double rt_mul_double(double, double);
extern double rt_div_double(double, double);
extern double rt_neg_double(double);

/* Runtime array operations */
extern long *rt_array_push_long(RtArena *, long *, long);
extern double *rt_array_push_double(RtArena *, double *, double);
extern char *rt_array_push_char(RtArena *, char *, char);
extern char **rt_array_push_string(RtArena *, char **, const char *);
extern int *rt_array_push_bool(RtArena *, int *, int);
extern unsigned char *rt_array_push_byte(RtArena *, unsigned char *, unsigned char);
extern void **rt_array_push_ptr(RtArena *, void **, void *);

/* Runtime array print functions */
extern void rt_print_array_long(long *);
extern void rt_print_array_double(double *);
extern void rt_print_array_char(char *);
extern void rt_print_array_bool(int *);
extern void rt_print_array_byte(unsigned char *);
extern void rt_print_array_string(char **);

/* Runtime array clear */
extern void rt_array_clear(void *);

/* Runtime array pop functions */
extern long rt_array_pop_long(long *);
extern double rt_array_pop_double(double *);
extern char rt_array_pop_char(char *);
extern int rt_array_pop_bool(int *);
extern unsigned char rt_array_pop_byte(unsigned char *);
extern char *rt_array_pop_string(char **);
extern void *rt_array_pop_ptr(void **);

/* Runtime array concat functions */
extern long *rt_array_concat_long(RtArena *, long *, long *);
extern double *rt_array_concat_double(RtArena *, double *, double *);
extern char *rt_array_concat_char(RtArena *, char *, char *);
extern int *rt_array_concat_bool(RtArena *, int *, int *);
extern unsigned char *rt_array_concat_byte(RtArena *, unsigned char *, unsigned char *);
extern char **rt_array_concat_string(RtArena *, char **, char **);
extern void **rt_array_concat_ptr(RtArena *, void **, void **);

/* Runtime array slice functions (start, end, step) */
extern long *rt_array_slice_long(RtArena *, long *, long, long, long);
extern double *rt_array_slice_double(RtArena *, double *, long, long, long);
extern char *rt_array_slice_char(RtArena *, char *, long, long, long);
extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);
extern unsigned char *rt_array_slice_byte(RtArena *, unsigned char *, long, long, long);
extern char **rt_array_slice_string(RtArena *, char **, long, long, long);

/* Runtime array reverse functions */
extern long *rt_array_rev_long(RtArena *, long *);
extern double *rt_array_rev_double(RtArena *, double *);
extern char *rt_array_rev_char(RtArena *, char *);
extern int *rt_array_rev_bool(RtArena *, int *);
extern unsigned char *rt_array_rev_byte(RtArena *, unsigned char *);
extern char **rt_array_rev_string(RtArena *, char **);

/* Runtime array remove functions */
extern long *rt_array_rem_long(RtArena *, long *, long);
extern double *rt_array_rem_double(RtArena *, double *, long);
extern char *rt_array_rem_char(RtArena *, char *, long);
extern int *rt_array_rem_bool(RtArena *, int *, long);
extern unsigned char *rt_array_rem_byte(RtArena *, unsigned char *, long);
extern char **rt_array_rem_string(RtArena *, char **, long);

/* Runtime array insert functions */
extern long *rt_array_ins_long(RtArena *, long *, long, long);
extern double *rt_array_ins_double(RtArena *, double *, double, long);
extern char *rt_array_ins_char(RtArena *, char *, char, long);
extern int *rt_array_ins_bool(RtArena *, int *, int, long);
extern unsigned char *rt_array_ins_byte(RtArena *, unsigned char *, unsigned char, long);
extern char **rt_array_ins_string(RtArena *, char **, const char *, long);

/* Runtime array push (copy) functions */
extern long *rt_array_push_copy_long(RtArena *, long *, long);
extern double *rt_array_push_copy_double(RtArena *, double *, double);
extern char *rt_array_push_copy_char(RtArena *, char *, char);
extern int *rt_array_push_copy_bool(RtArena *, int *, int);
extern unsigned char *rt_array_push_copy_byte(RtArena *, unsigned char *, unsigned char);
extern char **rt_array_push_copy_string(RtArena *, char **, const char *);

/* Runtime array indexOf functions */
extern long rt_array_indexOf_long(long *, long);
extern long rt_array_indexOf_double(double *, double);
extern long rt_array_indexOf_char(char *, char);
extern long rt_array_indexOf_bool(int *, int);
extern long rt_array_indexOf_byte(unsigned char *, unsigned char);
extern long rt_array_indexOf_string(char **, const char *);

/* Runtime array contains functions */
extern int rt_array_contains_long(long *, long);
extern int rt_array_contains_double(double *, double);
extern int rt_array_contains_char(char *, char);
extern int rt_array_contains_bool(int *, int);
extern int rt_array_contains_byte(unsigned char *, unsigned char);
extern int rt_array_contains_string(char **, const char *);

/* Runtime array clone functions */
extern long *rt_array_clone_long(RtArena *, long *);
extern double *rt_array_clone_double(RtArena *, double *);
extern char *rt_array_clone_char(RtArena *, char *);
extern int *rt_array_clone_bool(RtArena *, int *);
extern unsigned char *rt_array_clone_byte(RtArena *, unsigned char *);
extern char **rt_array_clone_string(RtArena *, char **);

/* Runtime array join functions */
extern char *rt_array_join_long(RtArena *, long *, const char *);
extern char *rt_array_join_double(RtArena *, double *, const char *);
extern char *rt_array_join_char(RtArena *, char *, const char *);
extern char *rt_array_join_bool(RtArena *, int *, const char *);
extern char *rt_array_join_byte(RtArena *, unsigned char *, const char *);
extern char *rt_array_join_string(RtArena *, char **, const char *);

/* Runtime array create from static data */
extern long *rt_array_create_long(RtArena *, size_t, const long *);
extern double *rt_array_create_double(RtArena *, size_t, const double *);
extern char *rt_array_create_char(RtArena *, size_t, const char *);
extern int *rt_array_create_bool(RtArena *, size_t, const int *);
extern unsigned char *rt_array_create_byte(RtArena *, size_t, const unsigned char *);
extern char **rt_array_create_string(RtArena *, size_t, const char **);

/* Runtime array equality functions */
extern int rt_array_eq_long(long *, long *);
extern int rt_array_eq_double(double *, double *);
extern int rt_array_eq_char(char *, char *);
extern int rt_array_eq_bool(int *, int *);
extern int rt_array_eq_byte(unsigned char *, unsigned char *);
extern int rt_array_eq_string(char **, char **);

/* Runtime range creation */
extern long *rt_array_range(RtArena *, long, long);

/* TextFile static methods */
typedef struct RtTextFile RtTextFile;
extern RtTextFile *rt_text_file_open(RtArena *, const char *);
extern int rt_text_file_exists(const char *);
extern char *rt_text_file_read_all(RtArena *, const char *);
extern void rt_text_file_write_all(const char *, const char *);
extern void rt_text_file_delete(const char *);
extern void rt_text_file_copy(const char *, const char *);
extern void rt_text_file_move(const char *, const char *);
extern void rt_text_file_close(RtTextFile *);

/* TextFile instance reading methods */
extern long rt_text_file_read_char(RtTextFile *);
extern char *rt_text_file_read_word(RtArena *, RtTextFile *);
extern char *rt_text_file_read_line(RtArena *, RtTextFile *);
extern char *rt_text_file_instance_read_all(RtArena *, RtTextFile *);
extern char **rt_text_file_read_lines(RtArena *, RtTextFile *);
extern long rt_text_file_read_into(RtTextFile *, char *);

/* TextFile instance writing methods */
extern void rt_text_file_write_char(RtTextFile *, long);
extern void rt_text_file_write(RtTextFile *, const char *);
extern void rt_text_file_write_line(RtTextFile *, const char *);
extern void rt_text_file_print(RtTextFile *, const char *);
extern void rt_text_file_println(RtTextFile *, const char *);

/* TextFile state methods */
extern int rt_text_file_has_chars(RtTextFile *);
extern int rt_text_file_has_words(RtTextFile *);
extern int rt_text_file_has_lines(RtTextFile *);
extern int rt_text_file_is_eof(RtTextFile *);
extern long rt_text_file_position(RtTextFile *);
extern void rt_text_file_seek(RtTextFile *, long);
extern void rt_text_file_rewind(RtTextFile *);
extern void rt_text_file_flush(RtTextFile *);

/* TextFile properties */
extern char *rt_text_file_get_path(RtArena *, RtTextFile *);
extern char *rt_text_file_get_name(RtArena *, RtTextFile *);
extern long rt_text_file_get_size(RtTextFile *);

/* BinaryFile static methods */
typedef struct RtBinaryFile RtBinaryFile;
extern RtBinaryFile *rt_binary_file_open(RtArena *, const char *);
extern int rt_binary_file_exists(const char *);
extern unsigned char *rt_binary_file_read_all(RtArena *, const char *);
extern void rt_binary_file_write_all(const char *, unsigned char *);
extern void rt_binary_file_delete(const char *);
extern void rt_binary_file_copy(const char *, const char *);
extern void rt_binary_file_move(const char *, const char *);
extern void rt_binary_file_close(RtBinaryFile *);

/* BinaryFile instance reading methods */
extern long rt_binary_file_read_byte(RtBinaryFile *);
extern unsigned char *rt_binary_file_read_bytes(RtArena *, RtBinaryFile *, long);
extern unsigned char *rt_binary_file_instance_read_all(RtArena *, RtBinaryFile *);
extern long rt_binary_file_read_into(RtBinaryFile *, unsigned char *);

/* BinaryFile instance writing methods */
extern void rt_binary_file_write_byte(RtBinaryFile *, long);
extern void rt_binary_file_write_bytes(RtBinaryFile *, unsigned char *);

/* BinaryFile state methods */
extern int rt_binary_file_has_bytes(RtBinaryFile *);
extern int rt_binary_file_is_eof(RtBinaryFile *);
extern long rt_binary_file_position(RtBinaryFile *);
extern void rt_binary_file_seek(RtBinaryFile *, long);
extern void rt_binary_file_rewind(RtBinaryFile *);
extern void rt_binary_file_flush(RtBinaryFile *);

/* BinaryFile properties */
extern char *rt_binary_file_get_path(RtArena *, RtBinaryFile *);
extern char *rt_binary_file_get_name(RtArena *, RtBinaryFile *);
extern long rt_binary_file_get_size(RtBinaryFile *);

/* Standard streams (Stdin, Stdout, Stderr) */
extern char *rt_stdin_read_line(RtArena *);
extern long rt_stdin_read_char(void);
extern char *rt_stdin_read_word(RtArena *);
extern int rt_stdin_has_chars(void);
extern int rt_stdin_has_lines(void);
extern int rt_stdin_is_eof(void);
extern void rt_stdout_write(const char *);
extern void rt_stdout_write_line(const char *);
extern void rt_stdout_flush(void);
extern void rt_stderr_write(const char *);
extern void rt_stderr_write_line(const char *);
extern void rt_stderr_flush(void);

/* Global convenience functions */
extern char *rt_read_line(RtArena *);
extern void rt_println(const char *);
extern void rt_print_err(const char *);
extern void rt_print_err_ln(const char *);

/* Byte array extension methods */
extern char *rt_byte_array_to_string(RtArena *, unsigned char *);
extern char *rt_byte_array_to_string_latin1(RtArena *, unsigned char *);
extern char *rt_byte_array_to_hex(RtArena *, unsigned char *);
extern char *rt_byte_array_to_base64(RtArena *, unsigned char *);
extern unsigned char *rt_string_to_bytes(RtArena *, const char *);
extern unsigned char *rt_bytes_from_hex(RtArena *, const char *);
extern unsigned char *rt_bytes_from_base64(RtArena *, const char *);

/* Path utilities */
extern char *rt_path_directory(RtArena *, const char *);
extern char *rt_path_filename(RtArena *, const char *);
extern char *rt_path_extension(RtArena *, const char *);
extern char *rt_path_join2(RtArena *, const char *, const char *);
extern char *rt_path_join3(RtArena *, const char *, const char *, const char *);
extern char *rt_path_absolute(RtArena *, const char *);
extern int rt_path_exists(const char *);
extern int rt_path_is_file(const char *);
extern int rt_path_is_directory(const char *);

/* Directory operations */
extern char **rt_directory_list(RtArena *, const char *);
extern char **rt_directory_list_recursive(RtArena *, const char *);
extern void rt_directory_create(const char *);
extern void rt_directory_delete(const char *);
extern void rt_directory_delete_recursive(const char *);

/* String splitting methods */
extern char **rt_str_split_whitespace(RtArena *, const char *);
extern char **rt_str_split_lines(RtArena *, const char *);
extern int rt_str_is_blank(const char *);

/* Mutable string operations */
extern char *rt_string_with_capacity(RtArena *, size_t);
extern char *rt_string_from(RtArena *, const char *);
extern char *rt_string_ensure_mutable(RtArena *, char *);
extern char *rt_string_append(char *, const char *);

/* Time type and operations */
typedef struct RtTime RtTime;
extern RtTime *rt_time_now(RtArena *);
extern RtTime *rt_time_utc(RtArena *);
extern RtTime *rt_time_from_millis(RtArena *, long long);
extern RtTime *rt_time_from_seconds(RtArena *, long long);
extern void rt_time_sleep(long);
extern long long rt_time_get_millis(RtTime *);
extern long long rt_time_get_seconds(RtTime *);
extern long rt_time_get_year(RtTime *);
extern long rt_time_get_month(RtTime *);
extern long rt_time_get_day(RtTime *);
extern long rt_time_get_hour(RtTime *);
extern long rt_time_get_minute(RtTime *);
extern long rt_time_get_second(RtTime *);
extern long rt_time_get_weekday(RtTime *);
extern char *rt_time_format(RtArena *, RtTime *, const char *);
extern char *rt_time_to_iso(RtArena *, RtTime *);
extern char *rt_time_to_date(RtArena *, RtTime *);
extern char *rt_time_to_time(RtArena *, RtTime *);
extern RtTime *rt_time_add(RtArena *, RtTime *, long long);
extern RtTime *rt_time_add_seconds(RtArena *, RtTime *, long);
extern RtTime *rt_time_add_minutes(RtArena *, RtTime *, long);
extern RtTime *rt_time_add_hours(RtArena *, RtTime *, long);
extern RtTime *rt_time_add_days(RtArena *, RtTime *, long);
extern long long rt_time_diff(RtTime *, RtTime *);
extern int rt_time_is_before(RtTime *, RtTime *);
extern int rt_time_is_after(RtTime *, RtTime *);
extern int rt_time_equals(RtTime *, RtTime *);

/* Environment operations */
extern char *rt_env_get(RtArena *, const char *);
extern char **rt_env_names(RtArena *);

/* Forward declarations */
void demo_types(void);
void show_integers(void);
void show_doubles(void);
void show_strings(void);
void show_chars(void);
void show_booleans(void);
void show_type_conversion(void);
void demo_loops(void);
void show_while_loops(void);
void show_for_loops(void);
void show_foreach_loops(void);
void show_break_continue(void);
void show_nested_loops(void);
void demo_conditionals(void);
void demo_strings(void);
void demo_functions(void);
void greet(void);
void greet_person(char *);
void print_sum(long, long);
void demo_arrays(void);
long add_numbers(RtArena *, long, long);
long compute_sum(void);
void demo_memory(void);
void demo_lambda(void);
void demo_closure(void);
void demo_bytes(void);
void show_byte_basics(void);
void show_byte_values(void);
void show_byte_conversions(void);
void show_byte_arrays(void);
void demo_fileio(void);
void demo_textfile(void);
void demo_binaryfile(void);
void demo_file_utilities(void);
void demo_date(void);
void demo_time(void);

/* Lambda forward declarations */
static long __lambda_0__(void *__closure__, long x);
static long __lambda_1__(void *__closure__, long a, long b);
static long __lambda_2__(void *__closure__, long x);
static long __lambda_3__(void *__closure__, long a, long b);
static long __lambda_4__(void *__closure__, long x);
static long __lambda_5__(void *__closure__, long x);
static long __lambda_6__(void *__closure__, long x);
typedef struct __closure_7__ {
    void *fn;
    RtArena *arena;
    long *multiplier;
} __closure_7__;
static long __lambda_7__(void *__closure__, long x);

void demo_types() {
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                      Sindarin Type System                        │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    show_integers();
    show_doubles();
    show_strings();
    show_chars();
    show_booleans();
    show_type_conversion();
    goto demo_types_return;
demo_types_return:
    return;
}

void show_integers() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 1. Integer Type (int) ---\n");
    long a = 42L;
    long b = -17L;
    long c = 0L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, a);
        char *_r = rt_str_concat(__arena_1__, "a = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, b);
        char *_r = rt_str_concat(__arena_1__, "b = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, c);
        char *_r = rt_str_concat(__arena_1__, "c = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nArithmetic:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_add_long(a, b));
        char *_r = rt_str_concat(__arena_1__, "  a + b = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_sub_long(a, b));
        char *_r = rt_str_concat(__arena_1__, "  a - b = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_mul_long(a, 2L));
        char *_r = rt_str_concat(__arena_1__, "  a * 2 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_div_long(a, 5L));
        char *_r = rt_str_concat(__arena_1__, "  a / 5 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_mod_long(a, 5L));
        char *_r = rt_str_concat(__arena_1__, "  a % 5 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nIncrement/Decrement:\n");
    long x = 5L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        char *_r = rt_str_concat(__arena_1__, "  x = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_post_inc_long(&x);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        char *_r = rt_str_concat(__arena_1__, "  After x++: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_post_dec_long(&x);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        char *_r = rt_str_concat(__arena_1__, "  After x--: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nComparisons:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, 1L);
        char *_r = rt_str_concat(__arena_1__, "  10 == 10: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, 1L);
        char *_r = rt_str_concat(__arena_1__, "  10 != 5: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, 1L);
        char *_r = rt_str_concat(__arena_1__, "  10 > 5: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, 0L);
        char *_r = rt_str_concat(__arena_1__, "  10 < 5: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, 1L);
        char *_r = rt_str_concat(__arena_1__, "  10 >= 10: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, 1L);
        char *_r = rt_str_concat(__arena_1__, "  10 <= 10: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_integers_return;
show_integers_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_doubles() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 2. Double Type (double) ---\n");
    double pi = 3.1415899999999999;
    double e = 2.71828;
    double negative = -1.5;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__arena_1__, pi);
        char *_r = rt_str_concat(__arena_1__, "pi = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__arena_1__, e);
        char *_r = rt_str_concat(__arena_1__, "e = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__arena_1__, negative);
        char *_r = rt_str_concat(__arena_1__, "negative = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nArithmetic:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__arena_1__, rt_add_double(pi, e));
        char *_r = rt_str_concat(__arena_1__, "  pi + e = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__arena_1__, rt_mul_double(pi, 2.0));
        char *_r = rt_str_concat(__arena_1__, "  pi * 2.0 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__arena_1__, 3.3333333333333335);
        char *_r = rt_str_concat(__arena_1__, "  10.0 / 3.0 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nMixed operations:\n");
    double radius = 5.0;
    double area = rt_mul_double(rt_mul_double(pi, radius), radius);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__arena_1__, area);
        char *_r = rt_str_concat(__arena_1__, "  Circle area (r=5): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_doubles_return;
show_doubles_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_strings() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 3. String Type (str) ---\n");
    char * greeting = rt_to_string_string(__arena_1__, "Hello");
    char * name = rt_to_string_string(__arena_1__, "World");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "greeting = \"", greeting);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "name = \"", name);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * message = ({ char *_left = ({ char *_left = rt_str_concat(__arena_1__, greeting, ", "); char *_right = name; char *_res = rt_str_concat(__arena_1__, _left, _right);  _res; }); char *_right = "!"; char *_res = rt_str_concat(__arena_1__, _left, _right);  _res; });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Concatenated: ", message);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long age = 25L;
    double height = 5.9000000000000004;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, age);
        char *_p1 = rt_to_string_double(__arena_1__, height);
        char *_r = rt_str_concat(__arena_1__, "Interpolation: Age is ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ", height is ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * empty = rt_to_string_string(__arena_1__, "");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Empty string: \"", empty);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nString comparisons:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_eq_string("abc", "abc"));
        char *_r = rt_str_concat(__arena_1__, "  \"abc\" == \"abc\": ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_ne_string("abc", "xyz"));
        char *_r = rt_str_concat(__arena_1__, "  \"abc\" != \"xyz\": ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_lt_string("abc", "abd"));
        char *_r = rt_str_concat(__arena_1__, "  \"abc\" < \"abd\": ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_strings_return;
show_strings_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_chars() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 4. Character Type (char) ---\n");
    char letter = 'A';
    char digit = '7';
    char symbol = '@';
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__arena_1__, letter);
        char *_r = rt_str_concat(__arena_1__, "letter = '", _p0);
        _r = rt_str_concat(__arena_1__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__arena_1__, digit);
        char *_r = rt_str_concat(__arena_1__, "digit = '", _p0);
        _r = rt_str_concat(__arena_1__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__arena_1__, symbol);
        char *_r = rt_str_concat(__arena_1__, "symbol = '", _p0);
        _r = rt_str_concat(__arena_1__, _r, "'\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char tab = '\t';
    rt_print_string("\nEscape sequences:\n");
    rt_print_string("  Tab:");
    rt_print_char(tab);
    rt_print_string("between\n");
    char first = 'S';
    char * rest = rt_to_string_string(__arena_1__, "indarin");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__arena_1__, first);
        char *_r = rt_str_concat(__arena_1__, "  Combined: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, rest);
        _r = rt_str_concat(__arena_1__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_chars_return;
show_chars_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_booleans() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 5. Boolean Type (bool) ---\n");
    bool is_active = 1L;
    bool is_complete = 0L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, is_active);
        char *_r = rt_str_concat(__arena_1__, "is_active = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, is_complete);
        char *_r = rt_str_concat(__arena_1__, "is_complete = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long x = 10L;
    long y = 5L;
    bool greater = rt_gt_long(x, y);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        char *_p1 = rt_to_string_long(__arena_1__, y);
        char *_p2 = rt_to_string_bool(__arena_1__, greater);
        char *_r = rt_str_concat(__arena_1__, "\n", _p0);
        _r = rt_str_concat(__arena_1__, _r, " > ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, " = ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nNOT operator (!):\n");
    bool flag = 0L;
    if (rt_not_bool(flag)) {
        {
            rt_print_string("  !false = true\n");
        }
    }
    (flag = 1L);
    if (rt_not_bool(flag)) {
        {
            rt_print_string("  never printed\n");
        }
    }
    else {
        {
            rt_print_string("  !true = false\n\n");
        }
    }
    goto show_booleans_return;
show_booleans_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_type_conversion() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 6. Type Display in Strings ---\n");
    long i = 42L;
    double d = 3.1400000000000001;
    char * s = rt_to_string_string(__arena_1__, "hello");
    char c = 'X';
    bool b = 1L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, i);
        char *_r = rt_str_concat(__arena_1__, "int: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_double(__arena_1__, d);
        char *_r = rt_str_concat(__arena_1__, "double: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "str: ", s);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_char(__arena_1__, c);
        char *_r = rt_str_concat(__arena_1__, "char: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, b);
        char *_r = rt_str_concat(__arena_1__, "bool: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, i);
        char *_p1 = rt_to_string_double(__arena_1__, d);
        char *_p2 = rt_to_string_char(__arena_1__, c);
        char *_p3 = rt_to_string_bool(__arena_1__, b);
        char *_r = rt_str_concat(__arena_1__, "\nMixed: i=", _p0);
        _r = rt_str_concat(__arena_1__, _r, ", d=");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, ", s=");
        _r = rt_str_concat(__arena_1__, _r, s);
        _r = rt_str_concat(__arena_1__, _r, ", c=");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, ", b=");
        _r = rt_str_concat(__arena_1__, _r, _p3);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_type_conversion_return;
show_type_conversion_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_loops() {
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                      Sindarin Loop Features                      │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    show_while_loops();
    show_for_loops();
    show_foreach_loops();
    show_break_continue();
    show_nested_loops();
    goto demo_loops_return;
demo_loops_return:
    return;
}

void show_while_loops() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 1. While Loops ---\n");
    rt_print_string("Counting 1 to 5:\n");
    long i = 1L;
    while (rt_le_long(i, 5L)) {
        RtArena *__loop_arena_0__ = rt_arena_create(__arena_1__);
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_0__, i);
        char *_r = rt_str_concat(__loop_arena_0__, "  ", _p0);
        _r = rt_str_concat(__loop_arena_0__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            (i = rt_add_long(i, 1L));
        }
    __loop_cleanup_0__:
        rt_arena_destroy(__loop_arena_0__);
    }
    rt_print_string("\nFinding first power of 2 >= 100:\n");
    long power = 1L;
    while (rt_lt_long(power, 100L)) {
        RtArena *__loop_arena_1__ = rt_arena_create(__arena_1__);
        {
            (power = rt_mul_long(power, 2L));
        }
    __loop_cleanup_1__:
        rt_arena_destroy(__loop_arena_1__);
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, power);
        char *_r = rt_str_concat(__arena_1__, "  Result: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nCountdown:\n");
    long count = 5L;
    while (rt_gt_long(count, 0L)) {
        RtArena *__loop_arena_2__ = rt_arena_create(__arena_1__);
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_2__, count);
        char *_r = rt_str_concat(__loop_arena_2__, "  ", _p0);
        _r = rt_str_concat(__loop_arena_2__, _r, "...");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            (count = rt_sub_long(count, 1L));
        }
    __loop_cleanup_2__:
        rt_arena_destroy(__loop_arena_2__);
    }
    rt_print_string("  Liftoff!\n\n");
    goto show_while_loops_return;
show_while_loops_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_for_loops() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 2. For Loops ---\n");
    rt_print_string("For loop 0 to 4:\n");
    {
        long i = 0L;
        while (rt_lt_long(i, 5L)) {
            RtArena *__loop_arena_3__ = rt_arena_create(__arena_1__);
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_3__, i);
        char *_r = rt_str_concat(__loop_arena_3__, "  i = ", _p0);
        _r = rt_str_concat(__loop_arena_3__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_3__:
            rt_arena_destroy(__loop_arena_3__);
        __for_continue_4__:;
            rt_post_inc_long(&i);
        }
    }
    rt_print_string("\nFor loop 5 down to 1:\n");
    {
        long j = 5L;
        while (rt_ge_long(j, 1L)) {
            RtArena *__loop_arena_5__ = rt_arena_create(__arena_1__);
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_5__, j);
        char *_r = rt_str_concat(__loop_arena_5__, "  j = ", _p0);
        _r = rt_str_concat(__loop_arena_5__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_5__:
            rt_arena_destroy(__loop_arena_5__);
        __for_continue_6__:;
            rt_post_dec_long(&j);
        }
    }
    rt_print_string("\nFor loop with step of 2:\n");
    {
        long k = 0L;
        while (rt_le_long(k, 10L)) {
            RtArena *__loop_arena_7__ = rt_arena_create(__arena_1__);
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_7__, k);
        char *_r = rt_str_concat(__loop_arena_7__, "  k = ", _p0);
        _r = rt_str_concat(__loop_arena_7__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_7__:
            rt_arena_destroy(__loop_arena_7__);
        __for_continue_8__:;
            (k = rt_add_long(k, 2L));
        }
    }
    rt_print_string("\nSum of 1 to 10:\n");
    long sum = 0L;
    {
        long n = 1L;
        while (rt_le_long(n, 10L)) {
            RtArena *__loop_arena_9__ = rt_arena_create(__arena_1__);
            {
                (sum = rt_add_long(sum, n));
            }
        __loop_cleanup_9__:
            rt_arena_destroy(__loop_arena_9__);
        __for_continue_10__:;
            rt_post_inc_long(&n);
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, sum);
        char *_r = rt_str_concat(__arena_1__, "  Sum = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_for_loops_return;
show_for_loops_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_foreach_loops() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 3. For-Each Loops ---\n");
    long * numbers = rt_array_create_long(__arena_1__, 5, (long[]){10L, 20L, 30L, 40L, 50L});
    rt_print_string("Iterating over int array:\n");
    {
        long * __arr_0__ = numbers;
        long __len_0__ = rt_array_length(__arr_0__);
        for (long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            RtArena *__loop_arena_11__ = rt_arena_create(__arena_1__);
            long num = __arr_0__[__idx_0__];
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_11__, num);
        char *_r = rt_str_concat(__loop_arena_11__, "  ", _p0);
        _r = rt_str_concat(__loop_arena_11__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_11__:
            rt_arena_destroy(__loop_arena_11__);
        }
    }
    char * * fruits = rt_array_create_string(__arena_1__, 3, (char *[]){"apple", "banana", "cherry"});
    rt_print_string("\nIterating over string array:\n");
    {
        char * * __arr_1__ = fruits;
        long __len_1__ = rt_array_length(__arr_1__);
        for (long __idx_1__ = 0; __idx_1__ < __len_1__; __idx_1__++) {
            RtArena *__loop_arena_12__ = rt_arena_create(__arena_1__);
            char * fruit = __arr_1__[__idx_1__];
            {
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__loop_arena_12__, "  ", fruit);
        _r = rt_str_concat(__loop_arena_12__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_12__:
            rt_arena_destroy(__loop_arena_12__);
        }
    }
    rt_print_string("\nSum with for-each:\n");
    long total = 0L;
    {
        long * __arr_2__ = numbers;
        long __len_2__ = rt_array_length(__arr_2__);
        for (long __idx_2__ = 0; __idx_2__ < __len_2__; __idx_2__++) {
            RtArena *__loop_arena_13__ = rt_arena_create(__arena_1__);
            long n = __arr_2__[__idx_2__];
            {
                (total = rt_add_long(total, n));
            }
        __loop_cleanup_13__:
            rt_arena_destroy(__loop_arena_13__);
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, total);
        char *_r = rt_str_concat(__arena_1__, "  Total = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto show_foreach_loops_return;
show_foreach_loops_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_break_continue() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 4. Break and Continue ---\n");
    rt_print_string("Break at 5:\n");
    {
        long i = 1L;
        while (rt_le_long(i, 10L)) {
            RtArena *__loop_arena_14__ = rt_arena_create(__arena_1__);
            {
                if (rt_eq_long(i, 5L)) {
                    {
                        rt_print_string("  (breaking)\n");
                        { rt_arena_destroy(__loop_arena_14__); break; }
                    }
                }
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_14__, i);
        char *_r = rt_str_concat(__loop_arena_14__, "  i = ", _p0);
        _r = rt_str_concat(__loop_arena_14__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_14__:
            rt_arena_destroy(__loop_arena_14__);
        __for_continue_15__:;
            rt_post_inc_long(&i);
        }
    }
    rt_print_string("\nContinue (skip evens):\n");
    {
        long j = 1L;
        while (rt_le_long(j, 6L)) {
            RtArena *__loop_arena_16__ = rt_arena_create(__arena_1__);
            {
                if (rt_eq_long(rt_mod_long(j, 2L), 0L)) {
                    {
                        goto __loop_cleanup_16__;
                    }
                }
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_16__, j);
        char *_r = rt_str_concat(__loop_arena_16__, "  j = ", _p0);
        _r = rt_str_concat(__loop_arena_16__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_16__:
            rt_arena_destroy(__loop_arena_16__);
        __for_continue_17__:;
            rt_post_inc_long(&j);
        }
    }
    rt_print_string("\nBreak in while (find first > 50 divisible by 7):\n");
    long n = 50L;
    while (rt_lt_long(n, 100L)) {
        RtArena *__loop_arena_18__ = rt_arena_create(__arena_1__);
        {
            rt_post_inc_long(&n);
            if (rt_eq_long(rt_mod_long(n, 7L), 0L)) {
                {
                    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_18__, n);
        char *_r = rt_str_concat(__loop_arena_18__, "  Found: ", _p0);
        _r = rt_str_concat(__loop_arena_18__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
                    { rt_arena_destroy(__loop_arena_18__); break; }
                }
            }
        }
    __loop_cleanup_18__:
        rt_arena_destroy(__loop_arena_18__);
    }
    rt_print_string("\nContinue in for-each (skip 'banana'):\n");
    char * * fruits = rt_array_create_string(__arena_1__, 4, (char *[]){"apple", "banana", "cherry", "date"});
    {
        char * * __arr_3__ = fruits;
        long __len_3__ = rt_array_length(__arr_3__);
        for (long __idx_3__ = 0; __idx_3__ < __len_3__; __idx_3__++) {
            RtArena *__loop_arena_19__ = rt_arena_create(__arena_1__);
            char * fruit = __arr_3__[__idx_3__];
            {
                if (rt_eq_string(fruit, "banana")) {
                    {
                        goto __loop_cleanup_19__;
                    }
                }
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__loop_arena_19__, "  ", fruit);
        _r = rt_str_concat(__loop_arena_19__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_19__:
            rt_arena_destroy(__loop_arena_19__);
        }
    }
    rt_print_string("\n");
    goto show_break_continue_return;
show_break_continue_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_nested_loops() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 5. Nested Loops ---\n");
    rt_print_string("Multiplication table (1-3):\n");
    {
        long i = 1L;
        while (rt_le_long(i, 3L)) {
            RtArena *__loop_arena_20__ = rt_arena_create(__arena_1__);
            {
                {
                    long j = 1L;
                    while (rt_le_long(j, 3L)) {
                        RtArena *__loop_arena_22__ = rt_arena_create(__loop_arena_20__);
                        {
                            long product = rt_mul_long(i, j);
                            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_22__, i);
        char *_p1 = rt_to_string_long(__loop_arena_22__, j);
        char *_p2 = rt_to_string_long(__loop_arena_22__, product);
        char *_r = rt_str_concat(__loop_arena_22__, "  ", _p0);
        _r = rt_str_concat(__loop_arena_22__, _r, " x ");
        _r = rt_str_concat(__loop_arena_22__, _r, _p1);
        _r = rt_str_concat(__loop_arena_22__, _r, " = ");
        _r = rt_str_concat(__loop_arena_22__, _r, _p2);
        _r = rt_str_concat(__loop_arena_22__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
                        }
                    __loop_cleanup_22__:
                        rt_arena_destroy(__loop_arena_22__);
                    __for_continue_23__:;
                        rt_post_inc_long(&j);
                    }
                }
                rt_print_string("\n");
            }
        __loop_cleanup_20__:
            rt_arena_destroy(__loop_arena_20__);
        __for_continue_21__:;
            rt_post_inc_long(&i);
        }
    }
    rt_print_string("Triangle pattern:\n");
    {
        long row = 1L;
        while (rt_le_long(row, 5L)) {
            RtArena *__loop_arena_24__ = rt_arena_create(__arena_1__);
            {
                rt_print_string("  ");
                {
                    long col = 1L;
                    while (rt_le_long(col, row)) {
                        RtArena *__loop_arena_26__ = rt_arena_create(__loop_arena_24__);
                        {
                            rt_print_string("*");
                        }
                    __loop_cleanup_26__:
                        rt_arena_destroy(__loop_arena_26__);
                    __for_continue_27__:;
                        rt_post_inc_long(&col);
                    }
                }
                rt_print_string("\n");
            }
        __loop_cleanup_24__:
            rt_arena_destroy(__loop_arena_24__);
        __for_continue_25__:;
            rt_post_inc_long(&row);
        }
    }
    rt_print_string("\nNested for-each (pairs):\n");
    long * a = rt_array_create_long(__arena_1__, 2, (long[]){1L, 2L});
    long * b = rt_array_create_long(__arena_1__, 2, (long[]){10L, 20L});
    {
        long * __arr_4__ = a;
        long __len_4__ = rt_array_length(__arr_4__);
        for (long __idx_4__ = 0; __idx_4__ < __len_4__; __idx_4__++) {
            RtArena *__loop_arena_28__ = rt_arena_create(__arena_1__);
            long x = __arr_4__[__idx_4__];
            {
                {
                    long * __arr_5__ = b;
                    long __len_5__ = rt_array_length(__arr_5__);
                    for (long __idx_5__ = 0; __idx_5__ < __len_5__; __idx_5__++) {
                        RtArena *__loop_arena_29__ = rt_arena_create(__loop_arena_28__);
                        long y = __arr_5__[__idx_5__];
                        {
                            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_29__, x);
        char *_p1 = rt_to_string_long(__loop_arena_29__, y);
        char *_r = rt_str_concat(__loop_arena_29__, "  (", _p0);
        _r = rt_str_concat(__loop_arena_29__, _r, ", ");
        _r = rt_str_concat(__loop_arena_29__, _r, _p1);
        _r = rt_str_concat(__loop_arena_29__, _r, ")\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
                        }
                    __loop_cleanup_29__:
                        rt_arena_destroy(__loop_arena_29__);
                    }
                }
            }
        __loop_cleanup_28__:
            rt_arena_destroy(__loop_arena_28__);
        }
    }
    goto show_nested_loops_return;
show_nested_loops_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_conditionals() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                      Sindarin Conditionals                       │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    rt_print_string("--- If Statements ---\n");
    long x = 10L;
    if (rt_gt_long(x, 5L)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        rt_str_concat(__arena_1__, _p0, " is greater than 5\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    if (rt_eq_long(x, 10L)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        rt_str_concat(__arena_1__, _p0, " equals 10\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    rt_print_string("\n--- If-Else ---\n");
    long age = 20L;
    if (rt_ge_long(age, 18L)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, age);
        char *_r = rt_str_concat(__arena_1__, "Age ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ": Adult\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    else {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, age);
        char *_r = rt_str_concat(__arena_1__, "Age ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ": Minor\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    long score = 75L;
    if (rt_ge_long(score, 60L)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, score);
        char *_r = rt_str_concat(__arena_1__, "Score ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ": Pass\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    else {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, score);
        char *_r = rt_str_concat(__arena_1__, "Score ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ": Fail\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    rt_print_string("\n--- NOT Operator ---\n");
    bool flag = 0L;
    if (rt_not_bool(flag)) {
        {
            rt_print_string("!false = true\n");
        }
    }
    rt_print_string("\n--- AND (&&) and OR (||) ---\n");
    bool hasTicket = 1L;
    bool hasID = 1L;
    bool isVIP = 0L;
    if (((hasTicket != 0 && hasID != 0) ? 1L : 0L)) {
        {
            rt_print_string("Entry allowed (has ticket AND ID)\n");
        }
    }
    if (((hasTicket != 0 || isVIP != 0) ? 1L : 0L)) {
        {
            rt_print_string("Can enter (has ticket OR is VIP)\n");
        }
    }
    long temperature = 25L;
    if (((rt_gt_long(temperature, 20L) != 0 && rt_lt_long(temperature, 30L) != 0) ? 1L : 0L)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, temperature);
        char *_r = rt_str_concat(__arena_1__, "Temperature ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "C is comfortable\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    if (((rt_lt_long(temperature, 10L) != 0 || rt_gt_long(temperature, 35L) != 0) ? 1L : 0L)) {
        {
            rt_print_string("Extreme temperature!\n");
        }
    }
    else {
        {
            rt_print_string("Temperature is moderate\n");
        }
    }
    bool loggedIn = 1L;
    bool isAdmin = 0L;
    bool isModerator = 1L;
    if (((loggedIn != 0 && ((isAdmin != 0 || isModerator != 0) ? 1L : 0L) != 0) ? 1L : 0L)) {
        {
            rt_print_string("User can moderate content\n");
        }
    }
    rt_print_string("\n--- Comparisons ---\n");
    long a = 10L;
    long b = 20L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, a);
        char *_p1 = rt_to_string_long(__arena_1__, b);
        char *_r = rt_str_concat(__arena_1__, "a = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ", b = ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_eq_long(a, b));
        char *_r = rt_str_concat(__arena_1__, "a == b: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_ne_long(a, b));
        char *_r = rt_str_concat(__arena_1__, "a != b: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_lt_long(a, b));
        char *_r = rt_str_concat(__arena_1__, "a < b: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_gt_long(a, b));
        char *_r = rt_str_concat(__arena_1__, "a > b: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Even/Odd Check ---\n");
    long n = 7L;
    if (rt_eq_long(rt_mod_long(n, 2L), 0L)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, n);
        rt_str_concat(__arena_1__, _p0, " is even\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    else {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, n);
        rt_str_concat(__arena_1__, _p0, " is odd\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    (n = 12L);
    if (rt_eq_long(rt_mod_long(n, 2L), 0L)) {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, n);
        rt_str_concat(__arena_1__, _p0, " is even\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    else {
        {
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, n);
        rt_str_concat(__arena_1__, _p0, " is odd\n");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    rt_print_string("\n--- Max Example ---\n");
    long p = 5L;
    long q = 12L;
    long m = p;
    if (rt_gt_long(q, p)) {
        {
            (m = q);
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, p);
        char *_p1 = rt_to_string_long(__arena_1__, q);
        char *_p2 = rt_to_string_long(__arena_1__, m);
        char *_r = rt_str_concat(__arena_1__, "max(", _p0);
        _r = rt_str_concat(__arena_1__, _r, ", ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, ") = ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto demo_conditionals_return;
demo_conditionals_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_strings() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                        Sindarin Strings                          │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    rt_print_string("--- String Literals ---\n");
    char * hello = rt_to_string_string(__arena_1__, "Hello, World!");
    rt_print_string(hello);
    rt_print_string("\n");
    char * empty = rt_to_string_string(__arena_1__, "");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Empty string: \"", empty);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- String Length ---\n");
    char * greeting = rt_to_string_string(__arena_1__, "Hello");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, (long)strlen(greeting));
        char *_r = rt_str_concat(__arena_1__, "len(\"", greeting);
        _r = rt_str_concat(__arena_1__, _r, "\") = ");
        _r = rt_str_concat(__arena_1__, _r, _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_str_length(greeting));
        char *_r = rt_str_concat(__arena_1__, "\"", greeting);
        _r = rt_str_concat(__arena_1__, _r, "\".length = ");
        _r = rt_str_concat(__arena_1__, _r, _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * sentence = rt_to_string_string(__arena_1__, "The quick brown fox");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, (long)strlen(sentence));
        char *_r = rt_str_concat(__arena_1__, "len(\"", sentence);
        _r = rt_str_concat(__arena_1__, _r, "\") = ");
        _r = rt_str_concat(__arena_1__, _r, _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Concatenation ---\n");
    char * first = rt_to_string_string(__arena_1__, "Hello");
    char * second = rt_to_string_string(__arena_1__, "World");
    char * combined = ({ char *_left = rt_str_concat(__arena_1__, first, " "); char *_right = second; char *_res = rt_str_concat(__arena_1__, _left, _right);  _res; });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Combined: \"", combined);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Basic Interpolation ---\n");
    char * name = rt_to_string_string(__arena_1__, "Alice");
    long age = 30L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, age);
        char *_r = rt_str_concat(__arena_1__, "Name: ", name);
        _r = rt_str_concat(__arena_1__, _r, ", Age: ");
        _r = rt_str_concat(__arena_1__, _r, _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long x = 5L;
    long y = 3L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        char *_p1 = rt_to_string_long(__arena_1__, y);
        char *_p2 = rt_to_string_long(__arena_1__, rt_add_long(x, y));
        char *_r = rt_str_concat(__arena_1__, _p0, " + ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, " = ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        char *_p1 = rt_to_string_long(__arena_1__, y);
        char *_p2 = rt_to_string_long(__arena_1__, rt_mul_long(x, y));
        char *_r = rt_str_concat(__arena_1__, _p0, " * ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, " = ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Escaped Quotes in Interpolation ---\n");
    char * item = rt_to_string_string(__arena_1__, "widget");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Item name: \"", item);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = "Nested quotes: \"She said \\\"hello\\\"\"\n";
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Format Specifiers ---\n");
    double pi = 3.1415926535900001;
    double price = 42.5;
    long num = 255L;
    long count = 7L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_double(__arena_1__, pi, ".2f");
        char *_r = rt_str_concat(__arena_1__, "Pi (2 decimals): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_double(__arena_1__, pi, ".4f");
        char *_r = rt_str_concat(__arena_1__, "Pi (4 decimals): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_double(__arena_1__, price, ".2f");
        char *_r = rt_str_concat(__arena_1__, "Price: $", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_long(__arena_1__, num, "x");
        char *_r = rt_str_concat(__arena_1__, "255 in hex (lower): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_long(__arena_1__, num, "X");
        char *_r = rt_str_concat(__arena_1__, "255 in hex (upper): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_long(__arena_1__, count, "03d");
        char *_r = rt_str_concat(__arena_1__, "Count (3 digits): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_format_long(__arena_1__, count, "05d");
        char *_r = rt_str_concat(__arena_1__, "Count (5 digits): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Multi-line Interpolation ---\n");
    char * user = rt_to_string_string(__arena_1__, "Bob");
    long score = 95L;
    char * profile = ({
        char *_p0 = rt_to_string_long(__arena_1__, score);
        char *_r = rt_str_concat(__arena_1__, "User Profile:\n  Name: ", user);
        _r = rt_str_concat(__arena_1__, _r, "\n  Score: ");
        _r = rt_str_concat(__arena_1__, _r, _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n  Grade: A");
        _r;
    });
    rt_print_string(profile);
    rt_print_string("\n");
    long a = 10L;
    long b = 20L;
    char * report = ({
        char *_p0 = rt_to_string_long(__arena_1__, a);
        char *_p1 = rt_to_string_long(__arena_1__, b);
        char *_p2 = rt_to_string_long(__arena_1__, rt_add_long(a, b));
        char *_p3 = rt_to_string_long(__arena_1__, rt_mul_long(a, b));
        char *_r = rt_str_concat(__arena_1__, "Calculation Report:\n    Value A: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n    Value B: ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, "\n    Sum: ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, "\n    Product: ");
        _r = rt_str_concat(__arena_1__, _r, _p3);
        _r;
    });
    rt_print_string(report);
    rt_print_string("\n");
    rt_print_string("\n--- Nested Interpolation ---\n");
    long inner_val = 42L;
    char * outer = ({
        char *_p0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, inner_val);
        rt_str_concat(__arena_1__, "inner value is ", _p0);
    });
        rt_str_concat(__arena_1__, "Outer contains: ", _p0);
    });
    rt_print_string(outer);
    rt_print_string("\n");
    long level = 3L;
    char * deep = ({
        char *_p0 = ({
        char *_p0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, level);
        rt_str_concat(__arena_1__, "L3: ", _p0);
    });
        rt_str_concat(__arena_1__, "L2: ", _p0);
    });
        rt_str_concat(__arena_1__, "L1: ", _p0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Deep nesting: ", deep);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Escape Sequences ---\n");
    rt_print_string("Line 1\nLine 2\nLine 3\n");
    rt_print_string("Tab:\tValue\n");
    rt_print_string("Quote: \"Hello\"\n");
    rt_print_string("\n--- Comparisons ---\n");
    char * s1 = rt_to_string_string(__arena_1__, "apple");
    char * s2 = rt_to_string_string(__arena_1__, "apple");
    char * s3 = rt_to_string_string(__arena_1__, "banana");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_eq_string(s1, s2));
        char *_r = rt_str_concat(__arena_1__, "apple == apple: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_eq_string(s1, s3));
        char *_r = rt_str_concat(__arena_1__, "apple == banana: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Case Conversion ---\n");
    char * text = rt_to_string_string(__arena_1__, "Hello World");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Original: \"", text);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_toUpper(__arena_1__, text);
        char *_r = rt_str_concat(__arena_1__, "toUpper(): \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_toLower(__arena_1__, text);
        char *_r = rt_str_concat(__arena_1__, "toLower(): \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_toUpper(__arena_1__, "sindarin");
        char *_r = rt_str_concat(__arena_1__, "\"sindarin\".toUpper() = \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Trim ---\n");
    char * padded = rt_to_string_string(__arena_1__, "   hello world   ");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Original: \"", padded);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_trim(__arena_1__, padded);
        char *_r = rt_str_concat(__arena_1__, "trim(): \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Substring ---\n");
    char * phrase = rt_to_string_string(__arena_1__, "Hello, World!");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Original: \"", phrase);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_substring(__arena_1__, phrase, 0L, 5L);
        char *_r = rt_str_concat(__arena_1__, "substring(0, 5): \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_str_substring(__arena_1__, phrase, 7L, 12L);
        char *_r = rt_str_concat(__arena_1__, "substring(7, 12): \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- indexOf ---\n");
    char * haystack = rt_to_string_string(__arena_1__, "the quick brown fox");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "String: \"", haystack);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * search1 = rt_to_string_string(__arena_1__, "quick");
    char * search2 = rt_to_string_string(__arena_1__, "fox");
    char * search3 = rt_to_string_string(__arena_1__, "cat");
    long idx1 = rt_str_indexOf(haystack, search1);
    long idx2 = rt_str_indexOf(haystack, search2);
    long idx3 = rt_str_indexOf(haystack, search3);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, idx1);
        char *_r = rt_str_concat(__arena_1__, "indexOf(\"quick\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, idx2);
        char *_r = rt_str_concat(__arena_1__, "indexOf(\"fox\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, idx3);
        char *_r = rt_str_concat(__arena_1__, "indexOf(\"cat\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- String Search ---\n");
    char * filename = rt_to_string_string(__arena_1__, "document.txt");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "String: \"", filename);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * prefix1 = rt_to_string_string(__arena_1__, "doc");
    char * prefix2 = rt_to_string_string(__arena_1__, "file");
    char * suffix1 = rt_to_string_string(__arena_1__, ".txt");
    char * suffix2 = rt_to_string_string(__arena_1__, ".pdf");
    char * sub1 = rt_to_string_string(__arena_1__, "ment");
    char * sub2 = rt_to_string_string(__arena_1__, "xyz");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_startsWith(filename, prefix1));
        char *_r = rt_str_concat(__arena_1__, "startsWith(\"doc\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_startsWith(filename, prefix2));
        char *_r = rt_str_concat(__arena_1__, "startsWith(\"file\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_endsWith(filename, suffix1));
        char *_r = rt_str_concat(__arena_1__, "endsWith(\".txt\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_endsWith(filename, suffix2));
        char *_r = rt_str_concat(__arena_1__, "endsWith(\".pdf\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_contains(filename, sub1));
        char *_r = rt_str_concat(__arena_1__, "contains(\"ment\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_contains(filename, sub2));
        char *_r = rt_str_concat(__arena_1__, "contains(\"xyz\"): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Replace ---\n");
    char * original = rt_to_string_string(__arena_1__, "hello world");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Original: \"", original);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * oldStr = rt_to_string_string(__arena_1__, "world");
    char * newStr = rt_to_string_string(__arena_1__, "Sindarin");
    char * replaced = rt_str_replace(__arena_1__, original, oldStr, newStr);
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "replace(\"world\", \"Sindarin\"): \"", replaced);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Split ---\n");
    char * csv = rt_to_string_string(__arena_1__, "apple,banana,cherry");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "String: \"", csv);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * delim = rt_to_string_string(__arena_1__, ",");
    char * * parts = rt_str_split(__arena_1__, csv, delim);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(parts));
        char *_r = rt_str_concat(__arena_1__, "split(\",\") -> ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " parts:\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    {
        char * * __arr_6__ = parts;
        long __len_6__ = rt_array_length(__arr_6__);
        for (long __idx_6__ = 0; __idx_6__ < __len_6__; __idx_6__++) {
            RtArena *__loop_arena_30__ = rt_arena_create(__arena_1__);
            char * part = __arr_6__[__idx_6__];
            {
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__loop_arena_30__, "  - \"", part);
        _r = rt_str_concat(__loop_arena_30__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_30__:
            rt_arena_destroy(__loop_arena_30__);
        }
    }
    rt_print_string("\n--- Method Chaining ---\n");
    char * messy = rt_to_string_string(__arena_1__, "  HELLO WORLD  ");
    char * clean = ({ char *_obj_tmp = rt_str_trim(__arena_1__, messy); char *_res = rt_str_toLower(__arena_1__, _obj_tmp); _res; });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Original: \"", messy);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "trim().toLower(): \"", clean);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * chainTest = ({ char *_obj_tmp = rt_str_trim(__arena_1__, "  TEST  "); char *_res = rt_str_toUpper(__arena_1__, _obj_tmp); _res; });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Chained on literal: \"", chainTest);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- splitWhitespace ---\n");
    char * wsText = rt_to_string_string(__arena_1__, "hello   world\tfoo\nbar");
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Original: \"", wsText);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * * wsWords = rt_str_split_whitespace(__arena_1__, wsText);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(wsWords));
        char *_r = rt_str_concat(__arena_1__, "splitWhitespace() -> ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " words:\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    {
        char * * __arr_7__ = wsWords;
        long __len_7__ = rt_array_length(__arr_7__);
        for (long __idx_7__ = 0; __idx_7__ < __len_7__; __idx_7__++) {
            RtArena *__loop_arena_31__ = rt_arena_create(__arena_1__);
            char * wsWord = __arr_7__[__idx_7__];
            {
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__loop_arena_31__, "  - \"", wsWord);
        _r = rt_str_concat(__loop_arena_31__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_31__:
            rt_arena_destroy(__loop_arena_31__);
        }
    }
    rt_print_string("\n--- splitLines ---\n");
    char * multiLine = rt_to_string_string(__arena_1__, "Line 1\nLine 2\nLine 3");
    rt_print_string("Original (3 lines with \\n):\n");
    char * * lineArr = rt_str_split_lines(__arena_1__, multiLine);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(lineArr));
        char *_r = rt_str_concat(__arena_1__, "splitLines() -> ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " lines:\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    {
        char * * __arr_8__ = lineArr;
        long __len_8__ = rt_array_length(__arr_8__);
        for (long __idx_8__ = 0; __idx_8__ < __len_8__; __idx_8__++) {
            RtArena *__loop_arena_32__ = rt_arena_create(__arena_1__);
            char * ln = __arr_8__[__idx_8__];
            {
                ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__loop_arena_32__, "  \"", ln);
        _r = rt_str_concat(__loop_arena_32__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_32__:
            rt_arena_destroy(__loop_arena_32__);
        }
    }
    rt_print_string("\n--- isBlank ---\n");
    char * blankEmpty = rt_to_string_string(__arena_1__, "");
    char * blankSpaces = rt_to_string_string(__arena_1__, "   ");
    char * blankTabs = rt_to_string_string(__arena_1__, "\t\t");
    char * notBlank = rt_to_string_string(__arena_1__, "hello");
    char * notBlank2 = rt_to_string_string(__arena_1__, "  hi  ");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_is_blank(blankEmpty));
        char *_r = rt_str_concat(__arena_1__, "\"\" isBlank: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_is_blank(blankSpaces));
        char *_r = rt_str_concat(__arena_1__, "\"   \" isBlank: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_is_blank(blankTabs));
        char *_r = rt_str_concat(__arena_1__, "\"\\t\\t\" isBlank: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_is_blank(notBlank));
        char *_r = rt_str_concat(__arena_1__, "\"hello\" isBlank: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_str_is_blank(notBlank2));
        char *_r = rt_str_concat(__arena_1__, "\"  hi  \" isBlank: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto demo_strings_return;
demo_strings_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_functions() {
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                       Sindarin Functions                         │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    rt_print_string("--- Basic Functions ---\n");
    greet();
    rt_print_string("\n--- Parameters ---\n");
    greet_person("Alice");
    greet_person("Bob");
    print_sum(5L, 3L);
    print_sum(10L, 20L);
    rt_print_string("\n--- Return Values ---\n");
    rt_print_string("See main.sn for return value examples\n");
    rt_print_string("\n--- Recursion Example ---\n");
    rt_print_string("factorial(5) = 120\n");
    rt_print_string("fibonacci sequence: 0, 1, 1, 2, 3, 5, 8...\n");
    goto demo_functions_return;
demo_functions_return:
    return;
}

void greet() {
    rt_print_string("Hello from greet()!\n");
    goto greet_return;
greet_return:
    return;
}

void greet_person(char * name) {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Hello, ", name);
        _r = rt_str_concat(__arena_1__, _r, "!\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto greet_person_return;
greet_person_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void print_sum(long a, long b) {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    long sum = rt_add_long(a, b);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, a);
        char *_p1 = rt_to_string_long(__arena_1__, b);
        char *_p2 = rt_to_string_long(__arena_1__, sum);
        char *_r = rt_str_concat(__arena_1__, _p0, " + ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, " = ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto print_sum_return;
print_sum_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_arrays() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                        Sindarin Arrays                           │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    rt_print_string("--- Declaration ---\n");
    long * numbers = rt_array_create_long(__arena_1__, 5, (long[]){10L, 20L, 30L, 40L, 50L});
    rt_print_string("numbers = ");
    rt_print_array_long(numbers);
    rt_print_string("\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(numbers));
        char *_r = rt_str_concat(__arena_1__, "len(numbers) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(numbers));
        char *_r = rt_str_concat(__arena_1__, "numbers.length = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, numbers[0L]);
        char *_r = rt_str_concat(__arena_1__, "numbers[0] = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, numbers[2L]);
        char *_r = rt_str_concat(__arena_1__, "numbers[2] = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Push and Pop ---\n");
    long * arr = rt_array_create_long(__arena_1__, 0, (long[]){});
    rt_print_string("Starting with empty array: ");
    rt_print_array_long(arr);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(arr));
        char *_r = rt_str_concat(__arena_1__, " (length = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ")\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    (arr = rt_array_push_long(__arena_1__, arr, 10L));
    (arr = rt_array_push_long(__arena_1__, arr, 20L));
    (arr = rt_array_push_long(__arena_1__, arr, 30L));
    rt_print_string("After push(10), push(20), push(30): ");
    rt_print_array_long(arr);
    rt_print_string("\n");
    long popped = rt_array_pop_long(arr);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, popped);
        char *_r = rt_str_concat(__arena_1__, "pop() returned: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("After pop: ");
    rt_print_array_long(arr);
    rt_print_string("\n");
    rt_print_string("\n--- Insert and Remove ---\n");
    long * items = rt_array_create_long(__arena_1__, 5, (long[]){1L, 2L, 3L, 4L, 5L});
    rt_print_string("Starting: ");
    rt_print_array_long(items);
    rt_print_string("\n");
    (items = rt_array_ins_long(__arena_1__, items, 99L, 2L));
    rt_print_string("After insert(99, 2): ");
    rt_print_array_long(items);
    rt_print_string("\n");
    (items = rt_array_rem_long(__arena_1__, items, 2L));
    rt_print_string("After remove(2): ");
    rt_print_array_long(items);
    rt_print_string("\n");
    rt_print_string("\n--- Reverse ---\n");
    long * nums = rt_array_create_long(__arena_1__, 5, (long[]){1L, 2L, 3L, 4L, 5L});
    rt_print_string("Before reverse: ");
    rt_print_array_long(nums);
    rt_print_string("\n");
    (nums = rt_array_rev_long(__arena_1__, nums));
    rt_print_string("After reverse(): ");
    rt_print_array_long(nums);
    rt_print_string("\n");
    rt_print_string("\n--- Clone ---\n");
    long * original = rt_array_create_long(__arena_1__, 3, (long[]){10L, 20L, 30L});
    long * copy = rt_array_clone_long(__arena_1__, original);
    rt_print_string("Original: ");
    rt_print_array_long(original);
    rt_print_string("\n");
    rt_print_string("Clone: ");
    rt_print_array_long(copy);
    rt_print_string("\n");
    (copy = rt_array_push_long(__arena_1__, copy, 40L));
    rt_print_string("After pushing 40 to clone:\n");
    rt_print_string("  Original: ");
    rt_print_array_long(original);
    rt_print_string("\n");
    rt_print_string("  Clone: ");
    rt_print_array_long(copy);
    rt_print_string("\n");
    rt_print_string("\n--- Concat ---\n");
    long * first = rt_array_create_long(__arena_1__, 3, (long[]){1L, 2L, 3L});
    long * second = rt_array_create_long(__arena_1__, 3, (long[]){4L, 5L, 6L});
    rt_print_string("First: ");
    rt_print_array_long(first);
    rt_print_string("\n");
    rt_print_string("Second: ");
    rt_print_array_long(second);
    rt_print_string("\n");
    long * combined = rt_array_concat_long(__arena_1__, first, second);
    rt_print_string("first.concat(second): ");
    rt_print_array_long(combined);
    rt_print_string("\n");
    rt_print_string("First after concat: ");
    rt_print_array_long(first);
    rt_print_string(" (unchanged)\n");
    rt_print_string("\n--- IndexOf and Contains ---\n");
    long * search = rt_array_create_long(__arena_1__, 5, (long[]){10L, 20L, 30L, 40L, 50L});
    rt_print_string("Array: ");
    rt_print_array_long(search);
    rt_print_string("\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_indexOf_long(search, 30L));
        char *_r = rt_str_concat(__arena_1__, "indexOf(30) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_indexOf_long(search, 99L));
        char *_r = rt_str_concat(__arena_1__, "indexOf(99) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_array_contains_long(search, 30L));
        char *_r = rt_str_concat(__arena_1__, "contains(30) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_array_contains_long(search, 99L));
        char *_r = rt_str_concat(__arena_1__, "contains(99) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Join ---\n");
    char * * words = rt_array_create_string(__arena_1__, 3, (char *[]){"apple", "banana", "cherry"});
    rt_print_string("Array: ");
    rt_print_array_string(words);
    rt_print_string("\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_array_join_string(__arena_1__, words, ", ");
        char *_r = rt_str_concat(__arena_1__, "join(\", \") = \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_array_join_string(__arena_1__, words, " - ");
        char *_r = rt_str_concat(__arena_1__, "join(\" - \") = \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long * digits = rt_array_create_long(__arena_1__, 5, (long[]){1L, 2L, 3L, 4L, 5L});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_array_join_long(__arena_1__, digits, "-");
        char *_r = rt_str_concat(__arena_1__, "Int array joined: \"", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Clear ---\n");
    long * toclear = rt_array_create_long(__arena_1__, 5, (long[]){1L, 2L, 3L, 4L, 5L});
    rt_print_string("Before clear: ");
    rt_print_array_long(toclear);
    rt_print_string("\n");
    rt_array_clear(toclear);
    rt_print_string("After clear(): ");
    rt_print_array_long(toclear);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(toclear));
        char *_r = rt_str_concat(__arena_1__, " (length = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ")\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Slicing ---\n");
    long * slicetest = rt_array_create_long(__arena_1__, 5, (long[]){10L, 20L, 30L, 40L, 50L});
    rt_print_string("Array: ");
    rt_print_array_long(slicetest);
    rt_print_string("\n");
    long * s1 = rt_array_slice_long(__arena_1__, slicetest, 1L, 4L, LONG_MIN);
    rt_print_string("arr[1..4] = ");
    rt_print_array_long(s1);
    rt_print_string("\n");
    long * s2 = rt_array_slice_long(__arena_1__, slicetest, LONG_MIN, 3L, LONG_MIN);
    rt_print_string("arr[..3] = ");
    rt_print_array_long(s2);
    rt_print_string("\n");
    long * s3 = rt_array_slice_long(__arena_1__, slicetest, 2L, LONG_MIN, LONG_MIN);
    rt_print_string("arr[2..] = ");
    rt_print_array_long(s3);
    rt_print_string("\n");
    long * s4 = rt_array_slice_long(__arena_1__, slicetest, LONG_MIN, LONG_MIN, LONG_MIN);
    rt_print_string("arr[..] (full copy) = ");
    rt_print_array_long(s4);
    rt_print_string("\n");
    rt_print_string("\n--- Step Slicing ---\n");
    long * steptest = rt_array_create_long(__arena_1__, 10, (long[]){0L, 1L, 2L, 3L, 4L, 5L, 6L, 7L, 8L, 9L});
    rt_print_string("Array: ");
    rt_print_array_long(steptest);
    rt_print_string("\n");
    long * evens = rt_array_slice_long(__arena_1__, steptest, LONG_MIN, LONG_MIN, 2L);
    rt_print_string("arr[..:2] (every 2nd) = ");
    rt_print_array_long(evens);
    rt_print_string("\n");
    long * odds = rt_array_slice_long(__arena_1__, steptest, 1L, LONG_MIN, 2L);
    rt_print_string("arr[1..:2] (odds) = ");
    rt_print_array_long(odds);
    rt_print_string("\n");
    long * thirds = rt_array_slice_long(__arena_1__, steptest, LONG_MIN, LONG_MIN, 3L);
    rt_print_string("arr[..:3] (every 3rd) = ");
    rt_print_array_long(thirds);
    rt_print_string("\n");
    rt_print_string("\n--- Negative Indexing ---\n");
    long * negtest = rt_array_create_long(__arena_1__, 5, (long[]){10L, 20L, 30L, 40L, 50L});
    rt_print_string("Array: ");
    rt_print_array_long(negtest);
    rt_print_string("\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, negtest[(-1L) < 0 ? rt_array_length(negtest) + (-1L) : (-1L)]);
        char *_r = rt_str_concat(__arena_1__, "arr[-1] = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, negtest[(-2L) < 0 ? rt_array_length(negtest) + (-2L) : (-2L)]);
        char *_r = rt_str_concat(__arena_1__, "arr[-2] = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, negtest[(-3L) < 0 ? rt_array_length(negtest) + (-3L) : (-3L)]);
        char *_r = rt_str_concat(__arena_1__, "arr[-3] = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long * lasttwo = rt_array_slice_long(__arena_1__, negtest, -2L, LONG_MIN, LONG_MIN);
    rt_print_string("arr[-2..] = ");
    rt_print_array_long(lasttwo);
    rt_print_string("\n");
    long * notlast = rt_array_slice_long(__arena_1__, negtest, LONG_MIN, -1L, LONG_MIN);
    rt_print_string("arr[..-1] = ");
    rt_print_array_long(notlast);
    rt_print_string("\n");
    rt_print_string("\n--- For-Each Iteration ---\n");
    long * iterate = rt_array_create_long(__arena_1__, 3, (long[]){10L, 20L, 30L});
    rt_print_string("Iterating over ");
    rt_print_array_long(iterate);
    rt_print_string(":\n");
    {
        long * __arr_9__ = iterate;
        long __len_9__ = rt_array_length(__arr_9__);
        for (long __idx_9__ = 0; __idx_9__ < __len_9__; __idx_9__++) {
            RtArena *__loop_arena_33__ = rt_arena_create(__arena_1__);
            long x = __arr_9__[__idx_9__];
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_33__, x);
        char *_r = rt_str_concat(__loop_arena_33__, "  value: ", _p0);
        _r = rt_str_concat(__loop_arena_33__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_33__:
            rt_arena_destroy(__loop_arena_33__);
        }
    }
    long sum = 0L;
    {
        long * __arr_10__ = iterate;
        long __len_10__ = rt_array_length(__arr_10__);
        for (long __idx_10__ = 0; __idx_10__ < __len_10__; __idx_10__++) {
            RtArena *__loop_arena_34__ = rt_arena_create(__arena_1__);
            long n = __arr_10__[__idx_10__];
            {
                (sum = rt_add_long(sum, n));
            }
        __loop_cleanup_34__:
            rt_arena_destroy(__loop_arena_34__);
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, sum);
        char *_r = rt_str_concat(__arena_1__, "Sum = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Array Equality ---\n");
    long * eq1 = rt_array_create_long(__arena_1__, 3, (long[]){1L, 2L, 3L});
    long * eq2 = rt_array_create_long(__arena_1__, 3, (long[]){1L, 2L, 3L});
    long * eq3 = rt_array_create_long(__arena_1__, 3, (long[]){1L, 2L, 4L});
    long * eq4 = rt_array_create_long(__arena_1__, 2, (long[]){1L, 2L});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_array_eq_long(eq1, eq2));
        char *_r = rt_str_concat(__arena_1__, "{1,2,3} == {1,2,3}: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_array_eq_long(eq1, eq3));
        char *_r = rt_str_concat(__arena_1__, "{1,2,3} == {1,2,4}: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_array_eq_long(eq1, eq4));
        char *_r = rt_str_concat(__arena_1__, "{1,2,3} == {1,2}: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, (!rt_array_eq_long(eq1, eq3)));
        char *_r = rt_str_concat(__arena_1__, "{1,2,3} != {1,2,4}: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Range Literals ---\n");
    long * range1 = rt_array_range(__arena_1__, 1L, 6L);
    rt_print_string("1..6 = ");
    rt_print_array_long(range1);
    rt_print_string("\n");
    long * range2 = rt_array_range(__arena_1__, 0L, 10L);
    rt_print_string("0..10 = ");
    rt_print_array_long(range2);
    rt_print_string("\n");
    long * withRange = rt_array_concat_long(__arena_1__, rt_array_concat_long(__arena_1__, rt_array_create_long(__arena_1__, 1, (long[]){0L}), rt_array_range(__arena_1__, 1L, 4L)), rt_array_create_long(__arena_1__, 1, (long[]){10L}));
    rt_print_string("{0, 1..4, 10} = ");
    rt_print_array_long(withRange);
    rt_print_string("\n");
    long * multiRange = rt_array_concat_long(__arena_1__, rt_array_range(__arena_1__, 1L, 3L), rt_array_range(__arena_1__, 10L, 13L));
    rt_print_string("{1..3, 10..13} = ");
    rt_print_array_long(multiRange);
    rt_print_string("\n");
    rt_print_string("\n--- Spread Operator ---\n");
    long * source = rt_array_create_long(__arena_1__, 3, (long[]){1L, 2L, 3L});
    rt_print_string("source = ");
    rt_print_array_long(source);
    rt_print_string("\n");
    long * spreadCopy = rt_array_clone_long(__arena_1__, source);
    rt_print_string("{...source} = ");
    rt_print_array_long(spreadCopy);
    rt_print_string("\n");
    long * extended = rt_array_concat_long(__arena_1__, rt_array_concat_long(__arena_1__, rt_array_concat_long(__arena_1__, rt_array_create_long(__arena_1__, 1, (long[]){0L}), rt_array_clone_long(__arena_1__, source)), rt_array_create_long(__arena_1__, 1, (long[]){4L})), rt_array_create_long(__arena_1__, 1, (long[]){5L}));
    rt_print_string("{0, ...source, 4, 5} = ");
    rt_print_array_long(extended);
    rt_print_string("\n");
    long * arr_a = rt_array_create_long(__arena_1__, 2, (long[]){1L, 2L});
    long * arr_b = rt_array_create_long(__arena_1__, 2, (long[]){3L, 4L});
    long * merged = rt_array_concat_long(__arena_1__, rt_array_clone_long(__arena_1__, arr_a), rt_array_clone_long(__arena_1__, arr_b));
    rt_print_string("{...{1,2}, ...{3,4}} = ");
    rt_print_array_long(merged);
    rt_print_string("\n");
    long * mixed = rt_array_concat_long(__arena_1__, rt_array_clone_long(__arena_1__, source), rt_array_range(__arena_1__, 10L, 13L));
    rt_print_string("{...source, 10..13} = ");
    rt_print_array_long(mixed);
    rt_print_string("\n");
    rt_print_string("\n--- Different Array Types ---\n");
    double * doubles = rt_array_create_double(__arena_1__, 3, (double[]){1.5, 2.5, 3.5});
    rt_print_string("double[]: ");
    rt_print_array_double(doubles);
    rt_print_string("\n");
    char * chars = rt_array_create_char(__arena_1__, 5, (char[]){'H', 'e', 'l', 'l', 'o'});
    rt_print_string("char[]: ");
    rt_print_array_char(chars);
    rt_print_string("\n");
    int * bools = rt_array_create_bool(__arena_1__, 3, (int[]){1L, 0L, 1L});
    rt_print_string("bool[]: ");
    rt_print_array_bool(bools);
    rt_print_string("\n");
    char * * strings = rt_array_create_string(__arena_1__, 2, (char *[]){"hello", "world"});
    rt_print_string("str[]: ");
    rt_print_array_string(strings);
    rt_print_string("\n");
    goto demo_arrays_return;
demo_arrays_return:
    rt_arena_destroy(__arena_1__);
    return;
}

long add_numbers(RtArena *__caller_arena__, long a, long b) {
    long _return_value = 0;
    _return_value = rt_add_long(a, b);
    goto add_numbers_return;
add_numbers_return:
    return _return_value;
}

long compute_sum() {
    long _return_value = 0;
    long sum = 0L;
    {
        long i = 1L;
        while (rt_le_long(i, 10L)) {
            {
                (sum = rt_add_long(sum, i));
            }
        __for_continue_35__:;
            rt_post_inc_long(&i);
        }
    }
    _return_value = sum;
    goto compute_sum_return;
compute_sum_return:
    return _return_value;
}

void demo_memory() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                   Sindarin Memory Management                     │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    rt_print_string("--- Shared Functions ---\n");
    rt_print_string("Shared functions use the caller's arena (efficient for helpers)\n");
    long result = add_numbers(__arena_1__, 10L, 20L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, result);
        char *_r = rt_str_concat(__arena_1__, "add_numbers(10, 20) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Private Functions ---\n");
    rt_print_string("Private functions have isolated arenas (safe for temporary work)\n");
    long sum = compute_sum();
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, sum);
        char *_r = rt_str_concat(__arena_1__, "compute_sum() = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Shared Blocks ---\n");
    rt_print_string("Shared blocks use the parent's arena\n");
    long x = 10L;
    {
        long y = 20L;
        (x = rt_add_long(x, y));
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, x);
        char *_r = rt_str_concat(__arena_1__, "After shared block: x = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Private Blocks ---\n");
    rt_print_string("Private blocks have isolated arenas (only primitives escape)\n");
    long computed = 0L;
    {
        RtArena *__arena_2__ = rt_arena_create(NULL);
        long a = 100L;
        long b = 200L;
        (computed = rt_add_long(a, b));
        rt_arena_destroy(__arena_2__);
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, computed);
        char *_r = rt_str_concat(__arena_1__, "After private block: computed = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Shared Loops ---\n");
    rt_print_string("Shared loops don't create per-iteration arenas\n");
    long total = 0L;
    {
        long i = 0L;
        while (rt_lt_long(i, 5L)) {
            {
                (total = rt_add_long(total, i));
            }
        __for_continue_36__:;
            rt_post_inc_long(&i);
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, total);
        char *_r = rt_str_concat(__arena_1__, "Sum from shared for: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long count = 0L;
    while (rt_lt_long(count, 3L)) {
        {
            (count = rt_add_long(count, 1L));
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, count);
        char *_r = rt_str_concat(__arena_1__, "Count from shared while: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long * arr = rt_array_create_long(__arena_1__, 5, (long[]){1L, 2L, 3L, 4L, 5L});
    long arrSum = 0L;
    {
        long * __arr_11__ = arr;
        long __len_11__ = rt_array_length(__arr_11__);
        for (long __idx_11__ = 0; __idx_11__ < __len_11__; __idx_11__++) {
            long n = __arr_11__[__idx_11__];
            {
                (arrSum = rt_add_long(arrSum, n));
            }
        }
    }
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, arrSum);
        char *_r = rt_str_concat(__arena_1__, "Sum from shared for-each: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- as val (Copy Semantics) ---\n");
    rt_print_string("'as val' creates independent copies of arrays/strings\n");
    long * original = rt_array_create_long(__arena_1__, 3, (long[]){10L, 20L, 30L});
    long * copy = rt_array_clone_long(__arena_1__, original);
    (original = rt_array_push_long(__arena_1__, original, 40L));
    rt_print_string("Original after push(40): ");
    rt_print_array_long(original);
    rt_print_string("\n");
    rt_print_string("Copy (unchanged): ");
    rt_print_array_long(copy);
    rt_print_string("\n");
    rt_print_string("\n--- as ref (Reference Semantics) ---\n");
    rt_print_string("'as ref' allocates primitives on heap (for escaping scopes)\n");
    long *value = (long *)rt_arena_alloc(__arena_1__, sizeof(long));
    *value = 42L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, (*value));
        char *_r = rt_str_concat(__arena_1__, "value (as ref) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    (*value = 100L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, (*value));
        char *_r = rt_str_concat(__arena_1__, "modified value = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto demo_memory_return;
demo_memory_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_lambda() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                     Sindarin Lambda Expressions                  │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    rt_print_string("Explicit type annotations:\n");
    __Closure__ * double_it = ({
    __Closure__ *__cl__ = rt_arena_alloc(__arena_1__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_0__;
    __cl__->arena = __arena_1__;
    __cl__;
});
    long result = ((long (*)(void *, long))double_it->fn)(double_it, 5L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, result);
        char *_r = rt_str_concat(__arena_1__, "  double_it(5) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    __Closure__ * add = ({
    __Closure__ *__cl__ = rt_arena_alloc(__arena_1__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_1__;
    __cl__->arena = __arena_1__;
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, ((long (*)(void *, long, long))add->fn)(add, 3L, 4L));
        char *_r = rt_str_concat(__arena_1__, "  add(3, 4) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nType inference (types inferred from declaration):\n");
    __Closure__ * triple = ({
    __Closure__ *__cl__ = rt_arena_alloc(__arena_1__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_2__;
    __cl__->arena = __arena_1__;
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, ((long (*)(void *, long))triple->fn)(triple, 7L));
        char *_r = rt_str_concat(__arena_1__, "  triple(7) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    __Closure__ * multiply = ({
    __Closure__ *__cl__ = rt_arena_alloc(__arena_1__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_3__;
    __cl__->arena = __arena_1__;
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, ((long (*)(void *, long, long))multiply->fn)(multiply, 6L, 8L));
        char *_r = rt_str_concat(__arena_1__, "  multiply(6, 8) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    __Closure__ * square = ({
    __Closure__ *__cl__ = rt_arena_alloc(__arena_1__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_4__;
    __cl__->arena = __arena_1__;
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, ((long (*)(void *, long))square->fn)(square, 9L));
        char *_r = rt_str_concat(__arena_1__, "  square(9) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    __Closure__ * negate = ({
    __Closure__ *__cl__ = rt_arena_alloc(__arena_1__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_5__;
    __cl__->arena = __arena_1__;
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, ((long (*)(void *, long))negate->fn)(negate, 42L));
        char *_r = rt_str_concat(__arena_1__, "  negate(42) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nLambdas with modifiers:\n");
    __Closure__ * increment = ({
    __Closure__ *__cl__ = rt_arena_alloc(__arena_1__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_6__;
    __cl__->arena = __arena_1__;
    __cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, ((long (*)(void *, long))increment->fn)(increment, 99L));
        char *_r = rt_str_concat(__arena_1__, "  increment(99) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nCombining lambdas:\n");
    long *x = (long *)rt_arena_alloc(__arena_1__, sizeof(long));
    *x = ((long (*)(void *, long))double_it->fn)(double_it, ((long (*)(void *, long, long))add->fn)(add, 1L, 2L));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, (*x));
        char *_r = rt_str_concat(__arena_1__, "  double_it(add(1, 2)) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long y = ((long (*)(void *, long))triple->fn)(triple, ((long (*)(void *, long, long))multiply->fn)(multiply, 2L, 3L));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, y);
        char *_r = rt_str_concat(__arena_1__, "  triple(multiply(2, 3)) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto demo_lambda_return;
demo_lambda_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_closure() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                        Sindarin Closures                         │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    long *multiplier = (long *)rt_arena_alloc(__arena_1__, sizeof(long));
    *multiplier = 3L;
    __Closure__ * times_three = ({
    __closure_7__ *__cl__ = rt_arena_alloc(__arena_1__, sizeof(__closure_7__));
    __cl__->fn = (void *)__lambda_7__;
    __cl__->arena = __arena_1__;
    __cl__->multiplier = multiplier;
    (__Closure__ *)__cl__;
});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, ((long (*)(void *, long))times_three->fn)(times_three, 5L));
        char *_r = rt_str_concat(__arena_1__, "times_three(5) = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto demo_closure_return;
demo_closure_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_bytes() {
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                       Sindarin Byte Type                         │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    show_byte_basics();
    show_byte_values();
    show_byte_conversions();
    show_byte_arrays();
    goto demo_bytes_return;
demo_bytes_return:
    return;
}

void show_byte_basics() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 1. Byte Basics ---\n");
    unsigned char zero = 0L;
    unsigned char mid = 128L;
    unsigned char max = 255L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, zero);
        char *_r = rt_str_concat(__arena_1__, "zero = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, mid);
        char *_r = rt_str_concat(__arena_1__, "mid = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, max);
        char *_r = rt_str_concat(__arena_1__, "max = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nByte comparisons:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_lt_long(zero, mid));
        char *_r = rt_str_concat(__arena_1__, "  0 < 128: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_lt_long(mid, max));
        char *_r = rt_str_concat(__arena_1__, "  128 < 255: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_eq_long(max, max));
        char *_r = rt_str_concat(__arena_1__, "  255 == 255: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    unsigned char a = 100L;
    unsigned char b = 100L;
    unsigned char c = 200L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_eq_long(a, b));
        char *_r = rt_str_concat(__arena_1__, "\n  a(100) == b(100): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_ne_long(a, c));
        char *_r = rt_str_concat(__arena_1__, "  a(100) != c(200): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n");
    goto show_byte_basics_return;
show_byte_basics_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_byte_values() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 2. Byte Values ---\n");
    rt_print_string("Range values:\n");
    unsigned char dec0 = 0L;
    unsigned char dec127 = 127L;
    unsigned char dec128 = 128L;
    unsigned char dec255 = 255L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, dec0);
        char *_r = rt_str_concat(__arena_1__, "  byte 0 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, dec127);
        char *_r = rt_str_concat(__arena_1__, "  byte 127 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, dec128);
        char *_r = rt_str_concat(__arena_1__, "  byte 128 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, dec255);
        char *_r = rt_str_concat(__arena_1__, "  byte 255 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nCommon ASCII values:\n");
    unsigned char nullByte = 0L;
    unsigned char space = 32L;
    unsigned char letterA = 65L;
    unsigned char letterZ = 90L;
    unsigned char letterALower = 97L;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, nullByte);
        char *_r = rt_str_concat(__arena_1__, "  NULL = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, space);
        char *_r = rt_str_concat(__arena_1__, "  Space = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, letterA);
        char *_r = rt_str_concat(__arena_1__, "  'A' = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, letterZ);
        char *_r = rt_str_concat(__arena_1__, "  'Z' = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, letterALower);
        char *_r = rt_str_concat(__arena_1__, "  'a' = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n");
    goto show_byte_values_return;
show_byte_values_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_byte_conversions() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 3. Byte Conversions ---\n");
    rt_print_string("Byte to int (implicit):\n");
    unsigned char b1 = 42L;
    long i1 = b1;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, i1);
        char *_r = rt_str_concat(__arena_1__, "  byte 42 -> int: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    unsigned char b2 = 255L;
    long i2 = b2;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, i2);
        char *_r = rt_str_concat(__arena_1__, "  byte 255 -> int: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nArithmetic with bytes:\n");
    unsigned char x = 100L;
    unsigned char y = 50L;
    long sum = rt_add_long(x, y);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, x);
        char *_p1 = rt_to_string_byte(__arena_1__, y);
        char *_p2 = rt_to_string_long(__arena_1__, sum);
        char *_r = rt_str_concat(__arena_1__, "  ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " + ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, " = ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long diff = rt_sub_long(x, y);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, x);
        char *_p1 = rt_to_string_byte(__arena_1__, y);
        char *_p2 = rt_to_string_long(__arena_1__, diff);
        char *_r = rt_str_concat(__arena_1__, "  ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " - ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, " = ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nLarge results:\n");
    unsigned char big1 = 200L;
    unsigned char big2 = 200L;
    long bigSum = rt_add_long(big1, big2);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, bigSum);
        char *_r = rt_str_concat(__arena_1__, "  200 + 200 = ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " (exceeds 255, int handles it)\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n");
    goto show_byte_conversions_return;
show_byte_conversions_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void show_byte_arrays() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 4. Byte Arrays ---\n");
    rt_print_string("Creating byte arrays:\n");
    unsigned char * data = rt_array_create_byte(__arena_1__, 5, (unsigned char[]){72L, 101L, 108L, 108L, 111L});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(data));
        char *_r = rt_str_concat(__arena_1__, "  Array length: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("  Contents (ASCII for 'Hello'):\n");
    {
        long * __arr_12__ = rt_array_range(__arena_1__, 0L, rt_array_length(data));
        long __len_12__ = rt_array_length(__arr_12__);
        for (long __idx_12__ = 0; __idx_12__ < __len_12__; __idx_12__++) {
            RtArena *__loop_arena_37__ = rt_arena_create(__arena_1__);
            long i = __arr_12__[__idx_12__];
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_37__, i);
        char *_p1 = rt_to_string_byte(__loop_arena_37__, data[(i) < 0 ? rt_array_length(data) + (i) : (i)]);
        char *_r = rt_str_concat(__loop_arena_37__, "    [", _p0);
        _r = rt_str_concat(__loop_arena_37__, _r, "] = ");
        _r = rt_str_concat(__loop_arena_37__, _r, _p1);
        _r = rt_str_concat(__loop_arena_37__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_37__:
            rt_arena_destroy(__loop_arena_37__);
        }
    }
    rt_print_string("\nModifying byte array:\n");
    (data[0L] = 74L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, data[0L]);
        char *_r = rt_str_concat(__arena_1__, "  Changed first byte to 74 (J): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nByte array from decimal:\n");
    unsigned char * nums = rt_array_create_byte(__arena_1__, 5, (unsigned char[]){0L, 64L, 128L, 192L, 255L});
    rt_print_string("  Values: ");
    {
        long * __arr_13__ = rt_array_range(__arena_1__, 0L, rt_array_length(nums));
        long __len_13__ = rt_array_length(__arr_13__);
        for (long __idx_13__ = 0; __idx_13__ < __len_13__; __idx_13__++) {
            RtArena *__loop_arena_38__ = rt_arena_create(__arena_1__);
            long i = __arr_13__[__idx_13__];
            {
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__loop_arena_38__, nums[(i) < 0 ? rt_array_length(nums) + (i) : (i)]);
        rt_str_concat(__loop_arena_38__, _p0, " ");
    });
        rt_print_string(_str_arg0);
    });
            }
        __loop_cleanup_38__:
            rt_arena_destroy(__loop_arena_38__);
        }
    }
    rt_print_string("\n");
    rt_print_string("\nByte array conversions:\n");
    unsigned char * hello = rt_array_create_byte(__arena_1__, 5, (unsigned char[]){72L, 101L, 108L, 108L, 111L});
    char * helloStr = rt_byte_array_to_string(__arena_1__, hello);
    char * helloHex = rt_byte_array_to_hex(__arena_1__, hello);
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "  toString(): \"", helloStr);
        _r = rt_str_concat(__arena_1__, _r, "\"\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "  toHex(): ", helloHex);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n");
    goto show_byte_arrays_return;
show_byte_arrays_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_fileio() {
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                       Sindarin File I/O                          │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    demo_textfile();
    demo_binaryfile();
    demo_file_utilities();
    goto demo_fileio_return;
demo_fileio_return:
    return;
}

void demo_textfile() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 1. TextFile Operations ---\n");
    rt_print_string("Writing entire content at once:\n");
    rt_text_file_write_all("/tmp/sindarin_demo.txt", "Hello from Sindarin!\nLine 2\nLine 3");
    rt_print_string("  Wrote 3 lines to /tmp/sindarin_demo.txt\n");
    rt_print_string("\nReading entire file at once:\n");
    char * content = rt_text_file_read_all(__arena_1__, "/tmp/sindarin_demo.txt");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_str_length(content));
        char *_r = rt_str_concat(__arena_1__, "  Content length: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " characters\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nReading the file line by line:\n");
    RtTextFile * reader = rt_text_file_open(__arena_1__, "/tmp/sindarin_demo.txt");
    long lineNum = 1L;
    while (rt_not_bool(rt_text_file_is_eof(reader))) {
        RtArena *__loop_arena_39__ = rt_arena_create(__arena_1__);
        {
            char * line = rt_text_file_read_line(__loop_arena_39__, reader);
            if (rt_gt_long(rt_str_length(line), 0L)) {
                {
                    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__loop_arena_39__, lineNum);
        char *_r = rt_str_concat(__loop_arena_39__, "  Line ", _p0);
        _r = rt_str_concat(__loop_arena_39__, _r, ": ");
        _r = rt_str_concat(__loop_arena_39__, _r, line);
        _r = rt_str_concat(__loop_arena_39__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
                    rt_post_inc_long(&lineNum);
                }
            }
        }
    __loop_cleanup_39__:
        rt_arena_destroy(__loop_arena_39__);
    }
    rt_text_file_close(reader);
    rt_print_string("\nReading all lines into array:\n");
    RtTextFile * reader2 = rt_text_file_open(__arena_1__, "/tmp/sindarin_demo.txt");
    char * * lines = rt_text_file_read_lines(__arena_1__, reader2);
    rt_text_file_close(reader2);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(lines));
        char *_r = rt_str_concat(__arena_1__, "  Got ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " lines\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nFile existence:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_text_file_exists("/tmp/sindarin_demo.txt"));
        char *_r = rt_str_concat(__arena_1__, "  /tmp/sindarin_demo.txt exists: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_text_file_exists("/tmp/nonexistent.txt"));
        char *_r = rt_str_concat(__arena_1__, "  /tmp/nonexistent.txt exists: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_text_file_delete("/tmp/sindarin_demo.txt");
    rt_print_string("\n");
    goto demo_textfile_return;
demo_textfile_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_binaryfile() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 2. BinaryFile Operations ---\n");
    rt_print_string("Writing bytes:\n");
    unsigned char * bytes = rt_array_create_byte(__arena_1__, 4, (unsigned char[]){255L, 66L, 0L, 171L});
    rt_binary_file_write_all("/tmp/sindarin_demo.bin", bytes);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(bytes));
        char *_r = rt_str_concat(__arena_1__, "  Wrote ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " bytes: 255, 66, 0, 171\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nReading binary file:\n");
    unsigned char * readBytes = rt_binary_file_read_all(__arena_1__, "/tmp/sindarin_demo.bin");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(readBytes));
        char *_r = rt_str_concat(__arena_1__, "  Read ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " bytes\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_byte(__arena_1__, readBytes[0L]);
        char *_p1 = rt_to_string_byte(__arena_1__, readBytes[1L]);
        char *_p2 = rt_to_string_byte(__arena_1__, readBytes[2L]);
        char *_p3 = rt_to_string_byte(__arena_1__, readBytes[3L]);
        char *_r = rt_str_concat(__arena_1__, "  Values: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ", ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, ", ");
        _r = rt_str_concat(__arena_1__, _r, _p2);
        _r = rt_str_concat(__arena_1__, _r, ", ");
        _r = rt_str_concat(__arena_1__, _r, _p3);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nReading byte by byte:\n");
    RtBinaryFile * reader = rt_binary_file_open(__arena_1__, "/tmp/sindarin_demo.bin");
    long b1 = rt_binary_file_read_byte(reader);
    long b2 = rt_binary_file_read_byte(reader);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, b1);
        char *_p1 = rt_to_string_long(__arena_1__, b2);
        char *_r = rt_str_concat(__arena_1__, "  First two bytes: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, ", ");
        _r = rt_str_concat(__arena_1__, _r, _p1);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_binary_file_close(reader);
    rt_print_string("\nBinary file existence:\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_binary_file_exists("/tmp/sindarin_demo.bin"));
        char *_r = rt_str_concat(__arena_1__, "  /tmp/sindarin_demo.bin exists: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_binary_file_delete("/tmp/sindarin_demo.bin");
    rt_print_string("\n");
    goto demo_binaryfile_return;
demo_binaryfile_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_file_utilities() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("--- 3. File Utilities ---\n");
    rt_print_string("Common file operations:\n");
    rt_text_file_write_all("/tmp/utility_test.txt", "Test content\nLine 2\nLine 3");
    char * path = rt_to_string_string(__arena_1__, "/tmp/utility_test.txt");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_text_file_exists(path));
        char *_r = rt_str_concat(__arena_1__, "  File exists: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    char * fileContent = rt_text_file_read_all(__arena_1__, path);
    char * * contentLines = rt_str_split_lines(__arena_1__, fileContent);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_array_length(contentLines));
        char *_r = rt_str_concat(__arena_1__, "  Number of lines: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\nCopy and move:\n");
    rt_text_file_copy(path, "/tmp/utility_copy.txt");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_text_file_exists("/tmp/utility_copy.txt"));
        char *_r = rt_str_concat(__arena_1__, "  Copied file exists: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_text_file_move("/tmp/utility_copy.txt", "/tmp/utility_moved.txt");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_text_file_exists("/tmp/utility_copy.txt"));
        char *_r = rt_str_concat(__arena_1__, "  Original copy exists: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_text_file_exists("/tmp/utility_moved.txt"));
        char *_r = rt_str_concat(__arena_1__, "  Moved file exists: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_text_file_delete("/tmp/utility_test.txt");
    rt_text_file_delete("/tmp/utility_moved.txt");
    rt_print_string("\n");
    goto demo_file_utilities_return;
demo_file_utilities_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_date() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                         Sindarin Date                            │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    rt_print_string("--- Creating Dates ---\n");
    RtDate * today = rt_date_today(__arena_1__);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, today);
        char *_r = rt_str_concat(__arena_1__, "Today: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * christmas = rt_date_from_ymd(__arena_1__, 2025L, 12L, 25L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, christmas);
        char *_r = rt_str_concat(__arena_1__, "Christmas: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * parsed = rt_date_from_string(__arena_1__, "2025-07-04");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, parsed);
        char *_r = rt_str_concat(__arena_1__, "Parsed: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * fromEpoch = rt_date_from_epoch_days(__arena_1__, 20088L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, fromEpoch);
        char *_r = rt_str_concat(__arena_1__, "From epoch days: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Date Components ---\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_get_year(today));
        char *_r = rt_str_concat(__arena_1__, "Year: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_get_month(today));
        char *_r = rt_str_concat(__arena_1__, "Month: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_get_day(today));
        char *_r = rt_str_concat(__arena_1__, "Day: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_get_weekday(today));
        char *_r = rt_str_concat(__arena_1__, "Weekday: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_get_day_of_year(today));
        char *_r = rt_str_concat(__arena_1__, "Day of year: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_get_epoch_days(today));
        char *_r = rt_str_concat(__arena_1__, "Epoch days: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Weekday Names ---\n");
    char * * names = rt_array_create_string(__arena_1__, 7, (char *[]){"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"});
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Today is ", names[(rt_date_get_weekday(today)) < 0 ? rt_array_length(names) + (rt_date_get_weekday(today)) : (rt_date_get_weekday(today))]);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Formatting ---\n");
    RtDate * d = rt_date_from_ymd(__arena_1__, 2025L, 12L, 25L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, d);
        char *_r = rt_str_concat(__arena_1__, "ISO: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_string(__arena_1__, d);
        char *_r = rt_str_concat(__arena_1__, "toString: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_format(__arena_1__, d, "YYYY-MM-DD");
        char *_r = rt_str_concat(__arena_1__, "YYYY-MM-DD: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_format(__arena_1__, d, "M/D/YYYY");
        char *_r = rt_str_concat(__arena_1__, "M/D/YYYY: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_format(__arena_1__, d, "DD/MM/YYYY");
        char *_r = rt_str_concat(__arena_1__, "DD/MM/YYYY: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_format(__arena_1__, d, "MMMM D, YYYY");
        char *_r = rt_str_concat(__arena_1__, "MMMM D, YYYY: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_format(__arena_1__, d, "ddd, MMM D");
        char *_r = rt_str_concat(__arena_1__, "ddd, MMM D: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Date Arithmetic ---\n");
    RtDate * start = rt_date_from_ymd(__arena_1__, 2025L, 1L, 15L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, start);
        char *_r = rt_str_concat(__arena_1__, "Start: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_add_days(__arena_1__, start, 10L));
        char *_r = rt_str_concat(__arena_1__, "addDays(10): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_add_days(__arena_1__, start, -5L));
        char *_r = rt_str_concat(__arena_1__, "addDays(-5): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_add_weeks(__arena_1__, start, 2L));
        char *_r = rt_str_concat(__arena_1__, "addWeeks(2): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_add_months(__arena_1__, start, 3L));
        char *_r = rt_str_concat(__arena_1__, "addMonths(3): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_add_years(__arena_1__, start, 1L));
        char *_r = rt_str_concat(__arena_1__, "addYears(1): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Month Boundaries ---\n");
    RtDate * jan31 = rt_date_from_ymd(__arena_1__, 2025L, 1L, 31L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, jan31);
        char *_r = rt_str_concat(__arena_1__, "Jan 31: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_add_months(__arena_1__, jan31, 1L));
        char *_r = rt_str_concat(__arena_1__, "addMonths(1): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * leapDay = rt_date_from_ymd(__arena_1__, 2024L, 2L, 29L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, leapDay);
        char *_r = rt_str_concat(__arena_1__, "Leap day 2024: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_add_years(__arena_1__, leapDay, 1L));
        char *_r = rt_str_concat(__arena_1__, "addYears(1): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Date Differences ---\n");
    RtDate * d1 = rt_date_from_ymd(__arena_1__, 2025L, 1L, 1L);
    RtDate * d2 = rt_date_from_ymd(__arena_1__, 2025L, 12L, 31L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_diff_days(d2, d1));
        char *_r = rt_str_concat(__arena_1__, "Days in 2025: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * birthday = rt_date_from_ymd(__arena_1__, 2025L, 6L, 15L);
    long daysUntil = rt_date_diff_days(birthday, today);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, daysUntil);
        char *_r = rt_str_concat(__arena_1__, "Days until Jun 15: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Start/End Methods ---\n");
    RtDate * mid = rt_date_from_ymd(__arena_1__, 2025L, 6L, 15L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, mid);
        char *_r = rt_str_concat(__arena_1__, "Date: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_start_of_month(__arena_1__, mid));
        char *_r = rt_str_concat(__arena_1__, "startOfMonth: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_end_of_month(__arena_1__, mid));
        char *_r = rt_str_concat(__arena_1__, "endOfMonth: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_start_of_year(__arena_1__, mid));
        char *_r = rt_str_concat(__arena_1__, "startOfYear: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, rt_date_end_of_year(__arena_1__, mid));
        char *_r = rt_str_concat(__arena_1__, "endOfYear: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Comparisons ---\n");
    RtDate * earlier = rt_date_from_ymd(__arena_1__, 2025L, 1L, 1L);
    RtDate * later = rt_date_from_ymd(__arena_1__, 2025L, 12L, 31L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_date_is_before(earlier, later));
        char *_r = rt_str_concat(__arena_1__, "Jan 1 isBefore Dec 31: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_date_is_after(later, earlier));
        char *_r = rt_str_concat(__arena_1__, "Dec 31 isAfter Jan 1: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * same1 = rt_date_from_ymd(__arena_1__, 2025L, 6L, 15L);
    RtDate * same2 = rt_date_from_string(__arena_1__, "2025-06-15");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_date_equals(same1, same2));
        char *_r = rt_str_concat(__arena_1__, "equals: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Weekend/Weekday ---\n");
    if (rt_date_is_weekend(today)) {
        {
            rt_print_string("Today is a weekend!\n");
        }
    }
    else {
        {
            rt_print_string("Today is a weekday\n");
        }
    }
    rt_print_string("\n--- Leap Year & Days in Month ---\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_date_is_leap_year(2024L));
        char *_r = rt_str_concat(__arena_1__, "2024 is leap year: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_date_is_leap_year(2025L));
        char *_r = rt_str_concat(__arena_1__, "2025 is leap year: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_days_in_month(2024L, 2L));
        char *_r = rt_str_concat(__arena_1__, "Days in Feb 2024: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_days_in_month(2025L, 2L));
        char *_r = rt_str_concat(__arena_1__, "Days in Feb 2025: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtDate * feb2024 = rt_date_from_ymd(__arena_1__, 2024L, 2L, 15L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_date_is_leap(feb2024));
        char *_r = rt_str_concat(__arena_1__, "Feb 2024 isLeapYear: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_date_get_days_in_month(feb2024));
        char *_r = rt_str_concat(__arena_1__, "Feb 2024 daysInMonth: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Conversion to Time ---\n");
    RtDate * dateOnly = rt_date_from_ymd(__arena_1__, 2025L, 6L, 15L);
    RtTime * asTime = rt_date_to_time(__arena_1__, dateOnly);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_date_to_iso(__arena_1__, dateOnly);
        char *_r = rt_str_concat(__arena_1__, "Date: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_to_iso(__arena_1__, asTime);
        char *_r = rt_str_concat(__arena_1__, "As Time: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto demo_date_return;
demo_date_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void demo_time() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("\n┌──────────────────────────────────────────────────────────────────┐\n");
    rt_print_string("│                         Sindarin Time                            │\n");
    rt_print_string("└──────────────────────────────────────────────────────────────────┘\n\n");
    rt_print_string("--- Creating Times ---\n");
    RtTime * now = rt_time_now(__arena_1__);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_to_iso(__arena_1__, now);
        char *_r = rt_str_concat(__arena_1__, "Now (local): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtTime * utc = rt_time_utc(__arena_1__);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_to_iso(__arena_1__, utc);
        char *_r = rt_str_concat(__arena_1__, "Now (UTC): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtTime * fromMs = rt_time_from_millis(__arena_1__, 1735500000000L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_to_iso(__arena_1__, fromMs);
        char *_r = rt_str_concat(__arena_1__, "From millis: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtTime * fromSec = rt_time_from_seconds(__arena_1__, 1735500000L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_to_iso(__arena_1__, fromSec);
        char *_r = rt_str_concat(__arena_1__, "From seconds: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Time Components ---\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_year(now));
        char *_r = rt_str_concat(__arena_1__, "Year: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_month(now));
        char *_r = rt_str_concat(__arena_1__, "Month: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_day(now));
        char *_r = rt_str_concat(__arena_1__, "Day: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_hour(now));
        char *_r = rt_str_concat(__arena_1__, "Hour: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_minute(now));
        char *_r = rt_str_concat(__arena_1__, "Minute: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_second(now));
        char *_r = rt_str_concat(__arena_1__, "Second: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_weekday(now));
        char *_r = rt_str_concat(__arena_1__, "Weekday: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_millis(now));
        char *_r = rt_str_concat(__arena_1__, "Millis since epoch: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_get_seconds(now));
        char *_r = rt_str_concat(__arena_1__, "Seconds since epoch: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Weekday Names ---\n");
    char * * names = rt_array_create_string(__arena_1__, 7, (char *[]){"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"});
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Today is ", names[(rt_time_get_weekday(now)) < 0 ? rt_array_length(names) + (rt_time_get_weekday(now)) : (rt_time_get_weekday(now))]);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Formatting ---\n");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_to_iso(__arena_1__, now);
        char *_r = rt_str_concat(__arena_1__, "ISO: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, now, "YYYY-MM-DD");
        char *_r = rt_str_concat(__arena_1__, "Date only: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, now, "HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "Time only: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, now, "YYYY-MM-DD");
        char *_r = rt_str_concat(__arena_1__, "YYYY-MM-DD: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, now, "HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "HH:mm:ss: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, now, "YYYY-MM-DD HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "YYYY-MM-DD HH:mm:ss: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, now, "M/D/YYYY");
        char *_r = rt_str_concat(__arena_1__, "M/D/YYYY: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, now, "h:mm A");
        char *_r = rt_str_concat(__arena_1__, "h:mm A: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, now, "h:mm:ss a");
        char *_r = rt_str_concat(__arena_1__, "h:mm:ss a: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Time Arithmetic ---\n");
    RtTime * base = rt_time_now(__arena_1__);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, base, "HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "Now: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, rt_time_add(__arena_1__, base, 5000L), "HH:mm:ss.SSS");
        char *_r = rt_str_concat(__arena_1__, "add(5000): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, rt_time_add_seconds(__arena_1__, base, 30L), "HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "addSeconds(30): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, rt_time_add_minutes(__arena_1__, base, 15L), "HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "addMinutes(15): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, rt_time_add_hours(__arena_1__, base, 2L), "HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "addHours(2): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, rt_time_add_days(__arena_1__, base, 1L), "YYYY-MM-DD HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "addDays(1): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, rt_time_add_hours(__arena_1__, base, -1L), "HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "addHours(-1): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, rt_time_add_days(__arena_1__, base, -7L), "YYYY-MM-DD");
        char *_r = rt_str_concat(__arena_1__, "addDays(-7): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Elapsed Time ---\n");
    RtTime * start = rt_time_now(__arena_1__);
    long sum = 0L;
    {
        long i = 0L;
        while (rt_lt_long(i, 10000L)) {
            RtArena *__loop_arena_40__ = rt_arena_create(__arena_1__);
            {
                (sum = rt_add_long(sum, i));
            }
        __loop_cleanup_40__:
            rt_arena_destroy(__loop_arena_40__);
        __for_continue_41__:;
            rt_post_inc_long(&i);
        }
    }
    long elapsed = rt_time_diff(rt_time_now(__arena_1__), start);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, elapsed);
        char *_r = rt_str_concat(__arena_1__, "Loop completed in ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "ms\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Time Differences ---\n");
    RtTime * t1 = rt_time_now(__arena_1__);
    rt_time_sleep(50L);
    RtTime * t2 = rt_time_now(__arena_1__);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_diff(t2, t1));
        char *_r = rt_str_concat(__arena_1__, "t2.diff(t1): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "ms\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_time_diff(t1, t2));
        char *_r = rt_str_concat(__arena_1__, "t1.diff(t2): ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "ms\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Comparisons ---\n");
    RtTime * earlier = rt_time_from_millis(__arena_1__, 1735500000000L);
    RtTime * later = rt_time_from_millis(__arena_1__, 1735500001000L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_time_is_before(earlier, later));
        char *_r = rt_str_concat(__arena_1__, "earlier isBefore later: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_time_is_after(later, earlier));
        char *_r = rt_str_concat(__arena_1__, "later isAfter earlier: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    RtTime * same1 = rt_time_from_millis(__arena_1__, 1735500000000L);
    RtTime * same2 = rt_time_from_millis(__arena_1__, 1735500000000L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_bool(__arena_1__, rt_time_equals(same1, same2));
        char *_r = rt_str_concat(__arena_1__, "equals: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Sleep ---\n");
    rt_print_string("Sleeping for 100ms...\n");
    RtTime * sleepStart = rt_time_now(__arena_1__);
    rt_time_sleep(100L);
    long sleepElapsed = rt_time_diff(rt_time_now(__arena_1__), sleepStart);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, sleepElapsed);
        char *_r = rt_str_concat(__arena_1__, "Slept for ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "ms\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Timestamps ---\n");
    RtTime * timestamp = rt_time_now(__arena_1__);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, timestamp, "YYYY-MM-DD HH:mm:ss");
        char *_r = rt_str_concat(__arena_1__, "[", _p0);
        _r = rt_str_concat(__arena_1__, _r, "] Event occurred\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, timestamp, "HH:mm:ss.SSS");
        char *_r = rt_str_concat(__arena_1__, "[", _p0);
        _r = rt_str_concat(__arena_1__, _r, "] Precise timestamp\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- File Naming ---\n");
    RtTime * fileTime = rt_time_now(__arena_1__);
    char * filename = ({
        char *_p0 = rt_time_format(__arena_1__, fileTime, "YYYYMMDD_HHmmss");
        char *_r = rt_str_concat(__arena_1__, "backup_", _p0);
        _r = rt_str_concat(__arena_1__, _r, ".txt");
        _r;
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__arena_1__, "Generated filename: ", filename);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    rt_print_string("\n--- Future Events ---\n");
    RtTime * eventNow = rt_time_now(__arena_1__);
    RtTime * eventTime = rt_time_add_minutes(__arena_1__, rt_time_add_hours(__arena_1__, eventNow, 2L), 30L);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_time_format(__arena_1__, eventTime, "h:mm A");
        char *_r = rt_str_concat(__arena_1__, "Event scheduled for: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long waitMs = rt_time_diff(eventTime, eventNow);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__arena_1__, rt_div_long(rt_div_long(waitMs, 1000L), 60L));
        char *_r = rt_str_concat(__arena_1__, "Time until event: ", _p0);
        _r = rt_str_concat(__arena_1__, _r, " minutes\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto demo_time_return;
demo_time_return:
    rt_arena_destroy(__arena_1__);
    return;
}

int main() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    int _return_value = 0;
    rt_print_string("╔══════════════════════════════════════════════════════════════════╗\n");
    rt_print_string("║           Welcome to the Sindarin Language Demo                  ║\n");
    rt_print_string("╚══════════════════════════════════════════════════════════════════╝\n\n");
    demo_types();
    demo_loops();
    demo_conditionals();
    demo_strings();
    demo_functions();
    demo_arrays();
    demo_memory();
    demo_lambda();
    demo_closure();
    demo_bytes();
    demo_fileio();
    demo_date();
    demo_time();
    rt_print_string("╔══════════════════════════════════════════════════════════════════╗\n");
    rt_print_string("║                    All Demos Complete!                           ║\n");
    rt_print_string("╚══════════════════════════════════════════════════════════════════╝\n");
    goto main_return;
main_return:
    rt_arena_destroy(__arena_1__);
    return _return_value;
}


/* Lambda function definitions */
static long __lambda_0__(void *__closure__, long x) {
    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;
    return rt_mul_long(x, 2L);
}

static long __lambda_1__(void *__closure__, long a, long b) {
    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;
    return rt_add_long(a, b);
}

static long __lambda_2__(void *__closure__, long x) {
    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;
    return rt_mul_long(x, 3L);
}

static long __lambda_3__(void *__closure__, long a, long b) {
    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;
    return rt_mul_long(a, b);
}

static long __lambda_4__(void *__closure__, long x) {
    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;
    return rt_mul_long(x, x);
}

static long __lambda_5__(void *__closure__, long x) {
    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;
    return rt_sub_long(0L, x);
}

static long __lambda_6__(void *__closure__, long x) {
    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;
    return rt_add_long(x, 1L);
}

static long __lambda_7__(void *__closure__, long x) {
    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;
    long *multiplier = ((__closure_7__ *)__closure__)->multiplier;
    return rt_mul_long(x, (*multiplier));
}

