#ifndef TYPE_CHECKER_EXPR_CALL_TIME_H
#define TYPE_CHECKER_EXPR_CALL_TIME_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Time/Date Method Type Checking
 * ============================================================================
 * Type checking for Time and Date method access (not calls).
 * Returns the function type for the method, or NULL if not a time/date method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

/* Type check Time methods
 * Handles: millis, seconds, year, month, day, hour, minute, second, weekday,
 *          format, toIso, toDate, toTime, add, addSeconds, addMinutes,
 *          addHours, addDays, diff, isBefore, isAfter, equals
 */
Type *type_check_time_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check Date methods
 * Handles: year, month, day, weekday, dayOfYear, epochDays, daysInMonth,
 *          isLeapYear, isWeekend, isWeekday, format, toIso, toString,
 *          addDays, addWeeks, addMonths, addYears, diffDays, startOfMonth,
 *          endOfMonth, startOfYear, endOfYear, isBefore, isAfter, equals, toTime
 */
Type *type_check_date_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_TIME_H */
