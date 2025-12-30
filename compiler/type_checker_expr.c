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

    /* Get function name for error messages */
    char func_name[128] = "<anonymous>";
    if (expr->as.call.callee->type == EXPR_VARIABLE)
    {
        int name_len = expr->as.call.callee->as.variable.name.length;
        int copy_len = name_len < 127 ? name_len : 127;
        memcpy(func_name, expr->as.call.callee->as.variable.name.start, copy_len);
        func_name[copy_len] = '\0';
    }

    if (callee_type == NULL)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Invalid callee '%s' in function call", func_name);
        type_error(expr->token, msg);
        return NULL;
    }
    if (callee_type->kind != TYPE_FUNCTION)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "'%s' is of type '%s', cannot call non-function",
                 func_name, type_name(callee_type));
        type_error(expr->token, msg);
        return NULL;
    }
    if (callee_type->as.function.param_count != expr->as.call.arg_count)
    {
        argument_count_error(expr->token, func_name,
                            callee_type->as.function.param_count,
                            expr->as.call.arg_count);
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
                argument_type_error(expr->token, func_name, i, param_type, arg_type);
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

/* Forward declaration for token_equals used in type_check_member */
static bool token_equals(Token tok, const char *str);

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
    /* Byte array extension methods - only available on byte[] */
    else if (object_type->kind == TYPE_ARRAY &&
             object_type->as.array.element_type->kind == TYPE_BYTE &&
             strcmp(expr->as.member.member_name.start, "toString") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for byte array toString method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_ARRAY &&
             object_type->as.array.element_type->kind == TYPE_BYTE &&
             strcmp(expr->as.member.member_name.start, "toStringLatin1") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for byte array toStringLatin1 method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_ARRAY &&
             object_type->as.array.element_type->kind == TYPE_BYTE &&
             strcmp(expr->as.member.member_name.start, "toHex") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for byte array toHex method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_ARRAY &&
             object_type->as.array.element_type->kind == TYPE_BYTE &&
             strcmp(expr->as.member.member_name.start, "toBase64") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for byte array toBase64 method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
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
    /* String toBytes() method - encodes string to UTF-8 byte array */
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "toBytes") == 0)
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toBytes method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 0);
    }
    /* String splitWhitespace() method - splits on any whitespace */
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "splitWhitespace") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string splitWhitespace method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 0);
    }
    /* String splitLines() method - splits on line endings (\n, \r\n, \r) */
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "splitLines") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string splitLines method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 0);
    }
    /* String isBlank() method - checks if empty or whitespace only */
    else if (object_type->kind == TYPE_STRING && strcmp(expr->as.member.member_name.start, "isBlank") == 0)
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string isBlank method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }
    /* TextFile instance methods */
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "readChar"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readChar method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "readWord"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readWord method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "readLine"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readLine method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "readAll"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readAll method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "readLines"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readLines method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "readInto"))
    {
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *char_array_type = ast_create_array_type(table->arena, char_type);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {char_array_type};
        DEBUG_VERBOSE("Returning function type for TextFile readInto method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "close"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile close method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }
    /* TextFile instance writing methods */
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "writeChar"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[1] = {char_type};
        DEBUG_VERBOSE("Returning function type for TextFile writeChar method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "write"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TextFile write method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "writeLine"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TextFile writeLine method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "print"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TextFile print method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "println"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TextFile println method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    /* TextFile state query methods */
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "hasChars"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile hasChars method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "hasWords"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile hasWords method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "hasLines"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile hasLines method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "isEof"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile isEof method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }
    /* TextFile position manipulation methods */
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "position"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile position method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "seek"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for TextFile seek method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "rewind"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile rewind method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "flush"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile flush method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }
    /* TextFile properties (accessed as member, but return value directly) */
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "path"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for TextFile path property");
        return string_type;
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "name"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for TextFile name property");
        return string_type;
    }
    else if (object_type->kind == TYPE_TEXT_FILE && token_equals(expr->as.member.member_name, "size"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning int type for TextFile size property");
        return int_type;
    }
    /* BinaryFile instance reading methods */
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "readByte"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile readByte method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "readBytes"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile readBytes method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "readAll"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile readAll method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "readInto"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {byte_array_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile readInto method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }
    /* BinaryFile instance writing methods */
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "writeByte"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile writeByte method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "writeBytes"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[1] = {byte_array_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile writeBytes method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    /* BinaryFile state query methods */
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "hasBytes"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile hasBytes method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "isEof"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile isEof method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }
    /* BinaryFile position manipulation methods */
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "position"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile position method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "seek"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile seek method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "rewind"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile rewind method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "flush"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile flush method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "close"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile close method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }
    /* BinaryFile properties */
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "path"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for BinaryFile path property");
        return string_type;
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "name"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for BinaryFile name property");
        return string_type;
    }
    else if (object_type->kind == TYPE_BINARY_FILE && token_equals(expr->as.member.member_name, "size"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning int type for BinaryFile size property");
        return int_type;
    }
    /* Time epoch getter methods */
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "millis"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time millis method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "seconds"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time seconds method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    /* Time date component getter methods */
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "year"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time year method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "month"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time month method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "day"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time day method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    /* Time time component getter methods */
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "hour"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time hour method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "minute"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time minute method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "second"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time second method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "weekday"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time weekday method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }
    /* Time formatting methods */
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "format"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning function type for Time format method");
        return ast_create_function_type(table->arena, str_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "toIso"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toIso method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "toDate"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toDate method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "toTime"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toTime method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }
    /* Time arithmetic methods */
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "add"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning function type for Time add method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "addSeconds"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning function type for Time addSeconds method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "addMinutes"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning function type for Time addMinutes method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "addHours"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning function type for Time addHours method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "addDays"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning function type for Time addDays method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "diff"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_TIME);
        DEBUG_VERBOSE("Returning function type for Time diff method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }
    /* Time comparison methods */
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "isBefore"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_TIME);
        DEBUG_VERBOSE("Returning function type for Time isBefore method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "isAfter"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_TIME);
        DEBUG_VERBOSE("Returning function type for Time isAfter method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }
    else if (object_type->kind == TYPE_TIME && token_equals(expr->as.member.member_name, "equals"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type **param_types = arena_alloc(table->arena, sizeof(Type *) * 1);
        param_types[0] = ast_create_primitive_type(table->arena, TYPE_TIME);
        DEBUG_VERBOSE("Returning function type for Time equals method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }
    else
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

static bool token_equals(Token tok, const char *str)
{
    size_t len = strlen(str);
    return tok.length == (int)len && strncmp(tok.start, str, len) == 0;
}

static Type *type_check_static_call(Expr *expr, SymbolTable *table)
{
    StaticCallExpr *call = &expr->as.static_call;
    Token type_name = call->type_name;
    Token method_name = call->method_name;

    /* Type check all arguments first */
    for (int i = 0; i < call->arg_count; i++)
    {
        Type *arg_type = type_check_expr(call->arguments[i], table);
        if (arg_type == NULL)
        {
            return NULL;
        }
    }

    /* TextFile static methods */
    if (token_equals(type_name, "TextFile"))
    {
        if (token_equals(method_name, "open"))
        {
            /* TextFile.open(path: str): TextFile */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TextFile.open requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.open requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TEXT_FILE);
        }
        else if (token_equals(method_name, "exists"))
        {
            /* TextFile.exists(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TextFile.exists requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.exists requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "readAll"))
        {
            /* TextFile.readAll(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TextFile.readAll requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.readAll requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "writeAll"))
        {
            /* TextFile.writeAll(path: str, content: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "TextFile.writeAll requires exactly 2 arguments (path, content)");
                return NULL;
            }
            Type *path_type = call->arguments[0]->expr_type;
            Type *content_type = call->arguments[1]->expr_type;
            if (path_type == NULL || path_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.writeAll first argument must be a string path");
                return NULL;
            }
            if (content_type == NULL || content_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.writeAll second argument must be a string content");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "delete"))
        {
            /* TextFile.delete(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TextFile.delete requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.delete requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "copy"))
        {
            /* TextFile.copy(src: str, dst: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "TextFile.copy requires exactly 2 arguments (src, dst)");
                return NULL;
            }
            Type *src_type = call->arguments[0]->expr_type;
            Type *dst_type = call->arguments[1]->expr_type;
            if (src_type == NULL || src_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.copy first argument must be a string source path");
                return NULL;
            }
            if (dst_type == NULL || dst_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.copy second argument must be a string destination path");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "move"))
        {
            /* TextFile.move(src: str, dst: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "TextFile.move requires exactly 2 arguments (src, dst)");
                return NULL;
            }
            Type *src_type = call->arguments[0]->expr_type;
            Type *dst_type = call->arguments[1]->expr_type;
            if (src_type == NULL || src_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.move first argument must be a string source path");
                return NULL;
            }
            if (dst_type == NULL || dst_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.move second argument must be a string destination path");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown TextFile static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* BinaryFile static methods */
    if (token_equals(type_name, "BinaryFile"))
    {
        if (token_equals(method_name, "open"))
        {
            /* BinaryFile.open(path: str): BinaryFile */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "BinaryFile.open requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.open requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BINARY_FILE);
        }
        else if (token_equals(method_name, "exists"))
        {
            /* BinaryFile.exists(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "BinaryFile.exists requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.exists requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "readAll"))
        {
            /* BinaryFile.readAll(path: str): byte[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "BinaryFile.readAll requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.readAll requires a string argument");
                return NULL;
            }
            Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
            return ast_create_array_type(table->arena, byte_type);
        }
        else if (token_equals(method_name, "writeAll"))
        {
            /* BinaryFile.writeAll(path: str, data: byte[]): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "BinaryFile.writeAll requires exactly 2 arguments (path, data)");
                return NULL;
            }
            Type *path_type = call->arguments[0]->expr_type;
            Type *data_type = call->arguments[1]->expr_type;
            if (path_type == NULL || path_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.writeAll first argument must be a string path");
                return NULL;
            }
            if (data_type == NULL || data_type->kind != TYPE_ARRAY ||
                data_type->as.array.element_type->kind != TYPE_BYTE)
            {
                type_error(&method_name, "BinaryFile.writeAll second argument must be a byte array");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "delete"))
        {
            /* BinaryFile.delete(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "BinaryFile.delete requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.delete requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "copy"))
        {
            /* BinaryFile.copy(src: str, dst: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "BinaryFile.copy requires exactly 2 arguments (src, dst)");
                return NULL;
            }
            Type *src_type = call->arguments[0]->expr_type;
            Type *dst_type = call->arguments[1]->expr_type;
            if (src_type == NULL || src_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.copy first argument must be a string source path");
                return NULL;
            }
            if (dst_type == NULL || dst_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.copy second argument must be a string destination path");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "move"))
        {
            /* BinaryFile.move(src: str, dst: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "BinaryFile.move requires exactly 2 arguments (src, dst)");
                return NULL;
            }
            Type *src_type = call->arguments[0]->expr_type;
            Type *dst_type = call->arguments[1]->expr_type;
            if (src_type == NULL || src_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.move first argument must be a string source path");
                return NULL;
            }
            if (dst_type == NULL || dst_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.move second argument must be a string destination path");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown BinaryFile static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Time static methods */
    if (token_equals(type_name, "Time"))
    {
        if (token_equals(method_name, "now"))
        {
            /* Time.now(): Time */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Time.now takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TIME);
        }
        else if (token_equals(method_name, "utc"))
        {
            /* Time.utc(): Time */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Time.utc takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TIME);
        }
        else if (token_equals(method_name, "fromMillis"))
        {
            /* Time.fromMillis(ms: int): Time */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Time.fromMillis requires exactly 1 argument (ms)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Time.fromMillis requires an int argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TIME);
        }
        else if (token_equals(method_name, "fromSeconds"))
        {
            /* Time.fromSeconds(s: int): Time */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Time.fromSeconds requires exactly 1 argument (s)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Time.fromSeconds requires an int argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TIME);
        }
        else if (token_equals(method_name, "sleep"))
        {
            /* Time.sleep(ms: int): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Time.sleep requires exactly 1 argument (ms)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Time.sleep requires an int argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Time static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Stdin static methods - console input */
    if (token_equals(type_name, "Stdin"))
    {
        if (token_equals(method_name, "readLine"))
        {
            /* Stdin.readLine(): str */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readLine takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "readChar"))
        {
            /* Stdin.readChar(): int */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readChar takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_INT);
        }
        else if (token_equals(method_name, "readWord"))
        {
            /* Stdin.readWord(): str */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readWord takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "hasChars"))
        {
            /* Stdin.hasChars(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.hasChars takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "hasLines"))
        {
            /* Stdin.hasLines(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.hasLines takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "isEof"))
        {
            /* Stdin.isEof(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.isEof takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stdin method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Stdout static methods - console output */
    if (token_equals(type_name, "Stdout"))
    {
        if (token_equals(method_name, "write"))
        {
            /* Stdout.write(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stdout.write requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stdout.write requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "writeLine"))
        {
            /* Stdout.writeLine(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stdout.writeLine requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stdout.writeLine requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "flush"))
        {
            /* Stdout.flush(): void */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdout.flush takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stdout method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Stderr static methods - error output */
    if (token_equals(type_name, "Stderr"))
    {
        if (token_equals(method_name, "write"))
        {
            /* Stderr.write(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stderr.write requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stderr.write requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "writeLine"))
        {
            /* Stderr.writeLine(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stderr.writeLine requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stderr.writeLine requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "flush"))
        {
            /* Stderr.flush(): void */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stderr.flush takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stderr method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Bytes static methods - byte array decoding utilities */
    if (token_equals(type_name, "Bytes"))
    {
        if (token_equals(method_name, "fromHex"))
        {
            /* Bytes.fromHex(hex: str): byte[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Bytes.fromHex requires exactly 1 argument (hex string)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Bytes.fromHex requires a string argument");
                return NULL;
            }
            Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
            return ast_create_array_type(table->arena, byte_type);
        }
        else if (token_equals(method_name, "fromBase64"))
        {
            /* Bytes.fromBase64(b64: str): byte[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Bytes.fromBase64 requires exactly 1 argument (Base64 string)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Bytes.fromBase64 requires a string argument");
                return NULL;
            }
            Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
            return ast_create_array_type(table->arena, byte_type);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Bytes static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Path static methods - path manipulation utilities */
    if (token_equals(type_name, "Path"))
    {
        if (token_equals(method_name, "directory"))
        {
            /* Path.directory(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.directory requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.directory requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "filename"))
        {
            /* Path.filename(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.filename requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.filename requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "extension"))
        {
            /* Path.extension(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.extension requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.extension requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "join"))
        {
            /* Path.join(paths...: str): str - variable arguments, at least 2 */
            if (call->arg_count < 2)
            {
                type_error(&method_name, "Path.join requires at least 2 arguments");
                return NULL;
            }
            for (int i = 0; i < call->arg_count; i++)
            {
                Type *arg_type = call->arguments[i]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Path.join requires all arguments to be strings");
                    return NULL;
                }
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "absolute"))
        {
            /* Path.absolute(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.absolute requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.absolute requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "exists"))
        {
            /* Path.exists(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.exists requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.exists requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "isFile"))
        {
            /* Path.isFile(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.isFile requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.isFile requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "isDirectory"))
        {
            /* Path.isDirectory(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.isDirectory requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.isDirectory requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Path static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Directory static methods - directory operations */
    if (token_equals(type_name, "Directory"))
    {
        if (token_equals(method_name, "list"))
        {
            /* Directory.list(path: str): str[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.list requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.list requires a string argument");
                return NULL;
            }
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            return ast_create_array_type(table->arena, string_type);
        }
        else if (token_equals(method_name, "listRecursive"))
        {
            /* Directory.listRecursive(path: str): str[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.listRecursive requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.listRecursive requires a string argument");
                return NULL;
            }
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            return ast_create_array_type(table->arena, string_type);
        }
        else if (token_equals(method_name, "create"))
        {
            /* Directory.create(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.create requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.create requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "delete"))
        {
            /* Directory.delete(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.delete requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.delete requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "deleteRecursive"))
        {
            /* Directory.deleteRecursive(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.deleteRecursive requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.deleteRecursive requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Directory static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    type_error(&type_name, "Unknown static type");
    return NULL;
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
    case EXPR_STATIC_CALL:
        t = type_check_static_call(expr, table);
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
