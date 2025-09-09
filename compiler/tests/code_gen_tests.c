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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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

    setup_basic_token(&right_tok, TOKEN_INT_LITERAL, "2");
    token_set_int_literal(&right_tok, 2);
    LiteralValue rval = {.int_value = 2};
    Expr *right = ast_create_literal_expr(&arena, rval, int_type, false, &right_tok);

    Expr *bin_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &token);
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, bin_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "rt_add_long(1L, 2L);\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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

    Token right_tok;
    setup_basic_token(&right_tok, TOKEN_STRING_LITERAL, "\"world\"");
    token_set_string_literal(&right_tok, "world");
    LiteralValue rval = {.string_value = "world"};
    Expr *right = ast_create_literal_expr(&arena, rval, str_type, false, &right_tok);

    Expr *bin_expr = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, &token);
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
                                  "}\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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

    Expr *unary_expr = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, &token);
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, unary_expr, &token);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "rt_neg_long(5L);\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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

    Expr *assign_expr = ast_create_assign_expr(&arena, name_tok, value, &name_tok);
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, assign_expr, &name_tok);

    ast_module_add_statement(&arena, &module, var_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "long x = 0;\n"
                                  "x = 10L;\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token callee_tok;
    setup_basic_token(&callee_tok, TOKEN_IDENTIFIER, "print");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Expr *callee = ast_create_variable_expr(&arena, callee_tok, &callee_tok);

    Expr **args = NULL;
    int arg_count = 0;

    Expr *call_expr = ast_create_call_expr(&arena, callee, args, arg_count, &callee_tok);
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, call_expr, &callee_tok);

    ast_module_add_statement(&arena, &module, expr_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "print();\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
                                  "}\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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

    Token then_tok;
    setup_basic_token(&then_tok, TOKEN_IDENTIFIER, "print");
    Expr *dummy_expr = ast_create_variable_expr(&arena, then_tok, &then_tok);
    Stmt *then_stmt = ast_create_expr_stmt(&arena, dummy_expr, &then_tok);

    Stmt *if_stmt = ast_create_if_stmt(&arena, cond, then_stmt, NULL, &if_tok);

    ast_module_add_statement(&arena, &module, if_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "if (1L) {\n"
                                  "    print;\n"
                                  "}\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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

    Token body_tok;
    setup_basic_token(&body_tok, TOKEN_IDENTIFIER, "print");
    Expr *body_expr = ast_create_variable_expr(&arena, body_tok, &body_tok);
    Stmt *body = ast_create_expr_stmt(&arena, body_expr, &body_tok);

    Stmt *while_stmt = ast_create_while_stmt(&arena, cond, body, &while_tok);

    ast_module_add_statement(&arena, &module, while_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "while (1L) {\n"
                                  "    print;\n"
                                  "}\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
    Stmt *init_stmt = ast_create_var_decl_stmt(&arena, init_var_tok, int_type, init_val, &init_var_tok);

    // Condition: k < 5
    Token cond_left_tok;
    setup_basic_token(&cond_left_tok, TOKEN_IDENTIFIER, "k");
    Expr *cond_left = ast_create_variable_expr(&arena, cond_left_tok, &cond_left_tok);
    Token cond_right_tok;
    setup_basic_token(&cond_right_tok, TOKEN_INT_LITERAL, "5");
    token_set_int_literal(&cond_right_tok, 5);
    LiteralValue cval = {.int_value = 5};
    Expr *cond_right = ast_create_literal_expr(&arena, cval, int_type, false, &cond_right_tok);
    Token cond_op_tok;
    setup_basic_token(&cond_op_tok, TOKEN_LESS, "<");
    Expr *cond = ast_create_binary_expr(&arena, cond_left, TOKEN_LESS, cond_right, &cond_op_tok);

    // Increment: k++
    Token inc_tok;
    setup_basic_token(&inc_tok, TOKEN_IDENTIFIER, "k");
    Expr *inc_var = ast_create_variable_expr(&arena, inc_tok, &inc_tok);
    Expr *inc_expr = ast_create_increment_expr(&arena, inc_var, &inc_tok);

    // Body: simple print
    Token body_tok;
    setup_basic_token(&body_tok, TOKEN_IDENTIFIER, "print");
    Expr *body_expr = ast_create_variable_expr(&arena, body_tok, &body_tok);
    Stmt *body = ast_create_expr_stmt(&arena, body_expr, &body_tok);

    Stmt *for_stmt = ast_create_for_stmt(&arena, init_stmt, cond, inc_expr, body, &for_tok);

    ast_module_add_statement(&arena, &module, for_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "{\n"
                                  "    long k = 0L;\n"
                                  "    while (rt_lt_long(k, 5L)) {\n"
                                  "        print;\n"
                                  "        rt_post_inc_long(&k);\n"
                                  "    }\n"
                                  "}\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
                                  "}\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    code_gen_init(&arena, &gen, &sym_table, test_output_path);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token var_tok;
    setup_basic_token(&var_tok, TOKEN_IDENTIFIER, "counter");

    // Var decl first
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Stmt *decl = ast_create_var_decl_stmt(&arena, var_tok, int_type, NULL, &var_tok);

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Expr *inc_expr = ast_create_increment_expr(&arena, var_expr, &var_tok);
    Stmt *inc_stmt = ast_create_expr_stmt(&arena, inc_expr, &var_tok);

    ast_module_add_statement(&arena, &module, decl);
    ast_module_add_statement(&arena, &module, inc_stmt);

    code_gen_module(&gen, &module);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    char *expected = get_expected(&arena,
                                  "long counter = 0;\n"
                                  "rt_post_inc_long(&counter);\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
                                  "0L;\n\n"
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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

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

void test_code_gen_main()
{
    // test_code_gen_cleanup_null_output();
    // test_code_gen_headers_and_externs();
    // test_code_gen_literal_expression();
    test_code_gen_variable_expression();
    // test_code_gen_binary_expression_int_add();
    // test_code_gen_binary_expression_string_concat();
    // test_code_gen_unary_expression_negate();
    // test_code_gen_assign_expression();
    // test_code_gen_call_expression_simple();
    // test_code_gen_function_simple_void();
    // test_code_gen_function_with_params_and_return();
    // test_code_gen_main_function_special_case();
    // test_code_gen_block_statement();
    // test_code_gen_if_statement();
    // test_code_gen_while_statement();
    // test_code_gen_for_statement();
    // test_code_gen_string_free_in_block();
    // test_code_gen_increment_decrement();
    // test_code_gen_null_expression();
    // test_code_gen_new_label();
    // test_code_gen_module_no_main_adds_dummy();
}