// tests/unit/runtime/runtime_date_tests_arithmetic.c
// Tests for Date arithmetic operations (add_days, add_weeks, diff_days)

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"

/* ============================================================================
 * Date Arithmetic Tests - add_days
 * ============================================================================ */

void test_rt_date_add_days_positive()
{
    printf("Testing rt_date_add_days with positive values (future dates)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Add 1 day - tomorrow */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_add_days(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 16);

    /* Add 7 days - one week */
    result = rt_date_add_days(arena, d, 7);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 22);

    /* Add days crossing month boundary */
    d = rt_date_from_ymd(arena, 2025, 6, 28);
    result = rt_date_add_days(arena, d, 5);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 7);
    assert(rt_date_get_day(result) == 3);

    /* Add days crossing year boundary */
    d = rt_date_from_ymd(arena, 2025, 12, 30);
    result = rt_date_add_days(arena, d, 5);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 4);

    /* Add large number of days */
    d = rt_date_from_ymd(arena, 2025, 1, 1);
    result = rt_date_add_days(arena, d, 365);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 1);

    rt_arena_destroy(arena);
}

void test_rt_date_add_days_negative()
{
    printf("Testing rt_date_add_days with negative values (past dates)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Subtract 1 day - yesterday */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_add_days(arena, d, -1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 14);

    /* Subtract 7 days - one week ago */
    result = rt_date_add_days(arena, d, -7);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 8);

    /* Subtract days crossing month boundary */
    d = rt_date_from_ymd(arena, 2025, 7, 3);
    result = rt_date_add_days(arena, d, -5);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 28);

    /* Subtract days crossing year boundary */
    d = rt_date_from_ymd(arena, 2025, 1, 3);
    result = rt_date_add_days(arena, d, -5);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 29);

    /* Subtract large number of days */
    d = rt_date_from_ymd(arena, 2025, 1, 1);
    result = rt_date_add_days(arena, d, -365);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 2); /* 2024 is a leap year, so 366 days */

    rt_arena_destroy(arena);
}

void test_rt_date_add_days_zero()
{
    printf("Testing rt_date_add_days with zero (same date)...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Add 0 days - should return same date */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_add_days(arena, d, 0);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Verify epoch days are equal */
    assert(rt_date_get_epoch_days(result) == rt_date_get_epoch_days(d));

    /* Test on different date */
    d = rt_date_from_ymd(arena, 2000, 2, 29);
    result = rt_date_add_days(arena, d, 0);
    assert(rt_date_get_year(result) == 2000);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    rt_arena_destroy(arena);
}

void test_rt_date_add_days_null_handling()
{
    printf("Testing rt_date_add_days with NULL handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    RtDate *result = rt_date_add_days(arena, NULL, 5);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_add_days(NULL, d, 5);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

void test_rt_date_add_days_leap_year()
{
    printf("Testing rt_date_add_days with leap year handling...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Adding through Feb 29 in leap year */
    RtDate *d = rt_date_from_ymd(arena, 2024, 2, 28);
    RtDate *result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Adding from Feb 29 in leap year */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 1);

    /* Adding through Feb in non-leap year */
    d = rt_date_from_ymd(arena, 2025, 2, 28);
    result = rt_date_add_days(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 1);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Date Arithmetic Tests - add_weeks
 * ============================================================================ */

void test_rt_date_add_weeks_positive()
{
    printf("Testing rt_date_add_weeks with positive values...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Add 1 week = 7 days */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_add_weeks(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 22);

    /* Add 2 weeks = 14 days */
    result = rt_date_add_weeks(arena, d, 2);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 29);

    /* Add 4 weeks crossing month boundary */
    result = rt_date_add_weeks(arena, d, 4);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 7);
    assert(rt_date_get_day(result) == 13);

    /* Add weeks crossing year boundary */
    d = rt_date_from_ymd(arena, 2025, 12, 25);
    result = rt_date_add_weeks(arena, d, 2);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 8);

    rt_arena_destroy(arena);
}

void test_rt_date_add_weeks_negative()
{
    printf("Testing rt_date_add_weeks with negative values...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Subtract 2 weeks = 14 days */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_add_weeks(arena, d, -2);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 1);

    /* Subtract 1 week = 7 days */
    result = rt_date_add_weeks(arena, d, -1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 8);

    /* Subtract weeks crossing month boundary */
    d = rt_date_from_ymd(arena, 2025, 7, 5);
    result = rt_date_add_weeks(arena, d, -2);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 21);

    /* Subtract weeks crossing year boundary */
    d = rt_date_from_ymd(arena, 2025, 1, 10);
    result = rt_date_add_weeks(arena, d, -2);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 27);

    rt_arena_destroy(arena);
}

void test_rt_date_add_weeks_zero()
{
    printf("Testing rt_date_add_weeks with zero...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Add 0 weeks = same date */
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *result = rt_date_add_weeks(arena, d, 0);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Verify epoch days are equal */
    assert(rt_date_get_epoch_days(result) == rt_date_get_epoch_days(d));

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Date Arithmetic Tests - diff_days
 * ============================================================================ */

void test_rt_date_diff_days_positive()
{
    printf("Testing rt_date_diff_days with future.diffDays(past) > 0...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* future.diffDays(past) should be positive */
    RtDate *future = rt_date_from_ymd(arena, 2025, 6, 20);
    RtDate *past = rt_date_from_ymd(arena, 2025, 6, 15);
    long diff = rt_date_diff_days(future, past);
    assert(diff == 5);
    assert(diff > 0);

    /* One week difference */
    future = rt_date_from_ymd(arena, 2025, 6, 22);
    past = rt_date_from_ymd(arena, 2025, 6, 15);
    diff = rt_date_diff_days(future, past);
    assert(diff == 7);

    /* Crossing month boundary */
    future = rt_date_from_ymd(arena, 2025, 7, 5);
    past = rt_date_from_ymd(arena, 2025, 6, 28);
    diff = rt_date_diff_days(future, past);
    assert(diff == 7);

    /* Crossing year boundary */
    future = rt_date_from_ymd(arena, 2026, 1, 5);
    past = rt_date_from_ymd(arena, 2025, 12, 25);
    diff = rt_date_diff_days(future, past);
    assert(diff == 11);

    /* Large difference (one year) */
    future = rt_date_from_ymd(arena, 2026, 6, 15);
    past = rt_date_from_ymd(arena, 2025, 6, 15);
    diff = rt_date_diff_days(future, past);
    assert(diff == 365);

    rt_arena_destroy(arena);
}

void test_rt_date_diff_days_negative()
{
    printf("Testing rt_date_diff_days with past.diffDays(future) < 0...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* past.diffDays(future) should be negative */
    RtDate *past = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *future = rt_date_from_ymd(arena, 2025, 6, 20);
    long diff = rt_date_diff_days(past, future);
    assert(diff == -5);
    assert(diff < 0);

    /* One week difference */
    past = rt_date_from_ymd(arena, 2025, 6, 15);
    future = rt_date_from_ymd(arena, 2025, 6, 22);
    diff = rt_date_diff_days(past, future);
    assert(diff == -7);

    /* Crossing month boundary */
    past = rt_date_from_ymd(arena, 2025, 6, 28);
    future = rt_date_from_ymd(arena, 2025, 7, 5);
    diff = rt_date_diff_days(past, future);
    assert(diff == -7);

    /* Crossing year boundary */
    past = rt_date_from_ymd(arena, 2025, 12, 25);
    future = rt_date_from_ymd(arena, 2026, 1, 5);
    diff = rt_date_diff_days(past, future);
    assert(diff == -11);

    rt_arena_destroy(arena);
}

void test_rt_date_diff_days_zero()
{
    printf("Testing rt_date_diff_days with same.diffDays(same) == 0...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* same.diffDays(same) should be zero */
    RtDate *d1 = rt_date_from_ymd(arena, 2025, 6, 15);
    RtDate *d2 = rt_date_from_ymd(arena, 2025, 6, 15);
    long diff = rt_date_diff_days(d1, d2);
    assert(diff == 0);

    /* Using same pointer */
    diff = rt_date_diff_days(d1, d1);
    assert(diff == 0);

    /* Different years, same date value */
    d1 = rt_date_from_ymd(arena, 2000, 2, 29);
    d2 = rt_date_from_ymd(arena, 2000, 2, 29);
    diff = rt_date_diff_days(d1, d2);
    assert(diff == 0);

    rt_arena_destroy(arena);
}

void test_rt_date_diff_days_null_handling()
{
    printf("Testing rt_date_diff_days with NULL handling...\n");

    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL first date should return 0 */
    long diff = rt_date_diff_days(NULL, d);
    assert(diff == 0);

    /* NULL second date should return 0 */
    diff = rt_date_diff_days(d, NULL);
    assert(diff == 0);

    /* Both NULL should return 0 */
    diff = rt_date_diff_days(NULL, NULL);
    assert(diff == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main entry point for arithmetic tests
 * ============================================================================ */

void test_rt_date_arithmetic_main()
{
    printf("\n=== Date Arithmetic Tests ===\n");

    /* add_days tests */
    test_rt_date_add_days_positive();
    test_rt_date_add_days_negative();
    test_rt_date_add_days_zero();
    test_rt_date_add_days_null_handling();
    test_rt_date_add_days_leap_year();

    /* add_weeks tests */
    test_rt_date_add_weeks_positive();
    test_rt_date_add_weeks_negative();
    test_rt_date_add_weeks_zero();

    /* diff_days tests */
    test_rt_date_diff_days_positive();
    test_rt_date_diff_days_negative();
    test_rt_date_diff_days_zero();
    test_rt_date_diff_days_null_handling();

    printf("All Date arithmetic tests passed!\n");
}
