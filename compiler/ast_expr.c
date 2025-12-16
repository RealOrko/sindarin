#include "ast_expr.h"
#include "ast_type.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

Expr *ast_create_comparison_expr(Arena *arena, Expr *left, Expr *right, TokenType comparison_type, const Token *loc_token)
{
    if (left == NULL || right == NULL)
    {
        DEBUG_ERROR("Cannot create comparison with NULL expressions");
        return NULL;
    }

    return ast_create_binary_expr(arena, left, comparison_type, right, loc_token);
}

Expr *ast_create_binary_expr(Arena *arena, Expr *left, TokenType operator, Expr *right, const Token *loc_token)
{
    if (left == NULL || right == NULL)
    {
        return NULL;
    }

    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator = operator;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_unary_expr(Arena *arena, TokenType operator, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        return NULL;
    }

    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->as.unary.operator = operator;
    expr->as.unary.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_literal_expr(Arena *arena, LiteralValue value, Type *type, bool is_interpolated, const Token *loc_token)
{
    if (type == NULL)
    {
        return NULL;
    }

    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.value = value;
    expr->as.literal.type = type;
    expr->as.literal.is_interpolated = is_interpolated;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_variable_expr(Arena *arena, Token name, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_VARIABLE;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.variable.name.start = new_start;
    expr->as.variable.name.length = name.length;
    expr->as.variable.name.line = name.line;
    expr->as.variable.name.type = name.type;
    expr->as.variable.name.filename = name.filename;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_assign_expr(Arena *arena, Token name, Expr *value, const Token *loc_token)
{
    if (value == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ASSIGN;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.assign.name.start = new_start;
    expr->as.assign.name.length = name.length;
    expr->as.assign.name.line = name.line;
    expr->as.assign.name.type = name.type;
    expr->as.assign.name.filename = name.filename;
    expr->as.assign.value = value;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_call_expr(Arena *arena, Expr *callee, Expr **arguments, int arg_count, const Token *loc_token)
{
    if (callee == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_CALL;
    expr->as.call.callee = callee;
    expr->as.call.arguments = arguments;
    expr->as.call.arg_count = arg_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ARRAY;
    expr->as.array.elements = elements;
    expr->as.array.element_count = element_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_access_expr(Arena *arena, Expr *array, Expr *index, const Token *loc_token)
{
    if (array == NULL || index == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ARRAY_ACCESS;
    expr->as.array_access.array = array;
    expr->as.array_access.index = index;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_increment_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_INCREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_decrement_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_DECREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_interpolated_expr(Arena *arena, Expr **parts, int part_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_INTERPOLATED;
    expr->as.interpol.parts = parts;
    expr->as.interpol.part_count = part_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_member_expr(Arena *arena, Expr *object, Token member_name, const Token *loc_token)
{
    if (object == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_MEMBER;
    expr->as.member.object = object;
    char *new_start = arena_strndup(arena, member_name.start, member_name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.member.member_name.start = new_start;
    expr->as.member.member_name.length = member_name.length;
    expr->as.member.member_name.line = member_name.line;
    expr->as.member.member_name.type = member_name.type;
    expr->as.member.member_name.filename = member_name.filename;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}
