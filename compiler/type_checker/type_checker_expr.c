#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_expr_call.h"
#include "type_checker/type_checker_expr_array.h"
#include "type_checker/type_checker_expr_lambda.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_stmt.h"
#include "debug.h"
#include <string.h>

static Type *type_check_binary(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking binary expression with operator: %d", expr->as.binary.operator);
    Type *left = type_check_expr(expr->as.binary.left, table);
    Type *right = type_check_expr(expr->as.binary.right, table);
    if (left == NULL || right == NULL)
    {
        type_error(expr->token, "Invalid operand in binary expression");
        return NULL;
    }
    TokenType op = expr->as.binary.operator;
    if (is_comparison_operator(op))
    {
        /* Allow numeric type promotion for comparisons (int vs double) */
        if (!ast_type_equals(left, right))
        {
            /* Check if both are numeric types - promotion is allowed */
            if (is_numeric_type(left) && is_numeric_type(right))
            {
                /* This is valid - int and double can be compared */
                DEBUG_VERBOSE("Numeric type promotion in comparison allowed");
            }
            else
            {
                type_error(expr->token, "Type mismatch in comparison");
                return NULL;
            }
        }
        DEBUG_VERBOSE("Returning BOOL type for comparison operator");
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
    else if (is_arithmetic_operator(op))
    {
        Type *promoted = get_promoted_type(table->arena, left, right);
        if (promoted == NULL)
        {
            type_error(expr->token, "Invalid types for arithmetic operator");
            return NULL;
        }
        DEBUG_VERBOSE("Returning promoted type for arithmetic operator");
        return promoted;
    }
    else if (op == TOKEN_PLUS)
    {
        /* Check for numeric type promotion */
        Type *promoted = get_promoted_type(table->arena, left, right);
        if (promoted != NULL)
        {
            DEBUG_VERBOSE("Returning promoted type for numeric + operator");
            return promoted;
        }
        else if (left->kind == TYPE_STRING && is_printable_type(right))
        {
            DEBUG_VERBOSE("Returning STRING type for string + printable");
            return left;
        }
        else if (is_printable_type(left) && right->kind == TYPE_STRING)
        {
            DEBUG_VERBOSE("Returning STRING type for printable + string");
            return right;
        }
        else
        {
            type_error(expr->token, "Invalid types for + operator");
            return NULL;
        }
    }
    else if (op == TOKEN_AND || op == TOKEN_OR)
    {
        // Logical operators require boolean operands
        if (left->kind != TYPE_BOOL || right->kind != TYPE_BOOL)
        {
            type_error(expr->token, "Logical operators require boolean operands");
            return NULL;
        }
        DEBUG_VERBOSE("Returning BOOL type for logical operator");
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
    else
    {
        type_error(expr->token, "Invalid binary operator");
        return NULL;
    }
}

static Type *type_check_unary(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking unary expression with operator: %d", expr->as.unary.operator);
    Type *operand = type_check_expr(expr->as.unary.operand, table);
    if (operand == NULL)
    {
        type_error(expr->token, "Invalid operand in unary expression");
        return NULL;
    }
    if (expr->as.unary.operator == TOKEN_MINUS)
    {
        if (!is_numeric_type(operand))
        {
            type_error(expr->token, "Unary minus on non-numeric");
            return NULL;
        }
        DEBUG_VERBOSE("Returning operand type for unary minus");
        return operand;
    }
    else if (expr->as.unary.operator == TOKEN_BANG)
    {
        if (operand->kind != TYPE_BOOL)
        {
            type_error(expr->token, "Unary ! on non-bool");
            return NULL;
        }
        DEBUG_VERBOSE("Returning operand type for unary !");
        return operand;
    }
    type_error(expr->token, "Invalid unary operator");
    return NULL;
}

static Type *type_check_interpolated(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking interpolated string with %d parts", expr->as.interpol.part_count);
    for (int i = 0; i < expr->as.interpol.part_count; i++)
    {
        Type *part_type = type_check_expr(expr->as.interpol.parts[i], table);
        if (part_type == NULL)
        {
            type_error(expr->token, "Invalid expression in interpolated string part");
            return NULL;
        }
        if (!is_printable_type(part_type))
        {
            type_error(expr->token, "Non-printable type in interpolated string");
            return NULL;
        }
    }
    DEBUG_VERBOSE("Returning STRING type for interpolated string");
    return ast_create_primitive_type(table->arena, TYPE_STRING);
}

static Type *type_check_literal(Expr *expr, SymbolTable *table)
{
    (void)table;
    DEBUG_VERBOSE("Type checking literal expression");
    return expr->as.literal.type;
}

static Type *type_check_variable(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking variable: %.*s", expr->as.variable.name.length, expr->as.variable.name.start);
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
    if (sym == NULL)
    {
        undefined_variable_error(&expr->as.variable.name, table);
        return NULL;
    }
    if (sym->type == NULL)
    {
        type_error(&expr->as.variable.name, "Symbol has no type");
        return NULL;
    }
    DEBUG_VERBOSE("Variable type found: %d", sym->type->kind);
    return sym->type;
}

static Type *type_check_assign(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking assignment to variable: %.*s", expr->as.assign.name.length, expr->as.assign.name.start);

    /* Look up symbol first to get target type for inference */
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.assign.name);
    if (sym == NULL)
    {
        undefined_variable_error_for_assign(&expr->as.assign.name, table);
        return NULL;
    }

    /* If value is a lambda with missing types, infer from target variable's type */
    Expr *value_expr = expr->as.assign.value;
    if (value_expr != NULL && value_expr->type == EXPR_LAMBDA &&
        sym->type != NULL && sym->type->kind == TYPE_FUNCTION)
    {
        LambdaExpr *lambda = &value_expr->as.lambda;
        Type *func_type = sym->type;

        /* Check parameter count matches */
        if (lambda->param_count == func_type->as.function.param_count)
        {
            /* Infer missing parameter types */
            for (int i = 0; i < lambda->param_count; i++)
            {
                if (lambda->params[i].type == NULL)
                {
                    lambda->params[i].type = func_type->as.function.param_types[i];
                    DEBUG_VERBOSE("Inferred assignment lambda param %d type from target", i);
                }
            }

            /* Infer missing return type */
            if (lambda->return_type == NULL)
            {
                lambda->return_type = func_type->as.function.return_type;
                DEBUG_VERBOSE("Inferred assignment lambda return type from target");
            }
        }
    }

    Type *value_type = type_check_expr(value_expr, table);
    if (value_type == NULL)
    {
        type_error(expr->token, "Invalid value in assignment");
        return NULL;
    }
    if (!ast_type_equals(sym->type, value_type))
    {
        type_error(&expr->as.assign.name, "Type mismatch in assignment");
        return NULL;
    }

    // Escape analysis: check if non-primitive is escaping a private block
    // Symbol's arena_depth tells us when it was declared
    // Current arena_depth tells us if we're in a private block
    int current_depth = symbol_table_get_arena_depth(table);
    if (current_depth > sym->arena_depth && !can_escape_private(value_type))
    {
        type_error(&expr->as.assign.name,
                   "Cannot assign non-primitive type to variable declared outside private block");
        return NULL;
    }

    DEBUG_VERBOSE("Assignment type matches: %d", sym->type->kind);
    return sym->type;
}

static Type *type_check_index_assign(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking index assignment");

    /* Type check the array expression */
    Type *array_type = type_check_expr(expr->as.index_assign.array, table);
    if (array_type == NULL)
    {
        type_error(expr->token, "Invalid array in index assignment");
        return NULL;
    }

    if (array_type->kind != TYPE_ARRAY)
    {
        type_error(expr->token, "Cannot index into non-array type");
        return NULL;
    }

    /* Type check the index expression */
    Type *index_type = type_check_expr(expr->as.index_assign.index, table);
    if (index_type == NULL)
    {
        type_error(expr->token, "Invalid index expression");
        return NULL;
    }

    if (index_type->kind != TYPE_INT)
    {
        type_error(expr->token, "Array index must be an integer");
        return NULL;
    }

    /* Get element type from array */
    Type *element_type = array_type->as.array.element_type;

    /* Type check the value expression */
    Type *value_type = type_check_expr(expr->as.index_assign.value, table);
    if (value_type == NULL)
    {
        type_error(expr->token, "Invalid value in index assignment");
        return NULL;
    }

    /* Check that value type matches element type */
    if (!ast_type_equals(element_type, value_type))
    {
        type_error(expr->token, "Type mismatch in index assignment");
        return NULL;
    }

    DEBUG_VERBOSE("Index assignment type check passed");
    return element_type;
}

/* is_builtin_name, type_check_call - RELOCATED to type_checker_expr_call.c
 * Now exposed as is_builtin_name() and type_check_call_expression() */

/* type_check_array, type_check_array_access - RELOCATED to type_checker_expr_array.c */

static Type *type_check_increment_decrement(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking %s expression", expr->type == EXPR_INCREMENT ? "increment" : "decrement");
    Type *operand_type = type_check_expr(expr->as.operand, table);
    if (operand_type == NULL || !is_numeric_type(operand_type))
    {
        type_error(expr->token, "Increment/decrement on non-numeric type");
        return NULL;
    }
    return operand_type;
}

/* type_check_array_slice, type_check_range, type_check_spread - RELOCATED to type_checker_expr_array.c */

/* token_equals - RELOCATED to type_checker_expr_call.c (used by type_check_member) */

static Type *type_check_member(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking member access: %s", expr->as.member.member_name.start);
    Type *object_type = type_check_expr(expr->as.member.object, table);
    if (object_type == NULL)
    {
        return NULL;
    }

    /* Array methods - DELEGATED to type_checker_expr_call.c */
    if (object_type->kind == TYPE_ARRAY)
    {
        Type *result = type_check_array_method(expr, object_type, expr->as.member.member_name, table);
        if (result != NULL)
        {
            return result;
        }
        /* Fall through to error handling if not a valid array method */
    }

    /* String methods - DELEGATED to type_checker_expr_call.c */
    if (object_type->kind == TYPE_STRING)
    {
        Type *result = type_check_string_method(expr, object_type, expr->as.member.member_name, table);
        if (result != NULL)
        {
            return result;
        }
        /* Fall through to error handling if not a valid string method */
    }

    /* TextFile methods - DELEGATED to type_checker_expr_call.c */
    if (object_type->kind == TYPE_TEXT_FILE)
    {
        Type *result = type_check_text_file_method(expr, object_type, expr->as.member.member_name, table);
        if (result != NULL)
        {
            return result;
        }
        /* Fall through to error handling if not a valid TextFile method */
    }

    /* BinaryFile methods - DELEGATED to type_checker_expr_call.c */
    if (object_type->kind == TYPE_BINARY_FILE)
    {
        Type *result = type_check_binary_file_method(expr, object_type, expr->as.member.member_name, table);
        if (result != NULL)
        {
            return result;
        }
        /* Fall through to error handling if not a valid BinaryFile method */
    }

    /* Try Time method type checking */
    if (object_type->kind == TYPE_TIME)
    {
        Type *result = type_check_time_method(expr, object_type, expr->as.member.member_name, table);
        if (result != NULL)
        {
            return result;
        }
        /* Fall through to error handling if not a valid Time method */
    }

    /* No valid method found */
    {
        /* Create null-terminated member name for error message */
        char member_name[128];
        int name_len = expr->as.member.member_name.length;
        int copy_len = name_len < 127 ? name_len : 127;
        memcpy(member_name, expr->as.member.member_name.start, copy_len);
        member_name[copy_len] = '\0';

        invalid_member_error(expr->token, object_type, member_name);
        return NULL;
    }
}

/* type_check_lambda - RELOCATED to type_checker_expr_lambda.c */

/* type_check_static_call - RELOCATED to type_checker_expr_call.c
 * Handles TextFile.*, BinaryFile.*, Path.*, Directory.*, Time.*, Stdin.*, Stdout.*, Stderr.*, Bytes.* static methods
 * See type_check_static_method_call() in type_checker_expr_call.c
 */

/* type_check_sized_array_alloc - RELOCATED to type_checker_expr_array.c */

Type *type_check_expr(Expr *expr, SymbolTable *table)
{
    if (expr == NULL)
    {
        DEBUG_VERBOSE("Expression is NULL");
        return NULL;
    }
    if (expr->expr_type)
    {
        DEBUG_VERBOSE("Using cached expression type: %d", expr->expr_type->kind);
        return expr->expr_type;
    }
    Type *t = NULL;
    DEBUG_VERBOSE("Type checking expression type: %d", expr->type);
    switch (expr->type)
    {
    case EXPR_BINARY:
        t = type_check_binary(expr, table);
        break;
    case EXPR_UNARY:
        t = type_check_unary(expr, table);
        break;
    case EXPR_LITERAL:
        t = type_check_literal(expr, table);
        break;
    case EXPR_VARIABLE:
        t = type_check_variable(expr, table);
        break;
    case EXPR_ASSIGN:
        t = type_check_assign(expr, table);
        break;
    case EXPR_INDEX_ASSIGN:
        t = type_check_index_assign(expr, table);
        break;
    case EXPR_CALL:
        t = type_check_call_expression(expr, table);
        break;
    case EXPR_ARRAY:
        t = type_check_array(expr, table);
        break;
    case EXPR_ARRAY_ACCESS:
        t = type_check_array_access(expr, table);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        t = type_check_increment_decrement(expr, table);
        break;
    case EXPR_INTERPOLATED:
        t = type_check_interpolated(expr, table);
        break;
    case EXPR_MEMBER:
        t = type_check_member(expr, table);
        break;
    case EXPR_ARRAY_SLICE:
        t = type_check_array_slice(expr, table);
        break;
    case EXPR_RANGE:
        t = type_check_range(expr, table);
        break;
    case EXPR_SPREAD:
        t = type_check_spread(expr, table);
        break;
    case EXPR_LAMBDA:
        t = type_check_lambda(expr, table);
        break;
    case EXPR_STATIC_CALL:
        t = type_check_static_method_call(expr, table);
        break;
    case EXPR_SIZED_ARRAY_ALLOC:
        t = type_check_sized_array_alloc(expr, table);
        break;
    }
    expr->expr_type = t;
    if (t != NULL)
    {
        DEBUG_VERBOSE("Expression type check result: %d", t->kind);
    }
    else
    {
        DEBUG_VERBOSE("Expression type check failed: NULL type");
    }
    return t;
}
