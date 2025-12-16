// tests/code_gen_tests_array.c
// Array code generation tests

void test_code_gen_array_literal()
{
    DEBUG_INFO("Starting test_code_gen_array_literal");
    printf("Testing code_gen for array literal expressions...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");
    Token token;
    setup_basic_token(&token, TOKEN_ARRAY_LITERAL, "{1,2}");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Elements: 1, 2
    Token elem1_tok;
    setup_basic_token(&elem1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&elem1_tok, 1);
    LiteralValue val1 = {.int_value = 1};
    Expr *elem1 = ast_create_literal_expr(&arena, val1, int_type, false, &elem1_tok);
    elem1->expr_type = int_type;

    Token elem2_tok;
    setup_basic_token(&elem2_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&elem2_tok, 2);
    LiteralValue val2 = {.int_value = 2};
    Expr *elem2 = ast_create_literal_expr(&arena, val2, int_type, false, &elem2_tok);
    elem2->expr_type = int_type;

    Expr **elements = arena_alloc(&arena, 2 * sizeof(Expr *));
    elements[0] = elem1;
    elements[1] = elem2;
    int elem_count = 2;

    Expr *arr_expr = ast_create_array_expr(&arena, elements, elem_count, &token);
    arr_expr->expr_type = arr_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, arr_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // code_gen_array_expression generates rt_array_create_* for runtime arrays
    const char *expected = get_expected(&arena,
                                  "rt_array_create_long(2, (long[]){1L, 2L});\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_literal");
}

void test_code_gen_array_var_declaration_with_init()
{
    DEBUG_INFO("Starting test_code_gen_array_var_declaration_with_init");
    printf("Testing code_gen for array variable declaration with initializer...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Initializer: {3, 4}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{3,4}");

    Token elem3_tok;
    setup_basic_token(&elem3_tok, TOKEN_INT_LITERAL, "3");
    token_set_int_literal(&elem3_tok, 3);
    LiteralValue val3 = {.int_value = 3};
    Expr *elem3 = ast_create_literal_expr(&arena, val3, int_type, false, &elem3_tok);
    elem3->expr_type = int_type;

    Token elem4_tok;
    setup_basic_token(&elem4_tok, TOKEN_INT_LITERAL, "4");
    token_set_int_literal(&elem4_tok, 4);
    LiteralValue val4 = {.int_value = 4};
    Expr *elem4 = ast_create_literal_expr(&arena, val4, int_type, false, &elem4_tok);
    elem4->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 2 * sizeof(Expr *));
    init_elements[0] = elem3;
    init_elements[1] = elem4;
    int init_count = 2;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // Use the array in an expression to ensure it's generated
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    var_expr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, var_expr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * arr = rt_array_create_long(2, (long[]){3L, 4L}); arr;
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(2, (long[]){3L, 4L});\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_var_declaration_with_init");
}

void test_code_gen_array_var_declaration_without_init()
{
    DEBUG_INFO("Starting test_code_gen_array_var_declaration_without_init");
    printf("Testing code_gen for array variable declaration without initializer (default NULL)...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "empty_arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // No initializer, should default to NULL
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, NULL, &var_tok);

    // Use in expression
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    var_expr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, var_expr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * empty_arr = NULL; empty_arr;
    const char *expected = get_expected(&arena,
                                  "long * empty_arr = NULL;\n"
                                  "empty_arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_var_declaration_without_init");
}

void test_code_gen_array_access()
{
    DEBUG_INFO("Starting test_code_gen_array_access");
    printf("Testing code_gen for array access expressions...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr = {10, 20, 30}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{10,20,30}");

    Token e1_tok, e2_tok, e3_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "10");
    token_set_int_literal(&e1_tok, 10);
    LiteralValue v1 = {.int_value = 10};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    setup_basic_token(&e2_tok, TOKEN_INT_LITERAL, "20");
    token_set_int_literal(&e2_tok, 20);
    LiteralValue v2 = {.int_value = 20};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &e2_tok);
    e2->expr_type = int_type;

    setup_basic_token(&e3_tok, TOKEN_INT_LITERAL, "30");
    token_set_int_literal(&e3_tok, 30);
    LiteralValue v3 = {.int_value = 30};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &e3_tok);
    e3->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 3 * sizeof(Expr *));
    init_elements[0] = e1; init_elements[1] = e2; init_elements[2] = e3;
    int init_count = 3;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // Access: arr[1] (should be 20)
    Token access_tok;
    setup_basic_token(&access_tok, TOKEN_LEFT_BRACKET, "[");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Token idx_tok;
    setup_basic_token(&idx_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&idx_tok, 1);
    LiteralValue idx_val = {.int_value = 1};
    Expr *index = ast_create_literal_expr(&arena, idx_val, int_type, false, &idx_tok);
    index->expr_type = int_type;

    Expr *access_expr = ast_create_array_access_expr(&arena, arr_var, index, &access_tok);
    access_expr->expr_type = int_type;

    Stmt *access_stmt = ast_create_expr_stmt(&arena, access_expr, &access_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, access_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * arr = rt_array_create_long(3, (long[]){10L, 20L, 30L}); arr[1];
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(3, (long[]){10L, 20L, 30L});\n"
                                  "arr[1L];\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_access");
}

void test_code_gen_array_access_in_expression()
{
    DEBUG_INFO("Starting test_code_gen_array_access_in_expression");
    printf("Testing code_gen for array access in binary expressions...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // arr = {5, 10}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{5,10}");

    Token e1_tok, e2_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "5");
    token_set_int_literal(&e1_tok, 5);
    LiteralValue v1 = {.int_value = 5};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    setup_basic_token(&e2_tok, TOKEN_INT_LITERAL, "10");
    token_set_int_literal(&e2_tok, 10);
    LiteralValue v2 = {.int_value = 10};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &e2_tok);
    e2->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 2 * sizeof(Expr *));
    init_elements[0] = e1; init_elements[1] = e2;
    int init_count = 2;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // Binary: arr[0] + arr[1]
    Token bin_tok;
    setup_basic_token(&bin_tok, TOKEN_PLUS, "+");

    // Left: arr[0]
    Expr *arr_var_left = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var_left->expr_type = arr_type;
    Token idx0_tok;
    setup_basic_token(&idx0_tok, TOKEN_INT_LITERAL, "0");
    token_set_int_literal(&idx0_tok, 0);
    LiteralValue idx0 = {.int_value = 0};
    Expr *idx0_expr = ast_create_literal_expr(&arena, idx0, int_type, false, &idx0_tok);
    idx0_expr->expr_type = int_type;
    Expr *left_access = ast_create_array_access_expr(&arena, arr_var_left, idx0_expr, &var_tok);
    left_access->expr_type = int_type;

    // Right: arr[1]
    Expr *arr_var_right = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var_right->expr_type = arr_type;
    Token idx1_tok;
    setup_basic_token(&idx1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&idx1_tok, 1);
    LiteralValue idx1 = {.int_value = 1};
    Expr *idx1_expr = ast_create_literal_expr(&arena, idx1, int_type, false, &idx1_tok);
    idx1_expr->expr_type = int_type;
    Expr *right_access = ast_create_array_access_expr(&arena, arr_var_right, idx1_expr, &var_tok);
    right_access->expr_type = int_type;

    Expr *bin_expr = ast_create_binary_expr(&arena, left_access, TOKEN_PLUS, right_access, &bin_tok);
    bin_expr->expr_type = int_type;

    Stmt *bin_stmt = ast_create_expr_stmt(&arena, bin_expr, &bin_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, bin_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * arr = rt_array_create_long(2, (long[]){5L, 10L}); rt_add_long(arr[0], arr[1]);
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(2, (long[]){5L, 10L});\n"
                                  "rt_add_long(arr[0L], arr[1L]);\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_access_in_expression");
}

void test_code_gen_array_type_in_function_param()
{
    DEBUG_INFO("Starting test_code_gen_array_type_in_function_param");
    printf("Testing code_gen for array type in function parameters...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token fn_tok;
    setup_basic_token(&fn_tok, TOKEN_IDENTIFIER, "print_arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *void_ret = ast_create_primitive_type(&arena, TYPE_VOID);

    // Param: arr: int[]
    Parameter param;
    param.name = fn_tok; // Reuse token for name
    param.type = arr_type;

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    params[0] = param;
    int param_count = 1;

    // Empty body
    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    *body = NULL;
    int body_count = 0;

    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, params, param_count, void_ret, body, body_count, &fn_tok);

    ast_module_add_statement(&arena, &module, fn_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: void print_arr(long * arr) { ... }
    const char *expected = get_expected(&arena,
                                  "void print_arr(long * print_arr) {\n"
                                  "    goto print_arr_return;\n"
                                  "print_arr_return:\n"
                                  "    return;\n"
                                  "}\n\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_type_in_function_param");
}

void test_code_gen_array_of_arrays()
{
    DEBUG_INFO("Starting test_code_gen_array_of_arrays");
    printf("Testing code_gen for nested array types...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "nested");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *inner_arr = ast_create_array_type(&arena, int_type);
    Type *outer_arr = ast_create_array_type(&arena, inner_arr);

    // Simple init: {} (empty outer array)
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{}");
    Expr **empty_elements = NULL;
    int empty_count = 0;
    Expr *empty_init = ast_create_array_expr(&arena, empty_elements, empty_count, &init_tok);
    empty_init->expr_type = outer_arr;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, outer_arr, empty_init, &var_tok);

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    var_expr->expr_type = outer_arr;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, var_expr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * (*)[] nested = (long *[]){}; nested;
    // get_c_type for array of array: long * (*)[]
    const char *expected = get_expected(&arena,
                                  "long * (*)[] nested = (long *[]){};\n"
                                  "nested;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_of_arrays");
}

void test_code_gen_array_push_long()
{
    DEBUG_INFO("Starting test_code_gen_array_push");
    printf("Testing code_gen for array push operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr: int[] = {} (empty)
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{}");
    Expr **empty_elements = NULL;
    int empty_count = 0;
    Expr *empty_init = ast_create_array_expr(&arena, empty_elements, empty_count, &init_tok);
    empty_init->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, empty_init, &var_tok);

    // arr.push(1)
    Token push_tok;
    setup_basic_token(&push_tok, TOKEN_IDENTIFIER, "push");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);
    member->expr_type = ast_create_function_type(&arena, int_type, NULL, 0); // Assume void return for push

    Token arg_tok;
    setup_basic_token(&arg_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&arg_tok, 1);
    LiteralValue arg_val = {.int_value = 1};
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, int_type, false, &arg_tok);
    arg_expr->expr_type = int_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = arg_expr;
    int arg_count = 1;

    Expr *push_call = ast_create_call_expr(&arena, member, args, arg_count, &push_tok);
    push_call->expr_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Stmt *push_stmt = ast_create_expr_stmt(&arena, push_call, &push_tok);

    // Use arr in print or something, but for simplicity, just the push
    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, push_stmt);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: rt_array_push(arr, 1L); arr;
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(0, (long[]){});\n"
                                  "(arr = rt_array_push_long(arr, 1L));\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_push");
}

void test_code_gen_array_push_int()
{
    DEBUG_INFO("Starting test_code_gen_array_push_int");
    printf("Testing code_gen for int array push operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr: int[] = {} (empty)
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{}");
    Expr **empty_elements = NULL;
    int empty_count = 0;
    Expr *empty_init = ast_create_array_expr(&arena, empty_elements, empty_count, &init_tok);
    empty_init->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, empty_init, &var_tok);

    // arr.push(1)
    Token push_tok;
    setup_basic_token(&push_tok, TOKEN_IDENTIFIER, "push");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    member->expr_type = ast_create_function_type(&arena, void_type, NULL, 0);

    Token arg_tok;
    setup_basic_token(&arg_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&arg_tok, 1);
    LiteralValue arg_val = {.int_value = 1};
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, int_type, false, &arg_tok);
    arg_expr->expr_type = int_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = arg_expr;
    int arg_count = 1;

    Expr *push_call = ast_create_call_expr(&arena, member, args, arg_count, &push_tok);
    push_call->expr_type = void_type;

    Stmt *push_stmt = ast_create_expr_stmt(&arena, push_call, &push_tok);

    // Use arr in print or something, but for simplicity, just the push
    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, push_stmt);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: (arr = rt_array_push_long(arr, 1L)); arr;
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(0, (long[]){});\n"
                                  "(arr = rt_array_push_long(arr, 1L));\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_push_int");
}

void test_code_gen_array_push_double()
{
    DEBUG_INFO("Starting test_code_gen_array_push_double");
    printf("Testing code_gen for double array push operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *arr_type = ast_create_array_type(&arena, double_type);

    // Declare arr: double[] = {} (empty)
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{}");
    Expr **empty_elements = NULL;
    int empty_count = 0;
    Expr *empty_init = ast_create_array_expr(&arena, empty_elements, empty_count, &init_tok);
    empty_init->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, empty_init, &var_tok);

    // arr.push(1.0)
    Token push_tok;
    setup_basic_token(&push_tok, TOKEN_IDENTIFIER, "push");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    member->expr_type = ast_create_function_type(&arena, void_type, NULL, 0);

    Token arg_tok;
    setup_basic_token(&arg_tok, TOKEN_DOUBLE_LITERAL, "1.0");
    token_set_double_literal(&arg_tok, 1.0);
    LiteralValue arg_val = {.double_value = 1.0};
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, double_type, false, &arg_tok);
    arg_expr->expr_type = double_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = arg_expr;
    int arg_count = 1;

    Expr *push_call = ast_create_call_expr(&arena, member, args, arg_count, &push_tok);
    push_call->expr_type = void_type;

    Stmt *push_stmt = ast_create_expr_stmt(&arena, push_call, &push_tok);

    // Use arr in print or something, but for simplicity, just the push
    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, push_stmt);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: double * arr = rt_array_create_double(0, (double[]){}); (arr = rt_array_push_double(arr, 1.0)); arr;
    const char *expected = get_expected(&arena,
                                  "double * arr = rt_array_create_double(0, (double[]){});\n"
                                  "(arr = rt_array_push_double(arr, 1.0));\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_push_double");
}

void test_code_gen_array_push_char()
{
    DEBUG_INFO("Starting test_code_gen_array_push_char");
    printf("Testing code_gen for char array push operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *arr_type = ast_create_array_type(&arena, char_type);

    // Declare arr: char[] = {} (empty)
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{}");
    Expr **empty_elements = NULL;
    int empty_count = 0;
    Expr *empty_init = ast_create_array_expr(&arena, empty_elements, empty_count, &init_tok);
    empty_init->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, empty_init, &var_tok);

    // arr.push('a')
    Token push_tok;
    setup_basic_token(&push_tok, TOKEN_IDENTIFIER, "push");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    member->expr_type = ast_create_function_type(&arena, void_type, NULL, 0);

    Token arg_tok;
    setup_basic_token(&arg_tok, TOKEN_CHAR_LITERAL, "'a'");
    token_set_char_literal(&arg_tok, 'a');
    LiteralValue arg_val = {.char_value = 'a'};
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, char_type, false, &arg_tok);
    arg_expr->expr_type = char_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = arg_expr;
    int arg_count = 1;

    Expr *push_call = ast_create_call_expr(&arena, member, args, arg_count, &push_tok);
    push_call->expr_type = void_type;

    Stmt *push_stmt = ast_create_expr_stmt(&arena, push_call, &push_tok);

    // Use arr in print or something, but for simplicity, just the push
    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, push_stmt);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: char * arr = rt_array_create_char(0, (char[]){}); (arr = rt_array_push_char(arr, 'a')); arr;
    const char *expected = get_expected(&arena,
                                  "char * arr = rt_array_create_char(0, (char[]){});\n"
                                  "(arr = rt_array_push_char(arr, 'a'));\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_push_char");
}

void test_code_gen_array_push_bool()
{
    DEBUG_INFO("Starting test_code_gen_array_push_bool");
    printf("Testing code_gen for bool array push operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *arr_type = ast_create_array_type(&arena, bool_type);

    // Declare arr: bool[] = {} (empty)
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{}");
    Expr **empty_elements = NULL;
    int empty_count = 0;
    Expr *empty_init = ast_create_array_expr(&arena, empty_elements, empty_count, &init_tok);
    empty_init->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, empty_init, &var_tok);

    // arr.push(true)
    Token push_tok;
    setup_basic_token(&push_tok, TOKEN_IDENTIFIER, "push");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    member->expr_type = ast_create_function_type(&arena, void_type, NULL, 0);

    Token arg_tok;
    setup_basic_token(&arg_tok, TOKEN_BOOL_LITERAL, "true");
    token_set_bool_literal(&arg_tok, true);
    LiteralValue arg_val = {.bool_value = true};
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, bool_type, false, &arg_tok);
    arg_expr->expr_type = bool_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = arg_expr;
    int arg_count = 1;

    Expr *push_call = ast_create_call_expr(&arena, member, args, arg_count, &push_tok);
    push_call->expr_type = void_type;

    Stmt *push_stmt = ast_create_expr_stmt(&arena, push_call, &push_tok);

    // Use arr in print or something, but for simplicity, just the push
    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, push_stmt);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: bool * arr = rt_array_create_bool(0, (bool[]){}); (arr = rt_array_push_bool(arr, 1)); arr;
    const char *expected = get_expected(&arena,
                                  "bool * arr = rt_array_create_bool(0, (bool[]){});\n"
                                  "(arr = rt_array_push_bool(arr, 1L));\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_push_bool");
}

void test_code_gen_array_push_string()
{
    DEBUG_INFO("Starting test_code_gen_array_push_string");
    printf("Testing code_gen for string array push operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *arr_type = ast_create_array_type(&arena, string_type);

    // Declare arr: string[] = {} (empty)
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{}");
    Expr **empty_elements = NULL;
    int empty_count = 0;
    Expr *empty_init = ast_create_array_expr(&arena, empty_elements, empty_count, &init_tok);
    empty_init->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, empty_init, &var_tok);

    // arr.push("hello")
    Token push_tok;
    setup_basic_token(&push_tok, TOKEN_IDENTIFIER, "push");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    member->expr_type = ast_create_function_type(&arena, void_type, NULL, 0);

    Token arg_tok;
    setup_basic_token(&arg_tok, TOKEN_STRING_LITERAL, "\"hello\"");
    token_set_string_literal(&arg_tok, "hello");
    LiteralValue arg_val = {.string_value = "hello"};
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, string_type, false, &arg_tok);
    arg_expr->expr_type = string_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = arg_expr;
    int arg_count = 1;

    Expr *push_call = ast_create_call_expr(&arena, member, args, arg_count, &push_tok);
    push_call->expr_type = void_type;

    Stmt *push_stmt = ast_create_expr_stmt(&arena, push_call, &push_tok);

    // Use arr in print or something, but for simplicity, just the push
    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, push_stmt);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: char ** arr = rt_array_create_string(0, (char *[]){}); (arr = rt_array_push_string(arr, "hello")); arr;
    const char *expected = get_expected(&arena,
                                  "char * * arr = rt_array_create_string(0, (char *[]){});\n"
                                  "(arr = rt_array_push_string(arr, \"hello\"));\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_push_string");
}

void test_code_gen_array_clear()
{
    DEBUG_INFO("Starting test_code_gen_array_clear");
    printf("Testing code_gen for array clear operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr: int[] = {1,2}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{1,2}");

    Token e1_tok, e2_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&e1_tok, 1);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    setup_basic_token(&e2_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&e2_tok, 2);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &e2_tok);
    e2->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 2 * sizeof(Expr *));
    init_elements[0] = e1; init_elements[1] = e2;
    int init_count = 2;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // arr.clear()
    Token clear_tok;
    setup_basic_token(&clear_tok, TOKEN_IDENTIFIER, "clear");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, clear_tok, &clear_tok);
    member->expr_type = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_VOID), NULL, 0);

    Expr **no_args = NULL;
    int arg_count = 0;

    Expr *clear_call = ast_create_call_expr(&arena, member, no_args, arg_count, &clear_tok);
    clear_call->expr_type = ast_create_primitive_type(&arena, TYPE_VOID);

    Stmt *clear_stmt = ast_create_expr_stmt(&arena, clear_call, &clear_tok);

    // Use arr after clear
    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, clear_stmt);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: rt_array_clear(arr); arr;
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(2, (long[]){1L, 2L});\n"
                                  "rt_array_clear(arr);\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_clear");
}

void test_code_gen_array_concat()
{
    DEBUG_INFO("Starting test_code_gen_array_concat");
    printf("Testing code_gen for array concat operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr: int[] = {1}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{1}");

    Token e1_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&e1_tok, 1);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 1 * sizeof(Expr *));
    init_elements[0] = e1;
    int init_count = 1;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // arr.concat({2,3})
    Token concat_tok;
    setup_basic_token(&concat_tok, TOKEN_IDENTIFIER, "concat");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, concat_tok, &concat_tok);
    member->expr_type = ast_create_function_type(&arena, arr_type, NULL, 0); // Returns new array?

    // Arg: {2,3}
    Token arg_tok;
    setup_basic_token(&arg_tok, TOKEN_ARRAY_LITERAL, "{2,3}");

    Token a1_tok, a2_tok;
    setup_basic_token(&a1_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&a1_tok, 2);
    LiteralValue av1 = {.int_value = 2};
    Expr *a1 = ast_create_literal_expr(&arena, av1, int_type, false, &a1_tok);
    a1->expr_type = int_type;

    setup_basic_token(&a2_tok, TOKEN_INT_LITERAL, "3");
    token_set_int_literal(&a2_tok, 3);
    LiteralValue av2 = {.int_value = 3};
    Expr *a2 = ast_create_literal_expr(&arena, av2, int_type, false, &a2_tok);
    a2->expr_type = int_type;

    Expr **arg_elements = arena_alloc(&arena, 2 * sizeof(Expr *));
    arg_elements[0] = a1; arg_elements[1] = a2;
    int arg_count = 2;

    Expr *arg_arr = ast_create_array_expr(&arena, arg_elements, arg_count, &arg_tok);
    arg_arr->expr_type = arr_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = arg_arr;
    int num_args = 1;

    Expr *concat_call = ast_create_call_expr(&arena, member, args, num_args, &concat_tok);
    concat_call->expr_type = arr_type;

    // Assign to var or just expr
    Token res_tok;
    setup_basic_token(&res_tok, TOKEN_IDENTIFIER, "result");
    Expr *res_var = ast_create_variable_expr(&arena, res_tok, &res_tok);
    res_var->expr_type = arr_type;
    Expr *assign = ast_create_assign_expr(&arena, res_tok, concat_call, &concat_tok);

    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign, &concat_tok);

    // Use result
    Expr *use_res = ast_create_variable_expr(&arena, res_tok, &res_tok);
    use_res->expr_type = arr_type;
    Stmt *use_stmt = ast_create_expr_stmt(&arena, use_res, &res_tok);

    // Also declare result var first
    Stmt *res_decl = ast_create_var_decl_stmt(&arena, res_tok, arr_type, NULL, &res_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, res_decl);
    ast_module_add_statement(&arena, &module, assign_stmt);
    ast_module_add_statement(&arena, &module, use_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long * result = rt_array_concat_long(arr, rt_array_create_long(2, (long[]){2L, 3L})); result;
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(1, (long[]){1L});\n"
                                  "long * result = NULL;\n"
                                  "result = rt_array_concat_long(arr, rt_array_create_long(2, (long[]){2L, 3L}));\n"
                                  "result;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_concat");
}

void test_code_gen_array_length()
{
    DEBUG_INFO("Starting test_code_gen_array_length");
    printf("Testing code_gen for array length property access...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr: int[] = {1,2,3}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{1,2,3}");

    Token e1_tok, e2_tok, e3_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&e1_tok, 1);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    setup_basic_token(&e2_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&e2_tok, 2);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &e2_tok);
    e2->expr_type = int_type;

    setup_basic_token(&e3_tok, TOKEN_INT_LITERAL, "3");
    token_set_int_literal(&e3_tok, 3);
    LiteralValue v3 = {.int_value = 3};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &e3_tok);
    e3->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 3 * sizeof(Expr *));
    init_elements[0] = e1; init_elements[1] = e2; init_elements[2] = e3;
    int init_count = 3;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // arr.length
    Token length_tok;
    setup_basic_token(&length_tok, TOKEN_IDENTIFIER, "length");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *length_member = ast_create_member_expr(&arena, arr_var, length_tok, &length_tok);
    length_member->expr_type = int_type; // length returns int

    Stmt *length_stmt = ast_create_expr_stmt(&arena, length_member, &length_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, length_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: arr->length; or rt_array_length(arr);
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(3, (long[]){1L, 2L, 3L});\n"
                                  "rt_array_length(arr);\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_length");
}

void test_code_gen_array_pop()
{
    DEBUG_INFO("Starting test_code_gen_array_pop");
    printf("Testing code_gen for array pop operation...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr: int[] = {1,2,3}
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{1,2,3}");

    Token e1_tok, e2_tok, e3_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&e1_tok, 1);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    setup_basic_token(&e2_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&e2_tok, 2);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &e2_tok);
    e2->expr_type = int_type;

    setup_basic_token(&e3_tok, TOKEN_INT_LITERAL, "3");
    token_set_int_literal(&e3_tok, 3);
    LiteralValue v3 = {.int_value = 3};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &e3_tok);
    e3->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 3 * sizeof(Expr *));
    init_elements[0] = e1; init_elements[1] = e2; init_elements[2] = e3;
    int init_count = 3;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // var result: int = arr.pop()
    Token res_tok;
    setup_basic_token(&res_tok, TOKEN_IDENTIFIER, "result");

    Token pop_tok;
    setup_basic_token(&pop_tok, TOKEN_IDENTIFIER, "pop");

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr *member = ast_create_member_expr(&arena, arr_var, pop_tok, &pop_tok);
    member->expr_type = ast_create_function_type(&arena, int_type, NULL, 0);

    Expr **no_args = NULL;
    int arg_count = 0;

    Expr *pop_call = ast_create_call_expr(&arena, member, no_args, arg_count, &pop_tok);
    pop_call->expr_type = int_type;

    Stmt *res_decl = ast_create_var_decl_stmt(&arena, res_tok, int_type, pop_call, &res_tok);

    // Use result and arr
    Expr *use_res = ast_create_variable_expr(&arena, res_tok, &res_tok);
    use_res->expr_type = int_type;
    Stmt *use_res_stmt = ast_create_expr_stmt(&arena, use_res, &res_tok);

    Expr *use_arr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    use_arr->expr_type = arr_type;
    Stmt *use_arr_stmt = ast_create_expr_stmt(&arena, use_arr, &var_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, res_decl);
    ast_module_add_statement(&arena, &module, use_res_stmt);
    ast_module_add_statement(&arena, &module, use_arr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: long result = rt_array_pop(arr); result; arr;
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(3, (long[]){1L, 2L, 3L});\n"
                                  "long result = rt_array_pop(arr);\n"
                                  "result;\n"
                                  "arr;\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_pop");
}

void test_code_gen_array_print()
{
    DEBUG_INFO("Starting test_code_gen_array_print");
    printf("Testing code_gen for printing array (call with array arg)...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token print_tok;
    setup_basic_token(&print_tok, TOKEN_IDENTIFIER, "print");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    // Assume print takes any (void*), but for array

    // var arr: int[] = {1,2}
    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "arr");

    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_ARRAY_LITERAL, "{1,2}");

    Token e1_tok, e2_tok;
    setup_basic_token(&e1_tok, TOKEN_INT_LITERAL, "1");
    token_set_int_literal(&e1_tok, 1);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &e1_tok);
    e1->expr_type = int_type;

    setup_basic_token(&e2_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&e2_tok, 2);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &e2_tok);
    e2->expr_type = int_type;

    Expr **init_elements = arena_alloc(&arena, 2 * sizeof(Expr *));
    init_elements[0] = e1; init_elements[1] = e2;
    int init_count = 2;

    Expr *init_arr = ast_create_array_expr(&arena, init_elements, init_count, &init_tok);
    init_arr->expr_type = arr_type;

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, arr_type, init_arr, &var_tok);

    // print(arr)
    Expr *print_callee = ast_create_variable_expr(&arena, print_tok, &print_tok);
    print_callee->expr_type = ast_create_function_type(&arena, void_type, NULL, 0); // Assume print(void*)

    Expr *arr_var = ast_create_variable_expr(&arena, var_tok, &var_tok);
    arr_var->expr_type = arr_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr *));
    args[0] = arr_var;
    int arg_count = 1;

    Expr *print_call = ast_create_call_expr(&arena, print_callee, args, arg_count, &print_tok);
    print_call->expr_type = void_type;

    Stmt *print_stmt = ast_create_expr_stmt(&arena, print_call, &print_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, print_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    // Expected: rt_print_pointer(arr); or similar, but since print is extern, assuming rt_print_array(arr);
    const char *expected = get_expected(&arena,
                                  "long * arr = rt_array_create_long(2, (long[]){1L, 2L});\n"
                                  "rt_print_array(arr);\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_array_print");
}

void test_code_gen_array_main()
{
    test_code_gen_array_literal();
    test_code_gen_array_var_declaration_with_init();
    test_code_gen_array_var_declaration_without_init();
    test_code_gen_array_access();
    test_code_gen_array_access_in_expression();
    test_code_gen_array_type_in_function_param();
    test_code_gen_array_of_arrays();
    test_code_gen_array_push_long();
    test_code_gen_array_push_int();
    test_code_gen_array_push_double();
    test_code_gen_array_push_char();
    test_code_gen_array_push_string();
    test_code_gen_array_push_bool();
    //test_code_gen_array_clear();
    //test_code_gen_array_concat();
    //test_code_gen_array_length();
    //test_code_gen_array_pop();
    //test_code_gen_array_print();
}
