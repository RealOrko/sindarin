// tests/unit/runtime/runtime_arithmetic_tests.c
// Tests for runtime arithmetic operations with overflow checking

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include "../runtime.h"

/* ============================================================================
 * Long Arithmetic Tests
 * ============================================================================ */

void test_rt_add_long_basic()
{
    printf("Testing rt_add_long basic operations...\n");

    assert(rt_add_long(1, 2) == 3);
    assert(rt_add_long(0, 0) == 0);
    assert(rt_add_long(-1, 1) == 0);
    assert(rt_add_long(-5, -3) == -8);
    assert(rt_add_long(100, -50) == 50);
    assert(rt_add_long(LONG_MAX - 1, 1) == LONG_MAX);
    assert(rt_add_long(LONG_MIN + 1, -1) == LONG_MIN);
}

void test_rt_sub_long_basic()
{
    printf("Testing rt_sub_long basic operations...\n");

    assert(rt_sub_long(5, 3) == 2);
    assert(rt_sub_long(0, 0) == 0);
    assert(rt_sub_long(-1, -1) == 0);
    assert(rt_sub_long(10, -5) == 15);
    assert(rt_sub_long(-10, 5) == -15);
    assert(rt_sub_long(LONG_MIN + 1, 1) == LONG_MIN);
    assert(rt_sub_long(LONG_MAX - 1, -1) == LONG_MAX);
}

void test_rt_mul_long_basic()
{
    printf("Testing rt_mul_long basic operations...\n");

    assert(rt_mul_long(3, 4) == 12);
    assert(rt_mul_long(0, 100) == 0);
    assert(rt_mul_long(100, 0) == 0);
    assert(rt_mul_long(-2, 3) == -6);
    assert(rt_mul_long(2, -3) == -6);
    assert(rt_mul_long(-2, -3) == 6);
    assert(rt_mul_long(1, 1000000) == 1000000);
    assert(rt_mul_long(-1, 1000000) == -1000000);
    assert(rt_mul_long(-1000000, -1) == 1000000);
}

void test_rt_div_long_basic()
{
    printf("Testing rt_div_long basic operations...\n");

    assert(rt_div_long(10, 2) == 5);
    assert(rt_div_long(10, 3) == 3);
    assert(rt_div_long(-10, 2) == -5);
    assert(rt_div_long(10, -2) == -5);
    assert(rt_div_long(-10, -2) == 5);
    assert(rt_div_long(0, 5) == 0);
    assert(rt_div_long(1000000, 1) == 1000000);
    assert(rt_div_long(-1000000, 1) == -1000000);
}

void test_rt_mod_long_basic()
{
    printf("Testing rt_mod_long basic operations...\n");

    assert(rt_mod_long(10, 3) == 1);
    assert(rt_mod_long(10, 5) == 0);
    assert(rt_mod_long(0, 5) == 0);
    assert(rt_mod_long(-10, 3) == -1);
    assert(rt_mod_long(10, -3) == 1);
    assert(rt_mod_long(-10, -3) == -1);
}

void test_rt_neg_long_basic()
{
    printf("Testing rt_neg_long basic operations...\n");

    assert(rt_neg_long(5) == -5);
    assert(rt_neg_long(-5) == 5);
    assert(rt_neg_long(0) == 0);
    assert(rt_neg_long(LONG_MAX) == -LONG_MAX);
}

/* ============================================================================
 * Long Comparison Tests (inline functions)
 * ============================================================================ */

void test_rt_long_comparisons()
{
    printf("Testing rt_*_long comparison operations...\n");

    /* Equal */
    assert(rt_eq_long(5, 5) == 1);
    assert(rt_eq_long(5, 6) == 0);
    assert(rt_eq_long(-5, -5) == 1);

    /* Not equal */
    assert(rt_ne_long(5, 6) == 1);
    assert(rt_ne_long(5, 5) == 0);

    /* Less than */
    assert(rt_lt_long(3, 5) == 1);
    assert(rt_lt_long(5, 3) == 0);
    assert(rt_lt_long(5, 5) == 0);
    assert(rt_lt_long(-5, -3) == 1);

    /* Less than or equal */
    assert(rt_le_long(3, 5) == 1);
    assert(rt_le_long(5, 5) == 1);
    assert(rt_le_long(5, 3) == 0);

    /* Greater than */
    assert(rt_gt_long(5, 3) == 1);
    assert(rt_gt_long(3, 5) == 0);
    assert(rt_gt_long(5, 5) == 0);

    /* Greater than or equal */
    assert(rt_ge_long(5, 3) == 1);
    assert(rt_ge_long(5, 5) == 1);
    assert(rt_ge_long(3, 5) == 0);
}

/* ============================================================================
 * Double Arithmetic Tests
 * ============================================================================ */

void test_rt_add_double_basic()
{
    printf("Testing rt_add_double basic operations...\n");

    assert(rt_add_double(1.5, 2.5) == 4.0);
    assert(rt_add_double(0.0, 0.0) == 0.0);
    assert(rt_add_double(-1.5, 1.5) == 0.0);
    assert(rt_add_double(-5.0, -3.0) == -8.0);

    /* Test with very small numbers */
    double small = rt_add_double(0.1, 0.2);
    assert(fabs(small - 0.3) < 0.0001);
}

void test_rt_sub_double_basic()
{
    printf("Testing rt_sub_double basic operations...\n");

    assert(rt_sub_double(5.0, 3.0) == 2.0);
    assert(rt_sub_double(0.0, 0.0) == 0.0);
    assert(rt_sub_double(-1.5, -1.5) == 0.0);
    assert(rt_sub_double(10.5, -5.5) == 16.0);
}

void test_rt_mul_double_basic()
{
    printf("Testing rt_mul_double basic operations...\n");

    assert(rt_mul_double(3.0, 4.0) == 12.0);
    assert(rt_mul_double(0.0, 100.0) == 0.0);
    assert(rt_mul_double(-2.0, 3.0) == -6.0);
    assert(rt_mul_double(-2.0, -3.0) == 6.0);
    assert(rt_mul_double(0.5, 2.0) == 1.0);
}

void test_rt_div_double_basic()
{
    printf("Testing rt_div_double basic operations...\n");

    assert(rt_div_double(10.0, 2.0) == 5.0);
    assert(rt_div_double(10.0, 4.0) == 2.5);
    assert(rt_div_double(-10.0, 2.0) == -5.0);
    assert(rt_div_double(1.0, 3.0) - 0.333333 < 0.0001);
}

void test_rt_neg_double_basic()
{
    printf("Testing rt_neg_double basic operations...\n");

    assert(rt_neg_double(5.0) == -5.0);
    assert(rt_neg_double(-5.0) == 5.0);
    assert(rt_neg_double(0.0) == 0.0);
    assert(rt_neg_double(DBL_MAX) == -DBL_MAX);
}

/* ============================================================================
 * Double Comparison Tests (inline functions)
 * ============================================================================ */

void test_rt_double_comparisons()
{
    printf("Testing rt_*_double comparison operations...\n");

    /* Equal */
    assert(rt_eq_double(5.0, 5.0) == 1);
    assert(rt_eq_double(5.0, 5.1) == 0);

    /* Not equal */
    assert(rt_ne_double(5.0, 5.1) == 1);
    assert(rt_ne_double(5.0, 5.0) == 0);

    /* Less than */
    assert(rt_lt_double(3.0, 5.0) == 1);
    assert(rt_lt_double(5.0, 3.0) == 0);
    assert(rt_lt_double(5.0, 5.0) == 0);

    /* Less than or equal */
    assert(rt_le_double(3.0, 5.0) == 1);
    assert(rt_le_double(5.0, 5.0) == 1);
    assert(rt_le_double(5.0, 3.0) == 0);

    /* Greater than */
    assert(rt_gt_double(5.0, 3.0) == 1);
    assert(rt_gt_double(3.0, 5.0) == 0);
    assert(rt_gt_double(5.0, 5.0) == 0);

    /* Greater than or equal */
    assert(rt_ge_double(5.0, 3.0) == 1);
    assert(rt_ge_double(5.0, 5.0) == 1);
    assert(rt_ge_double(3.0, 5.0) == 0);
}

/* ============================================================================
 * Boolean Operation Tests
 * ============================================================================ */

void test_rt_not_bool()
{
    printf("Testing rt_not_bool...\n");

    assert(rt_not_bool(0) == 1);
    assert(rt_not_bool(1) == 0);
    assert(rt_not_bool(42) == 0);  /* Any non-zero is truthy */
    assert(rt_not_bool(-1) == 0);
}

/* ============================================================================
 * Post Increment/Decrement Tests
 * ============================================================================ */

void test_rt_post_inc_long()
{
    printf("Testing rt_post_inc_long...\n");

    long val = 5;
    long result = rt_post_inc_long(&val);
    assert(result == 5);   /* Returns old value */
    assert(val == 6);      /* Variable is incremented */

    val = 0;
    result = rt_post_inc_long(&val);
    assert(result == 0);
    assert(val == 1);

    val = -1;
    result = rt_post_inc_long(&val);
    assert(result == -1);
    assert(val == 0);

    /* Test near max (but not at max to avoid overflow exit) */
    val = LONG_MAX - 1;
    result = rt_post_inc_long(&val);
    assert(result == LONG_MAX - 1);
    assert(val == LONG_MAX);
}

void test_rt_post_dec_long()
{
    printf("Testing rt_post_dec_long...\n");

    long val = 5;
    long result = rt_post_dec_long(&val);
    assert(result == 5);   /* Returns old value */
    assert(val == 4);      /* Variable is decremented */

    val = 1;
    result = rt_post_dec_long(&val);
    assert(result == 1);
    assert(val == 0);

    val = 0;
    result = rt_post_dec_long(&val);
    assert(result == 0);
    assert(val == -1);

    /* Test near min (but not at min to avoid overflow exit) */
    val = LONG_MIN + 1;
    result = rt_post_dec_long(&val);
    assert(result == LONG_MIN + 1);
    assert(val == LONG_MIN);
}

/* ============================================================================
 * String Comparison Tests (inline functions)
 * ============================================================================ */

void test_rt_string_comparisons()
{
    printf("Testing rt_*_string comparison operations...\n");

    /* Equal */
    assert(rt_eq_string("hello", "hello") == 1);
    assert(rt_eq_string("hello", "world") == 0);
    assert(rt_eq_string("", "") == 1);

    /* Not equal */
    assert(rt_ne_string("hello", "world") == 1);
    assert(rt_ne_string("hello", "hello") == 0);

    /* Less than (lexicographic) */
    assert(rt_lt_string("apple", "banana") == 1);
    assert(rt_lt_string("banana", "apple") == 0);
    assert(rt_lt_string("abc", "abd") == 1);
    assert(rt_lt_string("abc", "abc") == 0);

    /* Less than or equal */
    assert(rt_le_string("apple", "banana") == 1);
    assert(rt_le_string("apple", "apple") == 1);
    assert(rt_le_string("banana", "apple") == 0);

    /* Greater than */
    assert(rt_gt_string("banana", "apple") == 1);
    assert(rt_gt_string("apple", "banana") == 0);
    assert(rt_gt_string("apple", "apple") == 0);

    /* Greater than or equal */
    assert(rt_ge_string("banana", "apple") == 1);
    assert(rt_ge_string("apple", "apple") == 1);
    assert(rt_ge_string("apple", "banana") == 0);
}

/* ============================================================================
 * String Blank Check Tests
 * ============================================================================ */

void test_rt_str_is_blank()
{
    printf("Testing rt_str_is_blank...\n");

    /* Blank strings */
    assert(rt_str_is_blank(NULL) == 1);
    assert(rt_str_is_blank("") == 1);
    assert(rt_str_is_blank(" ") == 1);
    assert(rt_str_is_blank("  ") == 1);
    assert(rt_str_is_blank("\t") == 1);
    assert(rt_str_is_blank("\n") == 1);
    assert(rt_str_is_blank("\r") == 1);
    assert(rt_str_is_blank(" \t\n\r\v\f") == 1);

    /* Non-blank strings */
    assert(rt_str_is_blank("a") == 0);
    assert(rt_str_is_blank(" a") == 0);
    assert(rt_str_is_blank("a ") == 0);
    assert(rt_str_is_blank(" a ") == 0);
    assert(rt_str_is_blank("hello world") == 0);
}

/* ============================================================================
 * String Split Whitespace Tests
 * ============================================================================ */

void test_rt_str_split_whitespace()
{
    printf("Testing rt_str_split_whitespace...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Basic split */
    char **parts = rt_str_split_whitespace(arena, "hello world");
    assert(rt_array_length(parts) == 2);
    assert(strcmp(parts[0], "hello") == 0);
    assert(strcmp(parts[1], "world") == 0);

    /* Multiple spaces */
    parts = rt_str_split_whitespace(arena, "one   two    three");
    assert(rt_array_length(parts) == 3);
    assert(strcmp(parts[0], "one") == 0);
    assert(strcmp(parts[1], "two") == 0);
    assert(strcmp(parts[2], "three") == 0);

    /* Leading/trailing whitespace */
    parts = rt_str_split_whitespace(arena, "  hello  ");
    assert(rt_array_length(parts) == 1);
    assert(strcmp(parts[0], "hello") == 0);

    /* Mixed whitespace */
    parts = rt_str_split_whitespace(arena, "a\tb\nc\rd");
    assert(rt_array_length(parts) == 4);
    assert(strcmp(parts[0], "a") == 0);
    assert(strcmp(parts[1], "b") == 0);
    assert(strcmp(parts[2], "c") == 0);
    assert(strcmp(parts[3], "d") == 0);

    /* Empty string */
    parts = rt_str_split_whitespace(arena, "");
    assert(rt_array_length(parts) == 0);

    /* Only whitespace */
    parts = rt_str_split_whitespace(arena, "   ");
    assert(rt_array_length(parts) == 0);

    /* NULL input */
    parts = rt_str_split_whitespace(arena, NULL);
    assert(rt_array_length(parts) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * String Split Lines Tests
 * ============================================================================ */

void test_rt_str_split_lines()
{
    printf("Testing rt_str_split_lines...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Unix line endings */
    char **lines = rt_str_split_lines(arena, "line1\nline2\nline3");
    assert(rt_array_length(lines) == 3);
    assert(strcmp(lines[0], "line1") == 0);
    assert(strcmp(lines[1], "line2") == 0);
    assert(strcmp(lines[2], "line3") == 0);

    /* Windows line endings */
    lines = rt_str_split_lines(arena, "line1\r\nline2\r\nline3");
    assert(rt_array_length(lines) == 3);
    assert(strcmp(lines[0], "line1") == 0);
    assert(strcmp(lines[1], "line2") == 0);
    assert(strcmp(lines[2], "line3") == 0);

    /* Old Mac line endings (just \r) */
    lines = rt_str_split_lines(arena, "line1\rline2\rline3");
    assert(rt_array_length(lines) == 3);
    assert(strcmp(lines[0], "line1") == 0);
    assert(strcmp(lines[1], "line2") == 0);
    assert(strcmp(lines[2], "line3") == 0);

    /* Mixed line endings */
    lines = rt_str_split_lines(arena, "unix\nwindows\r\nmac\r");
    assert(rt_array_length(lines) == 3);
    assert(strcmp(lines[0], "unix") == 0);
    assert(strcmp(lines[1], "windows") == 0);
    assert(strcmp(lines[2], "mac") == 0);

    /* Empty lines */
    lines = rt_str_split_lines(arena, "line1\n\nline3");
    assert(rt_array_length(lines) == 3);
    assert(strcmp(lines[0], "line1") == 0);
    assert(strcmp(lines[1], "") == 0);
    assert(strcmp(lines[2], "line3") == 0);

    /* Single line (no newlines) */
    lines = rt_str_split_lines(arena, "single line");
    assert(rt_array_length(lines) == 1);
    assert(strcmp(lines[0], "single line") == 0);

    /* Empty string */
    lines = rt_str_split_lines(arena, "");
    assert(rt_array_length(lines) == 0);

    /* NULL input */
    lines = rt_str_split_lines(arena, NULL);
    assert(rt_array_length(lines) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_arithmetic_main()
{
    /* Long arithmetic */
    test_rt_add_long_basic();
    test_rt_sub_long_basic();
    test_rt_mul_long_basic();
    test_rt_div_long_basic();
    test_rt_mod_long_basic();
    test_rt_neg_long_basic();
    test_rt_long_comparisons();

    /* Double arithmetic */
    test_rt_add_double_basic();
    test_rt_sub_double_basic();
    test_rt_mul_double_basic();
    test_rt_div_double_basic();
    test_rt_neg_double_basic();
    test_rt_double_comparisons();

    /* Boolean */
    test_rt_not_bool();

    /* Increment/decrement */
    test_rt_post_inc_long();
    test_rt_post_dec_long();

    /* String comparisons */
    test_rt_string_comparisons();

    /* String utilities */
    test_rt_str_is_blank();
    test_rt_str_split_whitespace();
    test_rt_str_split_lines();
}
