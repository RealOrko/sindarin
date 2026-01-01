#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include "runtime.h"
#include "runtime/runtime_string.h"
#include "runtime/runtime_file.h"

/* rt_text_file_promote moved to runtime/runtime_text_file.c */

/* rt_binary_file_promote moved to runtime/runtime_binary_file.c */

/* rt_format_string, rt_print_* functions moved to runtime/runtime_string.c */

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

/* rt_eq_long, rt_ne_long, rt_lt_long, rt_le_long, rt_gt_long, rt_ge_long
 * are defined as static inline in runtime.h for inlining optimization */

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

/* rt_eq_double, rt_ne_double, rt_lt_double, rt_le_double, rt_gt_double, rt_ge_double
 * are defined as static inline in runtime.h for inlining optimization */

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

/* rt_not_bool is defined as static inline in runtime.h */

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

/* rt_eq_string, rt_ne_string, rt_lt_string, rt_le_string, rt_gt_string, rt_ge_string
 * are defined as static inline in runtime.h for inlining optimization */

/* Array functions moved to runtime/runtime_array.c */

/* ============================================================
   String Manipulation Functions
   ============================================================ */

/* rt_str_length, rt_str_substring, rt_str_indexOf, rt_str_split moved to runtime/runtime_string.c */

/* rt_str_trim, rt_str_toUpper, rt_str_toLower, rt_str_startsWith, rt_str_endsWith, rt_str_contains, rt_str_replace, rt_str_charAt moved to runtime/runtime_string.c */

/* rt_text_file_open, rt_text_file_exists, rt_text_file_read_all, rt_text_file_write_all, rt_text_file_delete, rt_text_file_copy, rt_text_file_move moved to runtime/runtime_text_file.c */

/* rt_text_file_close moved to runtime/runtime_text_file.c */

/* rt_text_file_read_char, rt_text_file_read_word, rt_text_file_read_line, rt_text_file_instance_read_all, rt_text_file_read_lines, rt_text_file_read_into moved to runtime/runtime_text_file.c */

/* rt_text_file_write_char, rt_text_file_write, rt_text_file_write_line, rt_text_file_print, rt_text_file_println moved to runtime/runtime_text_file.c */

/* rt_text_file_has_chars, rt_text_file_is_eof, rt_text_file_has_words, rt_text_file_has_lines moved to runtime/runtime_text_file.c */

/* rt_text_file_position, rt_text_file_seek, rt_text_file_rewind, rt_text_file_flush moved to runtime/runtime_text_file.c */

/* rt_text_file_get_path, rt_text_file_get_name, rt_text_file_get_size moved to runtime/runtime_text_file.c */

/* ============================================================
   BinaryFile Static Methods
   ============================================================ */

/* rt_binary_file_open, rt_binary_file_exists, rt_binary_file_read_all, rt_binary_file_write_all, rt_binary_file_delete, rt_binary_file_copy, rt_binary_file_move moved to runtime/runtime_binary_file.c */

/* rt_binary_file_close moved to runtime/runtime_binary_file.c */

/* ============================================================
   BinaryFile Instance Reading Methods
   ============================================================ */

/* rt_binary_file_read_byte moved to runtime/runtime_binary_file.c */

/* rt_binary_file_read_bytes moved to runtime/runtime_binary_file.c */

/* rt_binary_file_instance_read_all moved to runtime/runtime_binary_file.c */

/* rt_binary_file_read_into moved to runtime/runtime_binary_file.c */

/* rt_binary_file_write_byte, rt_binary_file_write_bytes moved to runtime/runtime_binary_file.c */

/* rt_binary_file_has_bytes, rt_binary_file_is_eof moved to runtime/runtime_binary_file.c */

/* rt_binary_file_position, rt_binary_file_seek, rt_binary_file_rewind, rt_binary_file_flush moved to runtime/runtime_binary_file.c */

/* rt_binary_file_get_path moved to runtime/runtime_binary_file.c */

/* rt_binary_file_get_name moved to runtime/runtime_binary_file.c */

/* rt_binary_file_get_size moved to runtime/runtime_binary_file.c */

/* ============================================================================
 * Standard Stream Operations (Stdin, Stdout, Stderr)
 * ============================================================================ */

/* Stdin functions moved to runtime/runtime_io.c:
 * - rt_stdin_read_line
 * - rt_stdin_read_char
 * - rt_stdin_read_word
 * - rt_stdin_has_chars
 * - rt_stdin_has_lines
 * - rt_stdin_is_eof
 */

/* Stdout and stderr functions moved to runtime/runtime_io.c:
 * - rt_stdout_write
 * - rt_stdout_write_line
 * - rt_stdout_flush
 * - rt_stderr_write
 * - rt_stderr_write_line
 * - rt_stderr_flush
 */

/* Convenience I/O functions moved to runtime/runtime_io.c:
 * - rt_read_line
 * - rt_println
 * - rt_print_err
 * - rt_print_err_ln
 */

/* ============================================================================
 * Byte Array Extension Methods
 * ============================================================================
 * RELOCATED to runtime/runtime_byte.c:
 * - base64_chars (encoding table)
 * - rt_byte_array_to_string
 * - rt_byte_array_to_string_latin1
 * - rt_byte_array_to_hex
 * - rt_byte_array_to_base64
 */

/* ============================================================================
 * String to Byte Array Conversions
 * ============================================================================
 * RELOCATED to runtime/runtime_byte.c:
 * - rt_string_to_bytes
 * - rt_bytes_from_hex
 * - base64_decode_table
 * - rt_bytes_from_base64
 */

/* Path utilities are now in runtime/runtime_path.c */

/* ============================================================================
 * Directory Operations
 * ============================================================================ */

#include <dirent.h>

/* RELOCATED to runtime/runtime_path.c:
 * - rt_create_string_array (helper)
 * - rt_push_string_to_array (helper)
 */

/* RELOCATED to runtime/runtime_path.c:
 * - rt_directory_list
 * - list_recursive_helper (static helper)
 * - rt_directory_list_recursive
 * - create_directory_recursive (static helper)
 * - rt_directory_create
 * - rt_directory_delete
 * - delete_recursive_helper (static helper)
 * - rt_directory_delete_recursive
 */

/* ============================================================================
 * String Splitting Methods
 * ============================================================================ */

/* Helper: Check if character is whitespace */
static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

/* Split string on any whitespace character */
char **rt_str_split_whitespace(RtArena *arena, const char *str) {
    if (str == NULL) {
        return rt_create_string_array(arena, 4);
    }

    char **result = rt_create_string_array(arena, 16);
    const char *p = str;

    while (*p) {
        /* Skip leading whitespace */
        while (*p && is_whitespace(*p)) {
            p++;
        }

        if (*p == '\0') break;

        /* Find end of word */
        const char *start = p;
        while (*p && !is_whitespace(*p)) {
            p++;
        }

        /* Add word to result */
        size_t len = p - start;
        char *word = rt_arena_alloc(arena, len + 1);
        memcpy(word, start, len);
        word[len] = '\0';
        result = rt_push_string_to_array(arena, result, word);
    }

    return result;
}

/* Split string on line endings (\n, \r\n, \r) */
char **rt_str_split_lines(RtArena *arena, const char *str) {
    if (str == NULL) {
        return rt_create_string_array(arena, 4);
    }

    char **result = rt_create_string_array(arena, 16);
    const char *p = str;
    const char *start = str;

    while (*p) {
        if (*p == '\n') {
            /* Unix line ending or end of Windows \r\n */
            size_t len = p - start;
            char *line = rt_arena_alloc(arena, len + 1);
            memcpy(line, start, len);
            line[len] = '\0';
            result = rt_push_string_to_array(arena, result, line);
            p++;
            start = p;
        } else if (*p == '\r') {
            /* Carriage return - check for \r\n or standalone \r */
            size_t len = p - start;
            char *line = rt_arena_alloc(arena, len + 1);
            memcpy(line, start, len);
            line[len] = '\0';
            result = rt_push_string_to_array(arena, result, line);
            p++;
            if (*p == '\n') {
                /* Windows \r\n - skip the \n too */
                p++;
            }
            start = p;
        } else {
            p++;
        }
    }

    /* Add final line if there's remaining content */
    if (p > start) {
        size_t len = p - start;
        char *line = rt_arena_alloc(arena, len + 1);
        memcpy(line, start, len);
        line[len] = '\0';
        result = rt_push_string_to_array(arena, result, line);
    }

    return result;
}

/* Check if string is empty or contains only whitespace */
int rt_str_is_blank(const char *str) {
    if (str == NULL || *str == '\0') {
        return 1;  /* NULL or empty string is blank */
    }

    const char *p = str;
    while (*p) {
        if (!is_whitespace(*p)) {
            return 0;  /* Found non-whitespace character */
        }
        p++;
    }

    return 1;  /* All characters were whitespace */
}

/* ============================================================================
 * Time Methods - RELOCATED to runtime/runtime_time.c
 * ============================================================================
 *
 * All 27 time functions have been relocated:
 *
 * Time Creation:
 * - rt_time_from_millis, rt_time_from_seconds, rt_time_now, rt_time_utc, rt_time_sleep
 *
 * Time Getters:
 * - rt_time_get_millis, rt_time_get_seconds, rt_time_get_year, rt_time_get_month
 * - rt_time_get_day, rt_time_get_hour, rt_time_get_minute, rt_time_get_second
 * - rt_time_get_weekday
 *
 * Time Formatters:
 * - rt_time_format, rt_time_to_iso, rt_time_to_date, rt_time_to_time
 *
 * Time Arithmetic:
 * - rt_time_add, rt_time_add_seconds, rt_time_add_minutes, rt_time_add_hours
 * - rt_time_add_days, rt_time_diff
 *
 * Time Comparison:
 * - rt_time_is_before, rt_time_is_after, rt_time_equals
 */
