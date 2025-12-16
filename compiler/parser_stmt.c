#include "parser_stmt.h"
#include "parser_util.h"
#include "parser_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

int is_at_function_boundary(Parser *parser)
{
    if (parser_check(parser, TOKEN_DEDENT))
    {
        return 1;
    }
    if (parser_check(parser, TOKEN_FN))
    {
        return 1;
    }
    if (parser_check(parser, TOKEN_EOF))
    {
        return 1;
    }
    return 0;
}

Stmt *parser_indented_block(Parser *parser)
{
    if (!parser_check(parser, TOKEN_INDENT))
    {
        parser_error(parser, "Expected indented block");
        return NULL;
    }
    parser_advance(parser);

    int current_indent = parser->lexer->indent_stack[parser->lexer->indent_size - 1];
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    while (!parser_is_at_end(parser) &&
           parser->lexer->indent_stack[parser->lexer->indent_size - 1] >= current_indent)
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }

        if (parser_check(parser, TOKEN_DEDENT))
        {
            break;
        }

        if (parser_check(parser, TOKEN_EOF))
        {
            break;
        }

        Stmt *stmt = parser_declaration(parser);
        if (stmt == NULL)
        {
            continue;
        }

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
        }
        statements[count++] = stmt;
    }

    if (parser_check(parser, TOKEN_DEDENT))
    {
        parser_advance(parser);
    }
    else if (parser->lexer->indent_stack[parser->lexer->indent_size - 1] < current_indent)
    {
        parser_error(parser, "Expected dedent to end block");
    }

    return ast_create_block_stmt(parser->arena, statements, count, NULL);
}

Stmt *parser_statement(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parser_var_declaration(parser);
    }
    if (parser_match(parser, TOKEN_IF))
    {
        return parser_if_statement(parser);
    }
    if (parser_match(parser, TOKEN_WHILE))
    {
        return parser_while_statement(parser);
    }
    if (parser_match(parser, TOKEN_FOR))
    {
        return parser_for_statement(parser);
    }
    if (parser_match(parser, TOKEN_RETURN))
    {
        return parser_return_statement(parser);
    }
    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        return parser_block_statement(parser);
    }

    return parser_expression_statement(parser);
}

Stmt *parser_declaration(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parser_var_declaration(parser);
    }
    if (parser_match(parser, TOKEN_FN))
    {
        return parser_function_declaration(parser);
    }
    if (parser_match(parser, TOKEN_IMPORT))
    {
        return parser_import_statement(parser);
    }

    return parser_statement(parser);
}

Stmt *parser_var_declaration(Parser *parser)
{
    Token var_token = parser->previous;
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected variable name");
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
    Type *type = parser_type(parser);

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        initializer = parser_expression(parser);
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after variable declaration");
    }

    return ast_create_var_decl_stmt(parser->arena, name, type, initializer, &var_token);
}

Stmt *parser_function_declaration(Parser *parser)
{
    Token fn_token = parser->previous;
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected function name");
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                if (param_count >= 255)
                {
                    parser_error_at_current(parser, "Cannot have more than 255 parameters");
                }
                Token param_name;
                if (parser_check(parser, TOKEN_IDENTIFIER))
                {
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                else
                {
                    parser_error_at_current(parser, "Expected parameter name");
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                Type *param_type = parser_type(parser);

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                    if (new_params == NULL)
                    {
                        exit(1);
                    }
                    if (params != NULL && param_count > 0)
                    {
                        memcpy(new_params, params, sizeof(Parameter) * param_count);
                    }
                    params = new_params;
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    Type *return_type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parser_type(parser);
    }

    Type **param_types = arena_alloc(parser->arena, sizeof(Type *) * param_count);
    if (param_types == NULL && param_count > 0)
    {
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);

    symbol_table_add_symbol(parser->symbol_table, name, function_type);

    parser_consume(parser, TOKEN_ARROW, "Expected '=>' before function body");
    skip_newlines(parser);

    Stmt *body = parser_indented_block(parser);
    if (body == NULL)
    {
        body = ast_create_block_stmt(parser->arena, NULL, 0, NULL);
    }

    Stmt **stmts = body->as.block.statements;
    int stmt_count = body->as.block.count;
    body->as.block.statements = NULL;

    return ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &fn_token);
}

Stmt *parser_return_statement(Parser *parser)
{
    Token keyword = parser->previous;
    keyword.start = arena_strndup(parser->arena, keyword.start, keyword.length);
    if (keyword.start == NULL)
    {
        parser_error_at_current(parser, "Out of memory");
        return NULL;
    }

    Expr *value = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        value = parser_expression(parser);
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after return value");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_return_stmt(parser->arena, keyword, value, &keyword);
}

Stmt *parser_if_statement(Parser *parser)
{
    Token if_token = parser->previous;
    Expr *condition = parser_expression(parser);
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after if condition");
    skip_newlines(parser);

    Stmt *then_branch;
    if (parser_check(parser, TOKEN_INDENT))
    {
        then_branch = parser_indented_block(parser);
    }
    else
    {
        then_branch = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = then_branch;
            block_stmts[1] = parser_indented_block(parser);
            then_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    Stmt *else_branch = NULL;
    skip_newlines(parser);
    if (parser_match(parser, TOKEN_ELSE))
    {
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after else");
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            else_branch = parser_indented_block(parser);
        }
        else
        {
            else_branch = parser_statement(parser);
            skip_newlines(parser);
            if (parser_check(parser, TOKEN_INDENT))
            {
                Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
                if (block_stmts == NULL)
                {
                    exit(1);
                }
                block_stmts[0] = else_branch;
                block_stmts[1] = parser_indented_block(parser);
                else_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
            }
        }
    }

    return ast_create_if_stmt(parser->arena, condition, then_branch, else_branch, &if_token);
}

Stmt *parser_while_statement(Parser *parser)
{
    Token while_token = parser->previous;
    Expr *condition = parser_expression(parser);
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after while condition");
    skip_newlines(parser);

    Stmt *body;
    if (parser_check(parser, TOKEN_INDENT))
    {
        body = parser_indented_block(parser);
    }
    else
    {
        body = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = body;
            block_stmts[1] = parser_indented_block(parser);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    return ast_create_while_stmt(parser->arena, condition, body, &while_token);
}

Stmt *parser_for_statement(Parser *parser)
{
    Token for_token = parser->previous;
    Stmt *initializer = NULL;
    if (parser_match(parser, TOKEN_VAR))
    {
        Token var_token = parser->previous;
        Token name;
        if (parser_check(parser, TOKEN_IDENTIFIER))
        {
            name = parser->current;
            parser_advance(parser);
            name.start = arena_strndup(parser->arena, name.start, name.length);
            if (name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
        }
        else
        {
            parser_error_at_current(parser, "Expected variable name");
            name = parser->current;
            parser_advance(parser);
            name.start = arena_strndup(parser->arena, name.start, name.length);
            if (name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
        }
        parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
        Type *type = parser_type(parser);
        Expr *init_expr = NULL;
        if (parser_match(parser, TOKEN_EQUAL))
        {
            init_expr = parser_expression(parser);
        }
        initializer = ast_create_var_decl_stmt(parser->arena, name, type, init_expr, &var_token);
    }
    else if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        Expr *init_expr = parser_expression(parser);
        initializer = ast_create_expr_stmt(parser->arena, init_expr, NULL);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after initializer");

    Expr *condition = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        condition = parser_expression(parser);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition");

    Expr *increment = NULL;
    if (!parser_check(parser, TOKEN_ARROW))
    {
        increment = parser_expression(parser);
    }
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after for clauses");
    skip_newlines(parser);

    Stmt *body;
    if (parser_check(parser, TOKEN_INDENT))
    {
        body = parser_indented_block(parser);
    }
    else
    {
        body = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = body;
            block_stmts[1] = parser_indented_block(parser);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    return ast_create_for_stmt(parser->arena, initializer, condition, increment, body, &for_token);
}

Stmt *parser_block_statement(Parser *parser)
{
    Token brace = parser->previous;
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    symbol_table_push_scope(parser->symbol_table);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser) || parser_check(parser, TOKEN_DEDENT))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt == NULL)
            continue;

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
        }
        statements[count++] = stmt;
    }

    symbol_table_pop_scope(parser->symbol_table);

    return ast_create_block_stmt(parser->arena, statements, count, &brace);
}

Stmt *parser_expression_statement(Parser *parser)
{
    Expr *expr = parser_expression(parser);

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after expression");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_expr_stmt(parser->arena, expr, &parser->previous);
}

Stmt *parser_import_statement(Parser *parser)
{
    Token import_token = parser->previous;
    Token module_name;
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        module_name = parser->previous;
        module_name.start = arena_strdup(parser->arena, parser->previous.literal.string_value);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
        module_name.length = strlen(module_name.start);
        module_name.line = parser->previous.line;
        module_name.filename = parser->previous.filename;
        module_name.type = TOKEN_STRING_LITERAL;
    }
    else
    {
        parser_error_at_current(parser, "Expected module name as string");
        module_name = parser->current;
        parser_advance(parser);
        module_name.start = arena_strndup(parser->arena, module_name.start, module_name.length);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after import statement");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_import_stmt(parser->arena, module_name, &import_token);
}
