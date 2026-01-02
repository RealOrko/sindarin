#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "runtime_time.h"
#include "runtime_date.h"

/* ============================================================================
 * Time Implementation
 * ============================================================================
 *
 * This module provides time operations for the Sindarin runtime.
 * Time is stored internally as milliseconds since the Unix epoch.
 */

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Helper function to create RtTime from milliseconds */
static RtTime *rt_time_create(RtArena *arena, long long milliseconds)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_create: NULL arena\n");
        return NULL;
    }
    RtTime *time = rt_arena_alloc(arena, sizeof(RtTime));
    if (time == NULL) {
        fprintf(stderr, "rt_time_create: allocation failed\n");
        exit(1);
    }
    time->milliseconds = milliseconds;
    return time;
}

/* Helper function to decompose RtTime into struct tm components */
static void rt_time_to_tm(RtTime *time, struct tm *tm_result)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_to_tm: NULL time\n");
        exit(1);
    }
    /* Use floor division for negative milliseconds
     * C's integer division truncates toward zero, but we need floor */
    long long ms = time->milliseconds;
    time_t secs;
    if (ms >= 0) {
        secs = ms / 1000;
    } else {
        /* Floor division: (ms - 999) / 1000 for negative ms */
        secs = (ms - 999) / 1000;
    }
    localtime_r(&secs, tm_result);
}

/* ============================================================================
 * Time Creation
 * ============================================================================ */

/* Create Time from milliseconds since Unix epoch */
RtTime *rt_time_from_millis(RtArena *arena, long long ms)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_from_millis: NULL arena\n");
        return NULL;
    }
    return rt_time_create(arena, ms);
}

/* Create Time from seconds since Unix epoch */
RtTime *rt_time_from_seconds(RtArena *arena, long long s)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_from_seconds: NULL arena\n");
        return NULL;
    }
    return rt_time_create(arena, s * 1000);
}

/* Get current local time */
RtTime *rt_time_now(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_now: NULL arena\n");
        return NULL;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
    return rt_time_create(arena, milliseconds);
}

/* Get current UTC time */
RtTime *rt_time_utc(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_utc: NULL arena\n");
        return NULL;
    }
    /* gettimeofday returns UTC time (seconds since Unix epoch in UTC) */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
    return rt_time_create(arena, milliseconds);
}

/* Pause execution for specified number of milliseconds */
void rt_time_sleep(long ms)
{
    if (ms <= 0) {
        return;  /* No sleep for zero or negative values */
    }
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* ============================================================================
 * Time Getters
 * ============================================================================ */

/* Get time as milliseconds since Unix epoch */
long long rt_time_get_millis(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_millis: NULL time\n");
        return 0;
    }
    return time->milliseconds;
}

/* Get time as seconds since Unix epoch */
long long rt_time_get_seconds(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_seconds: NULL time\n");
        return 0;
    }
    return time->milliseconds / 1000;
}

/* Get 4-digit year */
long rt_time_get_year(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_year: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_year + 1900;
}

/* Get month (1-12) */
long rt_time_get_month(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_month: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_mon + 1;
}

/* Get day of month (1-31) */
long rt_time_get_day(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_day: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_mday;
}

/* Get hour (0-23) */
long rt_time_get_hour(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_hour: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_hour;
}

/* Get minute (0-59) */
long rt_time_get_minute(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_minute: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_min;
}

/* Get second (0-59) */
long rt_time_get_second(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_second: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_sec;
}

/* Get day of week (0=Sunday, 6=Saturday) */
long rt_time_get_weekday(RtTime *time)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_weekday: NULL time\n");
        return 0;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    return tm.tm_wday;
}

/* ============================================================================
 * Time Formatters
 * ============================================================================ */

/* Return just the date portion (YYYY-MM-DD) */
char *rt_time_to_date(RtArena *arena, RtTime *time)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_to_date: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_to_date: NULL time\n");
        return NULL;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    char *result = rt_arena_alloc(arena, 16);
    if (result == NULL) {
        fprintf(stderr, "rt_time_to_date: allocation failed\n");
        exit(1);
    }
    sprintf(result, "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return result;
}

/* Return just the time portion (HH:mm:ss) */
char *rt_time_to_time(RtArena *arena, RtTime *time)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_to_time: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_to_time: NULL time\n");
        return NULL;
    }
    struct tm tm;
    rt_time_to_tm(time, &tm);
    char *result = rt_arena_alloc(arena, 16);
    if (result == NULL) {
        fprintf(stderr, "rt_time_to_time: allocation failed\n");
        exit(1);
    }
    sprintf(result, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    return result;
}

/* Return time in ISO 8601 format (YYYY-MM-DDTHH:mm:ss.SSSZ) */
char *rt_time_to_iso(RtArena *arena, RtTime *time)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_to_iso: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_to_iso: NULL time\n");
        return NULL;
    }
    time_t secs = time->milliseconds / 1000;
    long millis = time->milliseconds % 1000;
    struct tm tm;
    gmtime_r(&secs, &tm);
    char *result = rt_arena_alloc(arena, 32);
    if (result == NULL) {
        fprintf(stderr, "rt_time_to_iso: allocation failed\n");
        exit(1);
    }
    sprintf(result, "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, millis);
    return result;
}

/* Format time using pattern string - see pattern tokens in runtime.h */
char *rt_time_format(RtArena *arena, RtTime *time, const char *pattern)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_format: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_format: NULL time\n");
        return NULL;
    }
    if (pattern == NULL) {
        fprintf(stderr, "rt_time_format: NULL pattern\n");
        return NULL;
    }

    /* Decompose time into components */
    struct tm tm;
    rt_time_to_tm(time, &tm);
    long millis = time->milliseconds % 1000;

    /* Convert 24-hour to 12-hour format (0->12, 13-23->1-11, 12 stays 12) */
    int hour12 = tm.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;

    /* Allocate generous output buffer (pattern * 3 should cover expansions) */
    size_t buf_size = strlen(pattern) * 3 + 1;
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_time_format: allocation failed\n");
        exit(1);
    }

    /* Scan pattern and build output
     * Token matching order: longer tokens must be checked before shorter ones
     * to avoid incorrect partial matches (e.g., YYYY before YY, SSS before ss).
     * Unmatched characters are copied through as literals (-, :, /, T, space, etc.)
     */
    size_t out_pos = 0;
    for (size_t i = 0; pattern[i]; ) {
        /* Year tokens - check YYYY before YY to avoid incorrect matching */
        if (strncmp(&pattern[i], "YYYY", 4) == 0) {
            out_pos += sprintf(&result[out_pos], "%04d", tm.tm_year + 1900);
            i += 4;
        } else if (strncmp(&pattern[i], "YY", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", (tm.tm_year + 1900) % 100);
            i += 2;
        }
        /* Month tokens - check MM before M to avoid incorrect matching */
        else if (strncmp(&pattern[i], "MM", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_mon + 1);
            i += 2;
        } else if (strncmp(&pattern[i], "M", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_mon + 1);
            i += 1;
        }
        /* Day tokens - check DD before D to avoid incorrect matching */
        else if (strncmp(&pattern[i], "DD", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_mday);
            i += 2;
        } else if (strncmp(&pattern[i], "D", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_mday);
            i += 1;
        }
        /* Hour tokens (24-hour) - check HH before H to avoid incorrect matching */
        else if (strncmp(&pattern[i], "HH", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_hour);
            i += 2;
        } else if (strncmp(&pattern[i], "H", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_hour);
            i += 1;
        }
        /* Hour tokens (12-hour) - check hh before h to avoid incorrect matching */
        else if (strncmp(&pattern[i], "hh", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", hour12);
            i += 2;
        } else if (pattern[i] == 'h') {
            out_pos += sprintf(&result[out_pos], "%d", hour12);
            i += 1;
        }
        /* Minute tokens - check mm before m to avoid incorrect matching */
        else if (strncmp(&pattern[i], "mm", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_min);
            i += 2;
        } else if (strncmp(&pattern[i], "m", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_min);
            i += 1;
        }
        /* Milliseconds token - check SSS before ss/s to avoid incorrect matching */
        else if (strncmp(&pattern[i], "SSS", 3) == 0) {
            out_pos += sprintf(&result[out_pos], "%03ld", millis);
            i += 3;
        }
        /* Second tokens - check ss before s to avoid incorrect matching */
        else if (strncmp(&pattern[i], "ss", 2) == 0) {
            out_pos += sprintf(&result[out_pos], "%02d", tm.tm_sec);
            i += 2;
        } else if (strncmp(&pattern[i], "s", 1) == 0) {
            out_pos += sprintf(&result[out_pos], "%d", tm.tm_sec);
            i += 1;
        }
        /* AM/PM tokens */
        else if (pattern[i] == 'A') {
            out_pos += sprintf(&result[out_pos], "%s", tm.tm_hour < 12 ? "AM" : "PM");
            i += 1;
        } else if (pattern[i] == 'a') {
            out_pos += sprintf(&result[out_pos], "%s", tm.tm_hour < 12 ? "am" : "pm");
            i += 1;
        } else {
            /* No pattern match - copy character through */
            result[out_pos++] = pattern[i++];
        }
    }

    result[out_pos] = '\0';
    return result;
}

/* ============================================================================
 * Time Arithmetic
 * ============================================================================ */

RtTime *rt_time_add(RtArena *arena, RtTime *time, long long ms)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_add: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_add: NULL time\n");
        return NULL;
    }

    long long new_ms = time->milliseconds + ms;
    return rt_time_create(arena, new_ms);
}

RtTime *rt_time_add_seconds(RtArena *arena, RtTime *time, long seconds)
{
    return rt_time_add(arena, time, seconds * 1000LL);
}

RtTime *rt_time_add_minutes(RtArena *arena, RtTime *time, long minutes)
{
    return rt_time_add(arena, time, minutes * 60 * 1000LL);
}

RtTime *rt_time_add_hours(RtArena *arena, RtTime *time, long hours)
{
    return rt_time_add(arena, time, hours * 60 * 60 * 1000LL);
}

RtTime *rt_time_add_days(RtArena *arena, RtTime *time, long days)
{
    return rt_time_add(arena, time, days * 24 * 60 * 60 * 1000LL);
}

long long rt_time_diff(RtTime *time, RtTime *other)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_diff: NULL time\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_time_diff: NULL other\n");
        return 0;
    }

    return time->milliseconds - other->milliseconds;
}

/* ============================================================================
 * Time Comparison
 * ============================================================================ */

int rt_time_is_before(RtTime *time, RtTime *other)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_is_before: NULL time\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_time_is_before: NULL other\n");
        return 0;
    }

    return (time->milliseconds < other->milliseconds) ? 1 : 0;
}

int rt_time_is_after(RtTime *time, RtTime *other)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_is_after: NULL time\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_time_is_after: NULL other\n");
        return 0;
    }

    return (time->milliseconds > other->milliseconds) ? 1 : 0;
}

int rt_time_equals(RtTime *time, RtTime *other)
{
    if (time == NULL) {
        fprintf(stderr, "rt_time_equals: NULL time\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_time_equals: NULL other\n");
        return 0;
    }

    return (time->milliseconds == other->milliseconds) ? 1 : 0;
}

/* ============================================================================
 * Time/Date Conversion
 * ============================================================================ */

/* Convert Time to Date (extract just the date portion in local timezone) */
RtDate *rt_time_get_date(RtArena *arena, RtTime *time)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_time_get_date: NULL arena\n");
        return NULL;
    }
    if (time == NULL) {
        fprintf(stderr, "rt_time_get_date: NULL time\n");
        return NULL;
    }

    /* Use localtime to extract date components in the local timezone */
    struct tm tm;
    rt_time_to_tm(time, &tm);

    int year = tm.tm_year + 1900;
    int month = tm.tm_mon + 1;
    int day = tm.tm_mday;

    return rt_date_from_ymd(arena, year, month, day);
}
