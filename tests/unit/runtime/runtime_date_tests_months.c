// tests/unit/runtime/runtime_date_tests_months.c
// Tests for runtime date add_months and add_years operations

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Date add_months Tests
 * ============================================================================ */

static void test_rt_date_add_months_simple(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* (2025, 3, 15) + 1 month = (2025, 4, 15) - no clamping, day preserved */
    RtDate *d = rt_date_from_ymd(arena, 2025, 3, 15);
    RtDate *result = rt_date_add_months(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 15);

    /* (2025, 3, 15) + 2 months = (2025, 5, 15) - no clamping, day preserved */
    d = rt_date_from_ymd(arena, 2025, 3, 15);
    result = rt_date_add_months(arena, d, 2);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 5);
    assert(rt_date_get_day(result) == 15);

    /* (2025, 1, 10) + 5 months = (2025, 6, 10) - no clamping, day preserved */
    d = rt_date_from_ymd(arena, 2025, 1, 10);
    result = rt_date_add_months(arena, d, 5);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 10);

    /* (2025, 6, 20) + 3 months = (2025, 9, 20) - no clamping, day preserved */
    d = rt_date_from_ymd(arena, 2025, 6, 20);
    result = rt_date_add_months(arena, d, 3);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 9);
    assert(rt_date_get_day(result) == 20);

    /* Add 0 months - should return same date */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 0);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Add 0 months to 31-day month end - should return identical date */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 0);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 31);

    /* Add 0 months to leap year Feb 29 - should return identical date */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_months(arena, d, 0);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Simple add within year: January + 3 = April (no clamping) */
    d = rt_date_from_ymd(arena, 2025, 1, 10);
    result = rt_date_add_months(arena, d, 3);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 10);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_months_null_handling(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    RtDate *result = rt_date_add_months(arena, NULL, 2);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_add_months(NULL, d, 2);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_months_clamping(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Jan 31 + 1 month = Feb 28 (2025, non-leap year) */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Jan 31 + 1 month = Feb 29 (2024, leap year) */
    d = rt_date_from_ymd(arena, 2024, 1, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Mar 31 + 1 month = Apr 30 (April has 30 days) */
    d = rt_date_from_ymd(arena, 2025, 3, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 30);

    /* May 31 + 1 month = Jun 30 (June has 30 days) */
    d = rt_date_from_ymd(arena, 2025, 5, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 30);

    /* Jul 31 + 1 month = Aug 31 (no clamping, both have 31 days) */
    d = rt_date_from_ymd(arena, 2025, 7, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 8);
    assert(rt_date_get_day(result) == 31);

    /* Aug 31 + 1 month = Sep 30 (September has 30 days) */
    d = rt_date_from_ymd(arena, 2025, 8, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 9);
    assert(rt_date_get_day(result) == 30);

    /* Oct 31 + 1 month = Nov 30 (November has 30 days) */
    d = rt_date_from_ymd(arena, 2025, 10, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 11);
    assert(rt_date_get_day(result) == 30);

    /* Dec 31 + 1 month = Jan 31 next year (no clamping, January has 31 days) */
    d = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 31);

    /* Jan 31 + 2 months = Mar 31 (March has 31 days, no clamping) */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 2);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 31);

    /* Jan 30 + 1 month = Feb 28 (2025, day 30 also requires clamping) */
    d = rt_date_from_ymd(arena, 2025, 1, 30);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Jan 30 + 1 month = Feb 29 (2024, leap year, day 30 clamped to 29) */
    d = rt_date_from_ymd(arena, 2024, 1, 30);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Jan 29 + 1 month = Feb 28 (2025, non-leap, day 29 clamped to 28) */
    d = rt_date_from_ymd(arena, 2025, 1, 29);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Jan 29 + 1 month = Feb 29 (2024, leap year, no clamping needed) */
    d = rt_date_from_ymd(arena, 2024, 1, 29);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Mar 30 + 1 month = Apr 30 (no clamping, April has 30 days) */
    d = rt_date_from_ymd(arena, 2025, 3, 30);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 30);

    /* Apr 30 + 1 month = May 30 (no clamping, May has 31 days) */
    d = rt_date_from_ymd(arena, 2025, 4, 30);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 5);
    assert(rt_date_get_day(result) == 30);

    /* --- Multi-month jumps with clamping --- */

    /* Jan 31 + 3 months = Apr 30 (lands in 30-day month, skips Feb and Mar) */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 3);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 30);

    /* Jan 31 + 13 months = Feb 28 next year (non-leap, year crossing + clamping) */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 13);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Jan 31 + 13 months = Feb 29 next year (leap year, year crossing + clamping) */
    d = rt_date_from_ymd(arena, 2023, 1, 31);
    result = rt_date_add_months(arena, d, 13);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* May 31 + 4 months = Sep 30 (September has 30 days) */
    d = rt_date_from_ymd(arena, 2025, 5, 31);
    result = rt_date_add_months(arena, d, 4);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 9);
    assert(rt_date_get_day(result) == 30);

    /* Dec 31 + 2 months = Feb 28 next year (non-leap, year crossing + clamping) */
    d = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_add_months(arena, d, 2);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Dec 31 + 2 months = Feb 29 next year (leap year) */
    d = rt_date_from_ymd(arena, 2023, 12, 31);
    result = rt_date_add_months(arena, d, 2);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_months_year_boundary(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Dec + 1 month crosses to next January: (2025, 12, 15) + 1 = (2026, 1, 15) */
    d = rt_date_from_ymd(arena, 2025, 12, 15);
    result = rt_date_add_months(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 15);

    /* Nov + 2 months crosses year: (2025, 11, 20) + 2 = (2026, 1, 20) */
    d = rt_date_from_ymd(arena, 2025, 11, 20);
    result = rt_date_add_months(arena, d, 2);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 20);

    /* Nov + 3 months crosses year: (2025, 11, 20) + 3 = (2026, 2, 20) */
    d = rt_date_from_ymd(arena, 2025, 11, 20);
    result = rt_date_add_months(arena, d, 3);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 20);

    /* Oct + 3 months crosses year: (2025, 10, 5) + 3 = (2026, 1, 5) */
    d = rt_date_from_ymd(arena, 2025, 10, 5);
    result = rt_date_add_months(arena, d, 3);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 5);

    /* Dec 31 + 1 month = Jan 31 next year (no clamping, January has 31 days) */
    d = rt_date_from_ymd(arena, 2025, 12, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 31);

    /* Nov 30 + 2 months = Jan 30 next year (no clamping, January has 31 days) */
    d = rt_date_from_ymd(arena, 2025, 11, 30);
    result = rt_date_add_months(arena, d, 2);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 30);

    /* Jan - 1 month goes to previous December: (2025, 1, 10) + (-1) = (2024, 12, 10) */
    d = rt_date_from_ymd(arena, 2025, 1, 10);
    result = rt_date_add_months(arena, d, -1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 10);

    /* Mar - 5 months goes to previous year: (2025, 3, 5) + (-5) = (2024, 10, 5) */
    d = rt_date_from_ymd(arena, 2025, 3, 5);
    result = rt_date_add_months(arena, d, -5);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 10);
    assert(rt_date_get_day(result) == 5);

    /* Month-end clamping across year: (2024, 12, 31) + 2 = (2025, 2, 28) */
    d = rt_date_from_ymd(arena, 2024, 12, 31);
    result = rt_date_add_months(arena, d, 2);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Additional: crossing multiple years */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 24); /* 2 years */
    assert(rt_date_get_year(result) == 2027);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Additional: negative months crossing multiple years */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, -18); /* 1.5 years back */
    assert(rt_date_get_year(result) == 2023);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 15);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_months_edge_cases(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Adding 0 months returns same date */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 0);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Adding 12 months (1 full year) advances year correctly */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 12);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Adding 12 months from January: (2025, 1, 15) + 12 = (2026, 1, 15) */
    d = rt_date_from_ymd(arena, 2025, 1, 15);
    result = rt_date_add_months(arena, d, 12);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 15);

    /* Adding 24 months (2 years) advances year by 2 */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 24);
    assert(rt_date_get_year(result) == 2027);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Adding 24 months from January: (2025, 1, 15) + 24 = (2027, 1, 15) */
    d = rt_date_from_ymd(arena, 2025, 1, 15);
    result = rt_date_add_months(arena, d, 24);
    assert(rt_date_get_year(result) == 2027);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 15);

    /* Adding 12 months to month-end: (2025, 1, 31) + 12 = (2026, 1, 31) - no clamping */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 12);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 31);

    /* Adding -12 months (1 year back) reduces year correctly */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, -12);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Large positive value: adding 100 months (8 years + 4 months) */
    /* 2025-06-15 + 100 months = 2033-10-15 */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 100);
    assert(rt_date_get_year(result) == 2033);
    assert(rt_date_get_month(result) == 10);
    assert(rt_date_get_day(result) == 15);

    /* Large negative value: adding -100 months (8 years + 4 months back) */
    /* 2025-06-15 - 100 months = 2017-02-15 */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, -100);
    assert(rt_date_get_year(result) == 2017);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 15);

    /* Month-end with large jumps: Jan 31 + 13 months = Feb 28 of next year (2025->2026, non-leap) */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 13);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Month-end with large jumps to leap year: Jan 31 + 13 months = Feb 29 (2023->2024, leap) */
    d = rt_date_from_ymd(arena, 2023, 1, 31);
    result = rt_date_add_months(arena, d, 13);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Very large positive: 1200 months = 100 years */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 1200);
    assert(rt_date_get_year(result) == 2125);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Very large negative: -1200 months = 100 years back */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, -1200);
    assert(rt_date_get_year(result) == 1925);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_months_negative(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Simple same-year backward: (2025, 3, 15) + (-1) = (2025, 2, 15) */
    d = rt_date_from_ymd(arena, 2025, 3, 15);
    result = rt_date_add_months(arena, d, -1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 15);

    /* Multi-month same-year backward: (2025, 5, 20) + (-2) = (2025, 3, 20) */
    d = rt_date_from_ymd(arena, 2025, 5, 20);
    result = rt_date_add_months(arena, d, -2);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 20);

    /* Clamping backward from 31-day to Feb: (2025, 3, 31) + (-1) = (2025, 2, 28) */
    d = rt_date_from_ymd(arena, 2025, 3, 31);
    result = rt_date_add_months(arena, d, -1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Clamping backward from 31-day to 30-day month: (2025, 5, 31) + (-1) = (2025, 4, 30) */
    d = rt_date_from_ymd(arena, 2025, 5, 31);
    result = rt_date_add_months(arena, d, -1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 30);

    /* Year boundary backward: (2025, 1, 15) + (-1) = (2024, 12, 15) */
    d = rt_date_from_ymd(arena, 2025, 1, 15);
    result = rt_date_add_months(arena, d, -1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 15);

    /* Multi-month year boundary backward: (2025, 2, 15) + (-3) = (2024, 11, 15) */
    d = rt_date_from_ymd(arena, 2025, 2, 15);
    result = rt_date_add_months(arena, d, -3);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 11);
    assert(rt_date_get_day(result) == 15);

    /* Backward crossing to leap year Feb 29: (2025, 3, 31) + (-13) = (2024, 2, 29) */
    d = rt_date_from_ymd(arena, 2025, 3, 31);
    result = rt_date_add_months(arena, d, -13);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_months_leap_year_feb(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* (2024, 2, 29) + 12 months = (2025, 2, 28) - leap to non-leap year */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_months(arena, d, 12);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* (2024, 2, 29) + 48 months = (2028, 2, 29) - leap to leap year */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_months(arena, d, 48);
    assert(rt_date_get_year(result) == 2028);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* (2024, 1, 31) + 1 month = (2024, 2, 29) - leap year February */
    d = rt_date_from_ymd(arena, 2024, 1, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* (2025, 1, 31) + 1 month = (2025, 2, 28) - non-leap year February */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* (2020, 2, 29) + 1 month = (2020, 3, 29) - Feb 29 to March */
    d = rt_date_from_ymd(arena, 2020, 2, 29);
    result = rt_date_add_months(arena, d, 1);
    assert(rt_date_get_year(result) == 2020);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 29);

    /* (2024, 2, 29) + (-12) months = (2023, 2, 28) - backward leap to non-leap */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_months(arena, d, -12);
    assert(rt_date_get_year(result) == 2023);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Additional: (2024, 2, 29) + (-48) months = (2020, 2, 29) - backward leap to leap */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_months(arena, d, -48);
    assert(rt_date_get_year(result) == 2020);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_months_roundtrip_symmetry(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;
    RtDate *roundtrip;

    /* === Simple cases: round-trip returns original date === */

    /* Mid-month date: addMonths(n).addMonths(-n) returns original */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 3);
    roundtrip = rt_date_add_months(arena, result, -3);
    assert(rt_date_get_year(roundtrip) == 2025);
    assert(rt_date_get_month(roundtrip) == 6);
    assert(rt_date_get_day(roundtrip) == 15);

    /* Large round-trip: 12 months forward and back */
    d = rt_date_from_ymd(arena, 2025, 3, 10);
    result = rt_date_add_months(arena, d, 12);
    roundtrip = rt_date_add_months(arena, result, -12);
    assert(rt_date_get_year(roundtrip) == 2025);
    assert(rt_date_get_month(roundtrip) == 3);
    assert(rt_date_get_day(roundtrip) == 10);

    /* === Expected asymmetry for month-end dates (due to clamping) === */

    /*
     * DOCUMENTED ASYMMETRY: Jan 31 + 1 month = Feb 28, Feb 28 - 1 month = Jan 28
     * Round-trip does NOT return original when clamping occurs!
     * This is expected behavior - months have different lengths.
     */
    d = rt_date_from_ymd(arena, 2025, 1, 31);
    result = rt_date_add_months(arena, d, 1);      /* Jan 31 -> Feb 28 */
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);
    roundtrip = rt_date_add_months(arena, result, -1);  /* Feb 28 -> Jan 28 */
    assert(rt_date_get_year(roundtrip) == 2025);
    assert(rt_date_get_month(roundtrip) == 1);
    assert(rt_date_get_day(roundtrip) == 28);      /* NOT 31! This is expected. */

    /* Another asymmetry case: Mar 31 + 1 = Apr 30, Apr 30 - 1 = Mar 30 */
    d = rt_date_from_ymd(arena, 2025, 3, 31);
    result = rt_date_add_months(arena, d, 1);      /* Mar 31 -> Apr 30 */
    assert(rt_date_get_month(result) == 4);
    assert(rt_date_get_day(result) == 30);
    roundtrip = rt_date_add_months(arena, result, -1);  /* Apr 30 -> Mar 30 */
    assert(rt_date_get_month(roundtrip) == 3);
    assert(rt_date_get_day(roundtrip) == 30);      /* NOT 31! Expected. */

    /* Leap year asymmetry: Jan 31 2024 + 1 = Feb 29, Feb 29 - 1 = Jan 29 */
    d = rt_date_from_ymd(arena, 2024, 1, 31);
    result = rt_date_add_months(arena, d, 1);      /* Jan 31 -> Feb 29 */
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);
    roundtrip = rt_date_add_months(arena, result, -1);  /* Feb 29 -> Jan 29 */
    assert(rt_date_get_month(roundtrip) == 1);
    assert(rt_date_get_day(roundtrip) == 29);      /* NOT 31! Expected. */

    /* === Equivalence: addMonths(12) vs addYears(1) for non-Feb-29 dates === */

    /* Regular date: addMonths(12) == addYears(1) */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_months(arena, d, 12);
    RtDate *years_result = rt_date_add_years(arena, d, 1);
    assert(rt_date_get_year(result) == rt_date_get_year(years_result));
    assert(rt_date_get_month(result) == rt_date_get_month(years_result));
    assert(rt_date_get_day(result) == rt_date_get_day(years_result));

    /* Month-end non-Feb: addMonths(12) == addYears(1) */
    d = rt_date_from_ymd(arena, 2025, 3, 31);
    result = rt_date_add_months(arena, d, 12);
    years_result = rt_date_add_years(arena, d, 1);
    assert(rt_date_get_year(result) == rt_date_get_year(years_result));
    assert(rt_date_get_month(result) == rt_date_get_month(years_result));
    assert(rt_date_get_day(result) == rt_date_get_day(years_result));

    /* Feb 28 non-leap: addMonths(12) == addYears(1) */
    d = rt_date_from_ymd(arena, 2025, 2, 28);
    result = rt_date_add_months(arena, d, 12);
    years_result = rt_date_add_years(arena, d, 1);
    assert(rt_date_get_year(result) == rt_date_get_year(years_result));
    assert(rt_date_get_month(result) == rt_date_get_month(years_result));
    assert(rt_date_get_day(result) == rt_date_get_day(years_result));

    /* Feb 29 leap year: addMonths(12) and addYears(1) both clamp to Feb 28 */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_months(arena, d, 12);     /* Feb 29 2024 -> Feb 28 2025 */
    years_result = rt_date_add_years(arena, d, 1); /* Feb 29 2024 -> Feb 28 2025 */
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);
    assert(rt_date_get_year(years_result) == 2025);
    assert(rt_date_get_month(years_result) == 2);
    assert(rt_date_get_day(years_result) == 28);

    /* === Additional mathematical property tests === */

    /* Negative round-trip symmetry for simple cases */
    d = rt_date_from_ymd(arena, 2025, 8, 20);
    result = rt_date_add_months(arena, d, -5);
    roundtrip = rt_date_add_months(arena, result, 5);
    assert(rt_date_get_year(roundtrip) == 2025);
    assert(rt_date_get_month(roundtrip) == 8);
    assert(rt_date_get_day(roundtrip) == 20);

    /* Commutativity for simple cases: addMonths(a).addMonths(b) == addMonths(a+b) */
    d = rt_date_from_ymd(arena, 2025, 4, 15);
    RtDate *step1 = rt_date_add_months(arena, d, 3);
    RtDate *step2 = rt_date_add_months(arena, step1, 5);
    RtDate *direct = rt_date_add_months(arena, d, 8);
    assert(rt_date_get_year(step2) == rt_date_get_year(direct));
    assert(rt_date_get_month(step2) == rt_date_get_month(direct));
    assert(rt_date_get_day(step2) == rt_date_get_day(direct));

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Date add_years Tests
 * ============================================================================ */

static void test_rt_date_add_years_basic(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Non-Feb-29 date works normally: add 1 year */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_years(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Add multiple years */
    d = rt_date_from_ymd(arena, 2025, 3, 20);
    result = rt_date_add_years(arena, d, 5);
    assert(rt_date_get_year(result) == 2030);
    assert(rt_date_get_month(result) == 3);
    assert(rt_date_get_day(result) == 20);

    /* Add 0 years - same date */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_years(arena, d, 0);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Large positive value: +10 years */
    d = rt_date_from_ymd(arena, 2025, 7, 20);
    result = rt_date_add_years(arena, d, 10);
    assert(rt_date_get_year(result) == 2035);
    assert(rt_date_get_month(result) == 7);
    assert(rt_date_get_day(result) == 20);

    /* Large positive value: +100 years */
    d = rt_date_from_ymd(arena, 2025, 8, 25);
    result = rt_date_add_years(arena, d, 100);
    assert(rt_date_get_year(result) == 2125);
    assert(rt_date_get_month(result) == 8);
    assert(rt_date_get_day(result) == 25);

    /* Non-Feb-29 dates unaffected by leap year logic */
    /* Feb 28 in leap year stays Feb 28 */
    d = rt_date_from_ymd(arena, 2024, 2, 28);
    result = rt_date_add_years(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Jan 31 stays Jan 31 (no leap year effect) */
    d = rt_date_from_ymd(arena, 2024, 1, 31);
    result = rt_date_add_years(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 1);
    assert(rt_date_get_day(result) == 31);

    /* Dec 31 stays Dec 31 (no leap year effect) */
    d = rt_date_from_ymd(arena, 2024, 12, 31);
    result = rt_date_add_years(arena, d, 1);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 12);
    assert(rt_date_get_day(result) == 31);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_years_leap_clamping(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Feb 29, 2024 + 1 year = Feb 28, 2025 (leap to non-leap) */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_years(arena, d, 1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2025);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Feb 29, 2024 + 4 years = Feb 29, 2028 (leap to leap) */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_years(arena, d, 4);
    assert(rt_date_get_year(result) == 2028);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Feb 29, 2024 + 2 years = Feb 28, 2026 (leap to non-leap) */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_years(arena, d, 2);
    assert(rt_date_get_year(result) == 2026);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Feb 29, 2024 + 3 years = Feb 28, 2027 (leap to non-leap) */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_years(arena, d, 3);
    assert(rt_date_get_year(result) == 2027);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_years_negative(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d;
    RtDate *result;

    /* Non-leap date with negative years */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_years(arena, d, -1);
    assert(result != NULL);
    assert(rt_date_get_year(result) == 2024);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    /* Feb 29, 2024 - 1 year = Feb 28, 2023 (leap to non-leap) */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_years(arena, d, -1);
    assert(rt_date_get_year(result) == 2023);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 28);

    /* Feb 29, 2024 - 4 years = Feb 29, 2020 (leap to leap) */
    d = rt_date_from_ymd(arena, 2024, 2, 29);
    result = rt_date_add_years(arena, d, -4);
    assert(rt_date_get_year(result) == 2020);
    assert(rt_date_get_month(result) == 2);
    assert(rt_date_get_day(result) == 29);

    /* Moderate negative value: -10 years */
    d = rt_date_from_ymd(arena, 2025, 9, 10);
    result = rt_date_add_years(arena, d, -10);
    assert(rt_date_get_year(result) == 2015);
    assert(rt_date_get_month(result) == 9);
    assert(rt_date_get_day(result) == 10);

    /* Large negative value: -100 years */
    d = rt_date_from_ymd(arena, 2025, 6, 15);
    result = rt_date_add_years(arena, d, -100);
    assert(rt_date_get_year(result) == 1925);
    assert(rt_date_get_month(result) == 6);
    assert(rt_date_get_day(result) == 15);

    rt_arena_destroy(arena);
}

static void test_rt_date_add_years_null_handling(void)
{
    RtArena *arena = rt_arena_create(NULL);
    RtDate *d = rt_date_from_ymd(arena, 2025, 6, 15);

    /* NULL date should return NULL */
    RtDate *result = rt_date_add_years(arena, NULL, 1);
    assert(result == NULL);

    /* NULL arena should return NULL */
    result = rt_date_add_years(NULL, d, 1);
    assert(result == NULL);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_date_months_main(void)
{
    TEST_SECTION("Date Months/Years Arithmetic");

    /* add_months tests */
    TEST_RUN("date_add_months_simple", test_rt_date_add_months_simple);
    TEST_RUN("date_add_months_null_handling", test_rt_date_add_months_null_handling);
    TEST_RUN("date_add_months_clamping", test_rt_date_add_months_clamping);
    TEST_RUN("date_add_months_year_boundary", test_rt_date_add_months_year_boundary);
    TEST_RUN("date_add_months_edge_cases", test_rt_date_add_months_edge_cases);
    TEST_RUN("date_add_months_negative", test_rt_date_add_months_negative);
    TEST_RUN("date_add_months_leap_year_feb", test_rt_date_add_months_leap_year_feb);
    TEST_RUN("date_add_months_roundtrip_symmetry", test_rt_date_add_months_roundtrip_symmetry);

    /* add_years tests */
    TEST_RUN("date_add_years_basic", test_rt_date_add_years_basic);
    TEST_RUN("date_add_years_leap_clamping", test_rt_date_add_years_leap_clamping);
    TEST_RUN("date_add_years_negative", test_rt_date_add_years_negative);
    TEST_RUN("date_add_years_null_handling", test_rt_date_add_years_null_handling);
}
