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
    "#include <stdbool.h>\n\n"
    "/* Runtime string operations */\n"
    "extern char *rt_str_concat(char *, char *);\n"
    "extern void rt_free_string(char *);\n\n"
    "/* Runtime print functions */\n"
    "extern void rt_print_long(long);\n"
    "extern void rt_print_double(double);\n"
    "extern void rt_print_char(long);\n"
    "extern void rt_print_string(char *);\n"
    "extern void rt_print_bool(long);\n\n"
    "/* Runtime type conversions */\n"
    "extern char *rt_to_string_long(long);\n"
    "extern char *rt_to_string_double(double);\n"
    "extern char *rt_to_string_char(long);\n"
    "extern char *rt_to_string_bool(long);\n"
    "extern char *rt_to_string_string(char *);\n"
    "extern char *rt_to_string_void(void);\n"
    "extern char *rt_to_string_pointer(void *);\n\n"
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
    "extern long rt_eq_string(char *, char *);\n"
    "extern long rt_ne_string(char *, char *);\n"
    "extern long rt_lt_string(char *, char *);\n"
    "extern long rt_le_string(char *, char *);\n"
    "extern long rt_gt_string(char *, char *);\n"
    "extern long rt_ge_string(char *, char *);\n\n"
    "/* Runtime array operations */\n"
    "extern long *rt_array_push_long(long *, long);\n"
    "extern double *rt_array_push_double(double *, double);\n"
    "extern char *rt_array_push_char(char *, char);\n"
    "extern char **rt_array_push_string(char **, char *);\n"
    "extern int *rt_array_push_bool(int *, int);\n"
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
    "extern long *rt_array_concat_long(long *, long *);\n"
    "extern double *rt_array_concat_double(double *, double *);\n"
    "extern char *rt_array_concat_char(char *, char *);\n"
    "extern int *rt_array_concat_bool(int *, int *);\n"
    "extern char **rt_array_concat_string(char **, char **);\n\n"
    "/* Runtime array free functions */\n"
    "extern void rt_array_free(void *);\n"
    "extern void rt_array_free_string(char **);\n\n";

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
