#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <setjmp.h>
#include "runtime.h"
#ifdef _WIN32
#undef min
#undef max
#endif

/* User-specified includes */
#include "test_variadic_helper.c"

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
extern void rt_print_long(long long);
extern void rt_print_double(double);
extern void rt_print_char(long);
extern void rt_print_string(const char *);
extern void rt_print_bool(long);
extern void rt_print_byte(unsigned char);

/* Runtime type conversions */
extern char *rt_to_string_long(RtArena *, long long);
extern char *rt_to_string_double(RtArena *, double);
extern char *rt_to_string_char(RtArena *, char);
extern char *rt_to_string_bool(RtArena *, int);
extern char *rt_to_string_byte(RtArena *, unsigned char);
extern char *rt_to_string_string(RtArena *, const char *);
extern char *rt_to_string_void(RtArena *);
extern char *rt_to_string_pointer(RtArena *, void *);

/* Runtime format specifier functions */
extern char *rt_format_long(RtArena *, long long, const char *);
extern char *rt_format_double(RtArena *, double, const char *);
extern char *rt_format_string(RtArena *, const char *, const char *);

/* Runtime long arithmetic (comparisons are static inline in runtime.h) */
extern long long rt_add_long(long long, long long);
extern long long rt_sub_long(long long, long long);
extern long long rt_mul_long(long long, long long);
extern long long rt_div_long(long long, long long);
extern long long rt_mod_long(long long, long long);
extern long long rt_neg_long(long long);
extern long long rt_post_inc_long(long long *);
extern long long rt_post_dec_long(long long *);

/* Runtime double arithmetic (comparisons are static inline in runtime.h) */
extern double rt_add_double(double, double);
extern double rt_sub_double(double, double);
extern double rt_mul_double(double, double);
extern double rt_div_double(double, double);
extern double rt_neg_double(double);

/* Runtime array operations */
extern long long *rt_array_push_long(RtArena *, long long *, long long);
extern double *rt_array_push_double(RtArena *, double *, double);
extern char *rt_array_push_char(RtArena *, char *, char);
extern char **rt_array_push_string(RtArena *, char **, const char *);
extern int *rt_array_push_bool(RtArena *, int *, int);
extern unsigned char *rt_array_push_byte(RtArena *, unsigned char *, unsigned char);
extern void **rt_array_push_ptr(RtArena *, void **, void *);

/* Runtime array print functions */
extern void rt_print_array_long(long long *);
extern void rt_print_array_double(double *);
extern void rt_print_array_char(char *);
extern void rt_print_array_bool(int *);
extern void rt_print_array_byte(unsigned char *);
extern void rt_print_array_string(char **);

/* Runtime array clear */
extern void rt_array_clear(void *);

/* Runtime array pop functions */
extern long long rt_array_pop_long(long long *);
extern double rt_array_pop_double(double *);
extern char rt_array_pop_char(char *);
extern int rt_array_pop_bool(int *);
extern unsigned char rt_array_pop_byte(unsigned char *);
extern char *rt_array_pop_string(char **);
extern void *rt_array_pop_ptr(void **);

/* Runtime array concat functions */
extern long long *rt_array_concat_long(RtArena *, long long *, long long *);
extern double *rt_array_concat_double(RtArena *, double *, double *);
extern char *rt_array_concat_char(RtArena *, char *, char *);
extern int *rt_array_concat_bool(RtArena *, int *, int *);
extern unsigned char *rt_array_concat_byte(RtArena *, unsigned char *, unsigned char *);
extern char **rt_array_concat_string(RtArena *, char **, char **);
extern void **rt_array_concat_ptr(RtArena *, void **, void **);

/* Runtime array slice functions (start, end, step) */
extern long long *rt_array_slice_long(RtArena *, long long *, long, long, long);
extern double *rt_array_slice_double(RtArena *, double *, long, long, long);
extern char *rt_array_slice_char(RtArena *, char *, long, long, long);
extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);
extern unsigned char *rt_array_slice_byte(RtArena *, unsigned char *, long, long, long);
extern char **rt_array_slice_string(RtArena *, char **, long, long, long);

/* Runtime array reverse functions */
extern long long *rt_array_rev_long(RtArena *, long long *);
extern double *rt_array_rev_double(RtArena *, double *);
extern char *rt_array_rev_char(RtArena *, char *);
extern int *rt_array_rev_bool(RtArena *, int *);
extern unsigned char *rt_array_rev_byte(RtArena *, unsigned char *);
extern char **rt_array_rev_string(RtArena *, char **);

/* Runtime array remove functions */
extern long long *rt_array_rem_long(RtArena *, long long *, long);
extern double *rt_array_rem_double(RtArena *, double *, long);
extern char *rt_array_rem_char(RtArena *, char *, long);
extern int *rt_array_rem_bool(RtArena *, int *, long);
extern unsigned char *rt_array_rem_byte(RtArena *, unsigned char *, long);
extern char **rt_array_rem_string(RtArena *, char **, long);

/* Runtime array insert functions */
extern long long *rt_array_ins_long(RtArena *, long long *, long long, long);
extern double *rt_array_ins_double(RtArena *, double *, double, long);
extern char *rt_array_ins_char(RtArena *, char *, char, long);
extern int *rt_array_ins_bool(RtArena *, int *, int, long);
extern unsigned char *rt_array_ins_byte(RtArena *, unsigned char *, unsigned char, long);
extern char **rt_array_ins_string(RtArena *, char **, const char *, long);

/* Runtime array push (copy) functions */
extern long long *rt_array_push_copy_long(RtArena *, long long *, long long);
extern double *rt_array_push_copy_double(RtArena *, double *, double);
extern char *rt_array_push_copy_char(RtArena *, char *, char);
extern int *rt_array_push_copy_bool(RtArena *, int *, int);
extern unsigned char *rt_array_push_copy_byte(RtArena *, unsigned char *, unsigned char);
extern char **rt_array_push_copy_string(RtArena *, char **, const char *);

/* Runtime array indexOf functions */
extern long rt_array_indexOf_long(long long *, long long);
extern long rt_array_indexOf_double(double *, double);
extern long rt_array_indexOf_char(char *, char);
extern long rt_array_indexOf_bool(int *, int);
extern long rt_array_indexOf_byte(unsigned char *, unsigned char);
extern long rt_array_indexOf_string(char **, const char *);

/* Runtime array contains functions */
extern int rt_array_contains_long(long long *, long long);
extern int rt_array_contains_double(double *, double);
extern int rt_array_contains_char(char *, char);
extern int rt_array_contains_bool(int *, int);
extern int rt_array_contains_byte(unsigned char *, unsigned char);
extern int rt_array_contains_string(char **, const char *);

/* Runtime array clone functions */
extern long long *rt_array_clone_long(RtArena *, long long *);
extern double *rt_array_clone_double(RtArena *, double *);
extern char *rt_array_clone_char(RtArena *, char *);
extern int *rt_array_clone_bool(RtArena *, int *);
extern unsigned char *rt_array_clone_byte(RtArena *, unsigned char *);
extern char **rt_array_clone_string(RtArena *, char **);

/* Runtime array join functions */
extern char *rt_array_join_long(RtArena *, long long *, const char *);
extern char *rt_array_join_double(RtArena *, double *, const char *);
extern char *rt_array_join_char(RtArena *, char *, const char *);
extern char *rt_array_join_bool(RtArena *, int *, const char *);
extern char *rt_array_join_byte(RtArena *, unsigned char *, const char *);
extern char *rt_array_join_string(RtArena *, char **, const char *);

/* Runtime array create from static data */
extern long long *rt_array_create_long(RtArena *, size_t, const long long *);
extern double *rt_array_create_double(RtArena *, size_t, const double *);
extern char *rt_array_create_char(RtArena *, size_t, const char *);
extern int *rt_array_create_bool(RtArena *, size_t, const int *);
extern unsigned char *rt_array_create_byte(RtArena *, size_t, const unsigned char *);
extern char **rt_array_create_string(RtArena *, size_t, const char **);

/* Runtime array equality functions */
extern int rt_array_eq_long(long long *, long long *);
extern int rt_array_eq_double(double *, double *);
extern int rt_array_eq_char(char *, char *);
extern int rt_array_eq_bool(int *, int *);
extern int rt_array_eq_byte(unsigned char *, unsigned char *);
extern int rt_array_eq_string(char **, char **);

/* Runtime range creation */
extern long long *rt_array_range(RtArena *, long long, long long);

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

/* Native function extern declarations */
extern int32_t test_printf(char *, ...);

/* Forward declarations */
void test_string_format(void);
void test_int_format(void);
void test_double_format(void);
void test_char_format(void);
void test_bool_format(void);
void test_mixed_format(void);
void test_no_args_format(void);
void test_width_precision(void);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);
static RtAny __thunk_2(void);
static RtAny __thunk_3(void);
static RtAny __thunk_4(void);
static RtAny __thunk_5(void);
static RtAny __thunk_6(void);
static RtAny __thunk_7(void);

void test_string_format() {
    rt_print_string("  Testing printf with string arguments...\n");
    test_printf("    Hello, %s!\n", "World");
    test_printf("    Name: %s, Language: %s\n", "Alice", "Sindarin");
    goto test_string_format_return;
test_string_format_return:
    return;
}

void test_int_format() {
    rt_print_string("  Testing printf with integer arguments...\n");
    long long x = 42LL;
    long long y = -17LL;
    long long big = 1000000LL;
    test_printf("    Integer: %ld\n", x);
    test_printf("    Negative: %ld\n", y);
    test_printf("    Large: %ld\n", big);
    test_printf("    Multiple ints: %ld, %ld, %ld\n", 1LL, 2LL, 3LL);
    goto test_int_format_return;
test_int_format_return:
    return;
}

void test_double_format() {
    rt_print_string("  Testing printf with double arguments...\n");
    double pi = 3.1415926535900001;
    double e = 2.71828;
    test_printf("    Pi: %f\n", pi);
    test_printf("    e: %.5f\n", e);
    test_printf("    Formatted: %.2f\n", 123.456);
    goto test_double_format_return;
test_double_format_return:
    return;
}

void test_char_format() {
    rt_print_string("  Testing printf with char arguments...\n");
    char c = 'X';
    test_printf("    Char: %c\n", c);
    test_printf("    Multiple chars: %c%c%c\n", 'A', 'B', 'C');
    goto test_char_format_return;
test_char_format_return:
    return;
}

void test_bool_format() {
    rt_print_string("  Testing printf with bool arguments (as int)...\n");
    bool t = 1L;
    bool f = 0L;
    test_printf("    True: %ld\n", t);
    test_printf("    False: %ld\n", f);
    goto test_bool_format_return;
test_bool_format_return:
    return;
}

void test_mixed_format() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    rt_print_string("  Testing printf with mixed argument types...\n");
    char * name = rt_to_string_string(__arena_1__, "Test");
    long long count = 42LL;
    double value = 3.1400000000000001;
    char flag = '*';
    test_printf("    String: %s, Int: %ld, Double: %.2f, Char: %c\n", name, count, value, flag);
    test_printf("    Combined: %s scored %ld points with %.1f%% accuracy\n", "Player", 100LL, 95.5);
    goto test_mixed_format_return;
test_mixed_format_return:
    rt_arena_destroy(__arena_1__);
    return;
}

void test_no_args_format() {
    rt_print_string("  Testing printf with no variadic arguments...\n");
    test_printf("    Just a plain string\n");
    goto test_no_args_format_return;
test_no_args_format_return:
    return;
}

void test_width_precision() {
    rt_print_string("  Testing printf with width and precision...\n");
    test_printf("    Right aligned: %10s|\n", "test");
    test_printf("    Left aligned: %-10s|\n", "test");
    test_printf("    Zero padded: %05ld\n", 42LL);
    test_printf("    Precision: %.3f\n", 1.2345678899999999);
    goto test_width_precision_return;
test_width_precision_return:
    return;
}

int main() {
    RtArena *__arena_1__ = rt_arena_create(NULL);
    int _return_value = 0;
    rt_print_string("=== Variadic Function Interop Test ===\n\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena_1__;
        RtAny __intercepted = rt_call_intercepted("test_string_format", __args, 0, __thunk_0);
    } else {
        test_string_format();
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena_1__;
        RtAny __intercepted = rt_call_intercepted("test_int_format", __args, 0, __thunk_1);
    } else {
        test_int_format();
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena_1__;
        RtAny __intercepted = rt_call_intercepted("test_double_format", __args, 0, __thunk_2);
    } else {
        test_double_format();
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena_1__;
        RtAny __intercepted = rt_call_intercepted("test_char_format", __args, 0, __thunk_3);
    } else {
        test_char_format();
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena_1__;
        RtAny __intercepted = rt_call_intercepted("test_bool_format", __args, 0, __thunk_4);
    } else {
        test_bool_format();
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena_1__;
        RtAny __intercepted = rt_call_intercepted("test_mixed_format", __args, 0, __thunk_5);
    } else {
        test_mixed_format();
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena_1__;
        RtAny __intercepted = rt_call_intercepted("test_no_args_format", __args, 0, __thunk_6);
    } else {
        test_no_args_format();
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena_1__;
        RtAny __intercepted = rt_call_intercepted("test_width_precision", __args, 0, __thunk_7);
    } else {
        test_width_precision();
    }
    (void)0;
});
    rt_print_string("\n=== All variadic interop tests PASSED! ===\n");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_arena_destroy(__arena_1__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    test_string_format();
    return rt_box_nil();
}

static RtAny __thunk_1(void) {
    test_int_format();
    return rt_box_nil();
}

static RtAny __thunk_2(void) {
    test_double_format();
    return rt_box_nil();
}

static RtAny __thunk_3(void) {
    test_char_format();
    return rt_box_nil();
}

static RtAny __thunk_4(void) {
    test_bool_format();
    return rt_box_nil();
}

static RtAny __thunk_5(void) {
    test_mixed_format();
    return rt_box_nil();
}

static RtAny __thunk_6(void) {
    test_no_args_format();
    return rt_box_nil();
}

static RtAny __thunk_7(void) {
    test_width_precision();
    return rt_box_nil();
}

