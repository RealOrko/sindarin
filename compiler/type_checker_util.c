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

bool is_primitive_type(Type *type)
{
    if (type == NULL) return false;
    bool result = type->kind == TYPE_INT ||
                  type->kind == TYPE_LONG ||
                  type->kind == TYPE_DOUBLE ||
                  type->kind == TYPE_CHAR ||
                  type->kind == TYPE_BOOL ||
                  type->kind == TYPE_VOID;
    DEBUG_VERBOSE("Checking if type is primitive: %s", result ? "true" : "false");
    return result;
}

bool is_reference_type(Type *type)
{
    if (type == NULL) return false;
    bool result = type->kind == TYPE_STRING ||
                  type->kind == TYPE_ARRAY ||
                  type->kind == TYPE_FUNCTION;
    DEBUG_VERBOSE("Checking if type is reference: %s", result ? "true" : "false");
    return result;
}

bool can_escape_private(Type *type)
{
    /* Only primitive types can escape from private blocks/functions */
    return is_primitive_type(type);
}

void memory_context_init(MemoryContext *ctx)
{
    ctx->in_private_block = false;
    ctx->in_private_function = false;
    ctx->private_depth = 0;
}

void memory_context_enter_private(MemoryContext *ctx)
{
    ctx->in_private_block = true;
    ctx->private_depth++;
}

void memory_context_exit_private(MemoryContext *ctx)
{
    ctx->private_depth--;
    if (ctx->private_depth <= 0)
    {
        ctx->in_private_block = false;
        ctx->private_depth = 0;
    }
}

bool memory_context_is_private(MemoryContext *ctx)
{
    return ctx->in_private_block || ctx->in_private_function;
}

bool can_promote_numeric(Type *from, Type *to)
{
    if (from == NULL || to == NULL) return false;
    /* int can promote to double or long */
    if (from->kind == TYPE_INT && (to->kind == TYPE_DOUBLE || to->kind == TYPE_LONG))
        return true;
    /* long can promote to double */
    if (from->kind == TYPE_LONG && to->kind == TYPE_DOUBLE)
        return true;
    return false;
}

Type *get_promoted_type(Arena *arena, Type *left, Type *right)
{
    if (left == NULL || right == NULL) return NULL;

    /* If both are the same type, return it */
    if (ast_type_equals(left, right))
        return left;

    /* Check for numeric type promotion */
    if (is_numeric_type(left) && is_numeric_type(right))
    {
        /* double is the widest numeric type */
        if (left->kind == TYPE_DOUBLE || right->kind == TYPE_DOUBLE)
            return ast_create_primitive_type(arena, TYPE_DOUBLE);
        /* long is wider than int */
        if (left->kind == TYPE_LONG || right->kind == TYPE_LONG)
            return ast_create_primitive_type(arena, TYPE_LONG);
        /* both are int */
        return left;
    }

    /* No valid promotion */
    return NULL;
}
