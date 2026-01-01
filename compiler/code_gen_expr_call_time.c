/**
 * code_gen_expr_call_time.c - Code generation for Time method calls
 *
 * Contains implementations for generating C code from method calls on
 * Time type, including getters, formatters, arithmetic, and comparison methods.
 */

#include "code_gen_expr_call.h"
#include "code_gen_expr.h"
#include "code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Dispatch Time instance method calls */
char *code_gen_time_method_call(CodeGen *gen, const char *method_name,
                                 Expr *object, int arg_count, Expr **arguments)
{
    char *object_str = code_gen_expression(gen, object);

    /* Getter methods - return int/long */
    if (strcmp(method_name, "millis") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_millis(%s)", object_str);
    }
    if (strcmp(method_name, "seconds") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_seconds(%s)", object_str);
    }
    if (strcmp(method_name, "year") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_year(%s)", object_str);
    }
    if (strcmp(method_name, "month") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_month(%s)", object_str);
    }
    if (strcmp(method_name, "day") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_day(%s)", object_str);
    }
    if (strcmp(method_name, "hour") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_hour(%s)", object_str);
    }
    if (strcmp(method_name, "minute") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_minute(%s)", object_str);
    }
    if (strcmp(method_name, "second") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_second(%s)", object_str);
    }
    if (strcmp(method_name, "weekday") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_get_weekday(%s)", object_str);
    }

    /* Formatting methods - return string */
    if (strcmp(method_name, "format") == 0 && arg_count == 1) {
        char *pattern_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_format(%s, %s, %s)",
            ARENA_VAR(gen), object_str, pattern_str);
    }
    if (strcmp(method_name, "toIso") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_to_iso(%s, %s)",
            ARENA_VAR(gen), object_str);
    }
    if (strcmp(method_name, "toDate") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_to_date(%s, %s)",
            ARENA_VAR(gen), object_str);
    }
    if (strcmp(method_name, "toTime") == 0 && arg_count == 0) {
        return arena_sprintf(gen->arena, "rt_time_to_time(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    /* Arithmetic methods - return Time */
    if (strcmp(method_name, "add") == 0 && arg_count == 1) {
        char *ms_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_add(%s, %s, %s)",
            ARENA_VAR(gen), object_str, ms_str);
    }
    if (strcmp(method_name, "addSeconds") == 0 && arg_count == 1) {
        char *s_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_add_seconds(%s, %s, %s)",
            ARENA_VAR(gen), object_str, s_str);
    }
    if (strcmp(method_name, "addMinutes") == 0 && arg_count == 1) {
        char *m_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_add_minutes(%s, %s, %s)",
            ARENA_VAR(gen), object_str, m_str);
    }
    if (strcmp(method_name, "addHours") == 0 && arg_count == 1) {
        char *h_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_add_hours(%s, %s, %s)",
            ARENA_VAR(gen), object_str, h_str);
    }
    if (strcmp(method_name, "addDays") == 0 && arg_count == 1) {
        char *d_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_add_days(%s, %s, %s)",
            ARENA_VAR(gen), object_str, d_str);
    }
    if (strcmp(method_name, "diff") == 0 && arg_count == 1) {
        char *other_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_diff(%s, %s)",
            object_str, other_str);
    }

    /* Comparison methods - return bool */
    if (strcmp(method_name, "isBefore") == 0 && arg_count == 1) {
        char *other_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_is_before(%s, %s)",
            object_str, other_str);
    }
    if (strcmp(method_name, "isAfter") == 0 && arg_count == 1) {
        char *other_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_is_after(%s, %s)",
            object_str, other_str);
    }
    if (strcmp(method_name, "equals") == 0 && arg_count == 1) {
        char *other_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_time_equals(%s, %s)",
            object_str, other_str);
    }

    /* Method not handled here */
    return NULL;
}
