// tests/parser_tests_memory.c
// Parser tests for memory management syntax (as val, as ref, shared, private)

void test_var_decl_as_val_parsing()
{
    printf("Testing parser_execute var declaration with 'as val'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var arr: int[] as val = {1, 2, 3}\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.mem_qualifier == MEM_AS_VAL);
    assert(stmt->as.var_decl.type->kind == TYPE_ARRAY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_var_decl_as_ref_parsing()
{
    printf("Testing parser_execute var declaration with 'as ref'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int as ref = 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.mem_qualifier == MEM_AS_REF);
    assert(stmt->as.var_decl.type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_var_decl_default_qualifier_parsing()
{
    printf("Testing parser_execute var declaration with default qualifier...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x: int = 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.mem_qualifier == MEM_DEFAULT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_function_param_as_val_parsing()
{
    printf("Testing parser_execute function param with 'as val'...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn process(arr: int[] as val): void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.param_count == 1);
    assert(fn->as.function.params[0].mem_qualifier == MEM_AS_VAL);
    assert(fn->as.function.params[0].type->kind == TYPE_ARRAY);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_function_shared_modifier_parsing()
{
    printf("Testing parser_execute function with 'shared' modifier...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn helper() shared: int =>\n"
        "  return 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.modifier == FUNC_SHARED);
    assert(fn->as.function.return_type->kind == TYPE_INT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_function_private_modifier_parsing()
{
    printf("Testing parser_execute function with 'private' modifier...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn compute() private: double =>\n"
        "  return 3.14\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.modifier == FUNC_PRIVATE);
    assert(fn->as.function.return_type->kind == TYPE_DOUBLE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_function_default_modifier_parsing()
{
    printf("Testing parser_execute function with default modifier...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.modifier == FUNC_DEFAULT);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_shared_block_parsing()
{
    printf("Testing parser_execute shared block...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  shared =>\n"
        "    var x: int = 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 1);
    Stmt *block = fn->as.function.body[0];
    assert(block->type == STMT_BLOCK);
    assert(block->as.block.modifier == BLOCK_SHARED);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_private_block_parsing()
{
    printf("Testing parser_execute private block...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  private =>\n"
        "    var x: int = 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 1);
    Stmt *block = fn->as.function.body[0];
    assert(block->type == STMT_BLOCK);
    assert(block->as.block.modifier == BLOCK_PRIVATE);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_shared_while_loop_parsing()
{
    printf("Testing parser_execute shared while loop...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var i: int = 0\n"
        "  shared while i < 10 =>\n"
        "    i = i + 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *while_stmt = fn->as.function.body[1];
    assert(while_stmt->type == STMT_WHILE);
    assert(while_stmt->as.while_stmt.is_shared == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_shared_for_each_loop_parsing()
{
    printf("Testing parser_execute shared for-each loop...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var arr: int[] = {1, 2, 3}\n"
        "  shared for x in arr =>\n"
        "    print($\"{x}\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *for_stmt = fn->as.function.body[1];
    assert(for_stmt->type == STMT_FOR_EACH);
    assert(for_stmt->as.for_each_stmt.is_shared == true);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_regular_while_loop_not_shared_parsing()
{
    printf("Testing parser_execute regular while loop is not shared...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var i: int = 0\n"
        "  while i < 10 =>\n"
        "    i = i + 1\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *while_stmt = fn->as.function.body[1];
    assert(while_stmt->type == STMT_WHILE);
    assert(while_stmt->as.while_stmt.is_shared == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_regular_for_each_loop_not_shared_parsing()
{
    printf("Testing parser_execute regular for-each loop is not shared...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main(): void =>\n"
        "  var arr: int[] = {1, 2, 3}\n"
        "  for x in arr =>\n"
        "    print($\"{x}\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(fn->as.function.body_count == 2);
    Stmt *for_stmt = fn->as.function.body[1];
    assert(for_stmt->type == STMT_FOR_EACH);
    assert(for_stmt->as.for_each_stmt.is_shared == false);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_parser_memory_main()
{
    test_var_decl_as_val_parsing();
    test_var_decl_as_ref_parsing();
    test_var_decl_default_qualifier_parsing();
    test_function_param_as_val_parsing();
    test_function_shared_modifier_parsing();
    test_function_private_modifier_parsing();
    test_function_default_modifier_parsing();
    test_shared_block_parsing();
    test_private_block_parsing();
    test_shared_while_loop_parsing();
    test_shared_for_each_loop_parsing();
    test_regular_while_loop_not_shared_parsing();
    test_regular_for_each_loop_not_shared_parsing();
}
