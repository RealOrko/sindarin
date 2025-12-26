// tests/parser_tests_lambda.c
// Lambda parser tests - single-line and multi-line lambdas

void test_single_line_lambda_parsing()
{
    printf("Testing parser_execute single-line lambda...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var double_it: fn(int): int = fn(x: int): int => x * 2\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_LAMBDA);

    LambdaExpr *lambda = &stmt->as.var_decl.initializer->as.lambda;
    assert(lambda->param_count == 1);
    assert(lambda->has_stmt_body == 0);  /* Single-line lambda uses expression body */
    assert(lambda->body != NULL);
    assert(lambda->body->type == EXPR_BINARY);  /* x * 2 */

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_multi_line_lambda_parsing()
{
    printf("Testing parser_execute multi-line lambda...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var abs_val: fn(int): int = fn(x: int): int =>\n"
        "    if x < 0 =>\n"
        "        return 0 - x\n"
        "    return x\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_LAMBDA);

    LambdaExpr *lambda = &stmt->as.var_decl.initializer->as.lambda;
    assert(lambda->param_count == 1);
    assert(lambda->has_stmt_body == 1);  /* Multi-line lambda uses statement body */
    assert(lambda->body_stmt_count == 2);  /* if statement + return statement */
    assert(lambda->body_stmts != NULL);
    assert(lambda->body_stmts[0]->type == STMT_IF);
    assert(lambda->body_stmts[1]->type == STMT_RETURN);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_multi_line_lambda_with_loop_parsing()
{
    printf("Testing parser_execute multi-line lambda with loop...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var make_range: fn(int, int): int[] = fn(start: int, end: int): int[] =>\n"
        "    var result: int[] = {}\n"
        "    for var i: int = start; i < end; i++ =>\n"
        "        result.push(i)\n"
        "    return result\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(stmt->as.var_decl.initializer->type == EXPR_LAMBDA);

    LambdaExpr *lambda = &stmt->as.var_decl.initializer->as.lambda;
    assert(lambda->param_count == 2);
    assert(lambda->has_stmt_body == 1);
    assert(lambda->body_stmt_count == 3);  /* var decl + for loop + return */
    assert(lambda->body_stmts[0]->type == STMT_VAR_DECL);
    assert(lambda->body_stmts[1]->type == STMT_FOR);
    assert(lambda->body_stmts[2]->type == STMT_RETURN);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_parser_lambda_main()
{
    test_single_line_lambda_parsing();
    test_multi_line_lambda_parsing();
    test_multi_line_lambda_with_loop_parsing();
}
