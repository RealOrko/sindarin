#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #if !defined(__MINGW32__) && !defined(__MINGW64__)
    /* Only for MSVC/clang-cl, not MinGW */
    #include "../platform/compat_windows.h"
    #endif
    /* Windows uses localtime_s with swapped arguments */
    #define LOCALTIME_R(time_ptr, tm_ptr) localtime_s(tm_ptr, time_ptr)
#else
    #define LOCALTIME_R(time_ptr, tm_ptr) localtime_r(time_ptr, tm_ptr)
#endif

#include "runtime_date.h"
#include "runtime_time.h"

/* ============================================================================
 * Date Implementation
 * ============================================================================
 *
 * This module provides date operations for the Sindarin runtime.
 * Date is stored internally as days since the Unix epoch (1970-01-01).
 * The algorithm used for date calculations is based on the proleptic
 * Gregorian calendar.
 */

/* ============================================================================
 * Month and Weekday Name Arrays
 * ============================================================================ */

static const char *MONTH_NAMES_FULL[12] = {
    "January", "February", "March", "April",
    "May", "June", "July", "August",
    "September", "October", "November", "December"
};

static const char *MONTH_NAMES_SHORT[12] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

static const char *WEEKDAY_NAMES_FULL[7] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

static const char *WEEKDAY_NAMES_SHORT[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

/* Days in each month for non-leap years */
static const int days_in_months[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/* ============================================================================
 * Calendar Calculation Helpers
 * ============================================================================ */

/* Check if a year is a leap year */
int rt_date_is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/* Get number of days in a month (1-12) for a given year */
int rt_date_days_in_month(int year, int month)
{
    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && rt_date_is_leap_year(year)) {
        return 29;
    }
    return days_in_months[month - 1];
}

/* Convert year, month, day to days since epoch
 * Uses a modified Julian Day calculation optimized for days since 1970-01-01
 */
int32_t rt_date_days_from_ymd(int year, int month, int day)
{
    /* Adjust for months January and February
     * Treat them as months 13 and 14 of the previous year */
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;

    /* Calculate Julian Day Number
     * This formula gives days since a fixed epoch, then we adjust to Unix epoch */
    int32_t jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;

    /* Julian Day Number for 1970-01-01 is 2440588 */
    return jdn - 2440588;
}

/* Convert days since epoch to year, month, day
 * Inverse of days_from_ymd using the same algorithm in reverse */
void rt_date_ymd_from_days(int32_t days, int *year, int *month, int *day)
{
    /* Convert back to Julian Day Number */
    int32_t jdn = days + 2440588;

    /* Use the inverse algorithm */
    int a = jdn + 32044;
    int b = (4 * a + 3) / 146097;
    int c = a - (146097 * b) / 4;
    int d = (4 * c + 3) / 1461;
    int e = c - (1461 * d) / 4;
    int m = (5 * e + 2) / 153;

    *day = e - (153 * m + 2) / 5 + 1;
    *month = m + 3 - 12 * (m / 10);
    *year = 100 * b + d - 4800 + m / 10;
}

/* Get weekday from days since epoch (0=Sunday, 6=Saturday)
 * January 1, 1970 was a Thursday (day 4) */
int rt_date_weekday_from_days(int32_t days)
{
    /* Adjust for negative days and get modulo 7 */
    int weekday = (int)((days + 4) % 7);
    if (weekday < 0) {
        weekday += 7;
    }
    return weekday;
}

/* Get day of year (1-366) from days since epoch */
int rt_date_day_of_year(int32_t days)
{
    int year, month, day;
    rt_date_ymd_from_days(days, &year, &month, &day);

    int doy = day;
    for (int m = 1; m < month; m++) {
        doy += rt_date_days_in_month(year, m);
    }
    return doy;
}

/* Calculate target year and month from adding months to a date
 * Handles positive, negative, and zero months_to_add correctly
 * including year boundary crossings */
void rt_date_calculate_target_year_month(int year, int month, int months_to_add,
                                          int *out_year, int *out_month)
{
    /* Convert to 0-based month for arithmetic */
    int total_months = (year * 12) + (month - 1) + months_to_add;

    /* Calculate result year and month */
    *out_year = total_months / 12;
    *out_month = (total_months % 12) + 1;

    /* Handle negative modulo correctly */
    if (*out_month < 1) {
        *out_month += 12;
        (*out_year)--;
    }
}

/* Clamp a day value to the valid range for a given month
 * Used when adding months could result in an invalid day (e.g., Jan 31 + 1 month) */
int rt_date_clamp_day_to_month(int day, int year, int month)
{
    int max_day = rt_date_days_in_month(year, month);
    return (day < max_day) ? day : max_day;
}

/* ============================================================================
 * Date Validation
 * ============================================================================ */

/* Validate if year, month, day combination is valid */
int rt_date_is_valid_ymd(int year, int month, int day)
{
    /* Year must be in reasonable range (1-9999 for broad compatibility) */
    if (year < 1 || year > 9999) {
        return 0;
    }
    if (month < 1 || month > 12) {
        return 0;
    }
    if (day < 1) {
        return 0;
    }
    int max_day = rt_date_days_in_month(year, month);
    return day <= max_day;
}

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Helper function to create RtDate from days */
static RtDate *rt_date_create(RtArena *arena, int32_t days)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_create: NULL arena\n");
        return NULL;
    }
    RtDate *date = rt_arena_alloc(arena, sizeof(RtDate));
    if (date == NULL) {
        fprintf(stderr, "rt_date_create: allocation failed\n");
        exit(1);
    }
    date->days = days;
    return date;
}

/* ============================================================================
 * Date Creation
 * ============================================================================ */

/* Create date from days since epoch */
RtDate *rt_date_from_epoch_days(RtArena *arena, int32_t days)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_from_epoch_days: NULL arena\n");
        return NULL;
    }
    return rt_date_create(arena, days);
}

/* Create date from year, month, day */
RtDate *rt_date_from_ymd(RtArena *arena, int year, int month, int day)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_from_ymd: NULL arena\n");
        return NULL;
    }
    if (!rt_date_is_valid_ymd(year, month, day)) {
        fprintf(stderr, "rt_date_from_ymd: invalid date %d-%02d-%02d\n", year, month, day);
        exit(1);
    }
    int32_t days = rt_date_days_from_ymd(year, month, day);
    return rt_date_create(arena, days);
}

/* Parse date from ISO 8601 string (YYYY-MM-DD) */
RtDate *rt_date_from_string(RtArena *arena, const char *str)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_from_string: NULL arena\n");
        return NULL;
    }
    if (str == NULL) {
        fprintf(stderr, "rt_date_from_string: NULL string\n");
        exit(1);
    }

    /* Validate exact format: YYYY-MM-DD (10 characters) */
    size_t len = strlen(str);
    if (len != 10) {
        fprintf(stderr, "rt_date_from_string: invalid format '%s', expected YYYY-MM-DD (got length %zu)\n", str, len);
        exit(1);
    }

    /* Validate hyphens in correct positions */
    if (str[4] != '-' || str[7] != '-') {
        fprintf(stderr, "rt_date_from_string: invalid format '%s', expected YYYY-MM-DD\n", str);
        exit(1);
    }

    /* Validate all other characters are digits */
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue; /* Skip hyphens */
        if (str[i] < '0' || str[i] > '9') {
            fprintf(stderr, "rt_date_from_string: invalid format '%s', expected YYYY-MM-DD\n", str);
            exit(1);
        }
    }

    /* Parse the validated string */
    int year, month, day;
    sscanf(str, "%d-%d-%d", &year, &month, &day);

    return rt_date_from_ymd(arena, year, month, day);
}

/* Get current local date */
RtDate *rt_date_today(RtArena *arena)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_today: NULL arena\n");
        return NULL;
    }
    /* Get current Unix timestamp and convert to local date */
    time_t now = time(NULL);
    struct tm tm;
    LOCALTIME_R(&now, &tm);

    /* Convert local date to epoch days using our calendar algorithm */
    int32_t days = rt_date_days_from_ymd(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return rt_date_from_epoch_days(arena, days);
}

/* ============================================================================
 * Date Getters
 * ============================================================================ */

/* Get days since epoch */
int32_t rt_date_get_epoch_days(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_get_epoch_days: NULL date\n");
        return 0;
    }
    return date->days;
}

/* Get year component */
long rt_date_get_year(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_get_year: NULL date\n");
        return 0;
    }
    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    return year;
}

/* Get month component (1-12) */
long rt_date_get_month(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_get_month: NULL date\n");
        return 0;
    }
    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    return month;
}

/* Get day of month (1-31) */
long rt_date_get_day(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_get_day: NULL date\n");
        return 0;
    }
    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    return day;
}

/* Get weekday (0=Sunday, 6=Saturday) */
long rt_date_get_weekday(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_get_weekday: NULL date\n");
        return 0;
    }
    return rt_date_weekday_from_days(date->days);
}

/* Get day of year (1-366) */
long rt_date_get_day_of_year(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_get_day_of_year: NULL date\n");
        return 0;
    }
    return rt_date_day_of_year(date->days);
}

/* Get number of days in this date's month */
long rt_date_get_days_in_month(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_get_days_in_month: NULL date\n");
        return 0;
    }
    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    return rt_date_days_in_month(year, month);
}

/* Check if this date's year is a leap year */
int rt_date_is_leap(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_is_leap: NULL date\n");
        return 0;
    }
    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    return rt_date_is_leap_year(year);
}

/* Check if this date is a weekend (Saturday or Sunday) */
int rt_date_is_weekend(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_is_weekend: NULL date\n");
        return 0;
    }
    int weekday = rt_date_weekday_from_days(date->days);
    return (weekday == 0 || weekday == 6) ? 1 : 0;
}

/* Check if this date is a weekday (Monday-Friday) */
int rt_date_is_weekday(RtDate *date)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_is_weekday: NULL date\n");
        return 0;
    }
    int weekday = rt_date_weekday_from_days(date->days);
    return (weekday >= 1 && weekday <= 5) ? 1 : 0;
}

/* ============================================================================
 * Date Formatters
 * ============================================================================ */

/* Token types for date format patterns */
typedef enum {
    DATE_TOKEN_NONE = 0,
    DATE_TOKEN_YYYY,    /* 4-digit year */
    DATE_TOKEN_YY,      /* 2-digit year */
    DATE_TOKEN_MMMM,    /* Full month name */
    DATE_TOKEN_MMM,     /* Short month name */
    DATE_TOKEN_MM,      /* 2-digit month */
    DATE_TOKEN_M,       /* Month without padding */
    DATE_TOKEN_dddd,    /* Full weekday name */
    DATE_TOKEN_ddd,     /* Short weekday name */
    DATE_TOKEN_DD,      /* 2-digit day */
    DATE_TOKEN_D        /* Day without padding */
} DateTokenType;

/* Token identification result */
typedef struct {
    DateTokenType type;
    int length;
} DateToken;

/* Helper: identify a date format token at the given position
 * Uses longest-match strategy: checks longer tokens before shorter ones
 * Returns token type and length, or NONE with length 0 for non-tokens */
static DateToken identify_date_token(const char *pattern, int pos)
{
    DateToken result = { DATE_TOKEN_NONE, 0 };
    const char *p = &pattern[pos];

    /* Year tokens: YYYY before YY */
    if (strncmp(p, "YYYY", 4) == 0) {
        result.type = DATE_TOKEN_YYYY;
        result.length = 4;
    } else if (strncmp(p, "YY", 2) == 0) {
        result.type = DATE_TOKEN_YY;
        result.length = 2;
    }
    /* Month name tokens: MMMM before MMM before MM before M */
    else if (strncmp(p, "MMMM", 4) == 0) {
        result.type = DATE_TOKEN_MMMM;
        result.length = 4;
    } else if (strncmp(p, "MMM", 3) == 0) {
        result.type = DATE_TOKEN_MMM;
        result.length = 3;
    } else if (strncmp(p, "MM", 2) == 0) {
        result.type = DATE_TOKEN_MM;
        result.length = 2;
    } else if (p[0] == 'M' && !(p[1] >= 'a' && p[1] <= 'z')) {
        /* Single M token - only if not followed by lowercase letter (avoid 'May', 'Mon', etc.) */
        result.type = DATE_TOKEN_M;
        result.length = 1;
    }
    /* Weekday/Day tokens: dddd before ddd before DD before D */
    else if (strncmp(p, "dddd", 4) == 0) {
        result.type = DATE_TOKEN_dddd;
        result.length = 4;
    } else if (strncmp(p, "ddd", 3) == 0) {
        result.type = DATE_TOKEN_ddd;
        result.length = 3;
    } else if (strncmp(p, "DD", 2) == 0) {
        result.type = DATE_TOKEN_DD;
        result.length = 2;
    } else if (p[0] == 'D' && !(p[1] >= 'a' && p[1] <= 'z')) {
        /* Single D token - only if not followed by lowercase letter (avoid 'Date', 'Day', etc.) */
        result.type = DATE_TOKEN_D;
        result.length = 1;
    }

    return result;
}

/* Helper: get the maximum output size for a token type */
static size_t get_token_max_size(DateTokenType type)
{
    switch (type) {
        case DATE_TOKEN_YYYY: return 4;   /* "2025" */
        case DATE_TOKEN_YY:   return 2;   /* "25" */
        case DATE_TOKEN_MMMM: return 9;   /* "September" (longest) */
        case DATE_TOKEN_MMM:  return 3;   /* "Sep" */
        case DATE_TOKEN_MM:   return 2;   /* "01" */
        case DATE_TOKEN_M:    return 2;   /* "12" max */
        case DATE_TOKEN_dddd: return 9;   /* "Wednesday" (longest) */
        case DATE_TOKEN_ddd:  return 3;   /* "Wed" */
        case DATE_TOKEN_DD:   return 2;   /* "01" */
        case DATE_TOKEN_D:    return 2;   /* "31" max */
        case DATE_TOKEN_NONE: return 0;
    }
    return 0;
}

/* Helper: estimate maximum buffer size needed for format() output
 * Scans pattern and calculates maximum output size based on token types
 * Adds safety margin for null terminator and any edge cases */
static size_t estimate_format_buffer_size(const char *pattern)
{
    if (pattern == NULL) {
        return 32; /* Default safety size */
    }

    size_t size = 0;
    size_t len = strlen(pattern);

    for (size_t i = 0; i < len; ) {
        DateToken tok = identify_date_token(pattern, (int)i);
        if (tok.type != DATE_TOKEN_NONE) {
            /* Add maximum size for this token */
            size += get_token_max_size(tok.type);
            i += tok.length;
        } else {
            /* Literal character - add 1 */
            size++;
            i++;
        }
    }

    /* Add safety margin: null terminator + extra padding */
    return size + 32;
}

/* Helper: format a numeric date token into output buffer
 * Handles: YYYY, YY, MM, M, DD, D
 * Returns new buffer position after writing, or -1 if token not recognized */
static int format_date_number_token(const char *token, int value, char *output, int pos)
{
    if (strcmp(token, "YYYY") == 0) {
        /* 4-digit year */
        return pos + sprintf(&output[pos], "%04d", value);
    } else if (strcmp(token, "YY") == 0) {
        /* 2-digit year (modulo 100) */
        return pos + sprintf(&output[pos], "%02d", value % 100);
    } else if (strcmp(token, "MM") == 0) {
        /* 2-digit month with leading zero */
        return pos + sprintf(&output[pos], "%02d", value);
    } else if (strcmp(token, "M") == 0) {
        /* Month without padding */
        return pos + sprintf(&output[pos], "%d", value);
    } else if (strcmp(token, "DD") == 0) {
        /* 2-digit day with leading zero */
        return pos + sprintf(&output[pos], "%02d", value);
    } else if (strcmp(token, "D") == 0) {
        /* Day without padding */
        return pos + sprintf(&output[pos], "%d", value);
    }
    return -1; /* Token not recognized */
}

/* Helper: format a name-based date token into output buffer
 * Handles: ddd, dddd (weekday names), MMM, MMMM (month names)
 * Returns new buffer position after writing, or -1 if token not recognized */
static int format_date_name_token(const char *token, RtDate *date, char *output, int pos)
{
    if (strcmp(token, "dddd") == 0) {
        /* Full weekday name */
        long weekday = rt_date_get_weekday(date);
        const char *name = WEEKDAY_NAMES_FULL[weekday];
        strcpy(&output[pos], name);
        return pos + (int)strlen(name);
    } else if (strcmp(token, "ddd") == 0) {
        /* Short weekday name */
        long weekday = rt_date_get_weekday(date);
        const char *name = WEEKDAY_NAMES_SHORT[weekday];
        strcpy(&output[pos], name);
        return pos + (int)strlen(name);
    } else if (strcmp(token, "MMMM") == 0) {
        /* Full month name */
        long month = rt_date_get_month(date);
        const char *name = MONTH_NAMES_FULL[month - 1];
        strcpy(&output[pos], name);
        return pos + (int)strlen(name);
    } else if (strcmp(token, "MMM") == 0) {
        /* Short month name */
        long month = rt_date_get_month(date);
        const char *name = MONTH_NAMES_SHORT[month - 1];
        strcpy(&output[pos], name);
        return pos + (int)strlen(name);
    }
    return -1; /* Token not recognized */
}

/* Format date using pattern string */
char *rt_date_format(RtArena *arena, RtDate *date, const char *pattern)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_format: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_format: NULL date\n");
        return NULL;
    }
    if (pattern == NULL) {
        fprintf(stderr, "rt_date_format: NULL pattern\n");
        return NULL;
    }

    /* Decompose date into components for numeric tokens */
    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);

    /* Allocate output buffer based on pattern analysis */
    size_t buf_size = estimate_format_buffer_size(pattern);
    char *result = rt_arena_alloc(arena, buf_size);
    if (result == NULL) {
        fprintf(stderr, "rt_date_format: allocation failed\n");
        exit(1);
    }

    /* Scan pattern and build output using token identification */
    int out_pos = 0;
    size_t i = 0;
    while (pattern[i]) {
        DateToken tok = identify_date_token(pattern, (int)i);

        if (tok.type != DATE_TOKEN_NONE) {
            /* Process recognized token */
            int new_pos;
            switch (tok.type) {
                /* Numeric tokens - use format_date_number_token helper */
                case DATE_TOKEN_YYYY:
                    new_pos = format_date_number_token("YYYY", year, result, out_pos);
                    break;
                case DATE_TOKEN_YY:
                    new_pos = format_date_number_token("YY", year, result, out_pos);
                    break;
                case DATE_TOKEN_MM:
                    new_pos = format_date_number_token("MM", month, result, out_pos);
                    break;
                case DATE_TOKEN_M:
                    new_pos = format_date_number_token("M", month, result, out_pos);
                    break;
                case DATE_TOKEN_DD:
                    new_pos = format_date_number_token("DD", day, result, out_pos);
                    break;
                case DATE_TOKEN_D:
                    new_pos = format_date_number_token("D", day, result, out_pos);
                    break;

                /* Name tokens - use format_date_name_token helper */
                case DATE_TOKEN_MMMM:
                    new_pos = format_date_name_token("MMMM", date, result, out_pos);
                    break;
                case DATE_TOKEN_MMM:
                    new_pos = format_date_name_token("MMM", date, result, out_pos);
                    break;
                case DATE_TOKEN_dddd:
                    new_pos = format_date_name_token("dddd", date, result, out_pos);
                    break;
                case DATE_TOKEN_ddd:
                    new_pos = format_date_name_token("ddd", date, result, out_pos);
                    break;

                default:
                    new_pos = out_pos;
                    break;
            }
            out_pos = new_pos;
            i += tok.length;
        } else {
            /* No token match - copy literal character through */
            result[out_pos++] = pattern[i++];
        }
    }

    result[out_pos] = '\0';
    return result;
}

/* Format as ISO 8601 string (YYYY-MM-DD) */
char *rt_date_to_iso(RtArena *arena, RtDate *date)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_to_iso: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_to_iso: NULL date\n");
        return NULL;
    }

    long year = rt_date_get_year(date);
    long month = rt_date_get_month(date);
    long day = rt_date_get_day(date);

    char *result = rt_arena_alloc(arena, 16);
    if (result == NULL) {
        fprintf(stderr, "rt_date_to_iso: allocation failed\n");
        exit(1);
    }
    sprintf(result, "%04ld-%02ld-%02ld", year, month, day);
    return result;
}

/* Format as human-readable string */
char *rt_date_to_string(RtArena *arena, RtDate *date)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_to_string: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_to_string: NULL date\n");
        return NULL;
    }

    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);

    char *result = rt_arena_alloc(arena, 32);
    if (result == NULL) {
        fprintf(stderr, "rt_date_to_string: allocation failed\n");
        exit(1);
    }
    sprintf(result, "%s %d, %d", MONTH_NAMES_FULL[month - 1], day, year);
    return result;
}

/* ============================================================================
 * Date Arithmetic
 * ============================================================================ */

/* Add days to date */
RtDate *rt_date_add_days(RtArena *arena, RtDate *date, long days)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_add_days: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_add_days: NULL date\n");
        return NULL;
    }
    return rt_date_create(arena, date->days + (int32_t)days);
}

/* Add weeks to date */
RtDate *rt_date_add_weeks(RtArena *arena, RtDate *date, long weeks)
{
    return rt_date_add_days(arena, date, weeks * 7);
}

/* Add months to date (handles month-end edge cases) */
RtDate *rt_date_add_months(RtArena *arena, RtDate *date, int months)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_add_months: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_add_months: NULL date\n");
        return NULL;
    }

    /* Extract year, month, day from date */
    int year = rt_date_get_year(date);
    int month = rt_date_get_month(date);
    int day = rt_date_get_day(date);

    /* Calculate target year and month using helper */
    int target_year, target_month;
    rt_date_calculate_target_year_month(year, month, months, &target_year, &target_month);

    /* Clamp day to valid range for target month using helper */
    int target_day = rt_date_clamp_day_to_month(day, target_year, target_month);

    return rt_date_from_ymd(arena, target_year, target_month, target_day);
}

/* Add years to date (handles leap year edge cases) */
RtDate *rt_date_add_years(RtArena *arena, RtDate *date, long years)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_add_years: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_add_years: NULL date\n");
        return NULL;
    }

    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);

    int new_year = year + (int)years;

    /* Handle Feb 29 -> Feb 28 when going to non-leap year */
    if (month == 2 && day == 29 && !rt_date_is_leap_year(new_year)) {
        day = 28;
    }

    return rt_date_from_ymd(arena, new_year, month, day);
}

/* Get difference between dates in days */
long rt_date_diff_days(RtDate *date, RtDate *other)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_diff_days: NULL date\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_date_diff_days: NULL other\n");
        return 0;
    }
    return date->days - other->days;
}

/* Get start of month for this date */
RtDate *rt_date_start_of_month(RtArena *arena, RtDate *date)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_start_of_month: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_start_of_month: NULL date\n");
        return NULL;
    }

    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    return rt_date_from_ymd(arena, year, month, 1);
}

/* Get end of month for this date */
RtDate *rt_date_end_of_month(RtArena *arena, RtDate *date)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_end_of_month: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_end_of_month: NULL date\n");
        return NULL;
    }

    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    int last_day = rt_date_days_in_month(year, month);
    return rt_date_from_ymd(arena, year, month, last_day);
}

/* Get start of year for this date */
RtDate *rt_date_start_of_year(RtArena *arena, RtDate *date)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_start_of_year: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_start_of_year: NULL date\n");
        return NULL;
    }

    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    return rt_date_from_ymd(arena, year, 1, 1);
}

/* Get end of year for this date */
RtDate *rt_date_end_of_year(RtArena *arena, RtDate *date)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_end_of_year: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_end_of_year: NULL date\n");
        return NULL;
    }

    int year, month, day;
    rt_date_ymd_from_days(date->days, &year, &month, &day);
    return rt_date_from_ymd(arena, year, 12, 31);
}

/* ============================================================================
 * Date Comparison
 * ============================================================================ */

/* Check if date is before other */
int rt_date_is_before(RtDate *date, RtDate *other)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_is_before: NULL date\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_date_is_before: NULL other\n");
        return 0;
    }
    return (date->days < other->days) ? 1 : 0;
}

/* Check if date is after other */
int rt_date_is_after(RtDate *date, RtDate *other)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_is_after: NULL date\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_date_is_after: NULL other\n");
        return 0;
    }
    return (date->days > other->days) ? 1 : 0;
}

/* Check if dates are equal */
int rt_date_equals(RtDate *date, RtDate *other)
{
    if (date == NULL) {
        fprintf(stderr, "rt_date_equals: NULL date\n");
        return 0;
    }
    if (other == NULL) {
        fprintf(stderr, "rt_date_equals: NULL other\n");
        return 0;
    }
    return (date->days == other->days) ? 1 : 0;
}

/* ============================================================================
 * Date/Time Conversion
 * ============================================================================ */

/* Convert Date to Time (midnight UTC on that date) */
RtTime *rt_date_to_time(RtArena *arena, RtDate *date)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_date_to_time: NULL arena\n");
        return NULL;
    }
    if (date == NULL) {
        fprintf(stderr, "rt_date_to_time: NULL date\n");
        return NULL;
    }

    /* Convert days since epoch directly to milliseconds (midnight UTC).
     * This avoids using mktime which fails on Windows for pre-1970 dates,
     * and is consistent with rt_time_to_tm's UTC-based calculations. */
    long long ms = (long long)date->days * 86400LL * 1000LL;

    return rt_time_from_millis(arena, ms);
}
