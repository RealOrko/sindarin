#include "type_checker_expr.h"
#include "type_checker_util.h"
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
        if (!ast_type_equals(left, right))
        {
            type_error(expr->token, "Type mismatch in comparison");
            return NULL;
        }
        DEBUG_VERBOSE("Returning BOOL type for comparison operator");
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
    else if (is_arithmetic_operator(op))
    {
        if (!ast_type_equals(left, right) || !is_numeric_type(left))
        {
            type_error(expr->token, "Invalid types for arithmetic operator");
            return NULL;
        }
        DEBUG_VERBOSE("Returning left operand type for arithmetic operator");
        return left;
    }
    else if (op == TOKEN_PLUS)
    {
        if (is_numeric_type(left) && ast_type_equals(left, right))
        {
            DEBUG_VERBOSE("Returning left operand type for numeric + operator");
            return left;
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
    Type *value_type = type_check_expr(expr->as.assign.value, table);
    if (value_type == NULL)
    {
        type_error(expr->token, "Invalid value in assignment");
        return NULL;
    }
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.assign.name);
    if (sym == NULL)
    {
        type_error(&expr->as.assign.name, "Undefined variable for assignment");
        return NULL;
    }
    if (!ast_type_equals(sym->type, value_type))
    {
        type_error(&expr->as.assign.name, "Type mismatch in assignment");
        return NULL;
    }
    DEBUG_VERBOSE("Assignment type matches: %d", sym->type->kind);
    return sym->type;
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

    // len(arr) -> int
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

    // pop(arr) -> element type
    if (is_builtin_name(callee, "pop") && expr->as.call.arg_count == 1)
    {
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL) return NULL;
        if (arg_type->kind != TYPE_ARRAY)
        {
            type_error(expr->token, "pop() requires array argument");
            return NULL;
        }
        return arg_type->as.array.element_type;
    }

    // rev(arr) -> same array type
    if (is_builtin_name(callee, "rev") && expr->as.call.arg_count == 1)
    {
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL) return NULL;
        if (arg_type->kind != TYPE_ARRAY)
        {
            type_error(expr->token, "rev() requires array argument");
            return NULL;
        }
        return arg_type;
    }

    // push(elem, arr) -> same array type
    if (is_builtin_name(callee, "push") && expr->as.call.arg_count == 2)
    {
        Type *elem_type = type_check_expr(expr->as.call.arguments[0], table);
        Type *arr_type = type_check_expr(expr->as.call.arguments[1], table);
        if (elem_type == NULL || arr_type == NULL) return NULL;
        if (arr_type->kind != TYPE_ARRAY)
        {
            type_error(expr->token, "push() second argument must be array");
            return NULL;
        }
        if (!ast_type_equals(elem_type, arr_type->as.array.element_type))
        {
            type_error(expr->token, "push() element type must match array element type");
            return NULL;
        }
        return arr_type;
    }

    // rem(index, arr) -> same array type
    if (is_builtin_name(callee, "rem") && expr->as.call.arg_count == 2)
    {
        Type *idx_type = type_check_expr(expr->as.call.arguments[0], table);
        Type *arr_type = type_check_expr(expr->as.call.arguments[1], table);
        if (idx_type == NULL || arr_type == NULL) return NULL;
        if (!is_numeric_type(idx_type))
        {
            type_error(expr->token, "rem() index must be numeric");
            return NULL;
        }
        if (arr_type->kind != TYPE_ARRAY)
        {
            type_error(expr->token, "rem() second argument must be array");
            return NULL;
        }
        return arr_type;
    }

    // ins(elem, index, arr) -> same array type
    if (is_builtin_name(callee, "ins") && expr->as.call.arg_count == 3)
    {
        Type *elem_type = type_check_expr(expr->as.call.arguments[0], table);
        Type *idx_type = type_check_expr(expr->as.call.arguments[1], table);
        Type *arr_type = type_check_expr(expr->as.call.arguments[2], table);
        if (elem_type == NULL || idx_type == NULL || arr_type == NULL) return NULL;
        if (!is_numeric_type(idx_type))
        {
            type_error(expr->token, "ins() index must be numeric");
            return NULL;
        }
        if (arr_type->kind != TYPE_ARRAY)
        {
            type_error(expr->token, "ins() third argument must be array");
            return NULL;
        }
        if (!ast_type_equals(elem_type, arr_type->as.array.element_type))
        {
            type_error(expr->token, "ins() element type must match array element type");
            return NULL;
        }
        return arr_type;
    }

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
        Type *arg_type = type_check_expr(expr->as.call.arguments[i], table);
        if (arg_type == NULL)
        {
            type_error(expr->token, "Invalid argument in function call");
            return NULL;
        }
        Type *param_type = callee_type->as.function.param_types[i];
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
        Type *et = type_check_expr(expr->as.array.elements[i], table);
        if (et == NULL)
        {
            valid = false;
            continue;
        }
        if (elem_type == NULL)
        {
            elem_type = et;
            DEBUG_VERBOSE("First array element type: %d", elem_type->kind);
        }
        else
        {
            bool equal = false;
            if (elem_type->kind == et->kind)
            {
                if (elem_type->kind == TYPE_ARRAY || elem_type->kind == TYPE_FUNCTION)
                {
                    equal = ast_type_equals(elem_type, et);
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
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[1] = {int_type};
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
    else
    {
        type_error(expr->token, "Invalid member access");
        return NULL;
    }
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
