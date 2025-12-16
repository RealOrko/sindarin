// tests/code_gen_tests_expr.c
// Expression code generation tests

void test_code_gen_literal_expression()
{
    DEBUG_INFO("Starting test_code_gen_literal_expression");
    printf("Testing code_gen for literal expressions...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token token;
    setup_basic_token(&token, TOKEN_INT_LITERAL, "42");
    token_set_int_literal(&token, 42);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    LiteralValue val = {.int_value = 42};
    Expr *lit_expr = ast_create_literal_expr(&arena, val, int_type, false, &token);
    lit_expr->expr_type = int_type;
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, lit_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "42L;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_literal_expression");
}

void test_code_gen_variable_expression()
{
    DEBUG_INFO("Starting test_code_gen_variable_expression");
    printf("Testing code_gen for variable expressions...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_token;
    setup_basic_token(&var_token, TOKEN_IDENTIFIER, "x");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *init_expr = NULL; // Default 0
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_token, int_type, init_expr, &var_token);

    Expr *var_expr = ast_create_variable_expr(&arena, var_token, &var_token);
    var_expr->expr_type = int_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, var_expr, &var_token);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "long x = 0;\n"
                                  "x;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_variable_expression");
}

void test_code_gen_binary_expression_int_add()
{
    DEBUG_INFO("Starting test_code_gen_binary_expression_int_add");
    printf("Testing code_gen for binary int add...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token token;
    setup_basic_token(&token, TOKEN_PLUS, "+");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token left_tok, right_tok;
    setup_basic_token(&left_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&left_tok, 1);
    LiteralValue lval = {.int_value = 1};
    Expr *left = ast_create_literal_expr(&arena, lval, int_type, false, &left_tok);
    left->expr_type = int_type;

    setup_basic_token(&right_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&right_tok, 2);
    LiteralValue rval = {.int_value = 2};
    Expr *right = ast_create_literal_expr(&arena, rval, int_type, false, &right_tok);
    right->expr_type = int_type;

    Expr *bin_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &token);
    bin_expr->expr_type = int_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, bin_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "rt_add_long(1L, 2L);\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_binary_expression_int_add");
}

void test_code_gen_binary_expression_string_concat()
{
    DEBUG_INFO("Starting test_code_gen_binary_expression_string_concat");
    printf("Testing code_gen for string concat...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token token;
    setup_basic_token(&token, TOKEN_PLUS, "+");

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Token left_tok;
    setup_basic_token(&left_tok, TOKEN_STRING_LITERAL, "\"hello\"");
    token_set_string_literal(&left_tok, "hello");
    LiteralValue lval = {.string_value = "hello"};
    Expr *left = ast_create_literal_expr(&arena, lval, str_type, false, &left_tok);
    left->expr_type = str_type;

    Token right_tok;
    setup_basic_token(&right_tok, TOKEN_STRING_LITERAL, "\"world\"");
    token_set_string_literal(&right_tok, "world");
    LiteralValue rval = {.string_value = "world"};
    Expr *right = ast_create_literal_expr(&arena, rval, str_type, false, &right_tok);
    right->expr_type = str_type;

    Expr *bin_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &token);
    bin_expr->expr_type = str_type;
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, bin_expr, &token);
    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "{\n"
                                  "    char *_tmp = rt_str_concat(\"hello\", \"world\");\n"
                                  "    (void)_tmp;\n"
                                  "    rt_free_string(_tmp);\n"
                                  "}\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_binary_expression_string_concat");
}

void test_code_gen_unary_expression_negate()
{
    DEBUG_INFO("Starting test_code_gen_unary_expression_negate");
    printf("Testing code_gen for unary negate...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token token;
    setup_basic_token(&token, TOKEN_MINUS, "-");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token op_tok;
    setup_basic_token(&op_tok, TOKEN_INT_LITERAL, "5");
    token_set_int_literal(&op_tok, 5);
    LiteralValue val = {.int_value = 5};
    Expr *operand = ast_create_literal_expr(&arena, val, int_type, false, &op_tok);
    operand->expr_type = int_type;

    Expr *unary_expr = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, &token);
    unary_expr->expr_type = int_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, unary_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "rt_neg_long(5L);\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_unary_expression_negate");
}

void test_code_gen_assign_expression()
{
    DEBUG_INFO("Starting test_code_gen_assign_expression");
    printf("Testing code_gen for assign expressions...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token name_tok;
    setup_basic_token(&name_tok, TOKEN_IDENTIFIER, "x");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *init_expr = NULL;
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, name_tok, int_type, init_expr, &name_tok);

    Token val_tok;
    setup_basic_token(&val_tok, TOKEN_INT_LITERAL, "10");
    token_set_int_literal(&val_tok, 10);
    LiteralValue val = {.int_value = 10};

    Expr *value = ast_create_literal_expr(&arena, val, int_type, false, &val_tok);
    value->expr_type = int_type;

    Expr *assign_expr = ast_create_assign_expr(&arena, name_tok, value, &name_tok);
    assign_expr->expr_type = int_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, assign_expr, &name_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    const char *expected = get_expected(&arena,
                                  "long x = 0;\n"
                                  "(x = 10L);\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_assign_expression");
}

void test_code_gen_expr_main()
{
    test_code_gen_literal_expression();
    test_code_gen_variable_expression();
    test_code_gen_binary_expression_int_add();
    test_code_gen_binary_expression_string_concat();
    test_code_gen_unary_expression_negate();
    test_code_gen_assign_expression();
}
