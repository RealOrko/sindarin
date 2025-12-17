#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
 * Test assertion macros with helpful error messages.
 * These provide better debugging information than bare assert().
 */

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Condition: %s\n", #cond); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_EQ(actual, expected, msg) do { \
    if ((actual) != (expected)) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Expected: %s = %ld\n", #expected, (long)(expected)); \
        fprintf(stderr, "    Actual:   %s = %ld\n", #actual, (long)(actual)); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(actual, expected, msg) do { \
    const char *_a = (actual); \
    const char *_e = (expected); \
    if (_a == NULL && _e == NULL) { /* OK */ } \
    else if (_a == NULL || _e == NULL || strcmp(_a, _e) != 0) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Expected: \"%s\"\n", _e ? _e : "(null)"); \
        fprintf(stderr, "    Actual:   \"%s\"\n", _a ? _a : "(null)"); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(ptr, msg) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Expected: non-NULL\n"); \
        fprintf(stderr, "    Actual:   NULL\n"); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr, msg) do { \
    if ((ptr) != NULL) { \
        fprintf(stderr, "\n  ASSERTION FAILED: %s\n", msg); \
        fprintf(stderr, "    Expected: NULL\n"); \
        fprintf(stderr, "    Actual:   non-NULL (%p)\n", (void*)(ptr)); \
        fprintf(stderr, "    Location: %s:%d\n", __FILE__, __LINE__); \
        assert(0); \
    } \
} while(0)

#define TEST_ASSERT_TRUE(cond, msg) TEST_ASSERT((cond), msg)
#define TEST_ASSERT_FALSE(cond, msg) TEST_ASSERT(!(cond), msg)

/*
 * Test lifecycle macros
 */
#define TEST_BEGIN(name) \
    printf("Testing %s...\n", name); \
    DEBUG_INFO("Starting %s", name)

#define TEST_END(name) \
    DEBUG_INFO("Finished %s", name)

/*
 * Common runtime header string for code generation tests.
 * Matches the output of code_gen_externs().
 */
static const char *CODE_GEN_RUNTIME_HEADER =
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "#include <stdio.h>\n"
    "#include <stdbool.h>\n"
    "#include <limits.h>\n\n"
    "/* Runtime arena operations */\n"
    "typedef struct RtArena RtArena;\n"
    "extern RtArena *rt_arena_create(RtArena *parent);\n"
    "extern void rt_arena_destroy(RtArena *arena);\n"
    "extern void *rt_arena_alloc(RtArena *arena, size_t size);\n\n"
    "/* Closure type for lambdas */\n"
    "typedef struct __Closure__ { void *fn; RtArena *arena; } __Closure__;\n\n"
    "/* Runtime string operations */\n"
    "extern char *rt_str_concat(RtArena *, const char *, const char *);\n"
    "extern long rt_str_length(const char *);\n"
    "extern char *rt_str_substring(RtArena *, const char *, long, long);\n"
    "extern long rt_str_indexOf(const char *, const char *);\n"
    "extern char **rt_str_split(RtArena *, const char *, const char *);\n"
    "extern char *rt_str_trim(RtArena *, const char *);\n"
    "extern char *rt_str_toUpper(RtArena *, const char *);\n"
    "extern char *rt_str_toLower(RtArena *, const char *);\n"
    "extern int rt_str_startsWith(const char *, const char *);\n"
    "extern int rt_str_endsWith(const char *, const char *);\n"
    "extern int rt_str_contains(const char *, const char *);\n"
    "extern char *rt_str_replace(RtArena *, const char *, const char *, const char *);\n"
    "extern long rt_str_charAt(const char *, long);\n\n"
    "/* Runtime print functions */\n"
    "extern void rt_print_long(long);\n"
    "extern void rt_print_double(double);\n"
    "extern void rt_print_char(long);\n"
    "extern void rt_print_string(const char *);\n"
    "extern void rt_print_bool(long);\n\n"
    "/* Runtime type conversions */\n"
    "extern char *rt_to_string_long(RtArena *, long);\n"
    "extern char *rt_to_string_double(RtArena *, double);\n"
    "extern char *rt_to_string_char(RtArena *, char);\n"
    "extern char *rt_to_string_bool(RtArena *, int);\n"
    "extern char *rt_to_string_string(RtArena *, const char *);\n"
    "extern char *rt_to_string_void(RtArena *);\n"
    "extern char *rt_to_string_pointer(RtArena *, void *);\n\n"
    "/* Runtime long arithmetic */\n"
    "extern long rt_add_long(long, long);\n"
    "extern long rt_sub_long(long, long);\n"
    "extern long rt_mul_long(long, long);\n"
    "extern long rt_div_long(long, long);\n"
    "extern long rt_mod_long(long, long);\n"
    "extern long rt_neg_long(long);\n"
    "extern long rt_eq_long(long, long);\n"
    "extern long rt_ne_long(long, long);\n"
    "extern long rt_lt_long(long, long);\n"
    "extern long rt_le_long(long, long);\n"
    "extern long rt_gt_long(long, long);\n"
    "extern long rt_ge_long(long, long);\n"
    "extern long rt_post_inc_long(long *);\n"
    "extern long rt_post_dec_long(long *);\n\n"
    "/* Runtime double arithmetic */\n"
    "extern double rt_add_double(double, double);\n"
    "extern double rt_sub_double(double, double);\n"
    "extern double rt_mul_double(double, double);\n"
    "extern double rt_div_double(double, double);\n"
    "extern double rt_neg_double(double);\n"
    "extern long rt_eq_double(double, double);\n"
    "extern long rt_ne_double(double, double);\n"
    "extern long rt_lt_double(double, double);\n"
    "extern long rt_le_double(double, double);\n"
    "extern long rt_gt_double(double, double);\n"
    "extern long rt_ge_double(double, double);\n\n"
    "/* Runtime boolean and string comparisons */\n"
    "extern long rt_not_bool(long);\n"
    "extern long rt_eq_string(const char *, const char *);\n"
    "extern long rt_ne_string(const char *, const char *);\n"
    "extern long rt_lt_string(const char *, const char *);\n"
    "extern long rt_le_string(const char *, const char *);\n"
    "extern long rt_gt_string(const char *, const char *);\n"
    "extern long rt_ge_string(const char *, const char *);\n\n"
    "/* Runtime array operations */\n"
    "extern long *rt_array_push_long(RtArena *, long *, long);\n"
    "extern double *rt_array_push_double(RtArena *, double *, double);\n"
    "extern char *rt_array_push_char(RtArena *, char *, char);\n"
    "extern char **rt_array_push_string(RtArena *, char **, const char *);\n"
    "extern int *rt_array_push_bool(RtArena *, int *, int);\n"
    "extern long rt_array_length(void *);\n\n"
    "/* Runtime array print functions */\n"
    "extern void rt_print_array_long(long *);\n"
    "extern void rt_print_array_double(double *);\n"
    "extern void rt_print_array_char(char *);\n"
    "extern void rt_print_array_bool(int *);\n"
    "extern void rt_print_array_string(char **);\n\n"
    "/* Runtime array clear */\n"
    "extern void rt_array_clear(void *);\n\n"
    "/* Runtime array pop functions */\n"
    "extern long rt_array_pop_long(long *);\n"
    "extern double rt_array_pop_double(double *);\n"
    "extern char rt_array_pop_char(char *);\n"
    "extern int rt_array_pop_bool(int *);\n"
    "extern char *rt_array_pop_string(char **);\n\n"
    "/* Runtime array concat functions */\n"
    "extern long *rt_array_concat_long(RtArena *, long *, long *);\n"
    "extern double *rt_array_concat_double(RtArena *, double *, double *);\n"
    "extern char *rt_array_concat_char(RtArena *, char *, char *);\n"
    "extern int *rt_array_concat_bool(RtArena *, int *, int *);\n"
    "extern char **rt_array_concat_string(RtArena *, char **, char **);\n\n"
    "/* Runtime array slice functions (start, end, step) */\n"
    "extern long *rt_array_slice_long(RtArena *, long *, long, long, long);\n"
    "extern double *rt_array_slice_double(RtArena *, double *, long, long, long);\n"
    "extern char *rt_array_slice_char(RtArena *, char *, long, long, long);\n"
    "extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);\n"
    "extern char **rt_array_slice_string(RtArena *, char **, long, long, long);\n\n"
    "/* Runtime array reverse functions */\n"
    "extern long *rt_array_rev_long(RtArena *, long *);\n"
    "extern double *rt_array_rev_double(RtArena *, double *);\n"
    "extern char *rt_array_rev_char(RtArena *, char *);\n"
    "extern int *rt_array_rev_bool(RtArena *, int *);\n"
    "extern char **rt_array_rev_string(RtArena *, char **);\n\n"
    "/* Runtime array remove functions */\n"
    "extern long *rt_array_rem_long(RtArena *, long *, long);\n"
    "extern double *rt_array_rem_double(RtArena *, double *, long);\n"
    "extern char *rt_array_rem_char(RtArena *, char *, long);\n"
    "extern int *rt_array_rem_bool(RtArena *, int *, long);\n"
    "extern char **rt_array_rem_string(RtArena *, char **, long);\n\n"
    "/* Runtime array insert functions */\n"
    "extern long *rt_array_ins_long(RtArena *, long *, long, long);\n"
    "extern double *rt_array_ins_double(RtArena *, double *, double, long);\n"
    "extern char *rt_array_ins_char(RtArena *, char *, char, long);\n"
    "extern int *rt_array_ins_bool(RtArena *, int *, int, long);\n"
    "extern char **rt_array_ins_string(RtArena *, char **, const char *, long);\n\n"
    "/* Runtime array push (copy) functions */\n"
    "extern long *rt_array_push_copy_long(RtArena *, long *, long);\n"
    "extern double *rt_array_push_copy_double(RtArena *, double *, double);\n"
    "extern char *rt_array_push_copy_char(RtArena *, char *, char);\n"
    "extern int *rt_array_push_copy_bool(RtArena *, int *, int);\n"
    "extern char **rt_array_push_copy_string(RtArena *, char **, const char *);\n\n"
    "/* Runtime array indexOf functions */\n"
    "extern long rt_array_indexOf_long(long *, long);\n"
    "extern long rt_array_indexOf_double(double *, double);\n"
    "extern long rt_array_indexOf_char(char *, char);\n"
    "extern long rt_array_indexOf_bool(int *, int);\n"
    "extern long rt_array_indexOf_string(char **, const char *);\n\n"
    "/* Runtime array contains functions */\n"
    "extern int rt_array_contains_long(long *, long);\n"
    "extern int rt_array_contains_double(double *, double);\n"
    "extern int rt_array_contains_char(char *, char);\n"
    "extern int rt_array_contains_bool(int *, int);\n"
    "extern int rt_array_contains_string(char **, const char *);\n\n"
    "/* Runtime array clone functions */\n"
    "extern long *rt_array_clone_long(RtArena *, long *);\n"
    "extern double *rt_array_clone_double(RtArena *, double *);\n"
    "extern char *rt_array_clone_char(RtArena *, char *);\n"
    "extern int *rt_array_clone_bool(RtArena *, int *);\n"
    "extern char **rt_array_clone_string(RtArena *, char **);\n\n"
    "/* Runtime array join functions */\n"
    "extern char *rt_array_join_long(RtArena *, long *, const char *);\n"
    "extern char *rt_array_join_double(RtArena *, double *, const char *);\n"
    "extern char *rt_array_join_char(RtArena *, char *, const char *);\n"
    "extern char *rt_array_join_bool(RtArena *, int *, const char *);\n"
    "extern char *rt_array_join_string(RtArena *, char **, const char *);\n\n"
    "/* Runtime array create from static data */\n"
    "extern long *rt_array_create_long(RtArena *, size_t, const long *);\n"
    "extern double *rt_array_create_double(RtArena *, size_t, const double *);\n"
    "extern char *rt_array_create_char(RtArena *, size_t, const char *);\n"
    "extern int *rt_array_create_bool(RtArena *, size_t, const int *);\n"
    "extern char **rt_array_create_string(RtArena *, size_t, const char **);\n\n"
    "/* Runtime array equality functions */\n"
    "extern int rt_array_eq_long(long *, long *);\n"
    "extern int rt_array_eq_double(double *, double *);\n"
    "extern int rt_array_eq_char(char *, char *);\n"
    "extern int rt_array_eq_bool(int *, int *);\n"
    "extern int rt_array_eq_string(char **, char **);\n\n"
    "/* Runtime range creation */\n"
    "extern long *rt_array_range(RtArena *, long, long);\n\n"
    "/* Forward declarations */\n";

/*
 * Helper to build expected code gen output.
 * Combines runtime header with test-specific code.
 */
static inline char *build_expected_output(Arena *arena, const char *code)
{
    size_t header_len = strlen(CODE_GEN_RUNTIME_HEADER);
    size_t code_len = strlen(code);
    char *result = arena_alloc(arena, header_len + code_len + 1);
    if (result == NULL) {
        fprintf(stderr, "build_expected_output: allocation failed\n");
        exit(1);
    }
    memcpy(result, CODE_GEN_RUNTIME_HEADER, header_len);
    memcpy(result + header_len, code, code_len + 1);
    return result;
}

#endif /* TEST_UTILS_H */
