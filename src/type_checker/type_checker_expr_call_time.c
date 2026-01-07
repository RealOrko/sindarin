/* ============================================================================
 * type_checker_expr_call_time.c - Time/Date Method Type Checking
 * ============================================================================
 * Type checking for Time and Date method access (not calls).
 * Returns the function type for the method, or NULL if not a time/date method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

#include "type_checker/type_checker_expr_call_time.h"
#include "type_checker/type_checker_expr_call.h"
#include "debug.h"

/* ============================================================================
 * Time Method Type Checking
 * ============================================================================
 * Handles type checking for Time method access (not calls).
 * Returns the function type for the method, or NULL if not a Time method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

Type *type_check_time_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle Time types */
    if (object_type->kind != TYPE_TIME)
    {
        return NULL;
    }

    /* Time epoch getter methods */

    /* time.millis() -> int */
    if (token_equals(member_name, "millis"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time millis method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.seconds() -> int */
    if (token_equals(member_name, "seconds"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time seconds method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* Time date component getter methods */

    /* time.year() -> int */
    if (token_equals(member_name, "year"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time year method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.month() -> int */
    if (token_equals(member_name, "month"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time month method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.day() -> int */
    if (token_equals(member_name, "day"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time day method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* Time time component getter methods */

    /* time.hour() -> int */
    if (token_equals(member_name, "hour"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time hour method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.minute() -> int */
    if (token_equals(member_name, "minute"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time minute method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.second() -> int */
    if (token_equals(member_name, "second"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time second method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.weekday() -> int */
    if (token_equals(member_name, "weekday"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time weekday method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* Time formatting methods */

    /* time.format(pattern) -> str */
    if (token_equals(member_name, "format"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for Time format method");
        return ast_create_function_type(table->arena, str_type, param_types, 1);
    }

    /* time.toIso() -> str */
    if (token_equals(member_name, "toIso"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toIso method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* time.toDate() -> Date */
    if (token_equals(member_name, "toDate"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toDate method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* time.toTime() -> str */
    if (token_equals(member_name, "toTime"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toTime method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* Time arithmetic methods */

    /* time.add(millis) -> Time */
    if (token_equals(member_name, "add"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time add method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.addSeconds(seconds) -> Time */
    if (token_equals(member_name, "addSeconds"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time addSeconds method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.addMinutes(minutes) -> Time */
    if (token_equals(member_name, "addMinutes"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time addMinutes method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.addHours(hours) -> Time */
    if (token_equals(member_name, "addHours"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time addHours method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.addDays(days) -> Time */
    if (token_equals(member_name, "addDays"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time addDays method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.diff(other) -> int */
    if (token_equals(member_name, "diff"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[1] = {time_type};
        DEBUG_VERBOSE("Returning function type for Time diff method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* Time comparison methods */

    /* time.isBefore(other) -> bool */
    if (token_equals(member_name, "isBefore"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[1] = {time_type};
        DEBUG_VERBOSE("Returning function type for Time isBefore method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* time.isAfter(other) -> bool */
    if (token_equals(member_name, "isAfter"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[1] = {time_type};
        DEBUG_VERBOSE("Returning function type for Time isAfter method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* time.equals(other) -> bool */
    if (token_equals(member_name, "equals"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[1] = {time_type};
        DEBUG_VERBOSE("Returning function type for Time equals method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* Not a Time method */
    return NULL;
}

/* ============================================================================
 * Date Method Type Checking
 * ============================================================================
 * Handles type checking for Date method access (not calls).
 * Returns the function type for the method, or NULL if not a Date method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

Type *type_check_date_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle Date types */
    if (object_type->kind != TYPE_DATE)
    {
        return NULL;
    }

    /* Date getter methods returning int */

    /* date.year() -> int */
    if (token_equals(member_name, "year"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date year method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.month() -> int */
    if (token_equals(member_name, "month"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date month method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.day() -> int */
    if (token_equals(member_name, "day"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date day method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.weekday() -> int */
    if (token_equals(member_name, "weekday"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date weekday method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.dayOfYear() -> int */
    if (token_equals(member_name, "dayOfYear"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date dayOfYear method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.epochDays() -> int */
    if (token_equals(member_name, "epochDays"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date epochDays method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.daysInMonth() -> int */
    if (token_equals(member_name, "daysInMonth"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date daysInMonth method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* Date getter methods returning bool */

    /* date.isLeapYear() -> bool */
    if (token_equals(member_name, "isLeapYear"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date isLeapYear method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* date.isWeekend() -> bool */
    if (token_equals(member_name, "isWeekend"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date isWeekend method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* date.isWeekday() -> bool */
    if (token_equals(member_name, "isWeekday"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date isWeekday method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* Date formatting methods */

    /* date.format(pattern) -> str */
    if (token_equals(member_name, "format"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for Date format method");
        return ast_create_function_type(table->arena, str_type, param_types, 1);
    }

    /* date.toIso() -> str */
    if (token_equals(member_name, "toIso"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date toIso method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* date.toString() -> str */
    if (token_equals(member_name, "toString"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date toString method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* Date arithmetic methods returning Date */

    /* date.addDays(days) -> Date */
    if (token_equals(member_name, "addDays"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Date addDays method");
        return ast_create_function_type(table->arena, date_type, param_types, 1);
    }

    /* date.addWeeks(weeks) -> Date */
    if (token_equals(member_name, "addWeeks"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Date addWeeks method");
        return ast_create_function_type(table->arena, date_type, param_types, 1);
    }

    /* date.addMonths(months) -> Date */
    if (token_equals(member_name, "addMonths"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Date addMonths method");
        return ast_create_function_type(table->arena, date_type, param_types, 1);
    }

    /* date.addYears(years) -> Date */
    if (token_equals(member_name, "addYears"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Date addYears method");
        return ast_create_function_type(table->arena, date_type, param_types, 1);
    }

    /* date.diffDays(other) -> int */
    if (token_equals(member_name, "diffDays"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[1] = {date_type};
        DEBUG_VERBOSE("Returning function type for Date diffDays method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* Date boundary methods returning Date */

    /* date.startOfMonth() -> Date */
    if (token_equals(member_name, "startOfMonth"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date startOfMonth method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* date.endOfMonth() -> Date */
    if (token_equals(member_name, "endOfMonth"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date endOfMonth method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* date.startOfYear() -> Date */
    if (token_equals(member_name, "startOfYear"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date startOfYear method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* date.endOfYear() -> Date */
    if (token_equals(member_name, "endOfYear"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date endOfYear method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* Date comparison methods */

    /* date.isBefore(other) -> bool */
    if (token_equals(member_name, "isBefore"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[1] = {date_type};
        DEBUG_VERBOSE("Returning function type for Date isBefore method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* date.isAfter(other) -> bool */
    if (token_equals(member_name, "isAfter"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[1] = {date_type};
        DEBUG_VERBOSE("Returning function type for Date isAfter method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* date.equals(other) -> bool */
    if (token_equals(member_name, "equals"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[1] = {date_type};
        DEBUG_VERBOSE("Returning function type for Date equals method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* Date/Time conversion */

    /* date.toTime() -> Time */
    if (token_equals(member_name, "toTime"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date toTime method");
        return ast_create_function_type(table->arena, time_type, param_types, 0);
    }

    /* Not a Date method */
    return NULL;
}
