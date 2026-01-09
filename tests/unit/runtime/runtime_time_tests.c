// tests/unit/runtime/runtime_time_tests.c
// Tests for runtime time operations

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Time Creation Tests
 * ============================================================================ */

static void test_rt_time_from_millis(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Create time from milliseconds */
    RtTime *t = rt_time_from_millis(arena, 1000);
    assert(t != NULL);
    assert(t->milliseconds == 1000);

    /* Zero milliseconds */
    t = rt_time_from_millis(arena, 0);
    assert(t->milliseconds == 0);

    /* Large value (about year 2000) */
    long long y2k_ms = 946684800000LL;  /* 2000-01-01 00:00:00 UTC */
    t = rt_time_from_millis(arena, y2k_ms);
    assert(t->milliseconds == y2k_ms);

    /* Negative (before epoch) */
    t = rt_time_from_millis(arena, -1000);
    assert(t->milliseconds == -1000);

    rt_arena_destroy(arena);
}

static void test_rt_time_from_seconds(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Create time from seconds */
    RtTime *t = rt_time_from_seconds(arena, 1);
    assert(t != NULL);
    assert(t->milliseconds == 1000);

    t = rt_time_from_seconds(arena, 60);
    assert(t->milliseconds == 60000);

    t = rt_time_from_seconds(arena, 3600);
    assert(t->milliseconds == 3600000);

    /* Zero seconds */
    t = rt_time_from_seconds(arena, 0);
    assert(t->milliseconds == 0);

    rt_arena_destroy(arena);
}

static void test_rt_time_now(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t1 = rt_time_now(arena);
    assert(t1 != NULL);
    assert(t1->milliseconds > 0);

    /* Get current time using standard library for comparison */
    time_t now = time(NULL);
    long long now_ms = (long long)now * 1000;

    /* Should be within a few seconds of current time */
    long long diff = t1->milliseconds - now_ms;
    if (diff < 0) diff = -diff;
    assert(diff < 5000);  /* Within 5 seconds */

    /* Two calls should return similar times */
    RtTime *t2 = rt_time_now(arena);
    diff = t2->milliseconds - t1->milliseconds;
    if (diff < 0) diff = -diff;
    assert(diff < 1000);  /* Within 1 second */

    rt_arena_destroy(arena);
}

static void test_rt_time_utc(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_utc(arena);
    assert(t != NULL);
    assert(t->milliseconds > 0);

    /* Should be a reasonable current timestamp */
    time_t now = time(NULL);
    long long now_ms = (long long)now * 1000;

    long long diff = t->milliseconds - now_ms;
    if (diff < 0) diff = -diff;
    assert(diff < 5000);  /* Within 5 seconds */

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Time Getter Tests
 * ============================================================================ */

static void test_rt_time_get_millis(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_from_millis(arena, 123456789);
    assert(rt_time_get_millis(t) == 123456789);

    t = rt_time_from_millis(arena, 0);
    assert(rt_time_get_millis(t) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_time_get_seconds(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_from_millis(arena, 5500);
    assert(rt_time_get_seconds(t) == 5);

    t = rt_time_from_millis(arena, 60000);
    assert(rt_time_get_seconds(t) == 60);

    t = rt_time_from_millis(arena, 999);
    assert(rt_time_get_seconds(t) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_time_get_components(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Use a known timestamp: 2024-06-15 14:30:45 UTC */
    /* This is approximately 1718458245000 ms since epoch */
    long long known_ms = 1718458245000LL;
    RtTime *t = rt_time_from_millis(arena, known_ms);

    /* The exact values depend on timezone, so we just verify they're in valid ranges */
    long year = rt_time_get_year(t);
    assert(year >= 2024 && year <= 2025);

    long month = rt_time_get_month(t);
    assert(month >= 1 && month <= 12);

    long day = rt_time_get_day(t);
    assert(day >= 1 && day <= 31);

    long hour = rt_time_get_hour(t);
    assert(hour >= 0 && hour <= 23);

    long minute = rt_time_get_minute(t);
    assert(minute >= 0 && minute <= 59);

    long second = rt_time_get_second(t);
    assert(second >= 0 && second <= 59);

    rt_arena_destroy(arena);
}

static void test_rt_time_get_weekday(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Any valid time should return weekday 0-6 */
    RtTime *t = rt_time_now(arena);
    long weekday = rt_time_get_weekday(t);
    assert(weekday >= 0 && weekday <= 6);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Time Formatter Tests
 * ============================================================================ */

static void test_rt_time_format(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_now(arena);

    /* Format with year only - uses custom pattern "YYYY" */
    char *result = rt_time_format(arena, t, "YYYY");
    assert(result != NULL);
    assert(strlen(result) == 4);  /* Year is 4 digits */

    /* Format with full date - uses custom pattern */
    result = rt_time_format(arena, t, "YYYY-MM-DD");
    assert(result != NULL);
    assert(strlen(result) == 10);  /* YYYY-MM-DD */

    /* Format with time - uses custom pattern */
    result = rt_time_format(arena, t, "HH:mm:ss");
    assert(result != NULL);
    assert(strlen(result) == 8);  /* HH:MM:SS */

    rt_arena_destroy(arena);
}

static void test_rt_time_to_iso(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_now(arena);

    char *iso = rt_time_to_iso(arena, t);
    assert(iso != NULL);
    /* ISO format: YYYY-MM-DDTHH:MM:SS (19 chars) */
    assert(strlen(iso) >= 19);
    /* Check format structure */
    assert(iso[4] == '-');
    assert(iso[7] == '-');
    assert(iso[10] == 'T');
    assert(iso[13] == ':');
    assert(iso[16] == ':');

    rt_arena_destroy(arena);
}

static void test_rt_time_to_date(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_now(arena);

    char *date = rt_time_to_date(arena, t);
    assert(date != NULL);
    /* Date format: YYYY-MM-DD (10 chars) */
    assert(strlen(date) == 10);
    assert(date[4] == '-');
    assert(date[7] == '-');

    rt_arena_destroy(arena);
}

static void test_rt_time_to_time(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_now(arena);

    char *time_str = rt_time_to_time(arena, t);
    assert(time_str != NULL);
    /* Time format: HH:MM:SS (8 chars) */
    assert(strlen(time_str) == 8);
    assert(time_str[2] == ':');
    assert(time_str[5] == ':');

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Time Arithmetic Tests
 * ============================================================================ */

static void test_rt_time_add(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_from_millis(arena, 1000);

    /* Add positive milliseconds */
    RtTime *result = rt_time_add(arena, t, 500);
    assert(result->milliseconds == 1500);

    /* Original unchanged */
    assert(t->milliseconds == 1000);

    /* Add negative milliseconds */
    result = rt_time_add(arena, t, -300);
    assert(result->milliseconds == 700);

    /* Add zero */
    result = rt_time_add(arena, t, 0);
    assert(result->milliseconds == 1000);

    rt_arena_destroy(arena);
}

static void test_rt_time_add_seconds(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_from_millis(arena, 0);

    RtTime *result = rt_time_add_seconds(arena, t, 10);
    assert(result->milliseconds == 10000);

    result = rt_time_add_seconds(arena, t, -5);
    assert(result->milliseconds == -5000);

    rt_arena_destroy(arena);
}

static void test_rt_time_add_minutes(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_from_millis(arena, 0);

    RtTime *result = rt_time_add_minutes(arena, t, 1);
    assert(result->milliseconds == 60000);

    result = rt_time_add_minutes(arena, t, 5);
    assert(result->milliseconds == 300000);

    rt_arena_destroy(arena);
}

static void test_rt_time_add_hours(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_from_millis(arena, 0);

    RtTime *result = rt_time_add_hours(arena, t, 1);
    assert(result->milliseconds == 3600000);

    result = rt_time_add_hours(arena, t, 24);
    assert(result->milliseconds == 86400000);

    rt_arena_destroy(arena);
}

static void test_rt_time_add_days(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t = rt_time_from_millis(arena, 0);

    RtTime *result = rt_time_add_days(arena, t, 1);
    assert(result->milliseconds == 86400000LL);

    result = rt_time_add_days(arena, t, 7);
    assert(result->milliseconds == 604800000LL);

    result = rt_time_add_days(arena, t, -1);
    assert(result->milliseconds == -86400000LL);

    rt_arena_destroy(arena);
}

static void test_rt_time_diff(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t1 = rt_time_from_millis(arena, 5000);
    RtTime *t2 = rt_time_from_millis(arena, 3000);

    /* t1 - t2 */
    long long diff = rt_time_diff(t1, t2);
    assert(diff == 2000);

    /* t2 - t1 */
    diff = rt_time_diff(t2, t1);
    assert(diff == -2000);

    /* Same time */
    diff = rt_time_diff(t1, t1);
    assert(diff == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Time Comparison Tests
 * ============================================================================ */

static void test_rt_time_is_before(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *earlier = rt_time_from_millis(arena, 1000);
    RtTime *later = rt_time_from_millis(arena, 2000);
    RtTime *same = rt_time_from_millis(arena, 1000);

    assert(rt_time_is_before(earlier, later) == 1);
    assert(rt_time_is_before(later, earlier) == 0);
    assert(rt_time_is_before(earlier, same) == 0);  /* Not strictly before */

    rt_arena_destroy(arena);
}

static void test_rt_time_is_after(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *earlier = rt_time_from_millis(arena, 1000);
    RtTime *later = rt_time_from_millis(arena, 2000);
    RtTime *same = rt_time_from_millis(arena, 2000);

    assert(rt_time_is_after(later, earlier) == 1);
    assert(rt_time_is_after(earlier, later) == 0);
    assert(rt_time_is_after(later, same) == 0);  /* Not strictly after */

    rt_arena_destroy(arena);
}

static void test_rt_time_equals(void)
{
    RtArena *arena = rt_arena_create(NULL);

    RtTime *t1 = rt_time_from_millis(arena, 12345);
    RtTime *t2 = rt_time_from_millis(arena, 12345);
    RtTime *t3 = rt_time_from_millis(arena, 12346);

    assert(rt_time_equals(t1, t2) == 1);
    assert(rt_time_equals(t1, t3) == 0);
    assert(rt_time_equals(t1, t1) == 1);  /* Same pointer */

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_time_main(void)
{
    TEST_SECTION("Runtime Time");

    /* Creation */
    TEST_RUN("rt_time_from_millis", test_rt_time_from_millis);
    TEST_RUN("rt_time_from_seconds", test_rt_time_from_seconds);
    TEST_RUN("rt_time_now", test_rt_time_now);
    TEST_RUN("rt_time_utc", test_rt_time_utc);

    /* Getters */
    TEST_RUN("rt_time_get_millis", test_rt_time_get_millis);
    TEST_RUN("rt_time_get_seconds", test_rt_time_get_seconds);
    TEST_RUN("rt_time_get_components", test_rt_time_get_components);
    TEST_RUN("rt_time_get_weekday", test_rt_time_get_weekday);

    /* Formatters */
    TEST_RUN("rt_time_format", test_rt_time_format);
    TEST_RUN("rt_time_to_iso", test_rt_time_to_iso);
    TEST_RUN("rt_time_to_date", test_rt_time_to_date);
    TEST_RUN("rt_time_to_time", test_rt_time_to_time);

    /* Arithmetic */
    TEST_RUN("rt_time_add", test_rt_time_add);
    TEST_RUN("rt_time_add_seconds", test_rt_time_add_seconds);
    TEST_RUN("rt_time_add_minutes", test_rt_time_add_minutes);
    TEST_RUN("rt_time_add_hours", test_rt_time_add_hours);
    TEST_RUN("rt_time_add_days", test_rt_time_add_days);
    TEST_RUN("rt_time_diff", test_rt_time_diff);

    /* Comparison */
    TEST_RUN("rt_time_is_before", test_rt_time_is_before);
    TEST_RUN("rt_time_is_after", test_rt_time_is_after);
    TEST_RUN("rt_time_equals", test_rt_time_equals);
}
