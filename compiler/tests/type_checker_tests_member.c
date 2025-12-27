// tests/type_checker_tests_member.c
// Array member access type checker tests (length, push, pop, clear, concat)

void test_type_check_array_member_length()
{
    DEBUG_INFO("Starting test_type_check_array_member_length");
    printf("Testing type check for array.length member access...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit1_tok, lit2_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);
    Expr *elements[2] = {e1, e2};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 2, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Token len_tok;
    setup_token(&len_tok, TOKEN_IDENTIFIER, "len", 2, "test.sn", &arena);
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token member_tok;
    setup_token(&member_tok, TOKEN_IDENTIFIER, "length", 2, "test.sn", &arena);
    Expr *member = ast_create_member_expr(&arena, var_arr, member_tok, NULL);
    Stmt *len_decl = ast_create_var_decl_stmt(&arena, len_tok, int_type, member, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, len_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(member->expr_type != NULL);
    assert(ast_type_equals(member->expr_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_member_length");
}

void test_type_check_array_member_invalid()
{
    DEBUG_INFO("Starting test_type_check_array_member_invalid");
    printf("Testing type check for invalid array member access...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit_tok);
    Expr *elements[1] = {e1};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 1, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token invalid_tok;
    setup_token(&invalid_tok, TOKEN_IDENTIFIER, "invalid", 2, "test.sn", &arena);
    Expr *member = ast_create_member_expr(&arena, var_arr, invalid_tok, NULL);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, member, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_member_invalid");
}

void test_type_check_array_member_push()
{
    DEBUG_INFO("Starting test_type_check_array_member_push");
    printf("Testing type check for array.push member access...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    Expr *elements[1] = {e1};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 1, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token push_tok;
    setup_token(&push_tok, TOKEN_IDENTIFIER, "push", 2, "test.sn", &arena);
    Expr *push_member = ast_create_member_expr(&arena, var_arr, push_tok, NULL);
    Stmt *dummy_stmt = ast_create_expr_stmt(&arena, push_member, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, dummy_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(push_member->expr_type != NULL);
    assert(push_member->expr_type->kind == TYPE_FUNCTION);
    assert(push_member->expr_type->as.function.param_count == 1);
    assert(push_member->expr_type->as.function.param_types[0]->kind == TYPE_INT);
    assert(push_member->expr_type->as.function.return_type->kind == TYPE_VOID);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_member_push");
}

void test_type_check_array_member_pop()
{
    DEBUG_INFO("Starting test_type_check_array_member_pop");
    printf("Testing type check for array.pop member access...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    Expr *elements[1] = {e1};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 1, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token pop_tok;
    setup_token(&pop_tok, TOKEN_IDENTIFIER, "pop", 2, "test.sn", &arena);
    Expr *pop_member = ast_create_member_expr(&arena, var_arr, pop_tok, NULL);
    Stmt *dummy_stmt = ast_create_expr_stmt(&arena, pop_member, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, dummy_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(pop_member->expr_type != NULL);
    assert(pop_member->expr_type->kind == TYPE_FUNCTION);
    assert(pop_member->expr_type->as.function.param_count == 0);
    assert(pop_member->expr_type->as.function.return_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_member_pop");
}

void test_type_check_array_member_clear()
{
    DEBUG_INFO("Starting test_type_check_array_member_clear");
    printf("Testing type check for array.clear member access...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);

    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token clear_tok;
    setup_token(&clear_tok, TOKEN_IDENTIFIER, "clear", 2, "test.sn", &arena);
    Expr *clear_member = ast_create_member_expr(&arena, var_arr, clear_tok, NULL);
    Stmt *dummy_stmt = ast_create_expr_stmt(&arena, clear_member, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, dummy_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(clear_member->expr_type != NULL);
    assert(clear_member->expr_type->kind == TYPE_FUNCTION);
    assert(clear_member->expr_type->as.function.param_count == 0);
    assert(clear_member->expr_type->as.function.return_type->kind == TYPE_VOID);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_member_clear");
}

void test_type_check_array_member_concat()
{
    DEBUG_INFO("Starting test_type_check_array_member_concat");
    printf("Testing type check for array.concat member access...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    Expr *elements[1] = {e1};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 1, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token concat_tok;
    setup_token(&concat_tok, TOKEN_IDENTIFIER, "concat", 2, "test.sn", &arena);
    Expr *concat_member = ast_create_member_expr(&arena, var_arr, concat_tok, NULL);
    Stmt *dummy_stmt = ast_create_expr_stmt(&arena, concat_member, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, dummy_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(concat_member->expr_type != NULL);
    assert(concat_member->expr_type->kind == TYPE_FUNCTION);
    assert(concat_member->expr_type->as.function.param_count == 1);
    assert(concat_member->expr_type->as.function.param_types[0]->kind == TYPE_ARRAY);
    assert(concat_member->expr_type->as.function.param_types[0]->as.array.element_type->kind == TYPE_INT);
    assert(concat_member->expr_type->as.function.return_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(concat_member->expr_type->as.function.return_type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_member_concat");
}

void test_type_check_array_printable()
{
    DEBUG_INFO("Starting test_type_check_array_printable");
    printf("Testing type check for array as printable type...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit1_tok, lit2_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);
    Expr *elements[2] = {e1, e2};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 2, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    Token interp_tok;
    setup_token(&interp_tok, TOKEN_INTERPOL_STRING, "$\"{arr}\"", 2, "test.sn", &arena);
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Expr *parts[1] = {var_arr};
    char *fmts[1] = {NULL};
    Expr *interp = ast_create_interpolated_expr(&arena, parts, fmts, 1, &interp_tok);
    Stmt *interp_stmt = ast_create_expr_stmt(&arena, interp, &interp_tok);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, interp_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    assert(interp->expr_type != NULL);
    assert(interp->expr_type->kind == TYPE_STRING);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_printable");
}

void test_type_checker_member_main()
{
    test_type_check_array_member_length();
    test_type_check_array_member_invalid();
    test_type_check_array_member_push();
    test_type_check_array_member_pop();
    test_type_check_array_member_clear();
    test_type_check_array_member_concat();
    test_type_check_array_printable();
}
