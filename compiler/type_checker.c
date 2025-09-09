#include "type_checker.h"
#include "debug.h"
#include "lexer.h"
#include "parser.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static int had_type_error = 0;

static void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type);

static Type *type_check_expr(Expr *expr, SymbolTable *table);

static void type_error(Token *token, const char *msg)
{
    char error_buffer[256];
    if (token && token->line > 0 && token->filename)
    {
        snprintf(error_buffer, sizeof(error_buffer), "%s:%d: Type error: %s",
                 token->filename, token->line, msg);
    }
    else
    {
        snprintf(error_buffer, sizeof(error_buffer), "Type error: %s", msg);
    }
    DEBUG_ERROR("%s", error_buffer);
    DEBUG_VERBOSE("Type error occurred: %s", error_buffer);
    had_type_error = 1;
}

static bool is_numeric_type(Type *type)
{
    bool result = type && (type->kind == TYPE_INT || type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE);
    DEBUG_VERBOSE("Checking if type is numeric: %s", result ? "true" : "false");
    return result;
}

static bool is_comparison_operator(TokenType op)
{
    bool result = op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL || op == TOKEN_LESS ||
                  op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL;
    DEBUG_VERBOSE("Checking if operator is comparison: %s (op: %d)", result ? "true" : "false", op);
    return result;
}

static bool is_arithmetic_operator(TokenType op)
{
    bool result = op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_MODULO;
    DEBUG_VERBOSE("Checking if operator is arithmetic: %s (op: %d)", result ? "true" : "false", op);
    return result;
}

static bool is_printable_type(Type *type)
{
    bool result = type && (type->kind == TYPE_INT || type->kind == TYPE_LONG ||
                           type->kind == TYPE_DOUBLE || type->kind == TYPE_CHAR ||
                           type->kind == TYPE_STRING || type->kind == TYPE_BOOL ||
                           type->kind == TYPE_ARRAY);
    DEBUG_VERBOSE("Checking if type is printable: %s", result ? "true" : "false");
    return result;
}

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

static Type *type_check_call(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function call with %d arguments", expr->as.call.arg_count);
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

static Type *type_check_expr(Expr *expr, SymbolTable *table)
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
    {
        DEBUG_VERBOSE("Type checking array with %d elements", expr->as.array.element_count);
        if (expr->as.array.element_count == 0)
        {
            t = ast_create_array_type(table->arena, ast_create_primitive_type(table->arena, TYPE_NIL));
            DEBUG_VERBOSE("Empty array, returning NIL element type");
        }
        else
        {
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
                        t = NULL;
                        break;
                    }
                }
            }
            if (valid && elem_type != NULL)
            {
                t = ast_create_array_type(table->arena, elem_type);
                DEBUG_VERBOSE("Returning array type with element type: %d", elem_type->kind);
            }
            else
            {
                t = NULL;
            }
        }
        break;
    }
    case EXPR_ARRAY_ACCESS:
    {
        DEBUG_VERBOSE("Type checking array access");
        Type *array_t = type_check_expr(expr->as.array_access.array, table);
        if (array_t == NULL)
        {
            t = NULL;
            break;
        }
        if (array_t->kind != TYPE_ARRAY)
        {
            type_error(expr->token, "Cannot access non-array");
            t = NULL;
            break;
        }
        Type *index_t = type_check_expr(expr->as.array_access.index, table);
        if (index_t == NULL)
        {
            t = NULL;
            break;
        }
        if (!is_numeric_type(index_t))
        {
            type_error(expr->token, "Array index must be numeric type");
            t = NULL;
            break;
        }
        t = array_t->as.array.element_type;
        DEBUG_VERBOSE("Returning array element type: %d", t->kind);
        break;
    }
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
    {
        DEBUG_VERBOSE("Type checking %s expression", expr->type == EXPR_INCREMENT ? "increment" : "decrement");
        Type *operand_type = type_check_expr(expr->as.operand, table);
        t = operand_type;
        if (operand_type == NULL || !is_numeric_type(operand_type))
        {
            type_error(expr->token, "Increment/decrement on non-numeric type");
            t = NULL;
        }
        break;
    }
    case EXPR_INTERPOLATED:
        t = type_check_interpolated(expr, table);
        break;
    case EXPR_MEMBER:
    {
        DEBUG_VERBOSE("Type checking member access: %s", expr->as.member.member_name.start);
        Type *object_type = type_check_expr(expr->as.member.object, table);
        if (object_type == NULL)
        {
            t = NULL;
            break;
        }
        if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "length") == 0)
        {
            t = ast_create_primitive_type(table->arena, TYPE_INT);
            DEBUG_VERBOSE("Returning INT type for array length access");
        }
        else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "push") == 0)
        {
            Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
            Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
            Type *param_types[1] = {int_type};
            t = ast_create_function_type(table->arena, void_type, param_types, 1);
            DEBUG_VERBOSE("Returning function type for array push method");
        }
        else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "pop") == 0)
        {
            Type *param_types[] = {NULL};
            t = ast_create_function_type(table->arena, object_type->as.array.element_type, param_types, 0);
            DEBUG_VERBOSE("Returning function type for array pop method");
        }
        else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "clear") == 0)
        {
            Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
            Type *param_types[] = {NULL};
            t = ast_create_function_type(table->arena, void_type, param_types, 0);
            DEBUG_VERBOSE("Returning function type for array clear method");
        }
        else if (object_type->kind == TYPE_ARRAY && strcmp(expr->as.member.member_name.start, "concat") == 0)
        {
            Type *element_type = object_type->as.array.element_type;
            Type *param_array_type = ast_create_array_type(table->arena, element_type);
            Type *param_types[1] = {param_array_type};
            t = ast_create_function_type(table->arena, object_type, param_types, 1); // Return same array type as object
            DEBUG_VERBOSE("Returning function type for array concat method");
        }
        else
        {
            type_error(expr->token, "Invalid member access");
            t = NULL;
        }
        break;
    }
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

static void type_check_var_decl(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    (void)return_type;
    DEBUG_VERBOSE("Type checking variable declaration: %.*s", stmt->as.var_decl.name.length, stmt->as.var_decl.name.start);
    Type *decl_type = stmt->as.var_decl.type;
    Type *init_type = NULL;
    if (stmt->as.var_decl.initializer)
    {
        init_type = type_check_expr(stmt->as.var_decl.initializer, table);
        if (init_type == NULL)
        {
            symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, decl_type, SYMBOL_LOCAL);
            return;
        }
    }
    symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, decl_type, SYMBOL_LOCAL);
    if (init_type && !ast_type_equals(init_type, decl_type))
    {
        type_error(&stmt->as.var_decl.name, "Initializer type does not match variable type");
    }
}

static void type_check_function(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function with %d parameters", stmt->as.function.param_count);

    // Create function type from declaration
    Arena *arena = table->arena;
    Type **param_types = (Type **)arena_alloc(arena, sizeof(Type *) * stmt->as.function.param_count);
    for (int i = 0; i < stmt->as.function.param_count; i++) {
        param_types[i] = stmt->as.function.params[i].type;
    }
    Type *func_type = ast_create_function_type(arena, stmt->as.function.return_type, param_types, stmt->as.function.param_count);

    // Add function symbol to current scope (e.g., global)
    symbol_table_add_symbol_with_kind(table, stmt->as.function.name, func_type, SYMBOL_LOCAL);

    symbol_table_push_scope(table);

    for (int i = 0; i < stmt->as.function.param_count; i++)
    {
        Parameter param = stmt->as.function.params[i];
        DEBUG_VERBOSE("Adding parameter %d: %.*s", i, param.name.length, param.name.start);
        symbol_table_add_symbol_with_kind(table, param.name, param.type, SYMBOL_PARAM);
    }

    table->current->next_local_offset = table->current->next_param_offset;

    for (int i = 0; i < stmt->as.function.body_count; i++)
    {
        type_check_stmt(stmt->as.function.body[i], table, stmt->as.function.return_type);
    }
    symbol_table_pop_scope(table);
}

static void type_check_return(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking return statement");
    Type *value_type;
    if (stmt->as.return_stmt.value)
    {
        value_type = type_check_expr(stmt->as.return_stmt.value, table);
        if (value_type == NULL)
            return;
    }
    else
    {
        value_type = ast_create_primitive_type(table->arena, TYPE_VOID);
    }
    if (!ast_type_equals(value_type, return_type))
    {
        type_error(stmt->token, "Return type does not match function return type");
    }
}

static void type_check_block(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking block with %d statements", stmt->as.block.count);
    symbol_table_push_scope(table);
    for (int i = 0; i < stmt->as.block.count; i++)
    {
        type_check_stmt(stmt->as.block.statements[i], table, return_type);
    }
    symbol_table_pop_scope(table);
}

static void type_check_if(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking if statement");
    Type *cond_type = type_check_expr(stmt->as.if_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.if_stmt.condition->token, "If condition must be boolean");
    }
    type_check_stmt(stmt->as.if_stmt.then_branch, table, return_type);
    if (stmt->as.if_stmt.else_branch)
    {
        DEBUG_VERBOSE("Type checking else branch");
        type_check_stmt(stmt->as.if_stmt.else_branch, table, return_type);
    }
}

static void type_check_while(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking while statement");
    Type *cond_type = type_check_expr(stmt->as.while_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.while_stmt.condition->token, "While condition must be boolean");
    }
    type_check_stmt(stmt->as.while_stmt.body, table, return_type);
}

static void type_check_for(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for statement");
    symbol_table_push_scope(table);
    if (stmt->as.for_stmt.initializer)
    {
        type_check_stmt(stmt->as.for_stmt.initializer, table, return_type);
    }
    if (stmt->as.for_stmt.condition)
    {
        Type *cond_type = type_check_expr(stmt->as.for_stmt.condition, table);
        if (cond_type && cond_type->kind != TYPE_BOOL)
        {
            type_error(stmt->as.for_stmt.condition->token, "For condition must be boolean");
        }
    }
    if (stmt->as.for_stmt.increment)
    {
        type_check_expr(stmt->as.for_stmt.increment, table);
    }
    type_check_stmt(stmt->as.for_stmt.body, table, return_type);
    symbol_table_pop_scope(table);
}

static void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    if (stmt == NULL)
    {
        DEBUG_VERBOSE("Statement is NULL");
        return;
    }
    DEBUG_VERBOSE("Type checking statement type: %d", stmt->type);
    switch (stmt->type)
    {
    case STMT_EXPR:
        type_check_expr(stmt->as.expression.expression, table);
        break;
    case STMT_VAR_DECL:
        type_check_var_decl(stmt, table, return_type);
        break;
    case STMT_FUNCTION:
        type_check_function(stmt, table);
        break;
    case STMT_RETURN:
        type_check_return(stmt, table, return_type);
        break;
    case STMT_BLOCK:
        type_check_block(stmt, table, return_type);
        break;
    case STMT_IF:
        type_check_if(stmt, table, return_type);
        break;
    case STMT_WHILE:
        type_check_while(stmt, table, return_type);
        break;
    case STMT_FOR:
        type_check_for(stmt, table, return_type);
        break;
    case STMT_IMPORT:
        DEBUG_VERBOSE("Skipping type check for import statement");
        break;
    }
}

int type_check_module(Module *module, SymbolTable *table)
{
    DEBUG_VERBOSE("Starting type checking for module with %d statements", module->count);
    had_type_error = 0;
    for (int i = 0; i < module->count; i++)
    {
        type_check_stmt(module->statements[i], table, NULL);
    }
    DEBUG_VERBOSE("Type checking completed, had_type_error: %d", had_type_error);
    return !had_type_error;
}