// tests/code_gen_tests.c

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../arena.h"
#include "../debug.h"
#include "../code_gen.h"
#include "../ast.h"
#include "../token.h"
#include "../symbol_table.h"
#include "../file.h"

static const char *test_output_path = "test_output.c";
static const char *expected_output_path = "expected_output.c";

const char *get_expected(Arena *arena, const char *expected)
{
    const char *header =
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include <stdio.h>\n\n"
        "extern char *rt_str_concat(char *, char *);\n"
        "extern void rt_print_long(long);\n"
        "extern void rt_print_double(double);\n"
        "extern void rt_print_char(long);\n"
        "extern void rt_print_string(char *);\n"
        "extern void rt_print_bool(long);\n"
        "extern long rt_add_long(long, long);\n"
        "extern long rt_sub_long(long, long);\n"
        "extern long rt_mul_long(long, long);\n"
        "extern long rt_div_long(long, long);\n"
        "extern long rt_mod_long(long, long);\n"
        "extern long rt_eq_long(long, long);\n"
        "extern long rt_ne_long(long, long);\n"
        "extern long rt_lt_long(long, long);\n"
        "extern long rt_le_long(long, long);\n"
        "extern long rt_gt_long(long, long);\n"
        "extern long rt_ge_long(long, long);\n"
        "extern double rt_add_double(double, double);\n"
        "extern double rt_sub_double(double, double);\n"
        "extern double rt_mul_double(double, double);\n"
        "extern double rt_div_double(double, double);\n"
        "extern long rt_eq_double(double, double);\n"
        "extern long rt_ne_double(double, double);\n"
        "extern long rt_lt_double(double, double);\n"
        "extern long rt_le_double(double, double);\n"
        "extern long rt_gt_double(double, double);\n"
        "extern long rt_ge_double(double, double);\n"
        "extern long rt_neg_long(long);\n"
        "extern double rt_neg_double(double);\n"
        "extern long rt_not_bool(long);\n"
        "extern long rt_post_inc_long(long *);\n"
        "extern long rt_post_dec_long(long *);\n"
        "extern char *rt_to_string_long(long);\n"
        "extern char *rt_to_string_double(double);\n"
        "extern char *rt_to_string_char(long);\n"
        "extern char *rt_to_string_bool(long);\n"
        "extern char *rt_to_string_string(char *);\n"
        "extern long rt_eq_string(char *, char *);\n"
        "extern long rt_ne_string(char *, char *);\n"
        "extern long rt_lt_string(char *, char *);\n"
        "extern long rt_le_string(char *, char *);\n"
        "extern long rt_gt_string(char *, char *);\n"
        "extern long rt_ge_string(char *, char *);\n"
        "extern void rt_free_string(char *);\n\n";

    size_t total_len = strlen(header) + strlen(expected) + 1;
    char *expected_result = arena_alloc(arena, total_len);
    snprintf(expected_result, total_len, "%s%s", header, expected);

    return expected_result;
}

void create_expected_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    if (content)
    {
        size_t len = strlen(content);
        size_t written = fwrite(content, 1, len, file);
        assert(written == len);
    }
    fclose(file);
}

void remove_test_file(const char *path)
{
    remove(path);
}

void compare_output_files(const char *actual_path, const char *expected_path)
{
    DEBUG_VERBOSE("Entering compare_output_files with actual_path=%s, expected_path=%s", actual_path, expected_path);
    
    Arena read_arena;
    DEBUG_VERBOSE("Initializing arena with size=1MB");
    arena_init(&read_arena, 1024 * 1024);
    
    DEBUG_VERBOSE("Reading actual file: %s", actual_path);
    char *actual = file_read(&read_arena, actual_path);
    DEBUG_VERBOSE("Actual file contents: %s", actual ? actual : "NULL");
    
    DEBUG_VERBOSE("Reading expected file: %s", expected_path);
    char *expected = file_read(&read_arena, expected_path);
    DEBUG_VERBOSE("Expected file contents: %s", expected ? expected : "NULL");
    
    DEBUG_VERBOSE("Checking if file contents are non-null");
    assert(actual != NULL && expected != NULL);
    
    DEBUG_VERBOSE("Comparing file contents");
    assert(strcmp(actual, expected) == 0);
    
    DEBUG_VERBOSE("Freeing arena");
    arena_free(&read_arena);
}

void setup_basic_token(Token *token, TokenType type, const char *lexeme)
{
    token_init(token, type, lexeme, (int)strlen(lexeme), 1, "test.sn");
}

void test_code_gen_init_invalid_output_file()
{
    DEBUG_INFO("Starting test_code_gen_init_invalid_output_file");
    printf("Testing code_gen_init with invalid output path...\n");

    Arena arena;
    arena_init(&arena, 1024);
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    const char *invalid_path = "/invalid/path/test.c";
    code_gen_init(&arena, &gen, &sym_table, invalid_path);
    assert(gen.output == NULL); // fopen fails

    symbol_table_cleanup(&sym_table);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_init_invalid_output_file");
}

void test_code_gen_cleanup_null_output()
{
    DEBUG_INFO("Starting test_code_gen_cleanup_null_output");
    printf("Testing code_gen_cleanup with NULL output...\n");

    Arena arena;
    arena_init(&arena, 1024);
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    gen.output = NULL; // Simulate

    code_gen_cleanup(&gen); // Should do nothing

    symbol_table_cleanup(&sym_table);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_cleanup_null_output");
}

void test_code_gen_headers_and_externs()
{
    DEBUG_INFO("Starting test_code_gen_headers_and_externs");
    printf("Testing code_gen_headers and code_gen_externs...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");
    code_gen_module(&gen, &module);

    // Expected with full headers and externs + dummy main
    char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_headers_and_externs");
}

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

    char *expected = get_expected(&arena,
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

    char *expected = get_expected(&arena,
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

    char *expected = get_expected(&arena,
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

    // Note: Assuming the exact format from code_gen_binary_expression for strings uses rt_str_concat with temps and frees
    // For this test, using a placeholder based on the provided code snippet; adjust if full implementation differs
    char *expected = get_expected(&arena,
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

    char *expected = get_expected(&arena,
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

    char *expected = get_expected(&arena,
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

void test_code_gen_call_expression_simple()
{
    DEBUG_INFO("Starting test_code_gen_call_expression_simple");
    printf("Testing code_gen for call expressions...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token callee_tok;
    setup_basic_token(&callee_tok, TOKEN_IDENTIFIER, "print");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    Expr *callee = ast_create_variable_expr(&arena, callee_tok, &callee_tok);
    callee->expr_type = void_type;

    Token string_tok;
    setup_basic_token(&string_tok, TOKEN_STR, "\"Hello, world!\"");
    LiteralValue str_value;
    str_value.string_value = "Hello, world!"; 
    Expr *string_expr = ast_create_literal_expr(&arena, str_value, string_type, false, &string_tok);
    string_expr->expr_type = string_type;

    Expr **args = arena_alloc(&arena, sizeof(Expr*) * 1);
    args[0] = string_expr;
    int arg_count = 1;

    Expr *call_expr = ast_create_call_expr(&arena, callee, args, arg_count, &callee_tok);
    call_expr->expr_type = void_type;

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, call_expr, &callee_tok);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "rt_print_string(\"Hello, world!\");\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_call_expression_simple");
}

void test_code_gen_function_simple_void()
{
    DEBUG_INFO("Starting test_code_gen_function_simple_void");
    printf("Testing code_gen for simple void function...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token fn_tok;
    setup_basic_token(&fn_tok, TOKEN_IDENTIFIER, "myfn");

    Parameter *params = NULL;
    int param_count = 0;
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt **body = NULL;
    int body_count = 0;

    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, params, param_count, void_type, body, body_count, &fn_tok);

    ast_module_add_statement(&arena, &module, fn_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "void myfn() {\n"
                                  "    goto myfn_return;\n"
                                  "myfn_return:\n"
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

    DEBUG_INFO("Finished test_code_gen_function_simple_void");
}

void test_code_gen_function_with_params_and_return()
{
    DEBUG_INFO("Starting test_code_gen_function_with_params_and_return");
    printf("Testing code_gen for function with params and return...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token fn_tok;
    setup_basic_token(&fn_tok, TOKEN_IDENTIFIER, "add");

    // Params
    Token param_tok;
    setup_basic_token(&param_tok, TOKEN_IDENTIFIER, "a");
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Parameter param = {.name = param_tok, .type = int_type};

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    *params = param;
    int param_count = 1;

    // Return type int
    Type *ret_type = ast_create_primitive_type(&arena, TYPE_INT);

    // Body: return a;
    Token ret_tok;
    setup_basic_token(&ret_tok, TOKEN_RETURN, "return");

    Expr *var_expr = ast_create_variable_expr(&arena, param_tok, &param_tok);
    var_expr->expr_type = int_type;

    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, var_expr, &ret_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    *body = ret_stmt;
    int body_count = 1;

    Stmt *fn_stmt = ast_create_function_stmt(&arena, fn_tok, params, param_count, ret_type, body, body_count, &fn_tok);

    ast_module_add_statement(&arena, &module, fn_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "long add(long a) {\n"
                                  "    long _return_value = 0;\n"
                                  "    _return_value = a;\n"
                                  "    goto add_return;\n"
                                  "add_return:\n"
                                  "    return _return_value;\n"
                                  "}\n\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_function_with_params_and_return");
}

void test_code_gen_main_function_special_case()
{
    DEBUG_INFO("Starting test_code_gen_main_function_special_case");
    printf("Testing code_gen for main function (int return)...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token main_tok;
    setup_basic_token(&main_tok, TOKEN_IDENTIFIER, "main");

    Parameter *params = NULL;
    int param_count = 0;
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID); // But code_gen uses int for main
    Stmt **body = NULL;
    int body_count = 0;

    Stmt *main_stmt = ast_create_function_stmt(&arena, main_tok, params, param_count, void_type, body, body_count, &main_tok);

    ast_module_add_statement(&arena, &module, main_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    int _return_value = 0;\n"
                                  "    goto main_return;\n"
                                  "main_return:\n"
                                  "    return _return_value;\n"
                                  "}\n\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_main_function_special_case");
}

void test_code_gen_block_statement()
{
    DEBUG_INFO("Starting test_code_gen_block_statement");
    printf("Testing code_gen for block statements...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    // Block with var decl
    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "block_var");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, NULL, &var_tok);

    Stmt **stmts = arena_alloc(&arena, sizeof(Stmt *));
    *stmts = var_decl;
    int count = 1;

    Token block_tok;
    setup_basic_token(&block_tok, TOKEN_LEFT_BRACE, "{");
    Stmt *block = ast_create_block_stmt(&arena, stmts, count, &block_tok);

    ast_module_add_statement(&arena, &module, block);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "{\n"
                                  "    long block_var = 0;\n"
                                  "}\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_block_statement");
}

void test_code_gen_if_statement()
{
    DEBUG_INFO("Starting test_code_gen_if_statement");
    printf("Testing code_gen for if statements...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token if_tok;
    setup_basic_token(&if_tok, TOKEN_IF, "if");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Token cond_tok;
    setup_basic_token(&cond_tok, TOKEN_BOOL_LITERAL, "true");
    token_set_bool_literal(&cond_tok, 1);
    LiteralValue bval = {.bool_value = 1};
    Expr *cond = ast_create_literal_expr(&arena, bval, bool_type, false, &cond_tok);
    cond->expr_type = bool_type;

    Token then_tok;
    setup_basic_token(&then_tok, TOKEN_IDENTIFIER, "print");
    Expr *dummy_expr = ast_create_variable_expr(&arena, then_tok, &then_tok);
    dummy_expr->expr_type = bool_type;
    
    Stmt *then_stmt = ast_create_expr_stmt(&arena, dummy_expr, &then_tok);

    Stmt *if_stmt = ast_create_if_stmt(&arena, cond, then_stmt, NULL, &if_tok);

    ast_module_add_statement(&arena, &module, if_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "if (1L) {\n"
                                  "    print;\n"
                                  "}\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_if_statement");
}

void test_code_gen_while_statement()
{
    DEBUG_INFO("Starting test_code_gen_while_statement");
    printf("Testing code_gen for while statements...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token while_tok;
    setup_basic_token(&while_tok, TOKEN_WHILE, "while");

    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Token cond_tok;
    setup_basic_token(&cond_tok, TOKEN_BOOL_LITERAL, "true");
    token_set_bool_literal(&cond_tok, 1);
    LiteralValue bval = {.bool_value = 1};
    Expr *cond = ast_create_literal_expr(&arena, bval, bool_type, false, &cond_tok);
    cond->expr_type = bool_type;

    Token body_tok;
    setup_basic_token(&body_tok, TOKEN_IDENTIFIER, "print");
    Expr *body_expr = ast_create_variable_expr(&arena, body_tok, &body_tok);
    body_expr->expr_type = bool_type;

    Stmt *body = ast_create_expr_stmt(&arena, body_expr, &body_tok);

    Stmt *while_stmt = ast_create_while_stmt(&arena, cond, body, &while_tok);

    ast_module_add_statement(&arena, &module, while_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "while (1L) {\n"
                                  "    print;\n"
                                  "}\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_while_statement");
}

void test_code_gen_for_statement()
{
    DEBUG_INFO("Starting test_code_gen_for_statement");
    printf("Testing code_gen for for statements...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token for_tok;
    setup_basic_token(&for_tok, TOKEN_FOR, "for");

    // Initializer: var k: int = 0
    Token init_var_tok;
    setup_basic_token(&init_var_tok, TOKEN_IDENTIFIER, "k");
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token init_val_tok;
    setup_basic_token(&init_val_tok, TOKEN_INT_LITERAL, "0");
    token_set_int_literal(&init_val_tok, 0);
    LiteralValue ival = {.int_value = 0};
    Expr *init_val = ast_create_literal_expr(&arena, ival, int_type, false, &init_val_tok);
    init_val->expr_type = int_type;

    Stmt *init_stmt = ast_create_var_decl_stmt(&arena, init_var_tok, int_type, init_val, &init_var_tok);

    // Condition: k < 5
    Token cond_left_tok;
    setup_basic_token(&cond_left_tok, TOKEN_IDENTIFIER, "k");
    Expr *cond_left = ast_create_variable_expr(&arena, cond_left_tok, &cond_left_tok);
    cond_left->expr_type = int_type;
    Token cond_right_tok;
    setup_basic_token(&cond_right_tok, TOKEN_INT_LITERAL, "5");
    token_set_int_literal(&cond_right_tok, 5);
    LiteralValue cval = {.int_value = 5};
    Expr *cond_right = ast_create_literal_expr(&arena, cval, int_type, false, &cond_right_tok);
    cond_right->expr_type = int_type;
    Token cond_op_tok;
    setup_basic_token(&cond_op_tok, TOKEN_LESS, "<");
    Expr *cond = ast_create_binary_expr(&arena, cond_left, TOKEN_LESS, cond_right, &cond_op_tok);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    cond->expr_type = bool_type;

    // Increment: k++
    Token inc_tok;
    setup_basic_token(&inc_tok, TOKEN_IDENTIFIER, "k");
    Expr *inc_var = ast_create_variable_expr(&arena, inc_tok, &inc_tok);
    inc_var->expr_type = int_type;
    Expr *inc_expr = ast_create_increment_expr(&arena, inc_var, &inc_tok);
    inc_expr->expr_type = int_type;

    // Body: print(k)  [call to builtin print with arg k]
    Token body_tok;
    setup_basic_token(&body_tok, TOKEN_IDENTIFIER, "print");
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Expr *callee_print = ast_create_variable_expr(&arena, body_tok, &body_tok);
    // Type callee as function: void print(int)
    Type *print_param_types[1] = {int_type};
    Type *print_func_type = ast_create_function_type(&arena, void_type, print_param_types, 1);
    callee_print->expr_type = print_func_type;
    
    // Arg: k
    Token arg_k_tok;
    setup_basic_token(&arg_k_tok, TOKEN_IDENTIFIER, "k");
    Expr *arg_k = ast_create_variable_expr(&arena, arg_k_tok, &arg_k_tok);
    arg_k->expr_type = int_type;
    
    Expr *print_call = ast_create_call_expr(&arena, callee_print, &arg_k, 1, &body_tok);
    print_call->expr_type = void_type;
    
    Stmt *body = ast_create_expr_stmt(&arena, print_call, &body_tok);

    Stmt *for_stmt = ast_create_for_stmt(&arena, init_stmt, cond, inc_expr, body, &for_tok);

    ast_module_add_statement(&arena, &module, for_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "{\n"
                                  "    long k = 0L;\n"
                                  "    while (rt_lt_long(k, 5L)) {\n"
                                  "        rt_print_long(k);\n"
                                  "        rt_post_inc_long(&k);\n"
                                  "    }\n"
                                  "}\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_for_statement");
}

void test_code_gen_string_free_in_block()
{
    DEBUG_INFO("Starting test_code_gen_string_free_in_block");
    printf("Testing string freeing in blocks...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token str_tok;
    setup_basic_token(&str_tok, TOKEN_IDENTIFIER, "s");

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Token init_tok;
    setup_basic_token(&init_tok, TOKEN_STRING_LITERAL, "\"test\"");
    token_set_string_literal(&init_tok, "test");
    LiteralValue sval = {.string_value = "test"};
    Expr *init = ast_create_literal_expr(&arena, sval, str_type, false, &init_tok);
    Stmt *str_decl = ast_create_var_decl_stmt(&arena, str_tok, str_type, init, &str_tok);

    Stmt **block_stmts = arena_alloc(&arena, sizeof(Stmt *));
    *block_stmts = str_decl;
    int bcount = 1;

    Token block_tok;
    setup_basic_token(&block_tok, TOKEN_LEFT_BRACE, "{");
    Stmt *block = ast_create_block_stmt(&arena, block_stmts, bcount, &block_tok);

    ast_module_add_statement(&arena, &module, block);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "{\n"
                                  "    char * s = \"test\";\n"
                                  "    if (s) {\n"
                                  "        rt_free_string(s);\n"
                                  "    }\n"
                                  "}\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_string_free_in_block");
}

void test_code_gen_increment_decrement()
{
    DEBUG_INFO("Starting test_code_gen_increment_decrement");
    printf("Testing code_gen for ++ -- ...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "counter");

    // Var decl first
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, NULL, &var_tok);

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    var_expr->expr_type = int_type;
    Expr *inc_expr = ast_create_increment_expr(&arena, var_expr, &var_tok);
    inc_expr->expr_type = int_type;
    Stmt *inc_stmt = ast_create_expr_stmt(&arena, inc_expr, &var_tok);

    ast_module_add_statement(&arena, &module, decl);
    ast_module_add_statement(&arena, &module, inc_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "long counter = 0;\n"
                                  "rt_post_inc_long(&counter);\n"
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_increment_decrement");
}

void test_code_gen_null_expression()
{
    DEBUG_INFO("Starting test_code_gen_null_expression");
    printf("Testing code_gen_expression with NULL...\n");

    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token null_tok;
    setup_basic_token(&null_tok, TOKEN_NIL, "nil");
    Stmt *null_stmt = ast_create_expr_stmt(&arena, NULL, &null_tok);

    ast_module_add_statement(&arena, &module, null_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_null_expression");
}

void test_code_gen_new_label()
{
    DEBUG_INFO("Starting test_code_gen_new_label");
    printf("Testing code_gen_new_label...\n");

    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    int label1 = code_gen_new_label(&gen);
    int label2 = code_gen_new_label(&gen);

    assert(label1 == 0);
    assert(label2 == 1);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    remove_test_file(test_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_new_label");
}

void test_code_gen_module_no_main_adds_dummy()
{
    DEBUG_INFO("Starting test_code_gen_module_no_main_adds_dummy");
    printf("Testing code_gen_module adds dummy main if none...\n");

    Arena arena;
    arena_init(&arena, 1024);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    // Empty

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_module_no_main_adds_dummy");
}


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

    // Assuming code_gen_array_expression generates something like "(long[]){1L, 2L}"
    // Adjust expected based on actual implementation; here assuming pointer cast for dynamic array.
    char *expected = get_expected(&arena,
                                  "(long[]){1L, 2L};\n"
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

    // Expected: long * arr = (long[]){3L, 4L}; arr; (adjust based on actual get_c_type and init)
    // Assuming "long * arr = (long[]){3L, 4L};" for dynamic array simulation.
    char *expected = get_expected(&arena,
                                  "long * arr = (long[]){3L, 4L};\n"
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
    char *expected = get_expected(&arena,
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

    // Expected: long * arr = (long[]){10L, 20L, 30L}; arr[1];
    char *expected = get_expected(&arena,
                                  "long * arr = (long[]){10L, 20L, 30L};\n"
                                  "arr[1];\n"
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

    // Expected: long * arr = (long[]){5L, 10L}; rt_add_long(arr[0], arr[1]);
    char *expected = get_expected(&arena,
                                  "long * arr = (long[]){5L, 10L};\n"
                                  "rt_add_long(arr[0], arr[1]);\n"
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
    char *expected = get_expected(&arena,
                                  "void print_arr(long * arr) {\n"
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

    // Expected: long * (*)[] nested = (long * (*)[]){}; nested; (adjust for nested pointer types)
    // get_c_type for array of array: ((long[])[]) or long * (*)[]
    char *expected = get_expected(&arena,
                                  "long * (*)[] nested = (long * (*)[]){};\n"
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

void test_code_gen_main()
{
    test_code_gen_cleanup_null_output();
    test_code_gen_headers_and_externs();
    test_code_gen_literal_expression();
    test_code_gen_variable_expression();
    test_code_gen_binary_expression_int_add();
    test_code_gen_binary_expression_string_concat();
    test_code_gen_unary_expression_negate();
    test_code_gen_assign_expression();
    test_code_gen_call_expression_simple();
    test_code_gen_function_simple_void();
    test_code_gen_function_with_params_and_return();
    test_code_gen_main_function_special_case();
    test_code_gen_block_statement();
    test_code_gen_if_statement();
    test_code_gen_while_statement();
    test_code_gen_for_statement();
    test_code_gen_string_free_in_block();
    test_code_gen_increment_decrement();
    test_code_gen_null_expression();
    test_code_gen_new_label();
    test_code_gen_module_no_main_adds_dummy();
    test_code_gen_array_literal();
    test_code_gen_array_var_declaration_with_init();
    test_code_gen_array_var_declaration_without_init();
    test_code_gen_array_access();
    test_code_gen_array_access_in_expression();
    test_code_gen_array_type_in_function_param();
    test_code_gen_array_of_arrays();
}