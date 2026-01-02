#ifndef RUNTIME_DATE_H
#define RUNTIME_DATE_H

#include "runtime_arena.h"
#include <stdint.h>

/* ============================================================================
 * Date Type Definition
 * ============================================================================ */

/* Date handle - wrapper around days since epoch (1970-01-01) */
typedef struct RtDate {
    int32_t days;     /* Days since Unix epoch (1970-01-01), can be negative */
} RtDate;

/* ============================================================================
 * Calendar Helper Constants
 * ============================================================================ */

/* Month name arrays for formatting */
extern const char *rt_date_month_names_full[12];
extern const char *rt_date_month_names_short[12];

/* Weekday name arrays for formatting (0=Sunday) */
extern const char *rt_date_weekday_names_full[7];
extern const char *rt_date_weekday_names_short[7];

/* ============================================================================
 * Calendar Calculation Helpers
 * ============================================================================ */

/* Check if a year is a leap year */
int rt_date_is_leap_year(int year);

/* Get number of days in a month (1-12) for a given year */
int rt_date_days_in_month(int year, int month);

/* Convert year, month, day to days since epoch */
int32_t rt_date_days_from_ymd(int year, int month, int day);

/* Convert days since epoch to year, month, day (via output pointers) */
void rt_date_ymd_from_days(int32_t days, int *year, int *month, int *day);

/* Get weekday from days since epoch (0=Sunday, 6=Saturday) */
int rt_date_weekday_from_days(int32_t days);

/* Get day of year (1-366) from days since epoch */
int rt_date_day_of_year(int32_t days);

/* Calculate target year and month from adding months to a date */
void rt_date_calculate_target_year_month(int year, int month, int months_to_add,
                                          int *out_year, int *out_month);

/* Clamp a day value to the valid range for a given month */
int rt_date_clamp_day_to_month(int day, int year, int month);

/* ============================================================================
 * Date Validation
 * ============================================================================ */

/* Validate if year, month, day combination is valid */
int rt_date_is_valid_ymd(int year, int month, int day);

/* ============================================================================
 * Date Creation
 * ============================================================================ */

/* Create date from days since epoch */
RtDate *rt_date_from_epoch_days(RtArena *arena, int32_t days);

/* Create date from year, month, day */
RtDate *rt_date_from_ymd(RtArena *arena, int year, int month, int day);

/* Parse date from ISO 8601 string (YYYY-MM-DD) */
RtDate *rt_date_from_string(RtArena *arena, const char *str);

/* Get current local date */
RtDate *rt_date_today(RtArena *arena);

/* ============================================================================
 * Date Getters
 * ============================================================================ */

/* Get days since epoch */
int32_t rt_date_get_epoch_days(RtDate *date);

/* Get year component (e.g., 2024) */
long rt_date_get_year(RtDate *date);

/* Get month component (1-12) */
long rt_date_get_month(RtDate *date);

/* Get day of month (1-31) */
long rt_date_get_day(RtDate *date);

/* Get weekday (0=Sunday, 6=Saturday) */
long rt_date_get_weekday(RtDate *date);

/* Get day of year (1-366) */
long rt_date_get_day_of_year(RtDate *date);

/* Get number of days in this date's month */
long rt_date_get_days_in_month(RtDate *date);

/* Check if this date's year is a leap year */
int rt_date_is_leap(RtDate *date);

/* Check if this date is a weekend (Saturday or Sunday) */
int rt_date_is_weekend(RtDate *date);

/* Check if this date is a weekday (Monday-Friday) */
int rt_date_is_weekday(RtDate *date);

/* ============================================================================
 * Date Formatters
 * ============================================================================ */

/* Format date using pattern string */
char *rt_date_format(RtArena *arena, RtDate *date, const char *pattern);

/* Format as ISO 8601 string (YYYY-MM-DD) */
char *rt_date_to_iso(RtArena *arena, RtDate *date);

/* Format as human-readable string (e.g., "December 25, 2025") */
char *rt_date_to_string(RtArena *arena, RtDate *date);

/* ============================================================================
 * Date Arithmetic
 * ============================================================================ */

/* Add days to date */
RtDate *rt_date_add_days(RtArena *arena, RtDate *date, long days);

/* Add weeks to date */
RtDate *rt_date_add_weeks(RtArena *arena, RtDate *date, long weeks);

/* Add months to date (handles month-end edge cases) */
RtDate *rt_date_add_months(RtArena *arena, RtDate *date, int months);

/* Add years to date (handles leap year edge cases) */
RtDate *rt_date_add_years(RtArena *arena, RtDate *date, long years);

/* Get difference between dates in days */
long rt_date_diff_days(RtDate *date, RtDate *other);

/* Get start of month for this date */
RtDate *rt_date_start_of_month(RtArena *arena, RtDate *date);

/* Get end of month for this date */
RtDate *rt_date_end_of_month(RtArena *arena, RtDate *date);

/* Get start of year for this date */
RtDate *rt_date_start_of_year(RtArena *arena, RtDate *date);

/* Get end of year for this date */
RtDate *rt_date_end_of_year(RtArena *arena, RtDate *date);

/* ============================================================================
 * Date Comparison
 * ============================================================================ */

/* Check if date is before other */
int rt_date_is_before(RtDate *date, RtDate *other);

/* Check if date is after other */
int rt_date_is_after(RtDate *date, RtDate *other);

/* Check if dates are equal */
int rt_date_equals(RtDate *date, RtDate *other);

/* ============================================================================
 * Date/Time Conversion
 * ============================================================================ */

/* Forward declaration for RtTime */
struct RtTime;

/* Convert Date to Time (midnight on that date) */
struct RtTime *rt_date_to_time(RtArena *arena, RtDate *date);

#endif /* RUNTIME_DATE_H */
