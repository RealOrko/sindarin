#ifndef RUNTIME_TIME_H
#define RUNTIME_TIME_H

#include "runtime_arena.h"

/* ============================================================================
 * Time Type Definition
 * ============================================================================ */

/* Time handle - wrapper around epoch milliseconds */
typedef struct RtTime {
    long long milliseconds;     /* Milliseconds since Unix epoch (1970-01-01 00:00:00 UTC) */
} RtTime;

/* ============================================================================
 * Time Creation
 * ============================================================================ */

/* Create time from milliseconds since epoch */
RtTime *rt_time_from_millis(RtArena *arena, long long ms);

/* Create time from seconds since epoch */
RtTime *rt_time_from_seconds(RtArena *arena, long long s);

/* Get current local time */
RtTime *rt_time_now(RtArena *arena);

/* Get current UTC time */
RtTime *rt_time_utc(RtArena *arena);

/* Sleep for specified milliseconds */
void rt_time_sleep(long ms);

/* ============================================================================
 * Time Getters
 * ============================================================================ */

/* Get milliseconds since epoch */
long long rt_time_get_millis(RtTime *time);

/* Get seconds since epoch */
long long rt_time_get_seconds(RtTime *time);

/* Get year component (e.g., 2024) */
long rt_time_get_year(RtTime *time);

/* Get month component (1-12) */
long rt_time_get_month(RtTime *time);

/* Get day of month (1-31) */
long rt_time_get_day(RtTime *time);

/* Get hour component (0-23) */
long rt_time_get_hour(RtTime *time);

/* Get minute component (0-59) */
long rt_time_get_minute(RtTime *time);

/* Get second component (0-59) */
long rt_time_get_second(RtTime *time);

/* Get weekday (0=Sunday, 6=Saturday) */
long rt_time_get_weekday(RtTime *time);

/* ============================================================================
 * Time Formatters
 * ============================================================================ */

/* Format time using strftime-style pattern */
char *rt_time_format(RtArena *arena, RtTime *time, const char *pattern);

/* Format as ISO 8601 string (e.g., "2024-01-15T14:30:00") */
char *rt_time_to_iso(RtArena *arena, RtTime *time);

/* Format as date string (e.g., "2024-01-15") */
char *rt_time_to_date(RtArena *arena, RtTime *time);

/* Format as time string (e.g., "14:30:00") */
char *rt_time_to_time(RtArena *arena, RtTime *time);

/* ============================================================================
 * Time Arithmetic
 * ============================================================================ */

/* Add milliseconds to time */
RtTime *rt_time_add(RtArena *arena, RtTime *time, long long ms);

/* Add seconds to time */
RtTime *rt_time_add_seconds(RtArena *arena, RtTime *time, long seconds);

/* Add minutes to time */
RtTime *rt_time_add_minutes(RtArena *arena, RtTime *time, long minutes);

/* Add hours to time */
RtTime *rt_time_add_hours(RtArena *arena, RtTime *time, long hours);

/* Add days to time */
RtTime *rt_time_add_days(RtArena *arena, RtTime *time, long days);

/* Get difference between times in milliseconds */
long long rt_time_diff(RtTime *time, RtTime *other);

/* ============================================================================
 * Time Comparison
 * ============================================================================ */

/* Check if time is before other */
int rt_time_is_before(RtTime *time, RtTime *other);

/* Check if time is after other */
int rt_time_is_after(RtTime *time, RtTime *other);

/* Check if times are equal */
int rt_time_equals(RtTime *time, RtTime *other);

#endif /* RUNTIME_TIME_H */
