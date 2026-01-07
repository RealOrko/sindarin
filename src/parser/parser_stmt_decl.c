#include "parser/parser_stmt_decl.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "parser/parser_stmt.h"
#include "ast/ast_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

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

Stmt *parser_native_function_declaration(Parser *parser)
{
    Token native_token = parser->previous;  /* Points to 'fn' after 'native' */
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
    bool is_variadic = false;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                /* Check for variadic '...' - must be last in parameter list */
                if (parser_match(parser, TOKEN_SPREAD))
                {
                    is_variadic = true;
                    /* '...' must be followed by ')' - no more parameters allowed */
                    break;
                }

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
                /* Parse optional "as val" for parameter */
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

    /* Parse optional function modifier (shared/private) before return type */
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
    /* Mark function type as variadic if '...' was parsed */
    function_type->as.function.is_variadic = is_variadic;

    symbol_table_add_symbol(parser->symbol_table, name, function_type);

    Stmt **stmts = NULL;
    int stmt_count = 0;

    /* Native functions can have either:
     * 1. No body (just declaration) - ends with newline or semicolon
     * 2. A Sindarin body using '=>' (native fn with Sindarin implementation)
     */
    if (parser_match(parser, TOKEN_ARROW))
    {
        /* Native function with Sindarin body */
        skip_newlines(parser);

        /* Set native context so lambdas parsed in body are marked as native */
        int saved_in_native = parser->in_native_function;
        parser->in_native_function = 1;

        Stmt *body = parser_indented_block(parser);
        if (body == NULL)
        {
            body = ast_create_block_stmt(parser->arena, NULL, 0, NULL);
        }

        /* Restore native context */
        parser->in_native_function = saved_in_native;

        stmts = body->as.block.statements;
        stmt_count = body->as.block.count;
        body->as.block.statements = NULL;
    }
    else
    {
        /* Native function without body (external C function declaration) */
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            if (!parser_check(parser, TOKEN_NEWLINE) && !parser_check(parser, TOKEN_EOF) &&
                !parser_check(parser, TOKEN_FN) && !parser_check(parser, TOKEN_NATIVE) &&
                !parser_check(parser, TOKEN_VAR) && !parser_check(parser, TOKEN_DEDENT))
            {
                parser_consume(parser, TOKEN_NEWLINE, "Expected newline or '=>' after native function signature");
            }
        }
    }

    Stmt *func_stmt = ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &native_token);
    if (func_stmt != NULL)
    {
        func_stmt->as.function.modifier = func_modifier;
        func_stmt->as.function.is_native = true;
        func_stmt->as.function.is_variadic = is_variadic;
    }
    return func_stmt;
}

/* Parse a native function type: native fn(params): return_type */
Type *parser_native_function_type(Parser *parser)
{
    /* Consume 'fn' after 'native' */
    parser_consume(parser, TOKEN_FN, "Expected 'fn' after 'native' in type declaration");
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'fn' in native function type");

    Type **param_types = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            /* For native function types, we parse: param_name: type */
            /* Skip the parameter name if present */
            if (parser_check(parser, TOKEN_IDENTIFIER))
            {
                Token param_name = parser->current;
                parser_advance(parser);

                /* Check if followed by colon (named parameter) or it's a type name */
                if (parser_check(parser, TOKEN_COLON))
                {
                    parser_advance(parser); /* consume ':' */
                    /* Now parse the actual type */
                }
                else
                {
                    /* It was a type name, need to back up - but we can't.
                     * Instead, create the type from the identifier.
                     * Check if it's a known type */
                    Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, param_name);
                    if (type_symbol != NULL && type_symbol->type != NULL)
                    {
                        Type *param_type = ast_clone_type(parser->arena, type_symbol->type);
                        if (param_count >= param_capacity)
                        {
                            param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                            Type **new_params = arena_alloc(parser->arena, sizeof(Type *) * param_capacity);
                            if (new_params == NULL)
                            {
                                parser_error(parser, "Out of memory");
                                return NULL;
                            }
                            if (param_types != NULL && param_count > 0)
                            {
                                memcpy(new_params, param_types, sizeof(Type *) * param_count);
                            }
                            param_types = new_params;
                        }
                        param_types[param_count++] = param_type;
                        continue;
                    }
                    else
                    {
                        parser_error_at_current(parser, "Expected ':' after parameter name in native function type");
                        return NULL;
                    }
                }
            }

            Type *param_type = parser_type(parser);
            if (param_count >= param_capacity)
            {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                Type **new_params = arena_alloc(parser->arena, sizeof(Type *) * param_capacity);
                if (new_params == NULL)
                {
                    parser_error(parser, "Out of memory");
                    return NULL;
                }
                if (param_types != NULL && param_count > 0)
                {
                    memcpy(new_params, param_types, sizeof(Type *) * param_count);
                }
                param_types = new_params;
            }
            param_types[param_count++] = param_type;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameter types");
    parser_consume(parser, TOKEN_COLON, "Expected ':' before return type in native function type");
    Type *return_type = parser_type(parser);

    Type *func_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);
    func_type->as.function.is_native = true;

    return func_type;
}

Stmt *parser_type_declaration(Parser *parser)
{
    Token type_token = parser->previous;
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
        parser_error_at_current(parser, "Expected type alias name");
        return NULL;
    }

    parser_consume(parser, TOKEN_EQUAL, "Expected '=' after type alias name");

    Type *declared_type = NULL;

    /* Check for 'native fn(...)' (native callback type) or 'opaque' */
    if (parser_match(parser, TOKEN_NATIVE))
    {
        /* Parse native function type: native fn(params): return_type */
        declared_type = parser_native_function_type(parser);
        /* Store the typedef name for code generation */
        if (declared_type != NULL)
        {
            /* Create null-terminated string from token */
            char *typedef_name = arena_alloc(parser->arena, name.length + 1);
            strncpy(typedef_name, name.start, name.length);
            typedef_name[name.length] = '\0';
            declared_type->as.function.typedef_name = typedef_name;
        }
    }
    else if (parser_match(parser, TOKEN_OPAQUE))
    {
        /* Create the opaque type with the name */
        declared_type = ast_create_opaque_type(parser->arena, name.start);
    }
    else
    {
        parser_error_at_current(parser, "Expected 'opaque' or 'native fn' after '=' in type declaration");
        return NULL;
    }

    if (declared_type == NULL)
    {
        return NULL;
    }

    /* Register the type in the symbol table immediately so it can be used by later declarations */
    symbol_table_add_type(parser->symbol_table, name, declared_type);

    /* Consume optional newline/semicolon */
    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        if (!parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_FN) &&
            !parser_check(parser, TOKEN_NATIVE) && !parser_check(parser, TOKEN_VAR) &&
            !parser_check(parser, TOKEN_TYPE))
        {
            parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after type declaration");
        }
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_type_decl_stmt(parser->arena, name, declared_type, &type_token);
}
