// tests/unit/runtime/runtime_date_tests_format.c
// Tests for Date.format() operations

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"

/* ============================================================================
 * Date.format() Tests
 * ============================================================================ */

void test_rt_date_format_iso()
{
    printf("Testing rt_date_format ISO format...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test YYYY-MM-DD produces ISO format */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(strcmp(result, "2025-06-15") == 0);

    /* Single digit day/month with padding */
    d = rt_date_from_ymd(arena, 2025, 1, 5);
    result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(strcmp(result, "2025-01-05") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_us()
{
    printf("Testing rt_date_format US format...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test M/D/YYYY produces US format */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d, "M/D/YYYY");
    assert(strcmp(result, "6/15/2025") == 0);

    /* Single digit month and day */
    d = rt_date_from_ymd(arena, 2025, 1, 5);
    result = rt_date_format(arena, d, "M/D/YYYY");
    assert(strcmp(result, "1/5/2025") == 0);

    /* Double digit month and day */
    d = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_format(arena, d, "M/D/YYYY");
    assert(strcmp(result, "12/31/2025") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_full_readable()
{
    printf("Testing rt_date_format full readable format...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test MMMM D, YYYY produces full readable format */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d, "MMMM D, YYYY");
    assert(strcmp(result, "June 15, 2025") == 0);

    /* January */
    d = rt_date_from_ymd(arena, 2025, 1, 1);
    result = rt_date_format(arena, d, "MMMM D, YYYY");
    assert(strcmp(result, "January 1, 2025") == 0);

    /* December */
    d = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_format(arena, d, "MMMM D, YYYY");
    assert(strcmp(result, "December 31, 2025") == 0);

    /* September (longest month name) */
    d = rt_date_from_ymd(arena, 2025, 9, 10);
    result = rt_date_format(arena, d, "MMMM D, YYYY");
    assert(strcmp(result, "September 10, 2025") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_abbreviated()
{
    printf("Testing rt_date_format abbreviated format...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test ddd, MMM D produces abbreviated format */
    /* June 15, 2025 is a Sunday */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d, "ddd, MMM D");
    assert(strcmp(result, "Sun, Jun 15") == 0);

    /* Wednesday */
    d = rt_date_from_ymd(arena, 2025, 6, 11);
    result = rt_date_format(arena, d, "ddd, MMM D");
    assert(strcmp(result, "Wed, Jun 11") == 0);

    /* Saturday in December */
    d = rt_date_from_ymd(arena, 2025, 12, 6);
    result = rt_date_format(arena, d, "ddd, MMM D");
    assert(strcmp(result, "Sat, Dec 6") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_short_european()
{
    printf("Testing rt_date_format short European format...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test DD/MM/YY produces short European format */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d, "DD/MM/YY");
    assert(strcmp(result, "15/06/25") == 0);

    /* Single digit day and month with padding */
    d = rt_date_from_ymd(arena, 2025, 1, 5);
    result = rt_date_format(arena, d, "DD/MM/YY");
    assert(strcmp(result, "05/01/25") == 0);

    /* Year 2000 */
    d = rt_date_from_ymd(arena, 2000, 12, 31);
    result = rt_date_format(arena, d, "DD/MM/YY");
    assert(strcmp(result, "31/12/00") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_edge_cases()
{
    printf("Testing rt_date_format edge cases...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Consecutive tokens: YYYYMMDD */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d, "YYYYMMDD");
    assert(strcmp(result, "20250615") == 0);

    /* Token at start */
    result = rt_date_format(arena, d, "YYYY is the year");
    assert(strcmp(result, "2025 is the year") == 0);

    /* Token at end */
    result = rt_date_format(arena, d, "Year: YYYY");
    assert(strcmp(result, "Year: 2025") == 0);

    /* Empty pattern */
    result = rt_date_format(arena, d, "");
    assert(strcmp(result, "") == 0);

    /* Only literals */
    result = rt_date_format(arena, d, "Hello World");
    assert(strcmp(result, "Hello World") == 0);

    /* Mixed consecutive tokens */
    result = rt_date_format(arena, d, "DDMMYYYY");
    assert(strcmp(result, "15062025") == 0);

    /* YY at end */
    result = rt_date_format(arena, d, "MM/DD/YY");
    assert(strcmp(result, "06/15/25") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_all_tokens()
{
    printf("Testing rt_date_format with all token types...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* June 15, 2025 is a Sunday */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* Full weekday with full date */
    char *result = rt_date_format(arena, d, "dddd, MMMM D, YYYY");
    assert(strcmp(result, "Sunday, June 15, 2025") == 0);

    /* Short weekday with short month */
    result = rt_date_format(arena, d, "ddd MMM DD YYYY");
    assert(strcmp(result, "Sun Jun 15 2025") == 0);

    /* Complex pattern with many tokens */
    result = rt_date_format(arena, d, "[YY] YYYY-MM-DD (ddd)");
    assert(strcmp(result, "[25] 2025-06-15 (Sun)") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_various_dates()
{
    printf("Testing rt_date_format with various dates...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Leap year day */
    RtDate *d = rt_date_from_ymd(arena, 2024, 2, 29);
    char *result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(strcmp(result, "2024-02-29") == 0);

    /* Unix epoch */
    d = rt_date_from_ymd(arena, 1970, 1, 1);
    result = rt_date_format(arena, d, "MMMM D, YYYY");
    assert(strcmp(result, "January 1, 1970") == 0);

    /* Y2K */
    d = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_format(arena, d, "YY/MM/DD");
    assert(strcmp(result, "00/01/01") == 0);

    /* Far future */
    d = rt_date_from_ymd(arena, 2099, 12, 31);
    result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(strcmp(result, "2099-12-31") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Error Handling and Edge Cases Tests
 * ============================================================================ */

void test_rt_date_format_null_date()
{
    printf("Testing rt_date_format with NULL date...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* NULL date should return NULL (error indicator) */
    char *result = rt_date_format(arena, NULL, "YYYY-MM-DD");
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_format_null_pattern()
{
    printf("Testing rt_date_format with NULL pattern...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL pattern should return NULL (error indicator) */
    char *result = rt_date_format(arena, d, NULL);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_format_null_arena()
{
    printf("Testing rt_date_format with NULL arena...\n");

    /* Create a valid date first using a temp arena */
    RtArena *temp_arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(temp_arena, 2025, 6, 15);

    /* NULL arena should return NULL (error indicator) */
    char *result = rt_date_format(NULL, d, "YYYY-MM-DD");
    assert(result == NULL);

    rt_arena_destroy(temp_arena);
}

void test_rt_date_to_iso_null_handling()
{
    printf("Testing rt_date_to_iso with NULL handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    char *result = rt_date_to_iso(arena, NULL);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_to_iso(NULL, d);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_to_string_null_handling()
{
    printf("Testing rt_date_to_string with NULL handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    char *result = rt_date_to_string(arena, NULL);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_to_string(NULL, d);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_format_empty_pattern()
{
    printf("Testing rt_date_format with empty pattern...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* Empty pattern should return empty string */
    char *result = rt_date_format(arena, d, "");
    assert(result != NULL);
    assert(strcmp(result, "") == 0);
    assert(strlen(result) == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_no_tokens()
{
    printf("Testing rt_date_format with pattern containing no tokens...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* Pattern with no tokens should return pattern as-is */
    char *result = rt_date_format(arena, d, "Hello World");
    assert(strcmp(result, "Hello World") == 0);

    /* Pattern with special characters but no tokens */
    result = rt_date_format(arena, d, "[ ] { } ( ) - / \\ @ # $ % ^ & * ! ? < > | ~ ` + =");
    assert(strcmp(result, "[ ] { } ( ) - / \\ @ # $ % ^ & * ! ? < > | ~ ` + =") == 0);

    /* Numbers that aren't tokens */
    result = rt_date_format(arena, d, "12345 67890");
    assert(strcmp(result, "12345 67890") == 0);

    /* Single letters that could look like tokens but aren't followed by valid token chars */
    result = rt_date_format(arena, d, "abc xyz");
    assert(strcmp(result, "abc xyz") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_very_long_pattern()
{
    printf("Testing rt_date_format with very long patterns...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* Very long pattern with repeated tokens (1000+ chars output) */
    char long_pattern[2048];
    strcpy(long_pattern, "");
    for (int i = 0; i < 100; i++) {
        strcat(long_pattern, "YYYY-MM-DD ");
    }

    char *result = rt_date_format(arena, d, long_pattern);
    assert(result != NULL);

    /* Verify pattern was formatted correctly */
    assert(strlen(result) > 1000);
    /* Check first occurrence */
    assert(strncmp(result, "2025-06-15 ", 11) == 0);

    /* Very long pattern with all literal characters (no tokens) */
    char long_literal[4096];
    for (int i = 0; i < 4000; i++) {
        long_literal[i] = 'a' + (i % 26);
    }
    long_literal[4000] = '\0';

    result = rt_date_format(arena, d, long_literal);
    assert(result != NULL);
    assert(strlen(result) == 4000);

    rt_arena_destroy(arena);
}

void test_rt_date_format_buffer_overflow_protection()
{
    printf("Testing rt_date_format buffer overflow protection...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 9, 10); /* September = longest month name */

    /* Pattern that expands significantly (tokens produce longer output than pattern) */
    /* "MMMM" (4 chars) -> "September" (9 chars) */
    /* "dddd" (4 chars) -> "Wednesday" (9 chars) */
    char *result = rt_date_format(arena, d, "MMMM dddd MMMM dddd MMMM dddd");
    assert(result != NULL);
    /* Pattern is 30 chars, output should be around 60+ chars */
    assert(strlen(result) > 50);

    /* Pattern with all maximum-length tokens */
    d = rt_date_from_ymd(arena, 2025, 9, 10); /* Wednesday, September 10 */
    result = rt_date_format(arena, d, "dddd, MMMM D, YYYY");
    assert(result != NULL);
    assert(strcmp(result, "Wednesday, September 10, 2025") == 0);

    /* Many consecutive expanding tokens */
    result = rt_date_format(arena, d, "MMMMMMMMMMMM");  /* Not a token - should be literal */
    /* MMMM = September, then 8 literal M's */
    assert(result != NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_format_boundary_dates()
{
    printf("Testing rt_date_format with boundary dates...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Earliest supported date */
    RtDate *d = rt_date_from_ymd(arena, 1, 1, 1);
    char *result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(result != NULL);
    assert(strcmp(result, "0001-01-01") == 0);

    /* Latest supported date */
    d = rt_date_from_ymd(arena, 9999, 12, 31);
    result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(result != NULL);
    assert(strcmp(result, "9999-12-31") == 0);

    /* Negative epoch days (before 1970) */
    d = rt_date_from_ymd(arena, 1969, 12, 31);
    result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(result != NULL);
    assert(strcmp(result, "1969-12-31") == 0);

    /* 1900 (non-leap year despite % 4 == 0) */
    d = rt_date_from_ymd(arena, 1900, 2, 28);
    result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(result != NULL);
    assert(strcmp(result, "1900-02-28") == 0);

    /* 2000 (leap year: % 400 == 0) */
    d = rt_date_from_ymd(arena, 2000, 2, 29);
    result = rt_date_format(arena, d, "YYYY-MM-DD");
    assert(result != NULL);
    assert(strcmp(result, "2000-02-29") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_token_boundaries()
{
    printf("Testing rt_date_format token boundaries...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* Single token only */
    char *result = rt_date_format(arena, d, "YYYY");
    assert(strcmp(result, "2025") == 0);

    result = rt_date_format(arena, d, "MM");
    assert(strcmp(result, "06") == 0);

    result = rt_date_format(arena, d, "DD");
    assert(strcmp(result, "15") == 0);

    /* Token with leading literal */
    result = rt_date_format(arena, d, "Year:YYYY");
    assert(strcmp(result, "Year:2025") == 0);

    /* Token with trailing literal */
    result = rt_date_format(arena, d, "YYYY!");
    assert(strcmp(result, "2025!") == 0);

    /* Token surrounded by same char as token char (potential confusion) */
    result = rt_date_format(arena, d, "YYYYMMDD"); /* All touching */
    assert(strcmp(result, "20250615") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_partial_token_like_strings()
{
    printf("Testing rt_date_format with partial token-like strings...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* Y alone is not a token */
    char *result = rt_date_format(arena, d, "Y");
    assert(strcmp(result, "Y") == 0);

    /* YYY is YY + Y literal */
    result = rt_date_format(arena, d, "YYY");
    assert(strcmp(result, "25Y") == 0);

    /* YYYYY is YYYY + Y literal */
    result = rt_date_format(arena, d, "YYYYY");
    assert(strcmp(result, "2025Y") == 0);

    /* Single D followed by lowercase is literal */
    result = rt_date_format(arena, d, "Date: YYYY");
    assert(strcmp(result, "Date: 2025") == 0);

    /* Single M followed by lowercase is literal */
    result = rt_date_format(arena, d, "Month: MMMM");
    assert(strcmp(result, "Month: June") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main entry point for format tests
 * ============================================================================ */

void test_rt_date_format_main()
{
    printf("\n=== Date Format Tests ===\n");

    /* Basic format tests */
    test_rt_date_format_iso();
    test_rt_date_format_us();
    test_rt_date_format_full_readable();
    test_rt_date_format_abbreviated();
    test_rt_date_format_short_european();
    test_rt_date_format_edge_cases();
    test_rt_date_format_all_tokens();
    test_rt_date_format_various_dates();

    /* Error handling and edge cases */
    test_rt_date_format_null_date();
    test_rt_date_format_null_pattern();
    test_rt_date_format_null_arena();
    test_rt_date_to_iso_null_handling();
    test_rt_date_to_string_null_handling();
    test_rt_date_format_empty_pattern();
    test_rt_date_format_no_tokens();
    test_rt_date_format_very_long_pattern();
    test_rt_date_format_buffer_overflow_protection();
    test_rt_date_format_boundary_dates();
    test_rt_date_format_token_boundaries();
    test_rt_date_format_partial_token_like_strings();

    printf("All Date format tests passed!\n");
}
