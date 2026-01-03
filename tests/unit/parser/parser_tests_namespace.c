// tests/parser_tests_namespace.c
// Parser tests for namespace/import syntax

/* Test basic import without namespace */
void test_parse_import_basic()
{
    printf("Testing parse basic import without namespace...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mymodule\"\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace == NULL);
    /* Verify module name is captured correctly */
    assert(stmt->as.import.module_name.length == 8);
    assert(strncmp(stmt->as.import.module_name.start, "mymodule", 8) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test import with 'as' namespace */
void test_parse_import_as_namespace()
{
    printf("Testing parse import with 'as' namespace...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"utils/string_helpers\" as strings\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    /* Verify namespace is set */
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 7);
    assert(strncmp(stmt->as.import.namespace->start, "strings", 7) == 0);
    /* Verify module path is preserved */
    assert(strncmp(stmt->as.import.module_name.start, "utils/string_helpers", 20) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test namespace with numbers in identifier */
void test_parse_namespace_with_numbers()
{
    printf("Testing parse namespace with numbers in identifier...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"crypto\" as crypto2\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 7);
    assert(strncmp(stmt->as.import.namespace->start, "crypto2", 7) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test namespace starting with underscore */
void test_parse_namespace_underscore_start()
{
    printf("Testing parse namespace starting with underscore...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"internal\" as _internal\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 9);
    assert(strncmp(stmt->as.import.namespace->start, "_internal", 9) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test single-letter namespace */
void test_parse_namespace_single_letter()
{
    printf("Testing parse single-letter namespace...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"math\" as m\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 1);
    assert(stmt->as.import.namespace->start[0] == 'm');

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test long namespace name */
void test_parse_namespace_long_name()
{
    printf("Testing parse long namespace name...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"database/connection\" as database_connection_manager\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->length == 27);
    assert(strncmp(stmt->as.import.namespace->start, "database_connection_manager", 27) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test multiple imports with various namespace styles */
void test_parse_multiple_namespace_styles()
{
    printf("Testing parse multiple imports with various namespace styles...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "import \"lib1\"\n"
        "import \"lib2\" as l2\n"
        "import \"lib3\"\n"
        "import \"lib4\" as _l4\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 4);

    /* First: no namespace */
    assert(module->statements[0]->type == STMT_IMPORT);
    assert(module->statements[0]->as.import.namespace == NULL);

    /* Second: with namespace 'l2' */
    assert(module->statements[1]->type == STMT_IMPORT);
    assert(module->statements[1]->as.import.namespace != NULL);
    assert(strncmp(module->statements[1]->as.import.namespace->start, "l2", 2) == 0);

    /* Third: no namespace */
    assert(module->statements[2]->type == STMT_IMPORT);
    assert(module->statements[2]->as.import.namespace == NULL);

    /* Fourth: with namespace '_l4' */
    assert(module->statements[3]->type == STMT_IMPORT);
    assert(module->statements[3]->as.import.namespace != NULL);
    assert(strncmp(module->statements[3]->as.import.namespace->start, "_l4", 3) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: reserved keyword as namespace - 'var' */
void test_parse_invalid_namespace_keyword_var()
{
    printf("Testing parse invalid namespace: keyword 'var'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as var\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: reserved keyword as namespace - 'fn' */
void test_parse_invalid_namespace_keyword_fn()
{
    printf("Testing parse invalid namespace: keyword 'fn'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as fn\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: reserved keyword as namespace - 'return' */
void test_parse_invalid_namespace_keyword_return()
{
    printf("Testing parse invalid namespace: keyword 'return'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as return\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: reserved keyword as namespace - 'import' */
void test_parse_invalid_namespace_keyword_import()
{
    printf("Testing parse invalid namespace: keyword 'import'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as import\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: type keyword 'str' as namespace */
void test_parse_invalid_namespace_keyword_str()
{
    printf("Testing parse invalid namespace: type keyword 'str'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as str\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error - 'str' is a type keyword */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: type keyword 'int' as namespace */
void test_parse_invalid_namespace_keyword_int()
{
    printf("Testing parse invalid namespace: type keyword 'int'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as int\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error - 'int' is a type keyword */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: missing identifier after 'as' */
void test_parse_invalid_missing_namespace()
{
    printf("Testing parse invalid: missing namespace after 'as'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test invalid: number as namespace (starts with digit) */
void test_parse_invalid_namespace_starts_with_number()
{
    printf("Testing parse invalid: namespace starts with number...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"mod\" as 123abc\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    /* Parser should error - number is not a valid identifier */
    assert(module == NULL);
    assert(parser.had_error == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test AST structure: import token info preserved */
void test_parse_import_ast_token_info()
{
    printf("Testing parse import AST token info preserved...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "import \"my_module\" as mymod\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_IMPORT);

    /* Verify module name token info */
    assert(stmt->as.import.module_name.type == TOKEN_STRING_LITERAL);
    assert(stmt->as.import.module_name.line == 1);
    assert(stmt->as.import.module_name.length == 9);

    /* Verify namespace token info */
    assert(stmt->as.import.namespace != NULL);
    assert(stmt->as.import.namespace->type == TOKEN_IDENTIFIER);
    assert(stmt->as.import.namespace->line == 1);
    assert(stmt->as.import.namespace->length == 5);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Test import followed by function to ensure parser continues correctly */
void test_parse_import_followed_by_code()
{
    printf("Testing parse import followed by function...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "import \"math\" as m\n"
        "\n"
        "fn main(): void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 2);

    /* First statement is import */
    assert(module->statements[0]->type == STMT_IMPORT);
    assert(module->statements[0]->as.import.namespace != NULL);

    /* Second statement is function */
    assert(module->statements[1]->type == STMT_FUNCTION);
    assert(strncmp(module->statements[1]->as.function.name.start, "main", 4) == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

/* Main entry point for namespace parser tests */
void test_parser_namespace_main()
{
    printf("\n=== Parser Namespace Tests ===\n");
    test_parse_import_basic();
    test_parse_import_as_namespace();
    test_parse_namespace_with_numbers();
    test_parse_namespace_underscore_start();
    test_parse_namespace_single_letter();
    test_parse_namespace_long_name();
    test_parse_multiple_namespace_styles();
    test_parse_invalid_namespace_keyword_var();
    test_parse_invalid_namespace_keyword_fn();
    test_parse_invalid_namespace_keyword_return();
    test_parse_invalid_namespace_keyword_import();
    test_parse_invalid_namespace_keyword_str();
    test_parse_invalid_namespace_keyword_int();
    test_parse_invalid_missing_namespace();
    test_parse_invalid_namespace_starts_with_number();
    test_parse_import_ast_token_info();
    test_parse_import_followed_by_code();
    printf("All namespace parser tests passed!\n\n");
}
