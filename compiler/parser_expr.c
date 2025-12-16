#include "parser_expr.h"
#include "parser_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

Expr *parser_multi_line_expression(Parser *parser)
{
    Expr *expr = parser_expression(parser);

    while (parser_match(parser, TOKEN_NEWLINE))
    {
        Token op_token = parser->previous;
        Expr *right = parser_expression(parser);
        expr = ast_create_binary_expr(parser->arena, expr, TOKEN_PLUS, right, &op_token);
    }

    return expr;
}

Expr *parser_expression(Parser *parser)
{
    Expr *result = parser_assignment(parser);
    if (result == NULL)
    {
        parser_error_at_current(parser, "Expected expression");
        parser_advance(parser);
    }
    return result;
}

Expr *parser_assignment(Parser *parser)
{
    Expr *expr = parser_logical_or(parser);

    if (parser_match(parser, TOKEN_EQUAL))
    {
        Token equals = parser->previous;
        Expr *value = parser_assignment(parser);
        if (expr->type == EXPR_VARIABLE)
        {
            Token name = expr->as.variable.name;
            char *new_start = arena_strndup(parser->arena, name.start, name.length);
            if (new_start == NULL)
            {
                exit(1);
            }
            name.start = new_start;
            return ast_create_assign_expr(parser->arena, name, value, &equals);
        }
        parser_error(parser, "Invalid assignment target");
    }
    return expr;
}

Expr *parser_logical_or(Parser *parser)
{
    Expr *expr = parser_logical_and(parser);
    while (parser_match(parser, TOKEN_OR))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_logical_and(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_logical_and(Parser *parser)
{
    Expr *expr = parser_equality(parser);
    while (parser_match(parser, TOKEN_AND))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_equality(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_equality(Parser *parser)
{
    Expr *expr = parser_comparison(parser);
    while (parser_match(parser, TOKEN_BANG_EQUAL) || parser_match(parser, TOKEN_EQUAL_EQUAL))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_comparison(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_comparison(Parser *parser)
{
    Expr *expr = parser_term(parser);
    while (parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL) ||
           parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_term(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_term(Parser *parser)
{
    Expr *expr = parser_factor(parser);
    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_factor(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_factor(Parser *parser)
{
    Expr *expr = parser_unary(parser);
    while (parser_match(parser, TOKEN_STAR) || parser_match(parser, TOKEN_SLASH) || parser_match(parser, TOKEN_MODULO))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_unary(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_unary(Parser *parser)
{
    if (parser_match(parser, TOKEN_BANG) || parser_match(parser, TOKEN_MINUS))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_unary(parser);
        return ast_create_unary_expr(parser->arena, operator, right, &op);
    }
    return parser_postfix(parser);
}

Expr *parser_postfix(Parser *parser)
{
    Expr *expr = parser_primary(parser);
    for (;;)
    {
        if (parser_match(parser, TOKEN_LEFT_PAREN))
        {
            expr = parser_call(parser, expr);
        }
        else if (parser_match(parser, TOKEN_LEFT_BRACKET))
        {
            expr = parser_array_access(parser, expr);
        }
        else if (parser_match(parser, TOKEN_DOT))
        {
            Token dot = parser->previous;
            if (!parser_check(parser, TOKEN_IDENTIFIER))
            {
                parser_error_at_current(parser, "Expected identifier after '.'");
            }
            Token member_name = parser->current;
            parser_advance(parser);
            expr = ast_create_member_expr(parser->arena, expr, member_name, &dot);
        }
        else if (parser_match(parser, TOKEN_PLUS_PLUS))
        {
            expr = ast_create_increment_expr(parser->arena, expr, &parser->previous);
        }
        else if (parser_match(parser, TOKEN_MINUS_MINUS))
        {
            expr = ast_create_decrement_expr(parser->arena, expr, &parser->previous);
        }
        else
        {
            break;
        }
    }
    return expr;
}

Expr *parser_primary(Parser *parser)
{
    if (parser_match(parser, TOKEN_INT_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_INT), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_LONG_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_LONG), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_DOUBLE_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_DOUBLE), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_CHAR_LITERAL))
    {
        LiteralValue value;
        value.char_value = parser->previous.literal.char_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_CHAR), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        LiteralValue value;
        value.string_value = arena_strdup(parser->arena, parser->previous.literal.string_value);
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_BOOL_LITERAL))
    {
        LiteralValue value;
        value.bool_value = parser->previous.literal.bool_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_BOOL), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_NIL))
    {
        LiteralValue value;
        value.int_value = 0;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_NIL), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_IDENTIFIER))
    {
        Token var_token = parser->previous;
        var_token.start = arena_strndup(parser->arena, parser->previous.start, parser->previous.length);
        return ast_create_variable_expr(parser->arena, var_token, &parser->previous);
    }
    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        Expr *expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        Token left_brace = parser->previous;
        Expr **elements = NULL;
        int count = 0;
        int capacity = 0;

        if (!parser_check(parser, TOKEN_RIGHT_BRACE))
        {
            do
            {
                Expr *elem = parser_expression(parser);
                if (elem != NULL)
                {
                    if (count >= capacity)
                    {
                        capacity = capacity == 0 ? 8 : capacity * 2;
                        Expr **new_elements = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                        if (new_elements == NULL)
                        {
                            parser_error(parser, "Out of memory");
                            return NULL;
                        }
                        if (elements != NULL && count > 0)
                        {
                            memcpy(new_elements, elements, sizeof(Expr *) * count);
                        }
                        elements = new_elements;
                    }
                    elements[count++] = elem;
                }
            } while (parser_match(parser, TOKEN_COMMA));
        }

        parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after array elements");
        return ast_create_array_expr(parser->arena, elements, count, &left_brace);
    }

    if (parser_match(parser, TOKEN_INTERPOL_STRING))
    {
        Token interpol_token = parser->previous;
        const char *content = parser->previous.literal.string_value;
        Expr **parts = NULL;
        int capacity = 0;
        int count = 0;

        const char *p = content;
        const char *segment_start = p;

        while (*p)
        {
            if (*p == '{')
            {
                if (p > segment_start)
                {
                    int len = p - segment_start;
                    char *seg = arena_strndup(parser->arena, segment_start, len);
                    LiteralValue v;
                    v.string_value = seg;
                    Expr *seg_expr = ast_create_literal_expr(parser->arena, v, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &interpol_token);

                    if (count >= capacity)
                    {
                        capacity = capacity == 0 ? 8 : capacity * 2;
                        Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                        if (new_parts == NULL)
                        {
                            exit(1);
                        }
                        if (parts != NULL && count > 0)
                        {
                            memcpy(new_parts, parts, sizeof(Expr *) * count);
                        }
                        parts = new_parts;
                    }
                    parts[count++] = seg_expr;
                }

                p++;
                const char *expr_start = p;
                while (*p && *p != '}')
                    p++;
                if (!*p)
                {
                    parser_error_at_current(parser, "Unterminated interpolated expression");
                    LiteralValue zero = {0};
                    return ast_create_literal_expr(parser->arena, zero, ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
                }
                int expr_len = p - expr_start;
                char *expr_src = arena_strndup(parser->arena, expr_start, expr_len);

                Lexer sub_lexer;
                lexer_init(parser->arena, &sub_lexer, expr_src, "interpolated");
                Parser sub_parser;
                parser_init(parser->arena, &sub_parser, &sub_lexer, parser->symbol_table);
                sub_parser.symbol_table = parser->symbol_table;

                Expr *inner = parser_expression(&sub_parser);
                if (inner == NULL || sub_parser.had_error)
                {
                    parser_error_at_current(parser, "Invalid expression in interpolation");
                    LiteralValue zero = {0};
                    return ast_create_literal_expr(parser->arena, zero, ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
                }

                if (count >= capacity)
                {
                    capacity = capacity == 0 ? 8 : capacity * 2;
                    Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                    if (new_parts == NULL)
                    {
                        exit(1);
                    }
                    if (parts != NULL && count > 0)
                    {
                        memcpy(new_parts, parts, sizeof(Expr *) * count);
                    }
                    parts = new_parts;
                }
                parts[count++] = inner;

                if (parser->interp_count >= parser->interp_capacity)
                {
                    parser->interp_capacity = parser->interp_capacity ? parser->interp_capacity * 2 : 8;
                    char **new_interp_sources = arena_alloc(parser->arena, sizeof(char *) * parser->interp_capacity);
                    if (new_interp_sources == NULL)
                    {
                        exit(1);
                    }
                    if (parser->interp_sources != NULL && parser->interp_count > 0)
                    {
                        memcpy(new_interp_sources, parser->interp_sources, sizeof(char *) * parser->interp_count);
                    }
                    parser->interp_sources = new_interp_sources;
                }
                parser->interp_sources[parser->interp_count++] = expr_src;

                p++;
                segment_start = p;
            }
            else
            {
                p++;
            }
        }

        if (p > segment_start)
        {
            int len = p - segment_start;
            char *seg = arena_strndup(parser->arena, segment_start, len);
            LiteralValue v;
            v.string_value = seg;
            Expr *seg_expr = ast_create_literal_expr(parser->arena, v, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &interpol_token);

            if (count >= capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                if (new_parts == NULL)
                {
                    exit(1);
                }
                if (parts != NULL && count > 0)
                {
                    memcpy(new_parts, parts, sizeof(Expr *) * count);
                }
                parts = new_parts;
            }
            parts[count++] = seg_expr;
        }

        return ast_create_interpolated_expr(parser->arena, parts, count, &interpol_token);
    }

    parser_error_at_current(parser, "Expected expression");
    LiteralValue value;
    value.int_value = 0;
    return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_NIL), false, NULL);
}

Expr *parser_call(Parser *parser, Expr *callee)
{
    Token paren = parser->previous;
    Expr **arguments = NULL;
    int arg_count = 0;
    int capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            if (arg_count >= 255)
            {
                parser_error_at_current(parser, "Cannot have more than 255 arguments");
            }
            Expr *arg = parser_expression(parser);
            if (arg_count >= capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                Expr **new_arguments = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                if (new_arguments == NULL)
                {
                    exit(1);
                }
                if (arguments != NULL && arg_count > 0)
                {
                    memcpy(new_arguments, arguments, sizeof(Expr *) * arg_count);
                }
                arguments = new_arguments;
            }
            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
    return ast_create_call_expr(parser->arena, callee, arguments, arg_count, &paren);
}

Expr *parser_array_access(Parser *parser, Expr *array)
{
    Token bracket = parser->previous;
    Expr *start = NULL;
    Expr *end = NULL;
    Expr *step = NULL;

    // Check if this is a slice starting with '..' (e.g., [..] or [..end] or [..end:step])
    if (parser_match(parser, TOKEN_RANGE))
    {
        // This is a slice from the beginning: arr[..] or arr[..end] or arr[..end:step]
        if (!parser_check(parser, TOKEN_RIGHT_BRACKET) && !parser_check(parser, TOKEN_COLON))
        {
            end = parser_expression(parser);
        }
        // Check for step: arr[..end:step] or arr[..:step]
        if (parser_match(parser, TOKEN_COLON))
        {
            step = parser_expression(parser);
        }
        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after slice");
        return ast_create_array_slice_expr(parser->arena, array, NULL, end, step, &bracket);
    }

    // Parse the first expression (could be index or slice start)
    Expr *first = parser_expression(parser);

    // Check if this is a slice with start: arr[start..] or arr[start..end] or arr[start..end:step]
    if (parser_match(parser, TOKEN_RANGE))
    {
        start = first;
        // Check if there's an end expression
        if (!parser_check(parser, TOKEN_RIGHT_BRACKET) && !parser_check(parser, TOKEN_COLON))
        {
            end = parser_expression(parser);
        }
        // Check for step: arr[start..end:step] or arr[start..:step]
        if (parser_match(parser, TOKEN_COLON))
        {
            step = parser_expression(parser);
        }
        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after slice");
        return ast_create_array_slice_expr(parser->arena, array, start, end, step, &bracket);
    }

    // Regular array access: arr[index]
    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index");
    return ast_create_array_access_expr(parser->arena, array, first, &bracket);
}
