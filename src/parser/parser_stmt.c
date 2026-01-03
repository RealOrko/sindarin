#include "parser/parser_stmt.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "ast/ast_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

/* Parse optional "as val" or "as ref" memory qualifier */
MemoryQualifier parser_memory_qualifier(Parser *parser)
{
    if (parser_match(parser, TOKEN_AS))
    {
        if (parser_match(parser, TOKEN_VAL))
        {
            return MEM_AS_VAL;
        }
        else if (parser_match(parser, TOKEN_REF))
        {
            return MEM_AS_REF;
        }
        else
        {
            parser_error_at_current(parser, "Expected 'val' or 'ref' after 'as'");
        }
    }
    return MEM_DEFAULT;
}

/* Parse optional "shared" or "private" function modifier */
FunctionModifier parser_function_modifier(Parser *parser)
{
    if (parser_match(parser, TOKEN_SHARED))
    {
        return FUNC_SHARED;
    }
    else if (parser_match(parser, TOKEN_PRIVATE))
    {
        return FUNC_PRIVATE;
    }
    return FUNC_DEFAULT;
}

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

        /* Synchronize on error to prevent infinite loops */
        if (parser->panic_mode)
        {
            synchronize(parser);
        }

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
        return parser_while_statement(parser, false);
    }
    if (parser_match(parser, TOKEN_FOR))
    {
        return parser_for_statement(parser, false);
    }
    if (parser_match(parser, TOKEN_BREAK))
    {
        Token keyword = parser->previous;
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            parser_consume(parser, TOKEN_NEWLINE, "Expected newline after 'break'");
        }
        return ast_create_break_stmt(parser->arena, &keyword);
    }
    if (parser_match(parser, TOKEN_CONTINUE))
    {
        Token keyword = parser->previous;
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            parser_consume(parser, TOKEN_NEWLINE, "Expected newline after 'continue'");
        }
        return ast_create_continue_stmt(parser->arena, &keyword);
    }
    if (parser_match(parser, TOKEN_RETURN))
    {
        return parser_return_statement(parser);
    }
    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        return parser_block_statement(parser);
    }

    // Parse shared => block, shared while, shared for, or private => block
    if (parser_check(parser, TOKEN_SHARED))
    {
        Token block_token = parser->current;
        parser_advance(parser);  // consume shared

        // Check if followed by while or for (shared loop)
        if (parser_match(parser, TOKEN_WHILE))
        {
            return parser_while_statement(parser, true);
        }
        if (parser_match(parser, TOKEN_FOR))
        {
            return parser_for_statement(parser, true);
        }

        // Otherwise it's a shared block
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after shared");
        skip_newlines(parser);

        Stmt *block = parser_indented_block(parser);
        if (block == NULL)
        {
            block = ast_create_block_stmt(parser->arena, NULL, 0, &block_token);
        }
        block->as.block.modifier = BLOCK_SHARED;
        return block;
    }

    // Parse private => block
    if (parser_check(parser, TOKEN_PRIVATE))
    {
        Token block_token = parser->current;
        parser_advance(parser);  // consume private

        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after private");
        skip_newlines(parser);

        Stmt *block = parser_indented_block(parser);
        if (block == NULL)
        {
            block = ast_create_block_stmt(parser->arena, NULL, 0, &block_token);
        }
        block->as.block.modifier = BLOCK_PRIVATE;
        return block;
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

    // Type annotation is optional if there's an initializer (type inference)
    Type *type = NULL;
    MemoryQualifier mem_qualifier = MEM_DEFAULT;
    Expr *sized_array_size_expr = NULL;
    if (parser_match(parser, TOKEN_COLON))
    {
        ParsedType parsed = parser_type_with_size(parser);
        type = parsed.type;

        /* Check if this is a sized array type (e.g., int[10]) */
        if (parsed.is_sized_array)
        {
            sized_array_size_expr = parsed.size_expr;
        }

        // Parse optional "as val" or "as ref" after type
        mem_qualifier = parser_memory_qualifier(parser);
    }

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        initializer = parser_expression(parser);
    }

    /* If this is a sized array declaration, create the sized array alloc expression */
    if (sized_array_size_expr != NULL)
    {
        Expr *default_value = initializer; /* The initializer becomes the default value */
        initializer = ast_create_sized_array_alloc_expr(
            parser->arena, type, sized_array_size_expr, default_value, &var_token);
        /* The variable type becomes an array of the element type */
        type = ast_create_array_type(parser->arena, type);
    }

    // Must have either type annotation or initializer (or both)
    if (type == NULL && initializer == NULL)
    {
        parser_error_at_current(parser, "Variable declaration requires type annotation or initializer");
    }

    /* After multi-line lambda with statement body, we may be at the next statement already
     * (no NEWLINE token between DEDENT and the next statement). Also handle DEDENT case
     * when the var declaration is the last statement in a block. */
    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
    {
        /* Allow if we're at the next statement (IDENTIFIER covers print, etc.),
         * a keyword that starts a statement, end of block (DEDENT), or EOF */
        if (!parser_check(parser, TOKEN_IDENTIFIER) && !parser_check(parser, TOKEN_VAR) &&
            !parser_check(parser, TOKEN_FN) && !parser_check(parser, TOKEN_IF) &&
            !parser_check(parser, TOKEN_WHILE) && !parser_check(parser, TOKEN_FOR) &&
            !parser_check(parser, TOKEN_RETURN) && !parser_check(parser, TOKEN_BREAK) &&
            !parser_check(parser, TOKEN_CONTINUE) &&
            !parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_EOF))
        {
            parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after variable declaration");
        }
    }

    Stmt *stmt = ast_create_var_decl_stmt(parser->arena, name, type, initializer, &var_token);
    if (stmt != NULL)
    {
        stmt->as.var_decl.mem_qualifier = mem_qualifier;
    }
    return stmt;
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
                // Parse optional "as val" for parameter
                MemoryQualifier param_qualifier = parser_memory_qualifier(parser);

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
                params[param_count].mem_qualifier = param_qualifier;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    // Parse optional function modifier (shared/private) before return type
    FunctionModifier func_modifier = parser_function_modifier(parser);

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

    Stmt *func_stmt = ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &fn_token);
    if (func_stmt != NULL)
    {
        func_stmt->as.function.modifier = func_modifier;
    }
    return func_stmt;
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
            Stmt *indented = parser_indented_block(parser);
            block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
            then_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    Stmt *else_branch = NULL;
    skip_newlines(parser);
    if (parser_match(parser, TOKEN_ELSE))
    {
        /* Support 'else if' syntax sugar - if followed by 'if', parse if statement directly */
        if (parser_match(parser, TOKEN_IF))
        {
            /* Parse the 'if' statement as the else branch (no arrow needed between else and if) */
            else_branch = parser_if_statement(parser);
        }
        else
        {
            /* Original 'else =>' syntax */
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
                    Stmt *indented = parser_indented_block(parser);
                    block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
                    else_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
                }
            }
        }
    }

    return ast_create_if_stmt(parser->arena, condition, then_branch, else_branch, &if_token);
}

Stmt *parser_while_statement(Parser *parser, bool is_shared)
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
            Stmt *indented = parser_indented_block(parser);
            block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    Stmt *stmt = ast_create_while_stmt(parser->arena, condition, body, &while_token);
    if (stmt != NULL)
    {
        stmt->as.while_stmt.is_shared = is_shared;
    }
    return stmt;
}

Stmt *parser_for_statement(Parser *parser, bool is_shared)
{
    Token for_token = parser->previous;

    // Check for for-each syntax: for x in arr =>
    // We need to look ahead: if we see IDENTIFIER followed by IN, it's for-each
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        Token var_name = parser->current;
        parser_advance(parser);

        if (parser_check(parser, TOKEN_IN))
        {
            // This is a for-each loop
            parser_advance(parser); // consume 'in'
            var_name.start = arena_strndup(parser->arena, var_name.start, var_name.length);
            if (var_name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }

            Expr *iterable = parser_expression(parser);
            parser_consume(parser, TOKEN_ARROW, "Expected '=>' after for-each iterable");
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
                    Stmt *indented = parser_indented_block(parser);
                    block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
                    body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
                }
            }

            Stmt *stmt = ast_create_for_each_stmt(parser->arena, var_name, iterable, body, &for_token);
            if (stmt != NULL)
            {
                stmt->as.for_each_stmt.is_shared = is_shared;
            }
            return stmt;
        }
        else
        {
            // Not for-each, backtrack - need to re-parse as expression
            // Since we already consumed the identifier, we need to create a variable expr
            // and parse the rest as expression statement initializer
            var_name.start = arena_strndup(parser->arena, var_name.start, var_name.length);
            if (var_name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
            Expr *var_expr = ast_create_variable_expr(parser->arena, var_name, &var_name);

            // Parse the rest of the expression (e.g., = 0)
            Expr *init_expr = var_expr;
            if (parser_match(parser, TOKEN_EQUAL))
            {
                Expr *value = parser_expression(parser);
                init_expr = ast_create_assign_expr(parser->arena, var_name, value, &var_name);
            }
            else
            {
                // May have more expression parts like function call, etc.
                // For now, just use the variable as the initializer
                // This handles cases like: for i; i < 10; i++ =>
            }
            Stmt *initializer = ast_create_expr_stmt(parser->arena, init_expr, NULL);

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
                    Stmt *indented = parser_indented_block(parser);
                    block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
                    body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
                }
            }

            Stmt *stmt = ast_create_for_stmt(parser->arena, initializer, condition, increment, body, &for_token);
            if (stmt != NULL)
            {
                stmt->as.for_stmt.is_shared = is_shared;
            }
            return stmt;
        }
    }

    // Traditional for loop parsing
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
            Stmt *indented = parser_indented_block(parser);
            block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    Stmt *stmt = ast_create_for_stmt(parser->arena, initializer, condition, increment, body, &for_token);
    if (stmt != NULL)
    {
        stmt->as.for_stmt.is_shared = is_shared;
    }
    return stmt;
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

/* Helper to check if a token type is a reserved keyword */
static bool parser_is_keyword_token(TokenType type)
{
    switch (type)
    {
        case TOKEN_FN:
        case TOKEN_VAR:
        case TOKEN_RETURN:
        case TOKEN_IF:
        case TOKEN_ELSE:
        case TOKEN_FOR:
        case TOKEN_WHILE:
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
        case TOKEN_IN:
        case TOKEN_IMPORT:
        case TOKEN_NIL:
        case TOKEN_INT:
        case TOKEN_LONG:
        case TOKEN_DOUBLE:
        case TOKEN_CHAR:
        case TOKEN_STR:
        case TOKEN_BOOL:
        case TOKEN_BYTE:
        case TOKEN_VOID:
        case TOKEN_SHARED:
        case TOKEN_PRIVATE:
        case TOKEN_AS:
        case TOKEN_VAL:
        case TOKEN_REF:
        case TOKEN_BOOL_LITERAL:  /* true/false */
            return true;
        default:
            return false;
    }
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

    /* Check for optional 'as namespace' clause */
    Token *namespace = NULL;
    if (parser_match(parser, TOKEN_AS))
    {
        /* Next token must be an identifier (not a keyword) */
        if (parser_check(parser, TOKEN_IDENTIFIER))
        {
            parser_advance(parser);
            namespace = arena_alloc(parser->arena, sizeof(Token));
            if (namespace == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
            *namespace = parser->previous;
            namespace->start = arena_strndup(parser->arena, parser->previous.start, parser->previous.length);
            if (namespace->start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
        }
        else if (parser_is_keyword_token(parser->current.type))
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot use reserved keyword '%.*s' as namespace name",
                     parser->current.length, parser->current.start);
            parser_error_at_current(parser, msg);
            parser_advance(parser);  /* Skip the keyword to continue parsing */
        }
        else
        {
            parser_error_at_current(parser, "Expected namespace identifier after 'as'");
            if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
            {
                parser_advance(parser);  /* Skip unexpected token */
            }
        }
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after import statement");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_import_stmt(parser->arena, module_name, namespace, &import_token);
}
