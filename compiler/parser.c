#include "parser.h"
#include "parser_util.h"
#include "parser_expr.h"
#include "parser_stmt.h"
#include "debug.h"
#include "file.h"
#include <stdio.h>
#include <string.h>

void parser_init(Arena *arena, Parser *parser, Lexer *lexer, SymbolTable *symbol_table)
{
    parser->arena = arena;
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->symbol_table = symbol_table;
    parser->sized_array_pending = 0;
    parser->sized_array_size = NULL;

    Token print_token;
    print_token.start = arena_strdup(arena, "print");
    print_token.length = 5;
    print_token.type = TOKEN_IDENTIFIER;
    print_token.line = 0;
    print_token.filename = arena_strdup(arena, "<built-in>");

    Type *any_type = ast_create_primitive_type(arena, TYPE_ANY);
    Type **builtin_params = arena_alloc(arena, sizeof(Type *));
    builtin_params[0] = any_type;

    Type *print_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, print_token, print_type, SYMBOL_GLOBAL);

    Token to_string_token;
    to_string_token.start = arena_strdup(arena, "to_string");
    to_string_token.length = 9;
    to_string_token.type = TOKEN_IDENTIFIER;
    to_string_token.line = 0;
    to_string_token.filename = arena_strdup(arena, "<built-in>");

    Type *to_string_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, to_string_token, to_string_type, SYMBOL_GLOBAL);

    // Array built-in: len(arr) -> int (works on arrays and strings)
    Token len_token;
    len_token.start = arena_strdup(arena, "len");
    len_token.length = 3;
    len_token.type = TOKEN_IDENTIFIER;
    len_token.line = 0;
    len_token.filename = arena_strdup(arena, "<built-in>");
    Type *len_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_INT), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, len_token, len_type, SYMBOL_GLOBAL);

    // Console I/O convenience functions
    // readLine() -> str
    Token readLine_token;
    readLine_token.start = arena_strdup(arena, "readLine");
    readLine_token.length = 8;
    readLine_token.type = TOKEN_IDENTIFIER;
    readLine_token.line = 0;
    readLine_token.filename = arena_strdup(arena, "<built-in>");
    Type *readLine_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), NULL, 0);
    symbol_table_add_symbol_with_kind(parser->symbol_table, readLine_token, readLine_type, SYMBOL_GLOBAL);

    // println(any) -> void
    Token println_token;
    println_token.start = arena_strdup(arena, "println");
    println_token.length = 7;
    println_token.type = TOKEN_IDENTIFIER;
    println_token.line = 0;
    println_token.filename = arena_strdup(arena, "<built-in>");
    Type *println_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, println_token, println_type, SYMBOL_GLOBAL);

    // printErr(any) -> void
    Token printErr_token;
    printErr_token.start = arena_strdup(arena, "printErr");
    printErr_token.length = 8;
    printErr_token.type = TOKEN_IDENTIFIER;
    printErr_token.line = 0;
    printErr_token.filename = arena_strdup(arena, "<built-in>");
    Type *printErr_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, printErr_token, printErr_type, SYMBOL_GLOBAL);

    // printErrLn(any) -> void
    Token printErrLn_token;
    printErrLn_token.start = arena_strdup(arena, "printErrLn");
    printErrLn_token.length = 10;
    printErrLn_token.type = TOKEN_IDENTIFIER;
    printErrLn_token.line = 0;
    printErrLn_token.filename = arena_strdup(arena, "<built-in>");
    Type *printErrLn_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, printErrLn_token, printErrLn_type, SYMBOL_GLOBAL);

    // Note: Other array operations (push, pop, rev, rem, ins) are now method-style only:
    //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)

    parser->previous.type = TOKEN_ERROR;
    parser->previous.start = NULL;
    parser->previous.length = 0;
    parser->previous.line = 0;
    parser->current = parser->previous;

    parser_advance(parser);
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
}

void parser_cleanup(Parser *parser)
{
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->previous.start = NULL;
    parser->current.start = NULL;
}

Module *parser_execute(Parser *parser, const char *filename)
{
    Module *module = arena_alloc(parser->arena, sizeof(Module));
    ast_init_module(parser->arena, module, filename);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt != NULL)
        {
            ast_module_add_statement(parser->arena, module, stmt);
            ast_print_stmt(parser->arena, stmt, 0);
        }

        if (parser->panic_mode)
        {
            synchronize(parser);
        }
    }

    if (parser->had_error)
    {
        return NULL;
    }

    return module;
}

Module *parse_module_with_imports(Arena *arena, SymbolTable *symbol_table, const char *filename, char ***imported, int *imported_count, int *imported_capacity)
{
    char *source = file_read(arena, filename);
    if (!source)
    {
        DEBUG_ERROR("Failed to read file: %s", filename);
        return NULL;
    }

    Lexer lexer;
    lexer_init(arena, &lexer, source, filename);

    Parser parser;
    parser_init(arena, &parser, &lexer, symbol_table);

    Module *module = parser_execute(&parser, filename);
    if (!module || parser.had_error)
    {
        parser_cleanup(&parser);
        lexer_cleanup(&lexer);
        return NULL;
    }

    Stmt **all_statements = NULL;
    int all_count = 0;
    int all_capacity = 0;

    const char *dir_end = strrchr(filename, '/');
    size_t dir_len = dir_end ? (size_t)(dir_end - filename + 1) : 0;
    char *dir = NULL;
    if (dir_len > 0) {
        dir = arena_alloc(arena, dir_len + 1);
        if (!dir) {
            DEBUG_ERROR("Failed to allocate memory for directory path");
            parser_cleanup(&parser);
            lexer_cleanup(&lexer);
            return NULL;
        }
        strncpy(dir, filename, dir_len);
        dir[dir_len] = '\0';
    }

    for (int i = 0; i < module->count;)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_IMPORT)
        {
            Token mod_name = stmt->as.import.module_name;
            size_t mod_name_len = strlen(mod_name.start);
            size_t path_len = dir_len + mod_name_len + 4;
            char *import_path = arena_alloc(arena, path_len);
            if (!import_path)
            {
                DEBUG_ERROR("Failed to allocate memory for import path");
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                return NULL;
            }
            if (dir_len > 0) {
                strcpy(import_path, dir);
            } else {
                import_path[0] = '\0';
            }
            strcat(import_path, mod_name.start);
            strcat(import_path, ".sn");

            int already_imported = 0;
            for (int j = 0; j < *imported_count; j++)
            {
                if (strcmp((*imported)[j], import_path) == 0)
                {
                    already_imported = 1;
                    break;
                }
            }

            if (already_imported)
            {
                memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
                module->count--;
                continue;
            }

            if (*imported_count >= *imported_capacity)
            {
                *imported_capacity = *imported_capacity == 0 ? 8 : *imported_capacity * 2;
                char **new_imported = arena_alloc(arena, sizeof(char *) * *imported_capacity);
                if (!new_imported)
                {
                    DEBUG_ERROR("Failed to allocate memory for imported list");
                    parser_cleanup(&parser);
                    lexer_cleanup(&lexer);
                    return NULL;
                }
                if (*imported_count > 0)
                {
                    memmove(new_imported, *imported, sizeof(char *) * *imported_count);
                }
                *imported = new_imported;
            }
            (*imported)[(*imported_count)++] = import_path;

            Module *imported_module = parse_module_with_imports(arena, symbol_table, import_path, imported, imported_count, imported_capacity);
            if (!imported_module)
            {
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                return NULL;
            }

            int new_all_count = all_count + imported_module->count;
            int old_capacity = all_capacity;
            while (new_all_count > all_capacity)
            {
                all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
            }
            if (all_capacity > old_capacity || all_statements == NULL)
            {
                Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
                if (!new_statements)
                {
                    DEBUG_ERROR("Failed to allocate memory for statements");
                    parser_cleanup(&parser);
                    lexer_cleanup(&lexer);
                    return NULL;
                }
                if (all_count > 0 && all_statements != NULL)
                {
                    memmove(new_statements, all_statements, sizeof(Stmt *) * all_count);
                }
                all_statements = new_statements;
            }
            memmove(all_statements + all_count, imported_module->statements, sizeof(Stmt *) * imported_module->count);
            all_count = new_all_count;

            memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
            module->count--;
        }
        else
        {
            i++;
        }
    }

    int new_all_count = all_count + module->count;
    int old_capacity = all_capacity;
    while (new_all_count > all_capacity)
    {
        all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
    }
    if (all_capacity > old_capacity || all_statements == NULL)
    {
        Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
        if (!new_statements)
        {
            DEBUG_ERROR("Failed to allocate memory for statements");
            parser_cleanup(&parser);
            lexer_cleanup(&lexer);
            return NULL;
        }
        if (all_count > 0 && all_statements != NULL)
        {
            memmove(new_statements, all_statements, sizeof(Stmt *) * all_count);
        }
        all_statements = new_statements;
    }
    memmove(all_statements + all_count, module->statements, sizeof(Stmt *) * module->count);
    all_count = new_all_count;

    module->statements = all_statements;
    module->count = all_count;
    module->capacity = all_count;

    parser_cleanup(&parser);
    lexer_cleanup(&lexer);
    return module;
}
