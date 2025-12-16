#include "type_checker_util.h"
#include "debug.h"
#include <stdio.h>

static int had_type_error = 0;

void type_checker_reset_error(void)
{
    had_type_error = 0;
}

int type_checker_had_error(void)
{
    return had_type_error;
}

void type_checker_set_error(void)
{
    had_type_error = 1;
}

const char *type_name(Type *type)
{
    if (type == NULL) return "unknown";
    switch (type->kind) {
        case TYPE_INT:      return "int";
        case TYPE_LONG:     return "long";
        case TYPE_DOUBLE:   return "double";
        case TYPE_CHAR:     return "char";
        case TYPE_STRING:   return "str";
        case TYPE_BOOL:     return "bool";
        case TYPE_VOID:     return "void";
        case TYPE_NIL:      return "nil";
        case TYPE_ANY:      return "any";
        case TYPE_ARRAY:    return "array";
        case TYPE_FUNCTION: return "function";
        default:            return "unknown";
    }
}

void type_error(Token *token, const char *msg)
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

void type_mismatch_error(Token *token, Type *expected, Type *actual, const char *context)
{
    char error_buffer[512];
    if (token && token->line > 0 && token->filename)
    {
        snprintf(error_buffer, sizeof(error_buffer),
                 "%s:%d: Type error in %s: expected '%s', got '%s'",
                 token->filename, token->line, context,
                 type_name(expected), type_name(actual));
    }
    else
    {
        snprintf(error_buffer, sizeof(error_buffer),
                 "Type error in %s: expected '%s', got '%s'",
                 context, type_name(expected), type_name(actual));
    }
    DEBUG_ERROR("%s", error_buffer);
    had_type_error = 1;
}

bool is_numeric_type(Type *type)
{
    bool result = type && (type->kind == TYPE_INT || type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE);
    DEBUG_VERBOSE("Checking if type is numeric: %s", result ? "true" : "false");
    return result;
}

bool is_comparison_operator(TokenType op)
{
    bool result = op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL || op == TOKEN_LESS ||
                  op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL;
    DEBUG_VERBOSE("Checking if operator is comparison: %s (op: %d)", result ? "true" : "false", op);
    return result;
}

bool is_arithmetic_operator(TokenType op)
{
    bool result = op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_MODULO;
    DEBUG_VERBOSE("Checking if operator is arithmetic: %s (op: %d)", result ? "true" : "false", op);
    return result;
}

bool is_printable_type(Type *type)
{
    bool result = type && (type->kind == TYPE_INT || type->kind == TYPE_LONG ||
                           type->kind == TYPE_DOUBLE || type->kind == TYPE_CHAR ||
                           type->kind == TYPE_STRING || type->kind == TYPE_BOOL ||
                           type->kind == TYPE_ARRAY);
    DEBUG_VERBOSE("Checking if type is printable: %s", result ? "true" : "false");
    return result;
}
