/* ============================================================================
 * type_checker_expr_array.c - Array Expression Type Checking
 * ============================================================================
 * Type checking for array literals, array access, array slicing, range
 * expressions, spread operators, and sized array allocations.
 * Extracted from type_checker_expr.c for modularity.
 * ============================================================================ */

#include "type_checker/type_checker_expr_array.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_util.h"
#include "debug.h"

/* ============================================================================
 * Array Literal Type Checking
 * ============================================================================ */

Type *type_check_array(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking array with %d elements", expr->as.array.element_count);
    if (expr->as.array.element_count == 0)
    {
        DEBUG_VERBOSE("Empty array, returning NIL element type");
        return ast_create_array_type(table->arena, ast_create_primitive_type(table->arena, TYPE_NIL));
    }

    Type *elem_type = NULL;
    bool valid = true;
    for (int i = 0; i < expr->as.array.element_count; i++)
    {
        Expr *element = expr->as.array.elements[i];
        Type *et = type_check_expr(element, table);
        if (et == NULL)
        {
            valid = false;
            continue;
        }

        // For spread expressions, the type returned is the element type
        // For range expressions, the type returned is int[] (an array)
        // For regular expressions, we use the type directly
        Type *actual_elem_type = et;

        // If this is a range expression, get the element type of the resulting array
        if (element->type == EXPR_RANGE)
        {
            // Range returns int[], so element type is int
            actual_elem_type = et->as.array.element_type;
        }
        // Spread already returns the element type from type_check_spread

        if (elem_type == NULL)
        {
            elem_type = actual_elem_type;
            DEBUG_VERBOSE("First array element type: %d", elem_type->kind);
        }
        else
        {
            bool equal = false;
            if (elem_type->kind == actual_elem_type->kind)
            {
                if (elem_type->kind == TYPE_ARRAY || elem_type->kind == TYPE_FUNCTION)
                {
                    equal = ast_type_equals(elem_type, actual_elem_type);
                }
                else
                {
                    equal = true;
                }
            }
            if (!equal)
            {
                type_error(expr->token, "Array elements must have the same type");
                valid = false;
                return NULL;
            }
        }
    }
    if (valid && elem_type != NULL)
    {
        DEBUG_VERBOSE("Returning array type with element type: %d", elem_type->kind);
        return ast_create_array_type(table->arena, elem_type);
    }
    return NULL;
}

/* ============================================================================
 * Array Access Type Checking
 * ============================================================================ */

Type *type_check_array_access(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking array access");
    Type *array_t = type_check_expr(expr->as.array_access.array, table);
    if (array_t == NULL)
    {
        return NULL;
    }
    if (array_t->kind != TYPE_ARRAY)
    {
        type_error(expr->token, "Cannot access non-array");
        return NULL;
    }
    Type *index_t = type_check_expr(expr->as.array_access.index, table);
    if (index_t == NULL)
    {
        return NULL;
    }
    if (!is_numeric_type(index_t))
    {
        type_error(expr->token, "Array index must be numeric type");
        return NULL;
    }
    DEBUG_VERBOSE("Returning array element type: %d", array_t->as.array.element_type->kind);
    return array_t->as.array.element_type;
}

/* ============================================================================
 * Array Slice Type Checking
 * ============================================================================ */

Type *type_check_array_slice(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking array slice");
    Type *array_t = type_check_expr(expr->as.array_slice.array, table);
    if (array_t == NULL)
    {
        return NULL;
    }
    if (array_t->kind != TYPE_ARRAY)
    {
        type_error(expr->token, "Cannot slice non-array");
        return NULL;
    }
    // Type check start index if provided
    if (expr->as.array_slice.start != NULL)
    {
        Type *start_t = type_check_expr(expr->as.array_slice.start, table);
        if (start_t == NULL)
        {
            return NULL;
        }
        if (!is_numeric_type(start_t))
        {
            type_error(expr->token, "Slice start index must be numeric type");
            return NULL;
        }
    }
    // Type check end index if provided
    if (expr->as.array_slice.end != NULL)
    {
        Type *end_t = type_check_expr(expr->as.array_slice.end, table);
        if (end_t == NULL)
        {
            return NULL;
        }
        if (!is_numeric_type(end_t))
        {
            type_error(expr->token, "Slice end index must be numeric type");
            return NULL;
        }
    }
    DEBUG_VERBOSE("Returning array type for slice: %d", array_t->kind);
    // Slicing an array returns an array of the same element type
    return array_t;
}

/* ============================================================================
 * Range Expression Type Checking
 * ============================================================================ */

Type *type_check_range(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking range expression");
    Type *start_t = type_check_expr(expr->as.range.start, table);
    if (start_t == NULL)
    {
        type_error(expr->token, "Invalid start expression in range");
        return NULL;
    }
    if (!is_numeric_type(start_t))
    {
        type_error(expr->token, "Range start must be numeric type");
        return NULL;
    }
    Type *end_t = type_check_expr(expr->as.range.end, table);
    if (end_t == NULL)
    {
        type_error(expr->token, "Invalid end expression in range");
        return NULL;
    }
    if (!is_numeric_type(end_t))
    {
        type_error(expr->token, "Range end must be numeric type");
        return NULL;
    }
    // Range always produces an int[] array
    DEBUG_VERBOSE("Returning int[] type for range");
    return ast_create_array_type(table->arena, ast_create_primitive_type(table->arena, TYPE_INT));
}

/* ============================================================================
 * Spread Operator Type Checking
 * ============================================================================ */

Type *type_check_spread(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking spread expression");
    Type *array_t = type_check_expr(expr->as.spread.array, table);
    if (array_t == NULL)
    {
        type_error(expr->token, "Invalid expression in spread");
        return NULL;
    }
    if (array_t->kind != TYPE_ARRAY)
    {
        type_error(expr->token, "Spread operator requires an array");
        return NULL;
    }
    // Spread returns the element type (for type checking in array literals)
    DEBUG_VERBOSE("Returning element type for spread: %d", array_t->as.array.element_type->kind);
    return array_t->as.array.element_type;
}

/* ============================================================================
 * Sized Array Allocation Type Checking
 * ============================================================================ */

Type *type_check_sized_array_alloc(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking sized array allocation");

    /* Extract sized array allocation components */
    Type *element_type = expr->as.sized_array_alloc.element_type;
    Expr *size_expr = expr->as.sized_array_alloc.size_expr;
    Expr *default_value = expr->as.sized_array_alloc.default_value;

    DEBUG_VERBOSE("  element_type: %p, size_expr: %p, default_value: %p",
                  (void *)element_type, (void *)size_expr, (void *)default_value);

    /* 1. Validate size expression is an integer type */
    Type *size_type = type_check_expr(size_expr, table);
    if (size_type == NULL)
    {
        return NULL;
    }

    if (size_type->kind != TYPE_INT && size_type->kind != TYPE_LONG)
    {
        type_error(expr->token, "Array size must be an integer type");
        return NULL;
    }

    DEBUG_VERBOSE("  size expression type validated: %d", size_type->kind);

    /* 2. If default_value is present, verify it matches element_type */
    if (default_value != NULL)
    {
        Type *default_type = type_check_expr(default_value, table);
        if (default_type == NULL)
        {
            return NULL;
        }

        /* Check for exact type match */
        if (!ast_type_equals(element_type, default_type))
        {
            /* Check for numeric type promotion (e.g., int default for long array) */
            Type *promoted = get_promoted_type(table->arena, element_type, default_type);
            if (promoted == NULL || !ast_type_equals(promoted, element_type))
            {
                type_error(expr->token, "Default value type does not match array element type");
                return NULL;
            }
        }

        DEBUG_VERBOSE("  default value type validated");
    }

    /* 3. Return array type of element_type */
    DEBUG_VERBOSE("Returning sized array type with element type: %d", element_type->kind);
    return ast_create_array_type(table->arena, element_type);
}
