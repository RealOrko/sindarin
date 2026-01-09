// tests/unit/runtime/runtime_date_tests_boundaries.c
// Tests for runtime date boundary operations, comparisons, getters, and constructors

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"

/* ============================================================================
 * Date Month/Year Boundary Tests
 * ============================================================================ */

void test_rt_date_start_of_month()
{
    printf("Testing rt_date_start_of_month...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Any date in June 2025 returns June 1, 2025 */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_start_of_month(arena, d);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 1);

    /* First day of month should return same date */
    d = rt_date_from_ymd(arena, 2025, 6, 1);
    result = rt_date_start_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 1);

    /* Last day of month */
    d = rt_date_from_ymd(arena, 2025, 6, 30);
    result = rt_date_start_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 1);

    /* Different month - January */
    d = rt_date_from_ymd(arena, 2025, 1, 15);
    result = rt_date_start_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* Different month - December */
    d = rt_date_from_ymd(arena, 2025, 12, 25);
    result = rt_date_start_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 1);

    /* Leap year February */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_start_of_month(arena, d);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_start_of_month_null_handling()
{
    printf("Testing rt_date_start_of_month with NULL handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    RtDate *result = rt_date_start_of_month(arena, NULL);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_start_of_month(NULL, d);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_end_of_month()
{
    printf("Testing rt_date_end_of_month...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Feb 2024 returns Feb 29 (leap year) */
    RtDate *d = rt_date_from_ymd(arena, 2024, 2, 15);
    RtDate *result = rt_date_end_of_month(arena, d);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Feb 2025 returns Feb 28 (non-leap year) */
    d = rt_date_from_ymd(arena, 2025, 2, 15);
    result = rt_date_end_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* June 2025 returns June 30 */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_end_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 30);

    /* January (31 days) */
    d = rt_date_from_ymd(arena, 2025, 1, 15);
    result = rt_date_end_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 31);

    /* December (31 days) */
    d = rt_date_from_ymd(arena, 2025, 12, 1);
    result = rt_date_end_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* Already on last day */
    d = rt_date_from_ymd(arena, 2025, 6, 30);
    result = rt_date_end_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 30);

    /* April (30 days) */
    d = rt_date_from_ymd(arena, 2025, 4, 10);
    result = rt_date_end_of_month(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 30);

    rt_arena_destroy(arena);
}

void test_rt_date_end_of_month_null_handling()
{
    printf("Testing rt_date_end_of_month with NULL handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    RtDate *result = rt_date_end_of_month(arena, NULL);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_end_of_month(NULL, d);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_start_of_year()
{
    printf("Testing rt_date_start_of_year...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Any date in 2025 returns January 1, 2025 */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_start_of_year(arena, d);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* First day of year should return same date */
    d = rt_date_from_ymd(arena, 2025, 1, 1);
    result = rt_date_start_of_year(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* Last day of year */
    d = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_start_of_year(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* Different year - 2024 */
    d = rt_date_from_ymd(arena, 2024, 7, 4);
    result = rt_date_start_of_year(arena, d);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* Different year - 2000 (leap year) */
    d = rt_date_from_ymd(arena, 2000, 2, 29);
    result = rt_date_start_of_year(arena, d);
    assert(rt_date_get_year(result) == 2000);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* Early year - 1900 */
    d = rt_date_from_ymd(arena, 1900, 6, 15);
    result = rt_date_start_of_year(arena, d);
    assert(rt_date_get_year(result) == 1900);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_start_of_year_null_handling()
{
    printf("Testing rt_date_start_of_year with NULL handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    RtDate *result = rt_date_start_of_year(arena, NULL);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_start_of_year(NULL, d);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_end_of_year()
{
    printf("Testing rt_date_end_of_year...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Any date in 2025 returns December 31, 2025 */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_end_of_year(arena, d);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* First day of year should return December 31 of same year */
    d = rt_date_from_ymd(arena, 2025, 1, 1);
    result = rt_date_end_of_year(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* Last day of year should return same date */
    d = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_end_of_year(arena, d);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* Different year - 2024 */
    d = rt_date_from_ymd(arena, 2024, 7, 4);
    result = rt_date_end_of_year(arena, d);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* Different year - 2000 (leap year) */
    d = rt_date_from_ymd(arena, 2000, 2, 29);
    result = rt_date_end_of_year(arena, d);
    assert(rt_date_get_year(result) == 2000);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* Early year - 1900 */
    d = rt_date_from_ymd(arena, 1900, 6, 15);
    result = rt_date_end_of_year(arena, d);
    assert(rt_date_get_year(result) == 1900);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    rt_arena_destroy(arena);
}

void test_rt_date_end_of_year_null_handling()
{
    printf("Testing rt_date_end_of_year with NULL handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    RtDate *result = rt_date_end_of_year(arena, NULL);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_end_of_year(NULL, d);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_calculate_target_year_month_positive()
{
    printf("Testing rt_date_calculate_target_year_month with positive months...\n");

    int out_year, out_month;

    /* (2025, 1, 1) -> (2025, 2) - Add 1 month within same year */
    rt_date_calculate_target_year_month(2025, 1, 1, &out_year, &out_month);
    assert(out_year == 2025);
    assert(out_month == 2);

    /* (2025, 12, 2) -> (2026, 2) - Add 2 months crossing year boundary */
    rt_date_calculate_target_year_month(2025, 12, 2, &out_year, &out_month);
    assert(out_year == 2026);
    assert(out_month == 2);

    /* (2025, 1, 13) -> (2026, 2) - Add more than 12 months */
    rt_date_calculate_target_year_month(2025, 1, 13, &out_year, &out_month);
    assert(out_year == 2026);
    assert(out_month == 2);

    /* Add 24 months (2 years) */
    rt_date_calculate_target_year_month(2025, 6, 24, &out_year, &out_month);
    assert(out_year == 2027);
    assert(out_month == 6);
}

void test_rt_date_calculate_target_year_month_negative()
{
    printf("Testing rt_date_calculate_target_year_month with negative months...\n");

    int out_year, out_month;

    /* (2025, 3, -5) -> (2024, 10) - Subtract months crossing year boundary */
    rt_date_calculate_target_year_month(2025, 3, -5, &out_year, &out_month);
    assert(out_year == 2024);
    assert(out_month == 10);

    /* Subtract 1 month within same year */
    rt_date_calculate_target_year_month(2025, 6, -1, &out_year, &out_month);
    assert(out_year == 2025);
    assert(out_month == 5);

    /* Subtract 12 months (1 year) */
    rt_date_calculate_target_year_month(2025, 6, -12, &out_year, &out_month);
    assert(out_year == 2024);
    assert(out_month == 6);

    /* Subtract from January crosses to previous year December */
    rt_date_calculate_target_year_month(2025, 1, -1, &out_year, &out_month);
    assert(out_year == 2024);
    assert(out_month == 12);
}

void test_rt_date_calculate_target_year_month_zero()
{
    printf("Testing rt_date_calculate_target_year_month with zero months...\n");

    int out_year, out_month;

    /* (2025, 6, 0) -> (2025, 6) - Zero months returns same */
    rt_date_calculate_target_year_month(2025, 6, 0, &out_year, &out_month);
    assert(out_year == 2025);
    assert(out_month == 6);

    /* Test with different months */
    rt_date_calculate_target_year_month(2025, 1, 0, &out_year, &out_month);
    assert(out_year == 2025);
    assert(out_month == 1);

    rt_date_calculate_target_year_month(2025, 12, 0, &out_year, &out_month);
    assert(out_year == 2025);
    assert(out_month == 12);
}

void test_rt_date_clamp_day_to_month()
{
    printf("Testing rt_date_clamp_day_to_month...\n");

    /* clamp_day_to_month(31, 2025, 2) returns 28 (non-leap year February) */
    assert(rt_date_clamp_day_to_month(31, 2025, 2) == 28);

    /* clamp_day_to_month(31, 2024, 2) returns 29 (leap year February) */
    assert(rt_date_clamp_day_to_month(31, 2024, 2) == 29);

    /* clamp_day_to_month(31, 2025, 4) returns 30 (April has 30 days) */
    assert(rt_date_clamp_day_to_month(31, 2025, 4) == 30);

    /* clamp_day_to_month(15, 2025, 2) returns 15 (no clamping needed) */
    assert(rt_date_clamp_day_to_month(15, 2025, 2) == 15);

    /* Test with months that have 31 days - no clamping needed */
    assert(rt_date_clamp_day_to_month(31, 2025, 1) == 31);   /* January */
    assert(rt_date_clamp_day_to_month(31, 2025, 3) == 31);   /* March */
    assert(rt_date_clamp_day_to_month(31, 2025, 12) == 31);  /* December */

    /* Test clamping from 30 day months */
    assert(rt_date_clamp_day_to_month(30, 2025, 2) == 28);   /* Feb non-leap */
    assert(rt_date_clamp_day_to_month(30, 2024, 2) == 29);   /* Feb leap */

    /* Test day exactly equal to max - should return same */
    assert(rt_date_clamp_day_to_month(28, 2025, 2) == 28);
    assert(rt_date_clamp_day_to_month(29, 2024, 2) == 29);
    assert(rt_date_clamp_day_to_month(30, 2025, 4) == 30);
}

/* ============================================================================
 * Comprehensive Edge Case Tests
 * ============================================================================ */

void test_rt_date_epoch_boundaries()
{
    printf("Testing epoch boundary dates...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Epoch date: 1970-01-01 */
    d = rt_date_from_ymd(arena, 1970, 1, 1);
    assert(rt_date_get_year(d) == 1970);
    assert(rt_date_get_month(d) == 1);
    assert(rt_date_get_day(d) == 1);

    /* One day before epoch */
    result = rt_date_add_days(arena, d, -1);
    assert(rt_date_get_year(result) == 1969);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* One day after epoch */
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 1970);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 2);

    /* One year before epoch */
    result = rt_date_add_years(arena, d, -1);
    assert(rt_date_get_year(result) == 1969);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* One month before epoch */
    result = rt_date_add_months(arena, d, -1);
    assert(rt_date_get_year(result) == 1969);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 1);

    /* Diff between epoch and day before */
    RtDate *day_before = rt_date_from_ymd(arena, 1969, 12, 31);
    int64_t diff = rt_date_diff_days(d, day_before);
    assert(diff == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_year_boundary_transitions()
{
    printf("Testing year boundary transitions...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Dec 31 to Jan 1 with addDays */
    d = rt_date_from_ymd(arena, 2024, 12, 31);
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* Jan 1 to Dec 31 with addDays(-1) */
    d = rt_date_from_ymd(arena, 2025, 1, 1);
    result = rt_date_add_days(arena, d, -1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* Dec 31 + 1 week crosses year */
    d = rt_date_from_ymd(arena, 2024, 12, 31);
    result = rt_date_add_weeks(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 7);

    /* Dec 15 + 1 month = Jan 15 next year */
    d = rt_date_from_ymd(arena, 2024, 12, 15);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 15);

    /* Jan 15 - 1 month = Dec 15 previous year */
    d = rt_date_from_ymd(arena, 2025, 1, 15);
    result = rt_date_add_months(arena, d, -1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 15);

    rt_arena_destroy(arena);
}

void test_rt_date_leap_year_transitions()
{
    printf("Testing leap year transitions...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Feb 28, 2024 + 1 day = Feb 29, 2024 (leap year) */
    d = rt_date_from_ymd(arena, 2024, 2, 28);
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Feb 29, 2024 + 1 day = Mar 1, 2024 */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 1);

    /* Feb 28, 2025 + 1 day = Mar 1, 2025 (non-leap year) */
    d = rt_date_from_ymd(arena, 2025, 2, 28);
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 1);

    /* Mar 1, 2024 - 1 day = Feb 29, 2024 */
    d = rt_date_from_ymd(arena, 2024, 3, 1);
    result = rt_date_add_days(arena, d, -1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Mar 1, 2025 - 1 day = Feb 28, 2025 */
    d = rt_date_from_ymd(arena, 2025, 3, 1);
    result = rt_date_add_days(arena, d, -1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Feb 29, 2024 + 1 year = Feb 28, 2025 (clamped) */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_years(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Feb 28, 2025 + 1 year = Feb 28, 2026 */
    d = rt_date_from_ymd(arena, 2025, 2, 28);
    result = rt_date_add_years(arena, d, 1);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Diff days between Feb 28, 2024 and Mar 1, 2024 = 2 (leap year) */
    RtDate *feb28 = rt_date_from_ymd(arena, 2024, 2, 28);
    RtDate *mar1 = rt_date_from_ymd(arena, 2024, 3, 1);
    int64_t diff = rt_date_diff_days(mar1, feb28);
    assert(diff == 2);

    /* Diff days between Feb 28, 2025 and Mar 1, 2025 = 1 (non-leap year) */
    feb28 = rt_date_from_ymd(arena, 2025, 2, 28);
    mar1 = rt_date_from_ymd(arena, 2025, 3, 1);
    diff = rt_date_diff_days(mar1, feb28);
    assert(diff == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_large_arithmetic_values()
{
    printf("Testing large arithmetic values...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Add 10000 days */
    d = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_add_days(arena, d, 10000);
    assert(rt_date_get_year(result) == 2027);
    assert(rt_date_get_month(result) == 5);
    assert(rt_date_get_day(result) == 19);

    /* Subtract 10000 days */
    d = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_add_days(arena, d, -10000);
    assert(rt_date_get_year(result) == 1972);
    assert(rt_date_get_month(result) == 8);
    assert(rt_date_get_day(result) == 15);

    /* Add 1000 weeks (~19 years) */
    d = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_add_weeks(arena, d, 1000);
    assert(rt_date_get_year(result) == 2019);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 2);

    /* Add 500 months (~41 years) */
    d = rt_date_from_ymd(arena, 2000, 1, 15);
    result = rt_date_add_months(arena, d, 500);
    assert(rt_date_get_year(result) == 2041);
    assert(rt_date_get_month(result) == 9);
    assert(rt_date_get_day(result) == 15);

    /* Subtract 500 months */
    d = rt_date_from_ymd(arena, 2000, 1, 15);
    result = rt_date_add_months(arena, d, -500);
    assert(rt_date_get_year(result) == 1958);
    assert(rt_date_get_month(result) == 5);
    assert(rt_date_get_day(result) == 15);

    /* Add 500 years */
    d = rt_date_from_ymd(arena, 2000, 6, 15);
    result = rt_date_add_years(arena, d, 500);
    assert(rt_date_get_year(result) == 2500);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Subtract 500 years */
    d = rt_date_from_ymd(arena, 2000, 6, 15);
    result = rt_date_add_years(arena, d, -500);
    assert(rt_date_get_year(result) == 1500);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    rt_arena_destroy(arena);
}

void test_rt_date_far_future_dates()
{
    printf("Testing far future dates (year 3000, 5000)...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Year 3000 */
    d = rt_date_from_ymd(arena, 3000, 6, 15);
    assert(d != NULL);
    assert(rt_date_get_year(d) == 3000);
    assert(rt_date_get_month(d) == 6);
    assert(rt_date_get_day(d) == 15);

    /* Year 5000 */
    d = rt_date_from_ymd(arena, 5000, 12, 31);
    assert(d != NULL);
    assert(rt_date_get_year(d) == 5000);
    assert(rt_date_get_month(d) == 12);
    assert(rt_date_get_day(d) == 31);

    /* Arithmetic on far future dates */
    d = rt_date_from_ymd(arena, 3000, 1, 1);
    result = rt_date_add_days(arena, d, 365);
    assert(rt_date_get_year(result) == 3001);

    result = rt_date_add_months(arena, d, 12);
    assert(rt_date_get_year(result) == 3001);

    result = rt_date_add_years(arena, d, 100);
    assert(rt_date_get_year(result) == 3100);

    /* Format far future date */
    char *str = rt_date_to_iso(arena, d);
    assert(str != NULL);
    assert(strcmp(str, "3000-01-01") == 0);

    /* Comparison with far future */
    RtDate *d1 = rt_date_from_ymd(arena, 3000, 1, 1);
    RtDate *d2 = rt_date_from_ymd(arena, 5000, 1, 1);
    assert(rt_date_is_before(d1, d2) == 1);
    assert(rt_date_is_after(d2, d1) == 1);

    /* Diff days with large difference */
    int64_t diff = rt_date_diff_days(d2, d1);
    assert(diff > 0); /* Should be positive and large */

    rt_arena_destroy(arena);
}

void test_rt_date_far_past_dates()
{
    printf("Testing far past dates (year 1000, 1500)...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Year 1000 */
    d = rt_date_from_ymd(arena, 1000, 6, 15);
    assert(d != NULL);
    assert(rt_date_get_year(d) == 1000);
    assert(rt_date_get_month(d) == 6);
    assert(rt_date_get_day(d) == 15);

    /* Year 1500 */
    d = rt_date_from_ymd(arena, 1500, 1, 1);
    assert(d != NULL);
    assert(rt_date_get_year(d) == 1500);
    assert(rt_date_get_month(d) == 1);
    assert(rt_date_get_day(d) == 1);

    /* Arithmetic on far past dates */
    d = rt_date_from_ymd(arena, 1000, 12, 31);
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 1001);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    result = rt_date_add_months(arena, d, -12);
    assert(rt_date_get_year(result) == 999);

    result = rt_date_add_years(arena, d, -500);
    assert(rt_date_get_year(result) == 500);

    /* Format far past date */
    d = rt_date_from_ymd(arena, 1000, 1, 1);
    char *str = rt_date_to_iso(arena, d);
    assert(str != NULL);
    assert(strcmp(str, "1000-01-01") == 0);

    /* Year 1900 - special non-leap year */
    d = rt_date_from_ymd(arena, 1900, 2, 28);
    assert(d != NULL);
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_month(result) == 3); /* Skips Feb 29 */
    assert(rt_date_get_day(result) == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_all_methods_with_edge_dates()
{
    printf("Testing all methods handle edge dates without crashes...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Test with year 1 */
    d = rt_date_from_ymd(arena, 1, 1, 1);
    assert(d != NULL);
    assert(rt_date_get_year(d) == 1);
    assert(rt_date_get_month(d) == 1);
    assert(rt_date_get_day(d) == 1);
    assert(rt_date_get_weekday(d) >= 1 && rt_date_get_weekday(d) <= 7);
    assert(rt_date_get_day_of_year(d) == 1);
    assert(rt_date_get_days_in_month(d) == 31);
    assert(rt_date_is_leap(d) == 0); /* Year 1 is not a leap year */
    assert(rt_date_to_iso(arena, d) != NULL);
    assert(rt_date_to_string(arena, d) != NULL);
    assert(rt_date_format(arena, d, "YYYY-MM-DD") != NULL);
    result = rt_date_start_of_month(arena, d);
    assert(result != NULL);
    result = rt_date_end_of_month(arena, d);
    assert(result != NULL);
    result = rt_date_start_of_year(arena, d);
    assert(result != NULL);
    result = rt_date_end_of_year(arena, d);
    assert(result != NULL);

    /* Test with year 9999 */
    d = rt_date_from_ymd(arena, 9999, 12, 31);
    assert(d != NULL);
    assert(rt_date_get_year(d) == 9999);
    assert(rt_date_get_month(d) == 12);
    assert(rt_date_get_day(d) == 31);
    assert(rt_date_get_weekday(d) >= 1 && rt_date_get_weekday(d) <= 7);
    assert(rt_date_get_day_of_year(d) == 365);
    assert(rt_date_get_days_in_month(d) == 31);
    assert(rt_date_to_iso(arena, d) != NULL);
    assert(rt_date_to_string(arena, d) != NULL);
    assert(rt_date_format(arena, d, "YYYY-MM-DD") != NULL);
    result = rt_date_start_of_month(arena, d);
    assert(result != NULL);
    result = rt_date_end_of_month(arena, d);
    assert(result != NULL);
    result = rt_date_start_of_year(arena, d);
    assert(result != NULL);
    result = rt_date_end_of_year(arena, d);
    assert(result != NULL);

    /* Arithmetic operations on edge dates */
    d = rt_date_from_ymd(arena, 1, 1, 1);
    result = rt_date_add_days(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_day(result) == 2);

    result = rt_date_add_weeks(arena, d, 1);
    assert(result != NULL);

    result = rt_date_add_months(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_month(result) == 2);

    result = rt_date_add_years(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2);

    /* Date/Time conversion with edge dates */
    d = rt_date_from_ymd(arena, 1970, 1, 1); /* Epoch */
    RtTime *t = rt_date_to_time(arena, d);
    assert(t != NULL);
    /* Should be midnight on the epoch date */
    assert(rt_time_get_hour(t) == 0);
    assert(rt_time_get_minute(t) == 0);
    assert(rt_time_get_second(t) == 0);
    assert(rt_time_get_year(t) == 1970);
    assert(rt_time_get_month(t) == 1);
    assert(rt_time_get_day(t) == 1);

    d = rt_date_from_ymd(arena, 1969, 12, 31); /* Day before epoch */
    t = rt_date_to_time(arena, d);
    assert(t != NULL);
    /* Should be midnight on 1969-12-31 */
    assert(rt_time_get_hour(t) == 0);
    assert(rt_time_get_minute(t) == 0);
    assert(rt_time_get_second(t) == 0);
    assert(rt_time_get_year(t) == 1969);
    assert(rt_time_get_month(t) == 12);
    assert(rt_time_get_day(t) == 31);

    rt_arena_destroy(arena);
}

void test_rt_date_boundary_conditions()
{
    printf("Testing boundary conditions (month 0/13, day 0/32)...\n");

    /* Note: rt_date_from_ymd with invalid dates calls exit(1) in the runtime,
     * so we cannot test those cases directly. Instead, we test the validation
     * functions and the boundary behavior of valid dates. */

    RtArena *arena = rt_arena_create(NULL);

    /* Test daysInMonth with invalid month values (should return 0) */
    assert(rt_date_days_in_month(2025, 0) == 0);
    assert(rt_date_days_in_month(2025, 13) == 0);
    assert(rt_date_days_in_month(2025, -1) == 0);
    assert(rt_date_days_in_month(2025, 100) == 0);

    /* Test isValidYmd with boundary conditions */
    assert(rt_date_is_valid_ymd(2025, 0, 15) == 0);   /* Month 0 invalid */
    assert(rt_date_is_valid_ymd(2025, 13, 15) == 0);  /* Month 13 invalid */
    assert(rt_date_is_valid_ymd(2025, 1, 0) == 0);    /* Day 0 invalid */
    assert(rt_date_is_valid_ymd(2025, 1, 32) == 0);   /* Day 32 invalid */
    assert(rt_date_is_valid_ymd(2025, 2, 29) == 0);   /* Feb 29 in non-leap invalid */
    assert(rt_date_is_valid_ymd(2024, 2, 29) == 1);   /* Feb 29 in leap valid */
    assert(rt_date_is_valid_ymd(2025, 4, 31) == 0);   /* Apr 31 invalid */
    assert(rt_date_is_valid_ymd(2025, 6, 31) == 0);   /* Jun 31 invalid */
    assert(rt_date_is_valid_ymd(2025, 9, 31) == 0);   /* Sep 31 invalid */
    assert(rt_date_is_valid_ymd(2025, 11, 31) == 0);  /* Nov 31 invalid */

    /* Test valid boundary dates */
    RtDate *d;
    d = rt_date_from_ymd(arena, 2025, 1, 1);  /* First day of year */
    assert(d != NULL);

    d = rt_date_from_ymd(arena, 2025, 12, 31);  /* Last day of year */
    assert(d != NULL);

    d = rt_date_from_ymd(arena, 2024, 2, 29);  /* Leap day */
    assert(d != NULL);

    d = rt_date_from_ymd(arena, 2025, 1, 31);  /* 31-day month end */
    assert(d != NULL);

    d = rt_date_from_ymd(arena, 2025, 4, 30);  /* 30-day month end */
    assert(d != NULL);

    d = rt_date_from_ymd(arena, 2025, 2, 28);  /* Non-leap Feb end */
    assert(d != NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_month_end_clamping_all_months()
{
    printf("Testing month-end clamping for all months...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Jan 31 + 1 month = Feb 28/29 */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28); /* 2025 is not a leap year */

    d = rt_date_from_ymd(arena, 2024, 1, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29); /* 2024 is a leap year */

    /* Mar 31 + 1 month = Apr 30 */
    d = rt_date_from_ymd(arena, 2025, 3, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 30);

    /* May 31 + 1 month = Jun 30 */
    d = rt_date_from_ymd(arena, 2025, 5, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 30);

    /* Jul 31 + 1 month = Aug 31 (no clamping needed) */
    d = rt_date_from_ymd(arena, 2025, 7, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_month(result) == 8);
    assert(rt_date_get_day(result) == 31);

    /* Aug 31 + 1 month = Sep 30 */
    d = rt_date_from_ymd(arena, 2025, 8, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_month(result) == 9);
    assert(rt_date_get_day(result) == 30);

    /* Oct 31 + 1 month = Nov 30 */
    d = rt_date_from_ymd(arena, 2025, 10, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_month(result) == 11);
    assert(rt_date_get_day(result) == 30);

    /* Dec 31 + 1 month = Jan 31 (no clamping needed) */
    d = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 31);

    /* Jan 30 + 1 month = Feb 28 (clamped) */
    d = rt_date_from_ymd(arena, 2025, 1, 30);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Jan 29 + 1 month = Feb 28 in non-leap, Feb 29 in leap */
    d = rt_date_from_ymd(arena, 2025, 1, 29);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_day(result) == 28);

    d = rt_date_from_ymd(arena, 2024, 1, 29);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_day(result) == 29);

    rt_arena_destroy(arena);
}

void test_rt_date_diff_days_symmetry()
{
    printf("Testing diffDays symmetry...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test multiple date pairs for symmetry: a.diffDays(b) == -b.diffDays(a) */

    /* Same year dates */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 15);
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 6, 20);
    int64_t diff1 = rt_date_diff_days(d1, d2);
    int64_t diff2 = rt_date_diff_days(d2, d1);
    assert(diff1 == -diff2);

    /* Different year dates */
    d1 = rt_date_from_ymd(arena, 2020, 3, 15);
    d2 = rt_date_from_ymd(arena, 2025, 11, 30);
    diff1 = rt_date_diff_days(d1, d2);
    diff2 = rt_date_diff_days(d2, d1);
    assert(diff1 == -diff2);

    /* Crossing leap year boundary */
    d1 = rt_date_from_ymd(arena, 2024, 2, 28);
    d2 = rt_date_from_ymd(arena, 2024, 3, 1);
    diff1 = rt_date_diff_days(d1, d2);
    diff2 = rt_date_diff_days(d2, d1);
    assert(diff1 == -diff2);

    /* Epoch boundary */
    d1 = rt_date_from_ymd(arena, 1969, 12, 31);
    d2 = rt_date_from_ymd(arena, 1970, 1, 2);
    diff1 = rt_date_diff_days(d1, d2);
    diff2 = rt_date_diff_days(d2, d1);
    assert(diff1 == -diff2);
    assert(diff2 == 2);

    /* Large date range */
    d1 = rt_date_from_ymd(arena, 1900, 1, 1);
    d2 = rt_date_from_ymd(arena, 2100, 12, 31);
    diff1 = rt_date_diff_days(d1, d2);
    diff2 = rt_date_diff_days(d2, d1);
    assert(diff1 == -diff2);

    rt_arena_destroy(arena);
}

void test_rt_date_roundtrip_add_days()
{
    printf("Testing round-trip addDays...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *original;
    RtDate *result;

    /* Basic round-trip */
    original = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_days(arena, original, 100);
    result = rt_date_add_days(arena, result, -100);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Round-trip across year boundary */
    original = rt_date_from_ymd(arena, 2024, 12, 31);
    result = rt_date_add_days(arena, original, 365);
    result = rt_date_add_days(arena, result, -365);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    /* Round-trip with large values */
    original = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_add_days(arena, original, 10000);
    result = rt_date_add_days(arena, result, -10000);
    assert(rt_date_get_year(result) == 2000);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    /* Round-trip across epoch */
    original = rt_date_from_ymd(arena, 1970, 1, 1);
    result = rt_date_add_days(arena, original, -365);
    result = rt_date_add_days(arena, result, 365);
    assert(rt_date_get_year(result) == 1970);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_roundtrip_add_weeks()
{
    printf("Testing round-trip addWeeks...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *original;
    RtDate *result;

    /* Basic round-trip */
    original = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_weeks(arena, original, 52);
    result = rt_date_add_weeks(arena, result, -52);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Round-trip with large values */
    original = rt_date_from_ymd(arena, 2000, 6, 15);
    result = rt_date_add_weeks(arena, original, 1000);
    result = rt_date_add_weeks(arena, result, -1000);
    assert(rt_date_get_year(result) == 2000);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    rt_arena_destroy(arena);
}

void test_rt_date_consistency_weeks_days()
{
    printf("Testing consistency between weeks and days...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *original;
    RtDate *by_weeks;
    RtDate *by_days;

    /* addWeeks(n) should equal addDays(n * 7) */
    original = rt_date_from_ymd(arena, 2025, 6, 15);

    by_weeks = rt_date_add_weeks(arena, original, 10);
    by_days = rt_date_add_days(arena, original, 70);
    assert(rt_date_diff_days(by_weeks, by_days) == 0);

    by_weeks = rt_date_add_weeks(arena, original, -5);
    by_days = rt_date_add_days(arena, original, -35);
    assert(rt_date_diff_days(by_weeks, by_days) == 0);

    by_weeks = rt_date_add_weeks(arena, original, 100);
    by_days = rt_date_add_days(arena, original, 700);
    assert(rt_date_diff_days(by_weeks, by_days) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Date Comparison Tests
 * ============================================================================ */

void test_rt_date_is_before()
{
    printf("Testing rt_date_is_before...\n");

    RtArena *arena = rt_arena_create(NULL);

    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 1);
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 1, 2);
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 1, 1);

    /* d1 is before d2 */
    assert(rt_date_is_before(d1, d2) == 1);

    /* d2 is not before d1 */
    assert(rt_date_is_before(d2, d1) == 0);

    /* d1 is not before itself (same date) */
    assert(rt_date_is_before(d1, d3) == 0);

    /* Test with different years */
    RtDate *y1 = rt_date_from_ymd(arena, 2024, 12, 31);
    RtDate *y2 = rt_date_from_ymd(arena, 2025, 1, 1);
    assert(rt_date_is_before(y1, y2) == 1);
    assert(rt_date_is_before(y2, y1) == 0);

    /* Test with epoch dates */
    RtDate *epoch = rt_date_from_ymd(arena, 1970, 1, 1);
    RtDate *before_epoch = rt_date_from_ymd(arena, 1969, 12, 31);
    assert(rt_date_is_before(before_epoch, epoch) == 1);
    assert(rt_date_is_before(epoch, before_epoch) == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_is_before_null_handling()
{
    printf("Testing rt_date_is_before with NULL...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 1, 1);

    /* NULL date returns 0 */
    assert(rt_date_is_before(NULL, d) == 0);
    assert(rt_date_is_before(d, NULL) == 0);
    assert(rt_date_is_before(NULL, NULL) == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_is_after()
{
    printf("Testing rt_date_is_after...\n");

    RtArena *arena = rt_arena_create(NULL);

    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 1);
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 1, 2);
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 1, 1);

    /* d2 is after d1 */
    assert(rt_date_is_after(d2, d1) == 1);

    /* d1 is not after d2 */
    assert(rt_date_is_after(d1, d2) == 0);

    /* d1 is not after itself (same date) */
    assert(rt_date_is_after(d1, d3) == 0);

    /* Test with different years */
    RtDate *y1 = rt_date_from_ymd(arena, 2024, 12, 31);
    RtDate *y2 = rt_date_from_ymd(arena, 2025, 1, 1);
    assert(rt_date_is_after(y2, y1) == 1);
    assert(rt_date_is_after(y1, y2) == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_is_after_null_handling()
{
    printf("Testing rt_date_is_after with NULL...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 1, 1);

    /* NULL date returns 0 */
    assert(rt_date_is_after(NULL, d) == 0);
    assert(rt_date_is_after(d, NULL) == 0);
    assert(rt_date_is_after(NULL, NULL) == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_equals()
{
    printf("Testing rt_date_equals...\n");

    RtArena *arena = rt_arena_create(NULL);

    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 6, 16);

    /* Same date values are equal */
    assert(rt_date_equals(d1, d2) == 1);

    /* Different dates are not equal */
    assert(rt_date_equals(d1, d3) == 0);

    /* Test with epoch date */
    RtDate *epoch1 = rt_date_from_ymd(arena, 1970, 1, 1);
    RtDate *epoch2 = rt_date_from_epoch_days(arena, 0);
    assert(rt_date_equals(epoch1, epoch2) == 1);

    /* Test with dates before epoch */
    RtDate *before1 = rt_date_from_ymd(arena, 1969, 12, 31);
    RtDate *before2 = rt_date_from_epoch_days(arena, -1);
    assert(rt_date_equals(before1, before2) == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_equals_null_handling()
{
    printf("Testing rt_date_equals with NULL...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 1, 1);

    /* NULL date returns 0 */
    assert(rt_date_equals(NULL, d) == 0);
    assert(rt_date_equals(d, NULL) == 0);
    assert(rt_date_equals(NULL, NULL) == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_comparison_consistency()
{
    printf("Testing comparison method consistency...\n");

    RtArena *arena = rt_arena_create(NULL);

    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 6, 16);
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 6, 15);

    /* For d1 < d2: isBefore(d1, d2) and isAfter(d2, d1) */
    assert(rt_date_is_before(d1, d2) == 1);
    assert(rt_date_is_after(d2, d1) == 1);
    assert(rt_date_equals(d1, d2) == 0);

    /* For d1 == d3: !isBefore and !isAfter and equals */
    assert(rt_date_is_before(d1, d3) == 0);
    assert(rt_date_is_after(d1, d3) == 0);
    assert(rt_date_equals(d1, d3) == 1);

    /* Exactly one of (isBefore, equals, isAfter) should be true */
    int before = rt_date_is_before(d1, d2);
    int after = rt_date_is_after(d1, d2);
    int equal = rt_date_equals(d1, d2);
    assert(before + after + equal == 1);

    before = rt_date_is_before(d1, d3);
    after = rt_date_is_after(d1, d3);
    equal = rt_date_equals(d1, d3);
    assert(before + after + equal == 1);

    /* Reflexivity: a.equals(a) is always true */
    assert(rt_date_equals(d1, d1) == 1);
    assert(rt_date_equals(d2, d2) == 1);

    /* Reflexivity: a.isBefore(a) is always false */
    assert(rt_date_is_before(d1, d1) == 0);
    assert(rt_date_is_before(d2, d2) == 0);

    /* Reflexivity: a.isAfter(a) is always false */
    assert(rt_date_is_after(d1, d1) == 0);
    assert(rt_date_is_after(d2, d2) == 0);

    /* Transitivity: if a < b and b < c, then a < c */
    RtDate *a = rt_date_from_ymd(arena, 2025, 1, 1);
    RtDate *b = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *c = rt_date_from_ymd(arena, 2025, 12, 31);

    assert(rt_date_is_before(a, b) == 1);  /* a < b */
    assert(rt_date_is_before(b, c) == 1);  /* b < c */
    assert(rt_date_is_before(a, c) == 1);  /* a < c (transitivity) */

    /* Transitivity for isAfter: if a > b and b > c, then a > c */
    assert(rt_date_is_after(c, b) == 1);   /* c > b */
    assert(rt_date_is_after(b, a) == 1);   /* b > a */
    assert(rt_date_is_after(c, a) == 1);   /* c > a (transitivity) */

    /* Symmetry of equals: a.equals(b) implies b.equals(a) */
    RtDate *x = rt_date_from_ymd(arena, 2025, 3, 15);
    RtDate *y = rt_date_from_ymd(arena, 2025, 3, 15);
    assert(rt_date_equals(x, y) == 1);
    assert(rt_date_equals(y, x) == 1);

    /* Antisymmetry: if a.isBefore(b) then !b.isBefore(a) */
    assert(rt_date_is_before(a, b) == 1);
    assert(rt_date_is_before(b, a) == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Date/Time Conversion Tests
 * ============================================================================ */

void test_rt_date_to_time()
{
    printf("Testing rt_date_to_time...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Epoch date should convert to midnight local time on 1970-01-01 */
    RtDate *epoch = rt_date_from_ymd(arena, 1970, 1, 1);
    RtTime *t = rt_date_to_time(arena, epoch);
    assert(t != NULL);
    /* Verify it's midnight (hour, minute, second should be 0) */
    assert(rt_time_get_hour(t) == 0);
    assert(rt_time_get_minute(t) == 0);
    assert(rt_time_get_second(t) == 0);
    /* Verify the date components */
    assert(rt_time_get_year(t) == 1970);
    assert(rt_time_get_month(t) == 1);
    assert(rt_time_get_day(t) == 1);

    /* 2025-06-15 should convert to midnight on that day */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    t = rt_date_to_time(arena, d);
    assert(t != NULL);
    /* Verify it's midnight (hour, minute, second should be 0) */
    assert(rt_time_get_hour(t) == 0);
    assert(rt_time_get_minute(t) == 0);
    assert(rt_time_get_second(t) == 0);
    /* Verify the date components */
    assert(rt_time_get_year(t) == 2025);
    assert(rt_time_get_month(t) == 6);
    assert(rt_time_get_day(t) == 15);

    /* Day before epoch should be midnight on 1969-12-31 */
    RtDate *before = rt_date_from_ymd(arena, 1969, 12, 31);
    t = rt_date_to_time(arena, before);
    assert(t != NULL);
    /* Verify it's midnight (hour, minute, second should be 0) */
    assert(rt_time_get_hour(t) == 0);
    assert(rt_time_get_minute(t) == 0);
    assert(rt_time_get_second(t) == 0);
    /* Verify the date components */
    assert(rt_time_get_year(t) == 1969);
    assert(rt_time_get_month(t) == 12);
    assert(rt_time_get_day(t) == 31);

    /* Test that consecutive days differ by exactly 24 hours (86400000 ms) */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 3, 15);
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 3, 16);
    RtTime *t1 = rt_date_to_time(arena, d1);
    RtTime *t2 = rt_date_to_time(arena, d2);
    long long diff = rt_time_get_millis(t2) - rt_time_get_millis(t1);
    assert(diff == 86400000LL);

    rt_arena_destroy(arena);
}

void test_rt_date_to_time_null_handling()
{
    printf("Testing rt_date_to_time with NULL...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 1, 1);

    /* NULL arena or date returns NULL */
    assert(rt_date_to_time(NULL, d) == NULL);
    assert(rt_date_to_time(arena, NULL) == NULL);

    rt_arena_destroy(arena);
}

void test_rt_time_get_date()
{
    printf("Testing rt_time_get_date...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Use rt_date_to_time to create a midnight time, which is timezone-aware */
    RtDate *epoch_date = rt_date_from_ymd(arena, 1970, 1, 1);
    RtTime *t_epoch = rt_date_to_time(arena, epoch_date);
    RtDate *d = rt_time_get_date(arena, t_epoch);
    assert(d != NULL);
    assert(rt_date_get_year(d) == 1970);
    assert(rt_date_get_month(d) == 1);
    assert(rt_date_get_day(d) == 1);

    /* Time with non-zero hour/minute/second should still give correct date */
    /* Start from midnight on 2025-06-15 and add 14h 30m 45s */
    RtDate *d_test = rt_date_from_ymd(arena, 2025, 6, 15);
    RtTime *midnight = rt_date_to_time(arena, d_test);
    long long ms = rt_time_get_millis(midnight) + 14LL * 60LL * 60LL * 1000LL + 30LL * 60LL * 1000LL + 45LL * 1000LL;
    RtTime *t = rt_time_from_millis(arena, ms);
    d = rt_time_get_date(arena, t);
    assert(d != NULL);
    assert(rt_date_get_year(d) == 2025);
    assert(rt_date_get_month(d) == 6);
    assert(rt_date_get_day(d) == 15);

    /* Late in the day (23:59:59.999) should still give same date */
    ms = rt_time_get_millis(midnight) + 23LL * 60LL * 60LL * 1000LL + 59LL * 60LL * 1000LL + 59LL * 1000LL + 999LL;
    t = rt_time_from_millis(arena, ms);
    d = rt_time_get_date(arena, t);
    assert(rt_date_get_year(d) == 2025);
    assert(rt_date_get_month(d) == 6);
    assert(rt_date_get_day(d) == 15);

    /* Just after midnight should give next date */
    long long ms_per_day = 24LL * 60LL * 60LL * 1000LL;
    ms = rt_time_get_millis(midnight) + ms_per_day; /* next day */
    t = rt_time_from_millis(arena, ms);
    d = rt_time_get_date(arena, t);
    assert(rt_date_get_year(d) == 2025);
    assert(rt_date_get_month(d) == 6);
    assert(rt_date_get_day(d) == 16);

    rt_arena_destroy(arena);
}

void test_rt_time_get_date_null_handling()
{
    printf("Testing rt_time_get_date with NULL...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtTime *t = rt_time_from_millis(arena, 0);

    /* NULL arena or time returns NULL */
    assert(rt_time_get_date(NULL, t) == NULL);
    assert(rt_time_get_date(arena, NULL) == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_time_roundtrip()
{
    printf("Testing Date <-> Time round-trip conversions...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Date -> Time -> Date should preserve the date */
    RtDate *original = rt_date_from_ymd(arena, 2025, 6, 15);
    RtTime *time = rt_date_to_time(arena, original);
    RtDate *result = rt_time_get_date(arena, time);
    assert(rt_date_equals(original, result) == 1);

    /* Test with epoch */
    original = rt_date_from_ymd(arena, 1970, 1, 1);
    time = rt_date_to_time(arena, original);
    result = rt_time_get_date(arena, time);
    assert(rt_date_equals(original, result) == 1);

#ifndef _WIN32
    /* Test with date before epoch - Skip on Windows where mktime doesn't support pre-1970 dates */
    original = rt_date_from_ymd(arena, 1969, 12, 31);
    time = rt_date_to_time(arena, original);
    result = rt_time_get_date(arena, time);
    assert(rt_date_equals(original, result) == 1);
#endif

    /* Test with leap year date */
    original = rt_date_from_ymd(arena, 2024, 2, 29);
    time = rt_date_to_time(arena, original);
    result = rt_time_get_date(arena, time);
    assert(rt_date_equals(original, result) == 1);

    /* Test with various dates - all post-1970 for Windows compatibility */
    RtDate *dates[] = {
        rt_date_from_ymd(arena, 2000, 1, 1),
        rt_date_from_ymd(arena, 1999, 12, 31),
        rt_date_from_ymd(arena, 2100, 12, 31),
    };
    for (int i = 0; i < 3; i++) {
        time = rt_date_to_time(arena, dates[i]);
        result = rt_time_get_date(arena, time);
        assert(rt_date_equals(dates[i], result) == 1);
    }

#ifndef _WIN32
    /* Test with pre-1970 date - Unix only */
    original = rt_date_from_ymd(arena, 1900, 1, 1);
    time = rt_date_to_time(arena, original);
    result = rt_time_get_date(arena, time);
    assert(rt_date_equals(original, result) == 1);
#endif

    rt_arena_destroy(arena);
}

void test_rt_time_get_date_negative_times()
{
#ifdef _WIN32
    /* Skip on Windows - negative time_t values not supported by Windows time functions */
    printf("Testing rt_time_get_date with negative times... (skipped on Windows)\n");
#else
    printf("Testing rt_time_get_date with negative times...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtTime *t;

    /* Test that times before a given midnight still return the previous date */
    /* Start from midnight on 1970-01-01 in local timezone */
    RtDate *epoch_date = rt_date_from_ymd(arena, 1970, 1, 1);
    RtTime *midnight = rt_date_to_time(arena, epoch_date);
    long long midnight_ms = rt_time_get_millis(midnight);

    /* One millisecond before midnight should give the previous day */
    t = rt_time_from_millis(arena, midnight_ms - 1);
    d = rt_time_get_date(arena, t);
    assert(rt_date_get_year(d) == 1969);
    assert(rt_date_get_month(d) == 12);
    assert(rt_date_get_day(d) == 31);

    /* Get midnight on 1969-12-31 and test one ms before that gives 1969-12-30 */
    RtDate *dec31 = rt_date_from_ymd(arena, 1969, 12, 31);
    RtTime *dec31_midnight = rt_date_to_time(arena, dec31);
    t = rt_time_from_millis(arena, rt_time_get_millis(dec31_midnight) - 1);
    d = rt_time_get_date(arena, t);
    assert(rt_date_get_year(d) == 1969);
    assert(rt_date_get_month(d) == 12);
    assert(rt_date_get_day(d) == 30);

    rt_arena_destroy(arena);
#endif
}

/* ============================================================================
 * Date Static Constructor Tests
 * ============================================================================ */

void test_rt_date_today()
{
    printf("Testing rt_date_today...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Get today's date */
    RtDate *today = rt_date_today(arena);
    assert(today != NULL);

    /* Verify it returns a reasonable current date (year >= 2025) */
    long year = rt_date_get_year(today);
    assert(year >= 2025);

    /* Month should be 1-12 */
    long month = rt_date_get_month(today);
    assert(month >= 1 && month <= 12);

    /* Day should be 1-31 */
    long day = rt_date_get_day(today);
    assert(day >= 1 && day <= 31);

    /* Calling today twice should give same result (within same second) */
    RtDate *today2 = rt_date_today(arena);
    assert(rt_date_equals(today, today2));

    rt_arena_destroy(arena);
}

void test_rt_date_today_null_arena()
{
    printf("Testing rt_date_today null arena...\n");

    /* Should handle null arena gracefully (returns NULL) */
    RtDate *result = rt_date_today(NULL);
    assert(result == NULL);
}

void test_rt_date_from_ymd_valid()
{
    printf("Testing rt_date_from_ymd with valid dates...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test regular date: 2025-01-15 */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 15);
    assert(d1 != NULL);
    assert(rt_date_get_year(d1) == 2025);
    assert(rt_date_get_month(d1) == 1);
    assert(rt_date_get_day(d1) == 15);

    /* Test leap year date: 2024-02-29 */
    RtDate *d2 = rt_date_from_ymd(arena, 2024, 2, 29);
    assert(d2 != NULL);
    assert(rt_date_get_year(d2) == 2024);
    assert(rt_date_get_month(d2) == 2);
    assert(rt_date_get_day(d2) == 29);

    /* Test end of year: 2025-12-31 */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 12, 31);
    assert(d3 != NULL);
    assert(rt_date_get_year(d3) == 2025);
    assert(rt_date_get_month(d3) == 12);
    assert(rt_date_get_day(d3) == 31);

    /* Test first day of year: 2025-01-01 */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 1, 1);
    assert(d4 != NULL);
    assert(rt_date_get_year(d4) == 2025);
    assert(rt_date_get_month(d4) == 1);
    assert(rt_date_get_day(d4) == 1);

    /* Test Unix epoch: 1970-01-01 */
    RtDate *d5 = rt_date_from_ymd(arena, 1970, 1, 1);
    assert(d5 != NULL);
    assert(rt_date_get_year(d5) == 1970);
    assert(rt_date_get_month(d5) == 1);
    assert(rt_date_get_day(d5) == 1);
    assert(rt_date_get_epoch_days(d5) == 0);

    /* Test date before epoch: 1969-12-31 */
    RtDate *d6 = rt_date_from_ymd(arena, 1969, 12, 31);
    assert(d6 != NULL);
    assert(rt_date_get_year(d6) == 1969);
    assert(rt_date_get_month(d6) == 12);
    assert(rt_date_get_day(d6) == 31);
    assert(rt_date_get_epoch_days(d6) == -1);

    rt_arena_destroy(arena);
}

/* Note: rt_date_from_ymd with invalid dates calls exit(1), so we can't test that in unit tests.
 * The runtime design is to fail fast on invalid dates, which is validated at runtime. */

void test_rt_date_from_ymd_null_arena()
{
    printf("Testing rt_date_from_ymd null arena...\n");

    /* Should handle null arena gracefully */
    RtDate *result = rt_date_from_ymd(NULL, 2025, 1, 15);
    assert(result == NULL);
}

void test_rt_date_from_string_valid()
{
    printf("Testing rt_date_from_string with valid ISO format...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Standard ISO format: 2025-01-15 */
    RtDate *d1 = rt_date_from_string(arena, "2025-01-15");
    assert(d1 != NULL);
    assert(rt_date_get_year(d1) == 2025);
    assert(rt_date_get_month(d1) == 1);
    assert(rt_date_get_day(d1) == 15);

    /* Leap year date: 2024-02-29 */
    RtDate *d2 = rt_date_from_string(arena, "2024-02-29");
    assert(d2 != NULL);
    assert(rt_date_get_year(d2) == 2024);
    assert(rt_date_get_month(d2) == 2);
    assert(rt_date_get_day(d2) == 29);

    /* End of year: 2025-12-31 */
    RtDate *d3 = rt_date_from_string(arena, "2025-12-31");
    assert(d3 != NULL);
    assert(rt_date_get_year(d3) == 2025);
    assert(rt_date_get_month(d3) == 12);
    assert(rt_date_get_day(d3) == 31);

    /* Unix epoch: 1970-01-01 */
    RtDate *d4 = rt_date_from_string(arena, "1970-01-01");
    assert(d4 != NULL);
    assert(rt_date_get_epoch_days(d4) == 0);

    /* Date before epoch: 1969-12-31 */
    RtDate *d5 = rt_date_from_string(arena, "1969-12-31");
    assert(d5 != NULL);
    assert(rt_date_get_epoch_days(d5) == -1);

    /* Far future date: 2099-06-15 */
    RtDate *d6 = rt_date_from_string(arena, "2099-06-15");
    assert(d6 != NULL);
    assert(rt_date_get_year(d6) == 2099);

    /* Historical date: 1900-01-01 */
    RtDate *d7 = rt_date_from_string(arena, "1900-01-01");
    assert(d7 != NULL);
    assert(rt_date_get_year(d7) == 1900);

    rt_arena_destroy(arena);
}

/* Note: rt_date_from_string with invalid formats calls exit(1), so we can't test that in unit tests.
 * The runtime design is to fail fast on invalid dates/formats, which is validated at runtime. */

void test_rt_date_from_string_null_arena()
{
    printf("Testing rt_date_from_string null arena...\n");

    /* Should handle null arena gracefully */
    RtDate *result = rt_date_from_string(NULL, "2025-01-15");
    assert(result == NULL);
}

void test_rt_date_from_epoch_days_valid()
{
    printf("Testing rt_date_from_epoch_days with various values...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Epoch day 0 = 1970-01-01 */
    RtDate *d1 = rt_date_from_epoch_days(arena, 0);
    assert(d1 != NULL);
    assert(rt_date_get_year(d1) == 1970);
    assert(rt_date_get_month(d1) == 1);
    assert(rt_date_get_day(d1) == 1);

    /* Positive days: 1 = 1970-01-02 */
    RtDate *d2 = rt_date_from_epoch_days(arena, 1);
    assert(d2 != NULL);
    assert(rt_date_get_year(d2) == 1970);
    assert(rt_date_get_month(d2) == 1);
    assert(rt_date_get_day(d2) == 2);

    /* Negative days: -1 = 1969-12-31 */
    RtDate *d3 = rt_date_from_epoch_days(arena, -1);
    assert(d3 != NULL);
    assert(rt_date_get_year(d3) == 1969);
    assert(rt_date_get_month(d3) == 12);
    assert(rt_date_get_day(d3) == 31);

    /* Year 2000: day 10957 = 2000-01-01 (30 years * 365 + 7 leap days) */
    RtDate *d4 = rt_date_from_epoch_days(arena, 10957);
    assert(d4 != NULL);
    assert(rt_date_get_year(d4) == 2000);
    assert(rt_date_get_month(d4) == 1);
    assert(rt_date_get_day(d4) == 1);

    /* Full year from epoch: 365 = 1971-01-01 */
    RtDate *d5 = rt_date_from_epoch_days(arena, 365);
    assert(d5 != NULL);
    assert(rt_date_get_year(d5) == 1971);
    assert(rt_date_get_month(d5) == 1);
    assert(rt_date_get_day(d5) == 1);

    /* Large positive value: 20000 days from epoch */
    RtDate *d6 = rt_date_from_epoch_days(arena, 20000);
    assert(d6 != NULL);
    assert(rt_date_get_epoch_days(d6) == 20000);

    /* Large negative value: -20000 days from epoch */
    RtDate *d7 = rt_date_from_epoch_days(arena, -20000);
    assert(d7 != NULL);
    assert(rt_date_get_epoch_days(d7) == -20000);

    rt_arena_destroy(arena);
}

void test_rt_date_from_epoch_days_null_arena()
{
    printf("Testing rt_date_from_epoch_days null arena...\n");

    /* Should handle null arena gracefully */
    RtDate *result = rt_date_from_epoch_days(NULL, 0);
    assert(result == NULL);
}

void test_rt_date_from_epoch_days_roundtrip()
{
    printf("Testing rt_date_from_epoch_days roundtrip...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Create from YMD, get epoch days, recreate from epoch days */
    RtDate *original = rt_date_from_ymd(arena, 2025, 6, 15);
    int32_t days = rt_date_get_epoch_days(original);
    RtDate *recreated = rt_date_from_epoch_days(arena, days);

    assert(rt_date_equals(original, recreated));
    assert(rt_date_get_year(recreated) == 2025);
    assert(rt_date_get_month(recreated) == 6);
    assert(rt_date_get_day(recreated) == 15);

    /* Test with negative epoch days */
    original = rt_date_from_ymd(arena, 1960, 3, 20);
    days = rt_date_get_epoch_days(original);
    assert(days < 0); /* Before epoch */
    recreated = rt_date_from_epoch_days(arena, days);

    assert(rt_date_equals(original, recreated));
    assert(rt_date_get_year(recreated) == 1960);
    assert(rt_date_get_month(recreated) == 3);
    assert(rt_date_get_day(recreated) == 20);

    rt_arena_destroy(arena);
}

void test_rt_date_is_leap_year_static()
{
    printf("Testing rt_date_is_leap_year static function...\n");

    /* Standard leap years (divisible by 4) */
    assert(rt_date_is_leap_year(2024) == 1);
    assert(rt_date_is_leap_year(2020) == 1);
    assert(rt_date_is_leap_year(2016) == 1);
    assert(rt_date_is_leap_year(2004) == 1);  /* Required by verification criteria */

    /* Non-leap years */
    assert(rt_date_is_leap_year(2023) == 0);
    assert(rt_date_is_leap_year(2025) == 0);
    assert(rt_date_is_leap_year(2019) == 0);

    /* Century years not divisible by 400 are NOT leap years */
    assert(rt_date_is_leap_year(1900) == 0);
    assert(rt_date_is_leap_year(2100) == 0);
    assert(rt_date_is_leap_year(2200) == 0);
    assert(rt_date_is_leap_year(2300) == 0);

    /* Century years divisible by 400 ARE leap years */
    assert(rt_date_is_leap_year(2000) == 1);
    assert(rt_date_is_leap_year(1600) == 1);
    assert(rt_date_is_leap_year(2400) == 1);
}

void test_rt_date_days_in_month_static()
{
    printf("Testing rt_date_days_in_month static function...\n");

    /* 31-day months */
    assert(rt_date_days_in_month(2025, 1) == 31);  /* January */
    assert(rt_date_days_in_month(2025, 3) == 31);  /* March */
    assert(rt_date_days_in_month(2025, 5) == 31);  /* May */
    assert(rt_date_days_in_month(2025, 7) == 31);  /* July */
    assert(rt_date_days_in_month(2025, 8) == 31);  /* August */
    assert(rt_date_days_in_month(2025, 10) == 31); /* October */
    assert(rt_date_days_in_month(2025, 12) == 31); /* December */

    /* 30-day months */
    assert(rt_date_days_in_month(2025, 4) == 30);  /* April */
    assert(rt_date_days_in_month(2025, 6) == 30);  /* June */
    assert(rt_date_days_in_month(2025, 9) == 30);  /* September */
    assert(rt_date_days_in_month(2025, 11) == 30); /* November */

    /* February in leap year */
    assert(rt_date_days_in_month(2024, 2) == 29);
    assert(rt_date_days_in_month(2000, 2) == 29);

    /* February in non-leap year */
    assert(rt_date_days_in_month(2025, 2) == 28);
    assert(rt_date_days_in_month(1900, 2) == 28);
    assert(rt_date_days_in_month(2100, 2) == 28);

    /* Invalid month values return 0 */
    assert(rt_date_days_in_month(2025, 0) == 0);   /* Month 0 is invalid */
    assert(rt_date_days_in_month(2025, 13) == 0);  /* Month 13 is invalid */
    assert(rt_date_days_in_month(2025, -1) == 0);  /* Negative month is invalid */
    assert(rt_date_days_in_month(2025, 100) == 0); /* Large month is invalid */
}

void test_rt_date_is_valid_ymd()
{
    printf("Testing rt_date_is_valid_ymd...\n");

    /* Valid dates */
    assert(rt_date_is_valid_ymd(2025, 1, 1) == 1);
    assert(rt_date_is_valid_ymd(2025, 12, 31) == 1);
    assert(rt_date_is_valid_ymd(2024, 2, 29) == 1); /* Leap year */
    assert(rt_date_is_valid_ymd(2000, 2, 29) == 1); /* Century leap year */

    /* Invalid months */
    assert(rt_date_is_valid_ymd(2025, 0, 1) == 0);
    assert(rt_date_is_valid_ymd(2025, 13, 1) == 0);
    assert(rt_date_is_valid_ymd(2025, -1, 1) == 0);

    /* Invalid days */
    assert(rt_date_is_valid_ymd(2025, 1, 0) == 0);
    assert(rt_date_is_valid_ymd(2025, 1, 32) == 0);
    assert(rt_date_is_valid_ymd(2025, 4, 31) == 0); /* April has 30 days */
    assert(rt_date_is_valid_ymd(2025, 2, 29) == 0); /* Non-leap year */
    assert(rt_date_is_valid_ymd(1900, 2, 29) == 0); /* Century non-leap year */
}

/* ============================================================================
 * Date Getter Tests
 * ============================================================================ */

void test_rt_date_get_year_month_day()
{
    printf("Testing rt_date_get_year/month/day...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test date: 2025-06-15 */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    assert(rt_date_get_year(d1) == 2025);
    assert(rt_date_get_month(d1) == 6);
    assert(rt_date_get_day(d1) == 15);

    /* Test date: 2024-02-29 (leap year) */
    RtDate *d2 = rt_date_from_ymd(arena, 2024, 2, 29);
    assert(rt_date_get_year(d2) == 2024);
    assert(rt_date_get_month(d2) == 2);
    assert(rt_date_get_day(d2) == 29);

    /* Test date: 1970-01-01 (Unix epoch) */
    RtDate *d3 = rt_date_from_ymd(arena, 1970, 1, 1);
    assert(rt_date_get_year(d3) == 1970);
    assert(rt_date_get_month(d3) == 1);
    assert(rt_date_get_day(d3) == 1);

    /* Test date: 1969-12-31 (before epoch) */
    RtDate *d4 = rt_date_from_ymd(arena, 1969, 12, 31);
    assert(rt_date_get_year(d4) == 1969);
    assert(rt_date_get_month(d4) == 12);
    assert(rt_date_get_day(d4) == 31);

    /* Test date: 2000-01-01 (Y2K) */
    RtDate *d5 = rt_date_from_ymd(arena, 2000, 1, 1);
    assert(rt_date_get_year(d5) == 2000);
    assert(rt_date_get_month(d5) == 1);
    assert(rt_date_get_day(d5) == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_get_weekday()
{
    printf("Testing rt_date_get_weekday...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* 1970-01-01 was a Thursday (weekday 4) */
    RtDate *d1 = rt_date_from_ymd(arena, 1970, 1, 1);
    assert(rt_date_get_weekday(d1) == 4);

    /* 2025-01-05 is a Sunday (weekday 0) */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 1, 5);
    assert(rt_date_get_weekday(d2) == 0);

    /* 2025-01-06 is a Monday (weekday 1) */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 1, 6);
    assert(rt_date_get_weekday(d3) == 1);

    /* 2025-01-07 is a Tuesday (weekday 2) */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 1, 7);
    assert(rt_date_get_weekday(d4) == 2);

    /* 2025-01-08 is a Wednesday (weekday 3) */
    RtDate *d5 = rt_date_from_ymd(arena, 2025, 1, 8);
    assert(rt_date_get_weekday(d5) == 3);

    /* 2025-01-09 is a Thursday (weekday 4) */
    RtDate *d6 = rt_date_from_ymd(arena, 2025, 1, 9);
    assert(rt_date_get_weekday(d6) == 4);

    /* 2025-01-10 is a Friday (weekday 5) */
    RtDate *d7 = rt_date_from_ymd(arena, 2025, 1, 10);
    assert(rt_date_get_weekday(d7) == 5);

    /* 2025-01-11 is a Saturday (weekday 6) */
    RtDate *d8 = rt_date_from_ymd(arena, 2025, 1, 11);
    assert(rt_date_get_weekday(d8) == 6);

    rt_arena_destroy(arena);
}

void test_rt_date_get_day_of_year()
{
    printf("Testing rt_date_get_day_of_year...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Jan 1 is day 1 */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 1);
    assert(rt_date_get_day_of_year(d1) == 1);

    /* Jan 31 is day 31 */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 1, 31);
    assert(rt_date_get_day_of_year(d2) == 31);

    /* Feb 1 is day 32 */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 2, 1);
    assert(rt_date_get_day_of_year(d3) == 32);

    /* Dec 31 in non-leap year is day 365 */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 12, 31);
    assert(rt_date_get_day_of_year(d4) == 365);

    /* Dec 31 in leap year is day 366 */
    RtDate *d5 = rt_date_from_ymd(arena, 2024, 12, 31);
    assert(rt_date_get_day_of_year(d5) == 366);

    /* March 1 in non-leap year (Jan=31 + Feb=28 + 1 = 60) */
    RtDate *d6 = rt_date_from_ymd(arena, 2025, 3, 1);
    assert(rt_date_get_day_of_year(d6) == 60);

    /* March 1 in leap year (Jan=31 + Feb=29 + 1 = 61) */
    RtDate *d7 = rt_date_from_ymd(arena, 2024, 3, 1);
    assert(rt_date_get_day_of_year(d7) == 61);

    rt_arena_destroy(arena);
}

void test_rt_date_get_epoch_days()
{
    printf("Testing rt_date_get_epoch_days...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* 1970-01-01 is day 0 */
    RtDate *d1 = rt_date_from_ymd(arena, 1970, 1, 1);
    assert(rt_date_get_epoch_days(d1) == 0);

    /* 1970-01-02 is day 1 */
    RtDate *d2 = rt_date_from_ymd(arena, 1970, 1, 2);
    assert(rt_date_get_epoch_days(d2) == 1);

    /* 1969-12-31 is day -1 */
    RtDate *d3 = rt_date_from_ymd(arena, 1969, 12, 31);
    assert(rt_date_get_epoch_days(d3) == -1);

    /* Test roundtrip: epoch_days matches construction value */
    RtDate *d4 = rt_date_from_epoch_days(arena, 10000);
    assert(rt_date_get_epoch_days(d4) == 10000);

    RtDate *d5 = rt_date_from_epoch_days(arena, -5000);
    assert(rt_date_get_epoch_days(d5) == -5000);

    /* 2000-01-01 is day 10957 */
    RtDate *d6 = rt_date_from_ymd(arena, 2000, 1, 1);
    assert(rt_date_get_epoch_days(d6) == 10957);

    rt_arena_destroy(arena);
}

void test_rt_date_is_weekend()
{
    printf("Testing rt_date_is_weekend...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Sunday (2025-01-05) - is weekend */
    RtDate *sunday = rt_date_from_ymd(arena, 2025, 1, 5);
    assert(rt_date_is_weekend(sunday) == 1);

    /* Monday (2025-01-06) - not weekend */
    RtDate *monday = rt_date_from_ymd(arena, 2025, 1, 6);
    assert(rt_date_is_weekend(monday) == 0);

    /* Tuesday (2025-01-07) - not weekend */
    RtDate *tuesday = rt_date_from_ymd(arena, 2025, 1, 7);
    assert(rt_date_is_weekend(tuesday) == 0);

    /* Wednesday (2025-01-08) - not weekend */
    RtDate *wednesday = rt_date_from_ymd(arena, 2025, 1, 8);
    assert(rt_date_is_weekend(wednesday) == 0);

    /* Thursday (2025-01-09) - not weekend */
    RtDate *thursday = rt_date_from_ymd(arena, 2025, 1, 9);
    assert(rt_date_is_weekend(thursday) == 0);

    /* Friday (2025-01-10) - not weekend */
    RtDate *friday = rt_date_from_ymd(arena, 2025, 1, 10);
    assert(rt_date_is_weekend(friday) == 0);

    /* Saturday (2025-01-11) - is weekend */
    RtDate *saturday = rt_date_from_ymd(arena, 2025, 1, 11);
    assert(rt_date_is_weekend(saturday) == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_is_weekday()
{
    printf("Testing rt_date_is_weekday...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Sunday (2025-01-05) - not weekday */
    RtDate *sunday = rt_date_from_ymd(arena, 2025, 1, 5);
    assert(rt_date_is_weekday(sunday) == 0);

    /* Monday (2025-01-06) - is weekday */
    RtDate *monday = rt_date_from_ymd(arena, 2025, 1, 6);
    assert(rt_date_is_weekday(monday) == 1);

    /* Tuesday (2025-01-07) - is weekday */
    RtDate *tuesday = rt_date_from_ymd(arena, 2025, 1, 7);
    assert(rt_date_is_weekday(tuesday) == 1);

    /* Wednesday (2025-01-08) - is weekday */
    RtDate *wednesday = rt_date_from_ymd(arena, 2025, 1, 8);
    assert(rt_date_is_weekday(wednesday) == 1);

    /* Thursday (2025-01-09) - is weekday */
    RtDate *thursday = rt_date_from_ymd(arena, 2025, 1, 9);
    assert(rt_date_is_weekday(thursday) == 1);

    /* Friday (2025-01-10) - is weekday */
    RtDate *friday = rt_date_from_ymd(arena, 2025, 1, 10);
    assert(rt_date_is_weekday(friday) == 1);

    /* Saturday (2025-01-11) - not weekday */
    RtDate *saturday = rt_date_from_ymd(arena, 2025, 1, 11);
    assert(rt_date_is_weekday(saturday) == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_instance_days_in_month()
{
    printf("Testing rt_date_get_days_in_month (instance)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test that instance method matches static method for various dates */

    /* January 2025 - 31 days */
    RtDate *jan = rt_date_from_ymd(arena, 2025, 1, 15);
    assert(rt_date_get_days_in_month(jan) == 31);
    assert(rt_date_get_days_in_month(jan) == rt_date_days_in_month(2025, 1));

    /* February 2025 (non-leap) - 28 days */
    RtDate *feb_nonleap = rt_date_from_ymd(arena, 2025, 2, 15);
    assert(rt_date_get_days_in_month(feb_nonleap) == 28);
    assert(rt_date_get_days_in_month(feb_nonleap) == rt_date_days_in_month(2025, 2));

    /* February 2024 (leap) - 29 days */
    RtDate *feb_leap = rt_date_from_ymd(arena, 2024, 2, 15);
    assert(rt_date_get_days_in_month(feb_leap) == 29);
    assert(rt_date_get_days_in_month(feb_leap) == rt_date_days_in_month(2024, 2));

    /* April 2025 - 30 days */
    RtDate *apr = rt_date_from_ymd(arena, 2025, 4, 15);
    assert(rt_date_get_days_in_month(apr) == 30);
    assert(rt_date_get_days_in_month(apr) == rt_date_days_in_month(2025, 4));

    /* December 2025 - 31 days */
    RtDate *dec = rt_date_from_ymd(arena, 2025, 12, 25);
    assert(rt_date_get_days_in_month(dec) == 31);
    assert(rt_date_get_days_in_month(dec) == rt_date_days_in_month(2025, 12));

    rt_arena_destroy(arena);
}

void test_rt_date_instance_is_leap_year()
{
    printf("Testing rt_date_is_leap (instance)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test that instance method matches static method */

    /* 2024 is a leap year */
    RtDate *d2024 = rt_date_from_ymd(arena, 2024, 6, 15);
    assert(rt_date_is_leap(d2024) == 1);
    assert(rt_date_is_leap(d2024) == rt_date_is_leap_year(2024));

    /* 2025 is not a leap year */
    RtDate *d2025 = rt_date_from_ymd(arena, 2025, 6, 15);
    assert(rt_date_is_leap(d2025) == 0);
    assert(rt_date_is_leap(d2025) == rt_date_is_leap_year(2025));

    /* 2000 is a leap year (century divisible by 400) */
    RtDate *d2000 = rt_date_from_ymd(arena, 2000, 6, 15);
    assert(rt_date_is_leap(d2000) == 1);
    assert(rt_date_is_leap(d2000) == rt_date_is_leap_year(2000));

    /* 1900 is not a leap year (century not divisible by 400) */
    RtDate *d1900 = rt_date_from_ymd(arena, 1900, 6, 15);
    assert(rt_date_is_leap(d1900) == 0);
    assert(rt_date_is_leap(d1900) == rt_date_is_leap_year(1900));

    /* 2100 is not a leap year (century not divisible by 400) */
    RtDate *d2100 = rt_date_from_ymd(arena, 2100, 6, 15);
    assert(rt_date_is_leap(d2100) == 0);
    assert(rt_date_is_leap(d2100) == rt_date_is_leap_year(2100));

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Date toIso and toString Tests
 * ============================================================================ */

void test_rt_date_to_iso_format()
{
    printf("Testing rt_date_to_iso format...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test standard date */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_to_iso(arena, d1);
    assert(result != NULL);
    assert(strcmp(result, "2025-06-15") == 0);
    assert(strlen(result) == 10);
    assert(result[10] == '\0');  /* Verify null-terminated */

    /* Test date requiring zero-padding for month and day */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 1, 5);
    result = rt_date_to_iso(arena, d2);
    assert(result != NULL);
    assert(strcmp(result, "2025-01-05") == 0);

    /* Test first day of year */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 1, 1);
    result = rt_date_to_iso(arena, d3);
    assert(result != NULL);
    assert(strcmp(result, "2025-01-01") == 0);

    /* Test last day of year */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_to_iso(arena, d4);
    assert(result != NULL);
    assert(strcmp(result, "2025-12-31") == 0);

    /* Test leap year date */
    RtDate *d5 = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_to_iso(arena, d5);
    assert(result != NULL);
    assert(strcmp(result, "2024-02-29") == 0);

    /* Test Unix epoch */
    RtDate *d6 = rt_date_from_ymd(arena, 1970, 1, 1);
    result = rt_date_to_iso(arena, d6);
    assert(result != NULL);
    assert(strcmp(result, "1970-01-01") == 0);

    /* Test Y2K date */
    RtDate *d7 = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_to_iso(arena, d7);
    assert(result != NULL);
    assert(strcmp(result, "2000-01-01") == 0);

    /* Test date with double-digit month and single-digit day */
    RtDate *d8 = rt_date_from_ymd(arena, 2025, 10, 9);
    result = rt_date_to_iso(arena, d8);
    assert(result != NULL);
    assert(strcmp(result, "2025-10-09") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_to_string_format()
{
    printf("Testing rt_date_to_string format...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test January */
    RtDate *d_jan = rt_date_from_ymd(arena, 2025, 1, 15);
    char *result = rt_date_to_string(arena, d_jan);
    assert(result != NULL);
    assert(strcmp(result, "January 15, 2025") == 0);
    assert(result[strlen(result)] == '\0');  /* Verify null-terminated */

    /* Test February */
    RtDate *d_feb = rt_date_from_ymd(arena, 2025, 2, 28);
    result = rt_date_to_string(arena, d_feb);
    assert(result != NULL);
    assert(strcmp(result, "February 28, 2025") == 0);

    /* Test March */
    RtDate *d_mar = rt_date_from_ymd(arena, 2025, 3, 1);
    result = rt_date_to_string(arena, d_mar);
    assert(result != NULL);
    assert(strcmp(result, "March 1, 2025") == 0);

    /* Test April */
    RtDate *d_apr = rt_date_from_ymd(arena, 2025, 4, 30);
    result = rt_date_to_string(arena, d_apr);
    assert(result != NULL);
    assert(strcmp(result, "April 30, 2025") == 0);

    /* Test May */
    RtDate *d_may = rt_date_from_ymd(arena, 2025, 5, 5);
    result = rt_date_to_string(arena, d_may);
    assert(result != NULL);
    assert(strcmp(result, "May 5, 2025") == 0);

    /* Test June */
    RtDate *d_jun = rt_date_from_ymd(arena, 2025, 6, 21);
    result = rt_date_to_string(arena, d_jun);
    assert(result != NULL);
    assert(strcmp(result, "June 21, 2025") == 0);

    /* Test July */
    RtDate *d_jul = rt_date_from_ymd(arena, 2025, 7, 4);
    result = rt_date_to_string(arena, d_jul);
    assert(result != NULL);
    assert(strcmp(result, "July 4, 2025") == 0);

    /* Test August */
    RtDate *d_aug = rt_date_from_ymd(arena, 2025, 8, 15);
    result = rt_date_to_string(arena, d_aug);
    assert(result != NULL);
    assert(strcmp(result, "August 15, 2025") == 0);

    /* Test September (longest month name) */
    RtDate *d_sep = rt_date_from_ymd(arena, 2025, 9, 10);
    result = rt_date_to_string(arena, d_sep);
    assert(result != NULL);
    assert(strcmp(result, "September 10, 2025") == 0);

    /* Test October */
    RtDate *d_oct = rt_date_from_ymd(arena, 2025, 10, 31);
    result = rt_date_to_string(arena, d_oct);
    assert(result != NULL);
    assert(strcmp(result, "October 31, 2025") == 0);

    /* Test November */
    RtDate *d_nov = rt_date_from_ymd(arena, 2025, 11, 11);
    result = rt_date_to_string(arena, d_nov);
    assert(result != NULL);
    assert(strcmp(result, "November 11, 2025") == 0);

    /* Test December */
    RtDate *d_dec = rt_date_from_ymd(arena, 2025, 12, 25);
    result = rt_date_to_string(arena, d_dec);
    assert(result != NULL);
    assert(strcmp(result, "December 25, 2025") == 0);

    /* Test single-digit day */
    RtDate *d_single = rt_date_from_ymd(arena, 2025, 3, 9);
    result = rt_date_to_string(arena, d_single);
    assert(result != NULL);
    assert(strcmp(result, "March 9, 2025") == 0);

    /* Test double-digit day */
    RtDate *d_double = rt_date_from_ymd(arena, 2025, 3, 21);
    result = rt_date_to_string(arena, d_double);
    assert(result != NULL);
    assert(strcmp(result, "March 21, 2025") == 0);

    /* Test different years */
    RtDate *d_old = rt_date_from_ymd(arena, 1999, 12, 31);
    result = rt_date_to_string(arena, d_old);
    assert(result != NULL);
    assert(strcmp(result, "December 31, 1999") == 0);

    RtDate *d_y2k = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_to_string(arena, d_y2k);
    assert(result != NULL);
    assert(strcmp(result, "January 1, 2000") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Date format() Numeric Token Tests
 * ============================================================================ */

void test_rt_date_format_yyyy_token()
{
    printf("Testing rt_date_format YYYY token...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test YYYY returns 4-digit year */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d1, "YYYY");
    assert(result != NULL);
    assert(strcmp(result, "2025") == 0);
    assert(strlen(result) == 4);

    /* Test with year 2000 */
    RtDate *d2 = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_format(arena, d2, "YYYY");
    assert(strcmp(result, "2000") == 0);

    /* Test with year 1970 (Unix epoch) */
    RtDate *d3 = rt_date_from_ymd(arena, 1970, 1, 1);
    result = rt_date_format(arena, d3, "YYYY");
    assert(strcmp(result, "1970") == 0);

    /* Test with year 1999 */
    RtDate *d4 = rt_date_from_ymd(arena, 1999, 12, 31);
    result = rt_date_format(arena, d4, "YYYY");
    assert(strcmp(result, "1999") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_yy_token()
{
    printf("Testing rt_date_format YY token...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test YY returns 2-digit year */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d1, "YY");
    assert(result != NULL);
    assert(strcmp(result, "25") == 0);
    assert(strlen(result) == 2);

    /* Test with year 2000 (YY = 00) */
    RtDate *d2 = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_format(arena, d2, "YY");
    assert(strcmp(result, "00") == 0);

    /* Test with year 1999 (YY = 99) */
    RtDate *d3 = rt_date_from_ymd(arena, 1999, 12, 31);
    result = rt_date_format(arena, d3, "YY");
    assert(strcmp(result, "99") == 0);

    /* Test with year 2005 (YY = 05, with leading zero) */
    RtDate *d4 = rt_date_from_ymd(arena, 2005, 6, 15);
    result = rt_date_format(arena, d4, "YY");
    assert(strcmp(result, "05") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_mm_token()
{
    printf("Testing rt_date_format MM token...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test MM returns zero-padded month (01-12) */

    /* Single digit month with padding */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 15);
    char *result = rt_date_format(arena, d1, "MM");
    assert(result != NULL);
    assert(strcmp(result, "01") == 0);
    assert(strlen(result) == 2);

    /* Another single digit month */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 5, 15);
    result = rt_date_format(arena, d2, "MM");
    assert(strcmp(result, "05") == 0);

    /* Month 9 (boundary case) */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 9, 15);
    result = rt_date_format(arena, d3, "MM");
    assert(strcmp(result, "09") == 0);

    /* Double digit month - no padding needed */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 10, 15);
    result = rt_date_format(arena, d4, "MM");
    assert(strcmp(result, "10") == 0);

    /* Month 12 */
    RtDate *d5 = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_format(arena, d5, "MM");
    assert(strcmp(result, "12") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_m_token()
{
    printf("Testing rt_date_format M token...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test M returns month without padding (1-12) */

    /* Single digit month - no padding */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 15);
    char *result = rt_date_format(arena, d1, "M");
    assert(result != NULL);
    assert(strcmp(result, "1") == 0);
    assert(strlen(result) == 1);

    /* Another single digit month */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 5, 15);
    result = rt_date_format(arena, d2, "M");
    assert(strcmp(result, "5") == 0);

    /* Month 9 (boundary case) */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 9, 15);
    result = rt_date_format(arena, d3, "M");
    assert(strcmp(result, "9") == 0);

    /* Double digit month */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 10, 15);
    result = rt_date_format(arena, d4, "M");
    assert(strcmp(result, "10") == 0);
    assert(strlen(result) == 2);

    /* Month 12 */
    RtDate *d5 = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_format(arena, d5, "M");
    assert(strcmp(result, "12") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_dd_token()
{
    printf("Testing rt_date_format DD token...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test DD returns zero-padded day (01-31) */

    /* Single digit day with padding */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 1);
    char *result = rt_date_format(arena, d1, "DD");
    assert(result != NULL);
    assert(strcmp(result, "01") == 0);
    assert(strlen(result) == 2);

    /* Another single digit day */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 6, 5);
    result = rt_date_format(arena, d2, "DD");
    assert(strcmp(result, "05") == 0);

    /* Day 9 (boundary case) */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 6, 9);
    result = rt_date_format(arena, d3, "DD");
    assert(strcmp(result, "09") == 0);

    /* Double digit day - no padding needed */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 6, 10);
    result = rt_date_format(arena, d4, "DD");
    assert(strcmp(result, "10") == 0);

    /* Day 31 */
    RtDate *d5 = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_format(arena, d5, "DD");
    assert(strcmp(result, "31") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_d_token()
{
    printf("Testing rt_date_format D token...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test D returns day without padding (1-31) */

    /* Single digit day - no padding */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 1);
    char *result = rt_date_format(arena, d1, "D");
    assert(result != NULL);
    assert(strcmp(result, "1") == 0);
    assert(strlen(result) == 1);

    /* Another single digit day */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 6, 5);
    result = rt_date_format(arena, d2, "D");
    assert(strcmp(result, "5") == 0);

    /* Day 9 (boundary case) */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 6, 9);
    result = rt_date_format(arena, d3, "D");
    assert(strcmp(result, "9") == 0);

    /* Double digit day */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 6, 10);
    result = rt_date_format(arena, d4, "D");
    assert(strcmp(result, "10") == 0);
    assert(strlen(result) == 2);

    /* Day 31 */
    RtDate *d5 = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_format(arena, d5, "D");
    assert(strcmp(result, "31") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_yyyy_mm_dd_combined()
{
    printf("Testing rt_date_format YYYY-MM-DD combined...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test YYYY-MM-DD combines tokens correctly (ISO format) */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d1, "YYYY-MM-DD");
    assert(result != NULL);
    assert(strcmp(result, "2025-06-15") == 0);

    /* With single digit month and day (needs padding) */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 1, 5);
    result = rt_date_format(arena, d2, "YYYY-MM-DD");
    assert(strcmp(result, "2025-01-05") == 0);

    /* Year 2000 edge case */
    RtDate *d3 = rt_date_from_ymd(arena, 2000, 1, 1);
    result = rt_date_format(arena, d3, "YYYY-MM-DD");
    assert(strcmp(result, "2000-01-01") == 0);

    /* Last day of year */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_format(arena, d4, "YYYY-MM-DD");
    assert(strcmp(result, "2025-12-31") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_m_d_yyyy_combined()
{
    printf("Testing rt_date_format M/D/YYYY combined...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test M/D/YYYY produces US-style date */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    char *result = rt_date_format(arena, d1, "M/D/YYYY");
    assert(result != NULL);
    assert(strcmp(result, "6/15/2025") == 0);

    /* Single digit month and day - no padding */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 1, 5);
    result = rt_date_format(arena, d2, "M/D/YYYY");
    assert(strcmp(result, "1/5/2025") == 0);

    /* Double digit month and day */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_format(arena, d3, "M/D/YYYY");
    assert(strcmp(result, "12/31/2025") == 0);

    /* Mixed single/double digits */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 10, 5);
    result = rt_date_format(arena, d4, "M/D/YYYY");
    assert(strcmp(result, "10/5/2025") == 0);

    RtDate *d5 = rt_date_from_ymd(arena, 2025, 5, 25);
    result = rt_date_format(arena, d5, "M/D/YYYY");
    assert(strcmp(result, "5/25/2025") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Date format() Name Token Tests
 * ============================================================================ */

void test_rt_date_format_mmm_token()
{
    printf("Testing rt_date_format MMM token (short month names)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test all 12 months with short names */
    RtDate *d;
    char *result;

    /* January */
    d = rt_date_from_ymd(arena, 2025, 1, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(result != NULL);
    assert(strcmp(result, "Jan") == 0);

    /* February */
    d = rt_date_from_ymd(arena, 2025, 2, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Feb") == 0);

    /* March */
    d = rt_date_from_ymd(arena, 2025, 3, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Mar") == 0);

    /* April */
    d = rt_date_from_ymd(arena, 2025, 4, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Apr") == 0);

    /* May */
    d = rt_date_from_ymd(arena, 2025, 5, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "May") == 0);

    /* June */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Jun") == 0);

    /* July */
    d = rt_date_from_ymd(arena, 2025, 7, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Jul") == 0);

    /* August */
    d = rt_date_from_ymd(arena, 2025, 8, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Aug") == 0);

    /* September */
    d = rt_date_from_ymd(arena, 2025, 9, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Sep") == 0);

    /* October */
    d = rt_date_from_ymd(arena, 2025, 10, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Oct") == 0);

    /* November */
    d = rt_date_from_ymd(arena, 2025, 11, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Nov") == 0);

    /* December */
    d = rt_date_from_ymd(arena, 2025, 12, 15);
    result = rt_date_format(arena, d, "MMM");
    assert(strcmp(result, "Dec") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_mmmm_token()
{
    printf("Testing rt_date_format MMMM token (full month names)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test all 12 months with full names */
    RtDate *d;
    char *result;

    /* January */
    d = rt_date_from_ymd(arena, 2025, 1, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(result != NULL);
    assert(strcmp(result, "January") == 0);

    /* February */
    d = rt_date_from_ymd(arena, 2025, 2, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "February") == 0);

    /* March */
    d = rt_date_from_ymd(arena, 2025, 3, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "March") == 0);

    /* April */
    d = rt_date_from_ymd(arena, 2025, 4, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "April") == 0);

    /* May */
    d = rt_date_from_ymd(arena, 2025, 5, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "May") == 0);

    /* June */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "June") == 0);

    /* July */
    d = rt_date_from_ymd(arena, 2025, 7, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "July") == 0);

    /* August */
    d = rt_date_from_ymd(arena, 2025, 8, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "August") == 0);

    /* September */
    d = rt_date_from_ymd(arena, 2025, 9, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "September") == 0);

    /* October */
    d = rt_date_from_ymd(arena, 2025, 10, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "October") == 0);

    /* November */
    d = rt_date_from_ymd(arena, 2025, 11, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "November") == 0);

    /* December */
    d = rt_date_from_ymd(arena, 2025, 12, 15);
    result = rt_date_format(arena, d, "MMMM");
    assert(strcmp(result, "December") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_ddd_token()
{
    printf("Testing rt_date_format ddd token (short weekday names)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test all 7 weekdays with short names */
    /* Using week of Jan 5-11, 2025 (Sun-Sat) */
    RtDate *d;
    char *result;

    /* Sunday (Jan 5, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 5);
    result = rt_date_format(arena, d, "ddd");
    assert(result != NULL);
    assert(strcmp(result, "Sun") == 0);

    /* Monday (Jan 6, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 6);
    result = rt_date_format(arena, d, "ddd");
    assert(strcmp(result, "Mon") == 0);

    /* Tuesday (Jan 7, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 7);
    result = rt_date_format(arena, d, "ddd");
    assert(strcmp(result, "Tue") == 0);

    /* Wednesday (Jan 8, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 8);
    result = rt_date_format(arena, d, "ddd");
    assert(strcmp(result, "Wed") == 0);

    /* Thursday (Jan 9, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 9);
    result = rt_date_format(arena, d, "ddd");
    assert(strcmp(result, "Thu") == 0);

    /* Friday (Jan 10, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 10);
    result = rt_date_format(arena, d, "ddd");
    assert(strcmp(result, "Fri") == 0);

    /* Saturday (Jan 11, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 11);
    result = rt_date_format(arena, d, "ddd");
    assert(strcmp(result, "Sat") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_dddd_token()
{
    printf("Testing rt_date_format dddd token (full weekday names)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test all 7 weekdays with full names */
    /* Using week of Jan 5-11, 2025 (Sun-Sat) */
    RtDate *d;
    char *result;

    /* Sunday (Jan 5, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 5);
    result = rt_date_format(arena, d, "dddd");
    assert(result != NULL);
    assert(strcmp(result, "Sunday") == 0);

    /* Monday (Jan 6, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 6);
    result = rt_date_format(arena, d, "dddd");
    assert(strcmp(result, "Monday") == 0);

    /* Tuesday (Jan 7, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 7);
    result = rt_date_format(arena, d, "dddd");
    assert(strcmp(result, "Tuesday") == 0);

    /* Wednesday (Jan 8, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 8);
    result = rt_date_format(arena, d, "dddd");
    assert(strcmp(result, "Wednesday") == 0);

    /* Thursday (Jan 9, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 9);
    result = rt_date_format(arena, d, "dddd");
    assert(strcmp(result, "Thursday") == 0);

    /* Friday (Jan 10, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 10);
    result = rt_date_format(arena, d, "dddd");
    assert(strcmp(result, "Friday") == 0);

    /* Saturday (Jan 11, 2025) */
    d = rt_date_from_ymd(arena, 2025, 1, 11);
    result = rt_date_format(arena, d, "dddd");
    assert(strcmp(result, "Saturday") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_mmmm_d_yyyy_combined()
{
    printf("Testing rt_date_format MMMM D, YYYY combined...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test MMMM D, YYYY combines name and numeric tokens */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 15);
    char *result = rt_date_format(arena, d1, "MMMM D, YYYY");
    assert(result != NULL);
    assert(strcmp(result, "January 15, 2025") == 0);

    /* February with single digit day */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 2, 5);
    result = rt_date_format(arena, d2, "MMMM D, YYYY");
    assert(strcmp(result, "February 5, 2025") == 0);

    /* September (longest month name) */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 9, 21);
    result = rt_date_format(arena, d3, "MMMM D, YYYY");
    assert(strcmp(result, "September 21, 2025") == 0);

    /* December last day */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_format(arena, d4, "MMMM D, YYYY");
    assert(strcmp(result, "December 31, 2025") == 0);

    /* Different year */
    RtDate *d5 = rt_date_from_ymd(arena, 2000, 6, 1);
    result = rt_date_format(arena, d5, "MMMM D, YYYY");
    assert(strcmp(result, "June 1, 2000") == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_format_ddd_mmm_d_combined()
{
    printf("Testing rt_date_format ddd, MMM D combined...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Test ddd, MMM D produces abbreviated format */
    /* Sunday, January 5, 2025 */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 1, 5);
    char *result = rt_date_format(arena, d1, "ddd, MMM D");
    assert(result != NULL);
    assert(strcmp(result, "Sun, Jan 5") == 0);

    /* Monday, February 10, 2025 */
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 2, 10);
    result = rt_date_format(arena, d2, "ddd, MMM D");
    assert(strcmp(result, "Mon, Feb 10") == 0);

    /* Wednesday, June 11, 2025 */
    RtDate *d3 = rt_date_from_ymd(arena, 2025, 6, 11);
    result = rt_date_format(arena, d3, "ddd, MMM D");
    assert(strcmp(result, "Wed, Jun 11") == 0);

    /* Friday, December 5, 2025 */
    RtDate *d4 = rt_date_from_ymd(arena, 2025, 12, 5);
    result = rt_date_format(arena, d4, "ddd, MMM D");
    assert(strcmp(result, "Fri, Dec 5") == 0);

    /* Saturday in September */
    RtDate *d5 = rt_date_from_ymd(arena, 2025, 9, 6);
    result = rt_date_format(arena, d5, "ddd, MMM D");
    assert(strcmp(result, "Sat, Sep 6") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_date_boundaries_main()
{
    printf("\n=== Running Date Boundaries/Comparisons/Getters/Constructors Tests ===\n\n");

    /* Static constructor tests */
    test_rt_date_today();
    test_rt_date_today_null_arena();
    test_rt_date_from_ymd_valid();
    /* Note: rt_date_from_ymd invalid test skipped - runtime calls exit(1) on invalid dates */
    test_rt_date_from_ymd_null_arena();
    test_rt_date_from_string_valid();
    /* Note: rt_date_from_string invalid test skipped - runtime calls exit(1) on invalid formats */
    test_rt_date_from_string_null_arena();
    test_rt_date_from_epoch_days_valid();
    test_rt_date_from_epoch_days_null_arena();
    test_rt_date_from_epoch_days_roundtrip();
    test_rt_date_is_leap_year_static();
    test_rt_date_days_in_month_static();
    test_rt_date_is_valid_ymd();

    /* Date getter tests */
    test_rt_date_get_year_month_day();
    test_rt_date_get_weekday();
    test_rt_date_get_day_of_year();
    test_rt_date_get_epoch_days();
    test_rt_date_is_weekend();
    test_rt_date_is_weekday();
    test_rt_date_instance_days_in_month();
    test_rt_date_instance_is_leap_year();

    /* toIso and toString format tests */
    test_rt_date_to_iso_format();
    test_rt_date_to_string_format();

    /* Numeric token tests */
    test_rt_date_format_yyyy_token();
    test_rt_date_format_yy_token();
    test_rt_date_format_mm_token();
    test_rt_date_format_m_token();
    test_rt_date_format_dd_token();
    test_rt_date_format_d_token();
    test_rt_date_format_yyyy_mm_dd_combined();
    test_rt_date_format_m_d_yyyy_combined();

    /* Name token tests */
    test_rt_date_format_mmm_token();
    test_rt_date_format_mmmm_token();
    test_rt_date_format_ddd_token();
    test_rt_date_format_dddd_token();
    test_rt_date_format_mmmm_d_yyyy_combined();
    test_rt_date_format_ddd_mmm_d_combined();

    /* Date boundary tests */
    test_rt_date_start_of_month();
    test_rt_date_start_of_month_null_handling();
    test_rt_date_end_of_month();
    test_rt_date_end_of_month_null_handling();
    test_rt_date_start_of_year();
    test_rt_date_start_of_year_null_handling();
    test_rt_date_end_of_year();
    test_rt_date_end_of_year_null_handling();
    test_rt_date_calculate_target_year_month_positive();
    test_rt_date_calculate_target_year_month_negative();
    test_rt_date_calculate_target_year_month_zero();
    test_rt_date_clamp_day_to_month();

    /* Comprehensive edge case tests */
    test_rt_date_epoch_boundaries();
    test_rt_date_year_boundary_transitions();
    test_rt_date_leap_year_transitions();
    test_rt_date_large_arithmetic_values();
    test_rt_date_far_future_dates();
    test_rt_date_far_past_dates();
    test_rt_date_all_methods_with_edge_dates();
    test_rt_date_boundary_conditions();
    test_rt_date_month_end_clamping_all_months();
    test_rt_date_diff_days_symmetry();
    test_rt_date_roundtrip_add_days();
    test_rt_date_roundtrip_add_weeks();
    test_rt_date_consistency_weeks_days();

    /* Date comparison tests */
    test_rt_date_is_before();
    test_rt_date_is_before_null_handling();
    test_rt_date_is_after();
    test_rt_date_is_after_null_handling();
    test_rt_date_equals();
    test_rt_date_equals_null_handling();
    test_rt_date_comparison_consistency();

    /* Date/Time conversion tests */
    test_rt_date_to_time();
    test_rt_date_to_time_null_handling();
    test_rt_time_get_date();
    test_rt_time_get_date_null_handling();
    test_rt_date_time_roundtrip();
    test_rt_time_get_date_negative_times();

    printf("\n=== Date Boundaries/Comparisons/Getters/Constructors Tests Complete ===\n\n");
}
