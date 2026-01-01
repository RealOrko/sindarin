/* ============================================================================
 * type_checker_expr_lambda.c - Lambda Expression Type Checking
 * ============================================================================
 * Type checking for lambda expressions including parameter type inference,
 * return type validation, and body type checking.
 * Extracted from type_checker_expr.c for modularity.
 * ============================================================================ */

#include "type_checker/type_checker_expr_lambda.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_stmt.h"
#include "debug.h"

/* ============================================================================
 * Lambda Expression Type Checking
 * ============================================================================ */

Type *type_check_lambda(Expr *expr, SymbolTable *table)
{
    LambdaExpr *lambda = &expr->as.lambda;
    DEBUG_VERBOSE("Type checking lambda with %d params, modifier: %d", lambda->param_count, lambda->modifier);

    /* Check for missing types that couldn't be inferred */
    if (lambda->return_type == NULL)
    {
        type_error(expr->token,
                   "Cannot infer lambda return type. Provide explicit type or use typed variable declaration.");
        return NULL;
    }

    for (int i = 0; i < lambda->param_count; i++)
    {
        if (lambda->params[i].type == NULL)
        {
            type_error(expr->token,
                       "Cannot infer lambda parameter type. Provide explicit type or use typed variable declaration.");
            return NULL;
        }
    }

    /* Validate private lambda return type - only primitives allowed */
    if (lambda->modifier == FUNC_PRIVATE)
    {
        if (!can_escape_private(lambda->return_type))
        {
            type_error(expr->token,
                       "Private lambda can only return primitive types (int, double, bool, char)");
            return NULL;
        }
    }

    /* Validate parameter memory qualifiers */
    for (int i = 0; i < lambda->param_count; i++)
    {
        Parameter *param = &lambda->params[i];

        /* 'as ref' is only valid for primitive types (makes them heap-allocated) */
        if (param->mem_qualifier == MEM_AS_REF)
        {
            if (!is_primitive_type(param->type))
            {
                type_error(expr->token, "'as ref' can only be used with primitive types");
                return NULL;
            }
        }

        /* 'as val' is only meaningful for reference types (arrays, strings) */
        if (param->mem_qualifier == MEM_AS_VAL)
        {
            if (is_primitive_type(param->type))
            {
                type_error(expr->token, "'as val' is only meaningful for array types");
                return NULL;
            }
        }
    }

    /* Push new scope for lambda parameters */
    symbol_table_push_scope(table);

    /* Add parameters to scope */
    for (int i = 0; i < lambda->param_count; i++)
    {
        symbol_table_add_symbol_with_kind(table, lambda->params[i].name,
                                          lambda->params[i].type, SYMBOL_PARAM);
    }

    if (lambda->has_stmt_body)
    {
        /* Multi-line lambda with statement body */
        for (int i = 0; i < lambda->body_stmt_count; i++)
        {
            type_check_stmt(lambda->body_stmts[i], table, lambda->return_type);
        }
        /* Return type checking is handled by return statements within the body */
    }
    else
    {
        /* Single-line lambda with expression body */
        Type *body_type = type_check_expr(lambda->body, table);
        if (body_type == NULL)
        {
            symbol_table_pop_scope(table);
            type_error(expr->token, "Lambda body type check failed");
            return NULL;
        }

        /* Verify return type matches body */
        if (!ast_type_equals(body_type, lambda->return_type))
        {
            symbol_table_pop_scope(table);
            type_error(expr->token, "Lambda body type does not match declared return type");
            return NULL;
        }
    }

    symbol_table_pop_scope(table);

    /* Build function type */
    Type **param_types = NULL;
    if (lambda->param_count > 0)
    {
        param_types = arena_alloc(table->arena, sizeof(Type *) * lambda->param_count);
        for (int i = 0; i < lambda->param_count; i++)
        {
            param_types[i] = lambda->params[i].type;
        }
    }

    return ast_create_function_type(table->arena, lambda->return_type,
                                    param_types, lambda->param_count);
}
