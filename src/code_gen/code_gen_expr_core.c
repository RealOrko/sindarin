#include "code_gen/code_gen_expr_core.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_expr_lambda.h"
#include "code_gen/code_gen_expr_array.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

char *code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_literal_expression");
    Type *type = expr->type;
    switch (type->kind)
    {
    case TYPE_INT:
        return arena_sprintf(gen->arena, "%lldLL", (long long)expr->value.int_value);
    case TYPE_LONG:
        return arena_sprintf(gen->arena, "%lldLL", (long long)expr->value.int_value);
    case TYPE_DOUBLE:
    {
        char *str = arena_sprintf(gen->arena, "%.17g", expr->value.double_value);
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            str = arena_sprintf(gen->arena, "%s.0", str);
        }
        return str;
    }
    case TYPE_CHAR:
        return escape_char_literal(gen->arena, expr->value.char_value);
    case TYPE_STRING:
    {
        // This might break string interpolation
        /*char *escaped = escape_c_string(gen->arena, expr->value.string_value);
        return arena_sprintf(gen->arena, "rt_to_string_string(%s)", escaped);*/
        return escape_c_string(gen->arena, expr->value.string_value);
    }
    case TYPE_BOOL:
        return arena_sprintf(gen->arena, "%ldL", expr->value.bool_value ? 1L : 0L);
    case TYPE_NIL:
        return arena_strdup(gen->arena, "NULL");
    default:
        exit(1);
    }
    return NULL;
}

char *code_gen_variable_expression(CodeGen *gen, VariableExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_variable_expression");
    char *var_name = get_var_name(gen->arena, expr->name);

    // Check if we're inside a lambda and this is a lambda parameter.
    // Lambda parameters shadow outer variables, so don't look up in symbol table.
    if (gen->enclosing_lambda_count > 0)
    {
        LambdaExpr *innermost = gen->enclosing_lambdas[gen->enclosing_lambda_count - 1];
        if (is_lambda_param(innermost, var_name))
        {
            // It's a parameter of the innermost lambda - use directly, no dereference
            return var_name;
        }
    }

    // Check if variable is 'as ref' - if so, dereference it
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "(*%s)", var_name);
    }
    return var_name;
}

char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_assign_expression");
    char *var_name = get_var_name(gen->arena, expr->name);
    char *value_str = code_gen_expression(gen, expr->value);
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol == NULL)
    {
        exit(1);
    }
    Type *type = symbol->type;

    // Handle boxing when assigning to 'any' type
    if (type->kind == TYPE_ANY && expr->value->expr_type != NULL &&
        expr->value->expr_type->kind != TYPE_ANY)
    {
        value_str = code_gen_box_value(gen, value_str, expr->value->expr_type);
    }

    // Handle 'as ref' - dereference pointer for assignment
    if (symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "(*%s = %s)", var_name, value_str);
    }

    if (type->kind == TYPE_STRING)
    {
        // Skip freeing old value in arena context - arena handles cleanup
        if (gen->current_arena_var != NULL)
        {
            return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
        }
        return arena_sprintf(gen->arena, "({ char *_val = %s; if (%s) rt_free_string(%s); %s = _val; _val; })",
                             value_str, var_name, var_name, var_name);
    }
    else
    {
        return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
    }
}

char *code_gen_index_assign_expression(CodeGen *gen, IndexAssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_index_assign_expression");
    char *array_str = code_gen_expression(gen, expr->array);
    char *index_str = code_gen_expression(gen, expr->index);
    char *value_str = code_gen_expression(gen, expr->value);

    // Check if index is provably non-negative (literal >= 0 or tracked loop counter)
    if (is_provably_non_negative(gen, expr->index))
    {
        // Non-negative index - direct array access
        return arena_sprintf(gen->arena, "(%s[%s] = %s)",
                             array_str, index_str, value_str);
    }

    // Check if index is a negative literal - can simplify to: arr[len + idx]
    if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        // Negative literal - adjust by array length
        return arena_sprintf(gen->arena, "(%s[rt_array_length(%s) + %s] = %s)",
                             array_str, array_str, index_str, value_str);
    }

    // For potentially negative variable indices, generate runtime check
    return arena_sprintf(gen->arena, "(%s[(%s) < 0 ? rt_array_length(%s) + (%s) : (%s)] = %s)",
                         array_str, index_str, array_str, index_str, index_str, value_str);
}
