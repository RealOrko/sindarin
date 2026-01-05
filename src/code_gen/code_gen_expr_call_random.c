/**
 * code_gen_expr_call_random.c - Code generation for Random method calls
 *
 * Contains implementations for generating C code from method calls on
 * Random type, including value generation, batch generation, and collection operations.
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Helper to get the type suffix for collection operations based on element type */
static const char *get_type_suffix(Type *elem_type)
{
    switch (elem_type->kind)
    {
    case TYPE_INT:
        return "long";
    case TYPE_LONG:
        return "long";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "bool";
    case TYPE_BYTE:
        return "byte";
    default:
        return "long";
    }
}

/* Dispatch Random instance method calls */
char *code_gen_random_method_call(CodeGen *gen, Expr *expr, const char *method_name,
                                   Expr *object, int arg_count, Expr **arguments)
{
    char *object_str = code_gen_expression(gen, object);

    /* Value generation methods */
    if (strcmp(method_name, "int") == 0 && arg_count == 2)
    {
        char *min_str = code_gen_expression(gen, arguments[0]);
        char *max_str = code_gen_expression(gen, arguments[1]);
        return arena_sprintf(gen->arena, "rt_random_int(%s, %s, %s)",
                             object_str, min_str, max_str);
    }
    if (strcmp(method_name, "long") == 0 && arg_count == 2)
    {
        char *min_str = code_gen_expression(gen, arguments[0]);
        char *max_str = code_gen_expression(gen, arguments[1]);
        return arena_sprintf(gen->arena, "rt_random_long(%s, %s, %s)",
                             object_str, min_str, max_str);
    }
    if (strcmp(method_name, "double") == 0 && arg_count == 2)
    {
        char *min_str = code_gen_expression(gen, arguments[0]);
        char *max_str = code_gen_expression(gen, arguments[1]);
        return arena_sprintf(gen->arena, "rt_random_double(%s, %s, %s)",
                             object_str, min_str, max_str);
    }
    if (strcmp(method_name, "bool") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_random_bool(%s)", object_str);
    }
    if (strcmp(method_name, "byte") == 0 && arg_count == 0)
    {
        return arena_sprintf(gen->arena, "rt_random_byte(%s)", object_str);
    }
    if (strcmp(method_name, "bytes") == 0 && arg_count == 1)
    {
        char *count_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_random_bytes(%s, %s, %s)",
                             ARENA_VAR(gen), object_str, count_str);
    }
    if (strcmp(method_name, "gaussian") == 0 && arg_count == 2)
    {
        char *mean_str = code_gen_expression(gen, arguments[0]);
        char *stddev_str = code_gen_expression(gen, arguments[1]);
        return arena_sprintf(gen->arena, "rt_random_gaussian(%s, %s, %s)",
                             object_str, mean_str, stddev_str);
    }

    /* Batch generation methods */
    if (strcmp(method_name, "intMany") == 0 && arg_count == 3)
    {
        char *min_str = code_gen_expression(gen, arguments[0]);
        char *max_str = code_gen_expression(gen, arguments[1]);
        char *count_str = code_gen_expression(gen, arguments[2]);
        return arena_sprintf(gen->arena, "rt_random_int_many(%s, %s, %s, %s, %s)",
                             ARENA_VAR(gen), object_str, min_str, max_str, count_str);
    }
    if (strcmp(method_name, "longMany") == 0 && arg_count == 3)
    {
        char *min_str = code_gen_expression(gen, arguments[0]);
        char *max_str = code_gen_expression(gen, arguments[1]);
        char *count_str = code_gen_expression(gen, arguments[2]);
        return arena_sprintf(gen->arena, "rt_random_long_many(%s, %s, %s, %s, %s)",
                             ARENA_VAR(gen), object_str, min_str, max_str, count_str);
    }
    if (strcmp(method_name, "doubleMany") == 0 && arg_count == 3)
    {
        char *min_str = code_gen_expression(gen, arguments[0]);
        char *max_str = code_gen_expression(gen, arguments[1]);
        char *count_str = code_gen_expression(gen, arguments[2]);
        return arena_sprintf(gen->arena, "rt_random_double_many(%s, %s, %s, %s, %s)",
                             ARENA_VAR(gen), object_str, min_str, max_str, count_str);
    }
    if (strcmp(method_name, "boolMany") == 0 && arg_count == 1)
    {
        char *count_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_random_bool_many(%s, %s, %s)",
                             ARENA_VAR(gen), object_str, count_str);
    }
    if (strcmp(method_name, "gaussianMany") == 0 && arg_count == 3)
    {
        char *mean_str = code_gen_expression(gen, arguments[0]);
        char *stddev_str = code_gen_expression(gen, arguments[1]);
        char *count_str = code_gen_expression(gen, arguments[2]);
        return arena_sprintf(gen->arena, "rt_random_gaussian_many(%s, %s, %s, %s, %s)",
                             ARENA_VAR(gen), object_str, mean_str, stddev_str, count_str);
    }

    /* Collection operations - these need the expression to get the type from the first argument */
    if (strcmp(method_name, "choice") == 0 && arg_count == 1)
    {
        Type *arr_type = arguments[0]->expr_type;
        Type *elem_type = arr_type->as.array.element_type;
        const char *type_suffix = get_type_suffix(elem_type);
        char *arr_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_random_choice_%s(%s, %s, rt_array_length(%s))",
                             type_suffix, object_str, arr_str, arr_str);
    }
    if (strcmp(method_name, "shuffle") == 0 && arg_count == 1)
    {
        Type *arr_type = arguments[0]->expr_type;
        Type *elem_type = arr_type->as.array.element_type;
        const char *type_suffix = get_type_suffix(elem_type);
        char *arr_str = code_gen_expression(gen, arguments[0]);
        return arena_sprintf(gen->arena, "rt_random_shuffle_%s(%s, %s)",
                             type_suffix, object_str, arr_str);
    }
    if (strcmp(method_name, "weightedChoice") == 0 && arg_count == 2)
    {
        Type *arr_type = arguments[0]->expr_type;
        Type *elem_type = arr_type->as.array.element_type;
        const char *type_suffix = get_type_suffix(elem_type);
        char *items_str = code_gen_expression(gen, arguments[0]);
        char *weights_str = code_gen_expression(gen, arguments[1]);
        return arena_sprintf(gen->arena, "rt_random_weighted_choice_%s(%s, %s, %s)",
                             type_suffix, object_str, items_str, weights_str);
    }
    if (strcmp(method_name, "sample") == 0 && arg_count == 2)
    {
        Type *arr_type = arguments[0]->expr_type;
        Type *elem_type = arr_type->as.array.element_type;
        const char *type_suffix = get_type_suffix(elem_type);
        char *arr_str = code_gen_expression(gen, arguments[0]);
        char *count_str = code_gen_expression(gen, arguments[1]);
        return arena_sprintf(gen->arena, "rt_random_sample_%s(%s, %s, %s, %s)",
                             type_suffix, ARENA_VAR(gen), object_str, arr_str, count_str);
    }

    /* Method not handled here */
    (void)expr; /* Reserved for future use */
    return NULL;
}
