// tests/type_checker_tests.c

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../arena.h"
#include "../debug.h"
#include "../ast.h"
#include "../token.h"
#include "../type_checker.h"
#include "../symbol_table.h"

void setup_token(Token *tok, TokenType type, const char *lexeme, int line, const char *filename, Arena *arena) {
    tok->type = type;
    tok->line = line;
    size_t lex_len = strlen(lexeme);
    char *allocated_lexeme = (char *)arena_alloc(arena, lex_len + 1);
    memcpy(allocated_lexeme, lexeme, lex_len + 1);
    tok->start = allocated_lexeme;
    tok->length = (int)lex_len;
    // Also copy filename if needed (assuming it's const and safe to assign directly; otherwise allocate/copy similarly)
    tok->filename = filename;
}

static void setup_literal_token(Token *token, TokenType type, const char *lexeme_str, int line, const char *filename, Arena *arena)
{
    setup_token(token, type, lexeme_str, line, filename, arena);
}

void test_type_check_array_decl_no_init()
{
    DEBUG_INFO("Starting test_type_check_array_decl_no_init");
    printf("Testing type check for array declaration without initializer...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token name_tok;
    setup_token(&name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Type *elem_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, elem_type);

    // Note: This assumes type_check_var_decl handles no-initializer case without error
    // (i.e., does not check NIL against array type; just adds the declared type).
    Stmt *decl = ast_create_var_decl_stmt(&arena, name_tok, arr_type, NULL, NULL);

    ast_module_add_statement(&arena, &module, decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass with no type error

    // Verify symbol was added with correct type
    Symbol *sym = symbol_table_lookup_symbol(&table, name_tok);
    assert(sym != NULL);
    assert(ast_type_equals(sym->type, arr_type));

    symbol_table_cleanup(&table);  // Assuming free function exists
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_decl_no_init");
}

void test_type_check_array_decl_with_init_matching()
{
    DEBUG_INFO("Starting test_type_check_array_decl_with_init_matching");
    printf("Testing type check for array declaration with matching initializer...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token name_tok;
    setup_token(&name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Type *elem_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, elem_type);

    // Create array literal {1, 2}
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 1};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, elem_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 2, "test.sn", &arena);
    LiteralValue val2 = {.int_value = 2};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, elem_type, false, &lit2_tok);

    Expr *elements[2] = {lit1, lit2};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 2, &arr_tok);

    Stmt *decl = ast_create_var_decl_stmt(&arena, name_tok, arr_type, arr_lit, NULL);
    ast_module_add_statement(&arena, &module, decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    // Verify array literal type is inferred as int[]
    assert(arr_lit->expr_type != NULL);
    assert(arr_lit->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(arr_lit->expr_type->as.array.element_type, elem_type));

    // Verify initializer type matches declared type
    assert(ast_type_equals(arr_lit->expr_type, arr_type));

    // Verify symbol type
    Symbol *sym = symbol_table_lookup_symbol(&table, name_tok);
    assert(sym != NULL);
    assert(ast_type_equals(sym->type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_decl_with_init_matching");
}

void test_type_check_array_decl_with_init_mismatch()
{
    DEBUG_INFO("Starting test_type_check_array_decl_with_init_mismatch");
    printf("Testing type check for array declaration with mismatched initializer...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Token name_tok;
    setup_token(&name_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Create mismatched array literal {1.5} - double
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_DOUBLE_LITERAL, "1.5", 2, "test.sn", &arena);
    LiteralValue val = {.double_value = 1.5};
    Expr *lit = ast_create_literal_expr(&arena, val, double_type, false, &lit_tok);

    Expr *elements[1] = {lit};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 1, &arr_tok);

    Stmt *decl = ast_create_var_decl_stmt(&arena, name_tok, arr_type, arr_lit, NULL);
    ast_module_add_statement(&arena, &module, decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail due to type mismatch (double[] vs int[])

    // Verify array literal type is double[]
    assert(arr_lit->expr_type != NULL);
    assert(arr_lit->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(arr_lit->expr_type->as.array.element_type, double_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_decl_with_init_mismatch");
}

void test_type_check_array_literal_empty()
{
    DEBUG_INFO("Starting test_type_check_array_literal_empty");
    printf("Testing type check for empty array literal...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    // Empty array {}
    Expr *elements[0] = {NULL};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 0, &arr_tok);

    // Wrap in expr stmt for module
    Stmt *expr_stmt = ast_create_expr_stmt(&arena, arr_lit, &arr_tok);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    // Empty array should be array of NIL
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *empty_arr_type = ast_create_array_type(&arena, nil_type);
    assert(ast_type_equals(arr_lit->expr_type, empty_arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_literal_empty");
}

void test_type_check_array_literal_heterogeneous()
{
    DEBUG_INFO("Starting test_type_check_array_literal_heterogeneous");
    printf("Testing type check for heterogeneous array literal...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    // {1, 1.5} - int and double
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue val1 = {.int_value = 1};
    Expr *lit1 = ast_create_literal_expr(&arena, val1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_DOUBLE_LITERAL, "1.5", 1, "test.sn", &arena);
    LiteralValue val2 = {.double_value = 1.5};
    Expr *lit2 = ast_create_literal_expr(&arena, val2, double_type, false, &lit2_tok);

    Expr *elements[2] = {lit1, lit2};
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 2, &arr_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, arr_lit, &arr_tok);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail due to heterogeneous elements

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_literal_heterogeneous");
}

void test_type_check_array_access_valid()
{
    DEBUG_INFO("Starting test_type_check_array_access_valid");
    printf("Testing type check for valid array access...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Declare arr: int[] = {1, 2, 3}
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Token lit1_tok, lit2_tok, lit3_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);
    setup_literal_token(&lit3_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue v3 = {.int_value = 3};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &lit3_tok);
    Expr *elements[3] = {e1, e2, e3};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_init = ast_create_array_expr(&arena, elements, 3, &arr_lit_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, arr_init, NULL);

    // var x: int = arr[0]
    Token x_tok;
    setup_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token idx_tok;
    setup_literal_token(&idx_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue zero = {.int_value = 0};
    Expr *idx = ast_create_literal_expr(&arena, zero, int_type, false, &idx_tok);
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_arr, idx, &access_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, access, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, x_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    // Verify access type is int
    assert(access->expr_type != NULL);
    assert(ast_type_equals(access->expr_type, int_type));

    // Verify var_arr type is arr_type
    assert(var_arr->expr_type != NULL);
    assert(ast_type_equals(var_arr->expr_type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_valid");
}

void test_type_check_array_access_non_array()
{
    DEBUG_INFO("Starting test_type_check_array_access_non_array");
    printf("Testing type check for array access on non-array...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    // var num: int = 5
    Token num_tok;
    setup_token(&num_tok, TOKEN_IDENTIFIER, "num", 1, "test.sn", &arena);
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 5};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *num_decl = ast_create_var_decl_stmt(&arena, num_tok, int_type, lit, NULL);

    // num[0] - invalid
    Token idx_tok;
    setup_literal_token(&idx_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue zero = {.int_value = 0};
    Expr *idx = ast_create_literal_expr(&arena, zero, int_type, false, &idx_tok);
    Expr *var_num = ast_create_variable_expr(&arena, num_tok, NULL);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_num, idx, &access_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, access, &access_tok);
    ast_module_add_statement(&arena, &module, num_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail: access on non-array

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_non_array");
}

void test_type_check_array_access_invalid_index()
{
    DEBUG_INFO("Starting test_type_check_array_access_invalid_index");
    printf("Testing type check for array access with invalid index type...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);

    // var arr: int[] = {1}
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

    // arr["foo"] - invalid index
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token str_tok;
    setup_token(&str_tok, TOKEN_STRING_LITERAL, "\"foo\"", 2, "test.sn", &arena);
    LiteralValue str_val = {.string_value = arena_strdup(&arena, "foo")};
    Expr *str_idx = ast_create_literal_expr(&arena, str_val, str_type, false, &str_tok);
    Token access_tok;
    setup_token(&access_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *access = ast_create_array_access_expr(&arena, var_arr, str_idx, &access_tok);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, access, &access_tok);
    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail: non-numeric index

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_access_invalid_index");
}

void test_type_check_array_assignment_matching()
{
    DEBUG_INFO("Starting test_type_check_array_assignment_matching");
    printf("Testing type check for array assignment with matching type...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // var arr: int[]
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);

    // arr = {4, 5}
    Token lit4_tok;
    setup_literal_token(&lit4_tok, TOKEN_INT_LITERAL, "4", 2, "test.sn", &arena);
    LiteralValue v4 = {.int_value = 4};
    Expr *e4 = ast_create_literal_expr(&arena, v4, int_type, false, &lit4_tok);
    Token lit5_tok;
    setup_literal_token(&lit5_tok, TOKEN_INT_LITERAL, "5", 2, "test.sn", &arena);
    LiteralValue v5 = {.int_value = 5};
    Expr *e5 = ast_create_literal_expr(&arena, v5, int_type, false, &lit5_tok);
    Expr *new_elements[2] = {e4, e5};
    Token new_arr_tok;
    setup_token(&new_arr_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Expr *new_arr = ast_create_array_expr(&arena, new_elements, 2, &new_arr_tok);
    Expr *assign = ast_create_assign_expr(&arena, arr_tok, new_arr, NULL);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, assign_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    // Verify assign type matches arr_type
    assert(assign->expr_type != NULL);
    assert(ast_type_equals(assign->expr_type, arr_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_assignment_matching");
}

void test_type_check_array_assignment_mismatch()
{
    DEBUG_INFO("Starting test_type_check_array_assignment_mismatch");
    printf("Testing type check for array assignment with mismatched type...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    // var arr: int[]
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);

    // arr = {1.5} - double[]
    Token lit_tok;
    setup_literal_token(&lit_tok, TOKEN_DOUBLE_LITERAL, "1.5", 2, "test.sn", &arena);
    LiteralValue val = {.double_value = 1.5};
    Expr *lit = ast_create_literal_expr(&arena, val, double_type, false, &lit_tok);
    Expr *elements[1] = {lit};
    Token new_arr_tok;
    setup_token(&new_arr_tok, TOKEN_LEFT_BRACE, "{", 2, "test.sn", &arena);
    Expr *new_arr = ast_create_array_expr(&arena, elements, 1, &new_arr_tok);
    Expr *assign = ast_create_assign_expr(&arena, arr_tok, new_arr, NULL);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, assign_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail: type mismatch

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_assignment_mismatch");
}

void test_type_check_nested_array()
{
    DEBUG_INFO("Starting test_type_check_nested_array");
    printf("Testing type check for nested array types...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *inner_arr_type = ast_create_array_type(&arena, int_type);
    Type *outer_arr_type = ast_create_array_type(&arena, inner_arr_type);

    // var nested: int[][]
    Token nested_tok;
    setup_token(&nested_tok, TOKEN_IDENTIFIER, "nested", 1, "test.sn", &arena);
    Stmt *decl = ast_create_var_decl_stmt(&arena, nested_tok, outer_arr_type, NULL, NULL);
    ast_module_add_statement(&arena, &module, decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    // Verify symbol type
    Symbol *sym = symbol_table_lookup_symbol(&table, nested_tok);
    assert(sym != NULL);
    assert(ast_type_equals(sym->type, outer_arr_type));
    assert(sym->type->as.array.element_type->kind == TYPE_ARRAY);
    assert(sym->type->as.array.element_type->as.array.element_type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_nested_array");
}

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

    // var arr: int[] = {1, 2}
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    // ... (create arr_init similar to before)
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

    // var len: int = arr.length
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
    assert(no_error == 1);  // Should pass, length returns int

    // Verify member type is int
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

    // var arr: int[] = {1}
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

    // arr.invalid - should error
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token invalid_tok;
    setup_token(&invalid_tok, TOKEN_IDENTIFIER, "invalid", 2, "test.sn", &arena);
    Expr *member = ast_create_member_expr(&arena, var_arr, invalid_tok, NULL);

    Stmt *expr_stmt = ast_create_expr_stmt(&arena, member, NULL);
    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, expr_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  // Should fail: unknown member

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

    // var arr: int[] = {1} (to have a valid array)
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

    // arr.push (as member expr in a dummy stmt to type-check)
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token push_tok;
    setup_token(&push_tok, TOKEN_IDENTIFIER, "push", 2, "test.sn", &arena);
    Expr *push_member = ast_create_member_expr(&arena, var_arr, push_tok, NULL);
    Stmt *dummy_stmt = ast_create_expr_stmt(&arena, push_member, NULL);  // Dummy to force type-check

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, dummy_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass

    // Verify member type is function(void, int)
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

    // var arr: int[] = {1}
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

    // arr.pop (member expr)
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token pop_tok;
    setup_token(&pop_tok, TOKEN_IDENTIFIER, "pop", 2, "test.sn", &arena);
    Expr *pop_member = ast_create_member_expr(&arena, var_arr, pop_tok, NULL);
    Stmt *dummy_stmt = ast_create_expr_stmt(&arena, pop_member, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, dummy_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify member type is function(int)
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

    // var arr: int[]
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, arr_type, NULL, NULL);

    // arr.clear (member expr)
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token clear_tok;
    setup_token(&clear_tok, TOKEN_IDENTIFIER, "clear", 2, "test.sn", &arena);
    Expr *clear_member = ast_create_member_expr(&arena, var_arr, clear_tok, NULL);
    Stmt *dummy_stmt = ast_create_expr_stmt(&arena, clear_member, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, dummy_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify member type is function(void)
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

    // var arr: int[] = {1}
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

    // arr.concat (member expr; arg would be checked in full call test if added)
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Token concat_tok;
    setup_token(&concat_tok, TOKEN_IDENTIFIER, "concat", 2, "test.sn", &arena);
    Expr *concat_member = ast_create_member_expr(&arena, var_arr, concat_tok, NULL);
    Stmt *dummy_stmt = ast_create_expr_stmt(&arena, concat_member, NULL);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, dummy_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);

    // Verify member type is function(int[] -> int[])
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

    // var arr: int[] = {1, 2}
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

    // Dummy print(arr) - assume print is a call with arg arr (type-checks printable)
    // For isolation, just check arr_type is printable (but since no print def, test via interpolated)
    Token interp_tok;
    setup_token(&interp_tok, TOKEN_INTERPOL_STRING, "$\"{arr}\"", 2, "test.sn", &arena);
    Expr *var_arr = ast_create_variable_expr(&arena, arr_tok, NULL);
    Expr *parts[1] = {var_arr};
    Expr *interp = ast_create_interpolated_expr(&arena, parts, 1, &interp_tok);
    Stmt *interp_stmt = ast_create_expr_stmt(&arena, interp, &interp_tok);

    ast_module_add_statement(&arena, &module, arr_decl);
    ast_module_add_statement(&arena, &module, interp_stmt);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // No "non-printable" error

    // Verify interpolated type is string
    assert(interp->expr_type != NULL);
    assert(interp->expr_type->kind == TYPE_STRING);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_array_printable");
}

void test_type_check_function_return_array()
{
    DEBUG_INFO("Starting test_type_check_function_return_array");
    printf("Testing type check for function returning array, assigned to var...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);

    // Function: fn create_arr(): int[] => return {1, 2}
    Parameter *params = NULL;
    int param_count = 0;

    // Array literal {1, 2}
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);

    Expr *elements[2] = {e1, e2};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 2, &arr_lit_tok);

    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 1, "test.sn", &arena);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, arr_lit, &ret_tok);

    Stmt *body[1] = {ret_stmt};
    Token func_name_tok;
    setup_token(&func_name_tok, TOKEN_IDENTIFIER, "create_arr", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, param_count, arr_type, body, 1, &func_name_tok);

    // Var decl: var arr: int[] = create_arr()
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);

    Token call_name_tok;
    setup_token(&call_name_tok, TOKEN_IDENTIFIER, "create_arr", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_name_tok, NULL);
    Expr **args = NULL;
    int arg_count = 0;
    Expr *call = ast_create_call_expr(&arena, callee, args, arg_count, &call_name_tok);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, arr_type, call, NULL);

    // Add to module (function first, then var)
    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, var_decl);

    // Type check
    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass: function added, call resolves to int[], matches decl

    // Verify function symbol added (first pass)
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_name_tok);
    assert(func_sym != NULL);
    assert(func_sym->type != NULL);
    assert(func_sym->type->kind == TYPE_FUNCTION);
    assert(ast_type_equals(func_sym->type->as.function.return_type, arr_type));
    assert(func_sym->type->as.function.param_count == 0);

    // Verify var symbol
    Symbol *var_sym = symbol_table_lookup_symbol(&table, var_name_tok);
    assert(var_sym != NULL);
    assert(ast_type_equals(var_sym->type, arr_type));

    // Verify call expr_type
    assert(call->expr_type != NULL);
    assert(ast_type_equals(call->expr_type, arr_type));

    // Verify array literal in return infers correctly
    assert(arr_lit->expr_type != NULL);
    assert(arr_lit->expr_type->kind == TYPE_ARRAY);
    assert(ast_type_equals(arr_lit->expr_type->as.array.element_type, int_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_function_return_array");
}

void test_type_check_var_decl_function_call_array()
{
    DEBUG_INFO("Starting test_type_check_var_decl_function_call_array");
    printf("Testing type check for variable declaration with function call returning array...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);

    // FIX: Setup print_tok early for reuse in built-in symbol and call.
    Token print_tok;
    setup_token(&print_tok, TOKEN_IDENTIFIER, "print", 5, "test.sn", &arena);  // length=5 (fixed)

    // FIX: Add built-in print symbol before building module/AST.
    // fn print(s: string): void
    Type *print_arg_types[1] = {string_type};
    Type *print_func_type = ast_create_function_type(&arena, void_type, print_arg_types, 1);
    symbol_table_add_symbol_with_kind(&table, print_tok, print_func_type, SYMBOL_LOCAL);  // Use SYMBOL_GLOBAL if available

    // Function: fn declare_basic_int_array(): int[] =>
    //   var int_arr: int[] = {1, 2, 3}
    //   return int_arr
    Parameter *declare_params = NULL;
    int declare_param_count = 0;

    // Array literal {1, 2, 3} for init
    Token lit1_tok;
    setup_literal_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);  // length=1 (fixed)
    LiteralValue v1 = {.int_value = 1};
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);

    Token lit2_tok;
    setup_literal_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);  // length=1 (fixed)
    LiteralValue v2 = {.int_value = 2};
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);

    Token lit3_tok;
    setup_literal_token(&lit3_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);  // length=1 (fixed)
    LiteralValue v3 = {.int_value = 3};
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &lit3_tok);

    Expr *elements[3] = {e1, e2, e3};
    Token arr_lit_tok;
    setup_token(&arr_lit_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);  // length=1 (fixed)
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 3, &arr_lit_tok);

    // var int_arr: int[] = {1, 2, 3}
    Token int_arr_tok;
    setup_token(&int_arr_tok, TOKEN_IDENTIFIER, "int_arr", 7, "test.sn", &arena);  // length=7 (fixed)
    Stmt *int_arr_decl = ast_create_var_decl_stmt(&arena, int_arr_tok, arr_type, arr_lit, NULL);

    // return int_arr
    Token ret_tok;
    setup_token(&ret_tok, TOKEN_RETURN, "return", 6, "test.sn", &arena);  // length=6 (fixed)
    Expr *int_arr_var = ast_create_variable_expr(&arena, int_arr_tok, NULL);
    Stmt *ret_stmt = ast_create_return_stmt(&arena, ret_tok, int_arr_var, &ret_tok);

    Stmt *declare_body[2] = {int_arr_decl, ret_stmt};
    Token declare_name_tok;
    setup_token(&declare_name_tok, TOKEN_IDENTIFIER, "declare_basic_int_array", 22, "test.sn", &arena);  // length=22 (fixed)
    Stmt *declare_func = ast_create_function_stmt(&arena, declare_name_tok, declare_params, declare_param_count, arr_type, declare_body, 2, &declare_name_tok);

    // Function: fn print_basic_int_array(arr: int[]): void =>
    //   print($"Int Array: {arr}")
    // FIX: Allocate array of Parameter structs (not array of pointers).
    int print_param_count = 1;
    Parameter *print_params = (Parameter *)arena_alloc(&arena, sizeof(Parameter) * print_param_count);
    // FIX: Set fields directly on print_params[0] (no separate print_param).
    print_params[0].name.type = TOKEN_IDENTIFIER;
    print_params[0].name.start = arena_strdup(&arena, "arr");
    print_params[0].name.length = 3;
    print_params[0].name.line = 5;
    print_params[0].name.filename = "test.sn";
    print_params[0].type = arr_type;

    // Interpolated: $"Int Array: {arr}"
    // FIX: For realism, add a string literal part (optional, but improves test; type-check still passes without it).
    Token str_lit_tok;
    setup_literal_token(&str_lit_tok, TOKEN_STRING_LITERAL, "\"Int Array: \"", 13, "test.sn", &arena);  // length=13 (fixed, approx)
    LiteralValue str_val = {.string_value = arena_strdup(&arena, "Int Array: ")};
    Expr *str_part = ast_create_literal_expr(&arena, str_val, string_type, false, &str_lit_tok);

    Token interp_tok;
    setup_token(&interp_tok, TOKEN_INTERPOL_STRING, "$\"Int Array: {arr}\"", 18, "test.sn", &arena);  // length=18 (fixed, approx)
    Expr *arr_param_var = ast_create_variable_expr(&arena, print_params[0].name, NULL);  // FIX: Use print_params[0].name
    // FIX: Now 2 parts: string literal + arr var.
    Expr *interp_parts[2] = {str_part, arr_param_var};
    Expr *interp = ast_create_interpolated_expr(&arena, interp_parts, 2, &interp_tok);

    // Call: print(interp)
    // Reuse early print_tok for callee.
    Expr *print_callee = ast_create_variable_expr(&arena, print_tok, NULL);
    Expr *print_args[1] = {interp};
    Stmt *print_call_stmt = ast_create_expr_stmt(&arena, ast_create_call_expr(&arena, print_callee, print_args, 1, &print_tok), &print_tok);

    Stmt *print_body[1] = {print_call_stmt};
    Token print_name_tok;
    setup_token(&print_name_tok, TOKEN_IDENTIFIER, "print_basic_int_array", 20, "test.sn", &arena);  // length=20 (fixed)
    Stmt *print_func = ast_create_function_stmt(&arena, print_name_tok, print_params, print_param_count, void_type, print_body, 1, &print_name_tok);

    // Function: fn main(): void =>
    //   var arr: int[] = declare_basic_int_array()
    //   print_basic_int_array(arr)
    Parameter *main_params = NULL;
    int main_param_count = 0;

    // var arr: int[] = declare_basic_int_array()
    Token main_arr_tok;
    setup_token(&main_arr_tok, TOKEN_IDENTIFIER, "arr", 3, "test.sn", &arena);  // length=3 (fixed)
    Token main_call_name_tok;
    setup_token(&main_call_name_tok, TOKEN_IDENTIFIER, "declare_basic_int_array", 22, "test.sn", &arena);  // length=22 (fixed)
    Expr *main_callee = ast_create_variable_expr(&arena, main_call_name_tok, NULL);
    Expr **main_call_args = NULL;
    int main_call_arg_count = 0;
    Expr *main_call = ast_create_call_expr(&arena, main_callee, main_call_args, main_call_arg_count, &main_call_name_tok);
    Stmt *main_arr_decl = ast_create_var_decl_stmt(&arena, main_arr_tok, arr_type, main_call, NULL);

    // print_basic_int_array(arr)
    Token main_print_name_tok;
    setup_token(&main_print_name_tok, TOKEN_IDENTIFIER, "print_basic_int_array", 20, "test.sn", &arena);  // length=20 (fixed)
    Expr *main_print_callee = ast_create_variable_expr(&arena, main_print_name_tok, NULL);
    Expr *main_print_args[1] = {ast_create_variable_expr(&arena, main_arr_tok, NULL)};
    Expr *main_print_call = ast_create_call_expr(&arena, main_print_callee, main_print_args, 1, &main_print_name_tok);
    Stmt *main_print_stmt = ast_create_expr_stmt(&arena, main_print_call, &main_print_name_tok);

    Stmt *main_body[2] = {main_arr_decl, main_print_stmt};
    Token main_name_tok;
    setup_token(&main_name_tok, TOKEN_IDENTIFIER, "main", 4, "test.sn", &arena);  // length=4 (fixed)
    Stmt *main_func = ast_create_function_stmt(&arena, main_name_tok, main_params, main_param_count, void_type, main_body, 2, &main_name_tok);

    // Add all functions to module
    ast_module_add_statement(&arena, &module, declare_func);
    ast_module_add_statement(&arena, &module, print_func);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  // Should pass: all types match, array printable in interpolated, functions resolve correctly (incl. built-in print)

    // Verify declare function symbol
    Symbol *declare_sym = symbol_table_lookup_symbol(&table, declare_name_tok);
    assert(declare_sym != NULL);
    assert(declare_sym->type->kind == TYPE_FUNCTION);
    assert(ast_type_equals(declare_sym->type->as.function.return_type, arr_type));
    assert(declare_sym->type->as.function.param_count == 0);

    // Verify print function symbol
    Symbol *print_sym = symbol_table_lookup_symbol(&table, print_name_tok);
    assert(print_sym != NULL);
    assert(print_sym->type->kind == TYPE_FUNCTION);
    assert(ast_type_equals(print_sym->type->as.function.return_type, void_type));
    assert(print_sym->type->as.function.param_count == 1);
    assert(ast_type_equals(print_sym->type->as.function.param_types[0], arr_type));

    // Verify main function symbol
    Symbol *main_sym = symbol_table_lookup_symbol(&table, main_name_tok);
    assert(main_sym != NULL);
    assert(main_sym->type->kind == TYPE_FUNCTION);
    assert(ast_type_equals(main_sym->type->as.function.return_type, void_type));
    assert(main_sym->type->as.function.param_count == 0);

    // In main, verify call to declare returns arr_type
    assert(main_call->expr_type != NULL);
    assert(ast_type_equals(main_call->expr_type, arr_type));

    // Verify interpolated type is string
    assert(interp->expr_type != NULL);
    assert(interp->expr_type->kind == TYPE_STRING);

    // Assuming print is handled as built-in taking string -> void, but since not verified here, skip deep check

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_type_check_var_decl_function_call_array");
}

void test_type_checker_main() 
{
    test_type_check_array_decl_no_init();
    test_type_check_array_decl_with_init_matching();
    test_type_check_array_decl_with_init_mismatch();
    test_type_check_array_literal_empty();
    test_type_check_array_literal_heterogeneous();
    test_type_check_array_access_valid();
    test_type_check_array_access_non_array();
    test_type_check_array_access_invalid_index();
    test_type_check_array_assignment_matching();
    test_type_check_array_assignment_mismatch();
    test_type_check_nested_array();
    test_type_check_array_member_length();
    test_type_check_array_member_invalid();
    test_type_check_array_member_push();
    test_type_check_array_member_pop();
    test_type_check_array_member_clear();
    test_type_check_array_member_concat();
    test_type_check_array_printable();
    test_type_check_function_return_array();
    test_type_check_var_decl_function_call_array();
}
