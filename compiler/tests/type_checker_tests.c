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