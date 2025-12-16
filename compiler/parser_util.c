#include "parser_util.h"
#include "debug.h"
#include <stdio.h>

int parser_is_at_end(Parser *parser)
{
    return parser->current.type == TOKEN_EOF;
}

void skip_newlines(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        if (parser_check(parser, TOKEN_INDENT) || parser_check(parser, TOKEN_DEDENT))
        {
            break;
        }
    }
}

int skip_newlines_and_check_end(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }
    return parser_is_at_end(parser);
}

void parser_error(Parser *parser, const char *message)
{
    parser_error_at(parser, &parser->previous, message);
}

void parser_error_at_current(Parser *parser, const char *message)
{
    parser_error_at(parser, &parser->current, message);
}

void parser_error_at(Parser *parser, Token *token, const char *message)
{
    if (parser->panic_mode)
        return;

    parser->panic_mode = 1;
    parser->had_error = 1;

    fprintf(stderr, "[%s:%d] Error", parser->lexer->filename, token->line);
    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);

    parser->lexer->indent_size = 1;

    if (token == &parser->current)
    {
        parser_advance(parser);
    }
}

void parser_advance(Parser *parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = lexer_scan_token(parser->lexer);
        if (parser->current.type != TOKEN_ERROR)
            break;
        parser_error_at_current(parser, parser->current.start);
    }
}

void parser_consume(Parser *parser, TokenType type, const char *message)
{
    if (parser->current.type == type)
    {
        parser_advance(parser);
        return;
    }
    parser_error_at_current(parser, message);
}

int parser_check(Parser *parser, TokenType type)
{
    return parser->current.type == type;
}

int parser_match(Parser *parser, TokenType type)
{
    if (!parser_check(parser, type))
        return 0;
    parser_advance(parser);
    return 1;
}

void synchronize(Parser *parser)
{
    parser->panic_mode = 0;

    while (!parser_is_at_end(parser))
    {
        if (parser->previous.type == TOKEN_SEMICOLON || parser->previous.type == TOKEN_NEWLINE)
            return;

        switch (parser->current.type)
        {
        case TOKEN_FN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_RETURN:
        case TOKEN_IMPORT:
        case TOKEN_ELSE:
            return;
        case TOKEN_NEWLINE:
            parser_advance(parser);
            break;
        default:
            parser_advance(parser);
            break;
        }
    }
}

Type *parser_type(Parser *parser)
{
    Type *type = NULL;

    if (parser_match(parser, TOKEN_INT))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_INT);
    }
    else if (parser_match(parser, TOKEN_LONG))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_LONG);
    }
    else if (parser_match(parser, TOKEN_DOUBLE))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_DOUBLE);
    }
    else if (parser_match(parser, TOKEN_CHAR))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_CHAR);
    }
    else if (parser_match(parser, TOKEN_STR))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_STRING);
    }
    else if (parser_match(parser, TOKEN_BOOL))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_BOOL);
    }
    else if (parser_match(parser, TOKEN_VOID))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    }
    else
    {
        parser_error_at_current(parser, "Expected type");
        return ast_create_primitive_type(parser->arena, TYPE_NIL);
    }

    while (parser_match(parser, TOKEN_LEFT_BRACKET))
    {
        if (!parser_match(parser, TOKEN_RIGHT_BRACKET))
        {
            parser_error_at_current(parser, "Expected ']' after '['");
            return type;
        }
        type = ast_create_array_type(parser->arena, type);
    }

    return type;
}
