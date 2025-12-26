#include "type_checker_expr.h"
#include "type_checker_util.h"
#include "type_checker_stmt.h"
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
        type_error(&expr->as.variable.name, "Undefined variable");
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
        type_error(&expr->as.assign.name, "Undefined variable for assignment");
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

static bool is_builtin_name(Expr *callee, const char *name)
{
    if (callee->type != EXPR_VARIABLE) return false;
    Token tok = callee->as.variable.name;
    size_t len = strlen(name);
    return tok.length == (int)len && strncmp(tok.start, name, len) == 0;
}

static Type *type_check_call(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function call with %d arguments", expr->as.call.arg_count);

    // Handle array built-in functions specially
    Expr *callee = expr->as.call.callee;

    // len(arr) -> int (works on arrays and strings)
    if (is_builtin_name(callee, "len") && expr->as.call.arg_count == 1)
    {
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL) return NULL;
        if (arg_type->kind != TYPE_ARRAY && arg_type->kind != TYPE_STRING)
        {
            type_error(expr->token, "len() requires array or string argument");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }

    // Note: Other array operations are method-style only:
    //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)

    // Standard function call handling
    Type *callee_type = type_check_expr(expr->as.call.callee, table);
    if (callee_type == NULL)
    {
        type_error(expr->token, "Invalid callee in function call");
        return NULL;
    }
    if (callee_type->kind != TYPE_FUNCTION)
    {
        type_error(expr->token, "Callee is not a function");
        return NULL;
    }
    if (callee_type->as.function.param_count != expr->as.call.arg_count)
    {
        type_error(expr->token, "Argument count mismatch in call");
        return NULL;
    }
    for (int i = 0; i < expr->as.call.arg_count; i++)
    {
        Expr *arg_expr = expr->as.call.arguments[i];
        Type *param_type = callee_type->as.function.param_types[i];

        /* If argument is a lambda with missing types, infer from parameter type */
        if (arg_expr != NULL && arg_expr->type == EXPR_LAMBDA &&
            param_type != NULL && param_type->kind == TYPE_FUNCTION)
        {
            LambdaExpr *lambda = &arg_expr->as.lambda;
            Type *func_type = param_type;

            /* Check parameter count matches */
            if (lambda->param_count == func_type->as.function.param_count)
            {
                /* Infer missing parameter types */
                for (int j = 0; j < lambda->param_count; j++)
                {
                    if (lambda->params[j].type == NULL)
                    {
                        lambda->params[j].type = func_type->as.function.param_types[j];
                        DEBUG_VERBOSE("Inferred call argument lambda param %d type", j);
                    }
                }

                /* Infer missing return type */
                if (lambda->return_type == NULL)
                {
                    lambda->return_type = func_type->as.function.return_type;
                    DEBUG_VERBOSE("Inferred call argument lambda return type");
                }
            }
        }

        Type *arg_type = type_check_expr(arg_expr, table);
        if (arg_type == NULL)
        {
            type_error(expr->token, "Invalid argument in function call");
            return NULL;
        }
        if (param_type->kind == TYPE_ANY)
        {
            if (!is_printable_type(arg_type))
            {
                type_error(expr->token, "Unsupported type for built-in function");
                return NULL;
            }
        }
        else
        {
            if (!ast_type_equals(arg_type, param_type))
            {
                type_error(expr->token, "Argument type mismatch in call");
                return NULL;
            }
        }
    }
    DEBUG_VERBOSE("Returning function return type: %d", callee_type->as.function.return_type->kind);
    return callee_type->as.function.return_type;
}

static Type *type_check_array(Expr *expr, SymbolTable *table)
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

static Type *type_check_array_access(Expr *expr, SymbolTable *table)
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

static Type *type_check_array_slice(Expr *expr, SymbolTable *table)
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

static Type *type_check_range(Expr *expr, SymbolTable *table)
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

static Type *type_check_spread(Expr *expr, SymbolTable *table)
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

static Type *type_check_member(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking member access: %s", expr->as.member.member_name.start);
    Type *object_type = type_check_expr(expr->as.member.object, table);
    if (object_type == NULL)
    {
        return NULL;
    }
    if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "length") == 0)
    {
        DEBUG_VERBOSE("Returning INT type for array length access");
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "push") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array push method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "pop") == 0)
    {
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array pop method");
        return ast_create_function_type(table->arena, object_type->as.array.element_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "clear") == 0)
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array clear method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "concat") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *param_array_type = ast_create_array_type(table->arena, element_type);
        Type *param_types[1] = {param_array_type};
        DEBUG_VERBOSE("Returning function type for array concat method");
        return ast_create_function_type(table->arena, object_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "indexOf") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array indexOf method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "contains") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array contains method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "clone") == 0)
    {
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array clone method");
        return ast_create_function_type(table->arena, object_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "join") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for array join method");
        return ast_create_function_type(table->arena, string_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "reverse") == 0)
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array reverse method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "insert") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[2] = {element_type, int_type};
        DEBUG_VERBOSE("Returning function type for array insert method");
        return ast_create_function_type(table->arena, void_type, param_types, 2);
    }
    else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "remove") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *element_type = object_type->as.array.element_type;
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for array remove method");
        return ast_create_function_type(table->arena, element_type, param_types, 1);
    }
    // String methods
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "length") == 0)
    {
        DEBUG_VERBOSE("Returning INT type for string length access");
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "substring") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[2] = {int_type, int_type};
        DEBUG_VERBOSE("Returning function type for string substring method");
        return ast_create_function_type(table->arena, string_type, param_types, 2);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "indexOf") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string indexOf method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "split") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string split method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "trim") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string trim method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "toUpper") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toUpper method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "toLower") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toLower method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "startsWith") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string startsWith method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "endsWith") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string endsWith method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "contains") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string contains method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "replace") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[2] = {string_type, string_type};
        DEBUG_VERBOSE("Returning function type for string replace method");
        return ast_create_function_type(table->arena, string_type, param_types, 2);
    }
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "charAt") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for string charAt method");
        return ast_create_function_type(table->arena, char_type, param_types, 1);
    }
    else
    {
        type_error(expr->token, "Invalid member access");
        return NULL;
    }
}

static Type *type_check_lambda(Expr *expr, SymbolTable *table)
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
        t = type_check_call(expr, table);
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
