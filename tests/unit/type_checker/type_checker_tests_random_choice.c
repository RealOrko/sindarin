// tests/unit/type_checker/type_checker_tests_random_choice.c
// Tests for Random type checking: choice and weightedChoice operations
// Split from type_checker_tests_random.c

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"
#include "../ast/ast_expr.h"

/* ============================================================================
 * Tests for Random.choice() collection operation
 * ============================================================================ */

/* Test Random.choice(int[]) returns int */
static void test_random_choice_int_array_returns_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);

    /* Create an int[] array literal */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create array elements */
    Token elem1_tok;
    setup_token(&elem1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue elem1_val;
    elem1_val.int_value = 1;
    Expr *elem1 = ast_create_literal_expr(&arena, elem1_val, int_type, false, &elem1_tok);

    Token elem2_tok;
    setup_token(&elem2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue elem2_val;
    elem2_val.int_value = 2;
    Expr *elem2 = ast_create_literal_expr(&arena, elem2_val, int_type, false, &elem2_tok);

    Token elem3_tok;
    setup_token(&elem3_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue elem3_val;
    elem3_val.int_value = 3;
    Expr *elem3 = ast_create_literal_expr(&arena, elem3_val, int_type, false, &elem3_tok);

    Expr *elements[3] = {elem1, elem2, elem3};
    Expr *array_expr = ast_create_array_expr(&arena, elements, 3, &elem1_tok);
    array_expr->expr_type = int_array_type;

    Expr *args[1] = {array_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.choice(str[]) returns str */
static void test_random_choice_str_array_returns_str(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);

    /* Create a str[] array */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *str_array_type = ast_create_array_type(&arena, str_type);

    Token elem1_tok;
    setup_token(&elem1_tok, TOKEN_STRING_LITERAL, "\"red\"", 1, "test.sn", &arena);
    LiteralValue elem1_val;
    elem1_val.string_value = "red";
    Expr *elem1 = ast_create_literal_expr(&arena, elem1_val, str_type, false, &elem1_tok);

    Token elem2_tok;
    setup_token(&elem2_tok, TOKEN_STRING_LITERAL, "\"green\"", 1, "test.sn", &arena);
    LiteralValue elem2_val;
    elem2_val.string_value = "green";
    Expr *elem2 = ast_create_literal_expr(&arena, elem2_val, str_type, false, &elem2_tok);

    Expr *elements[2] = {elem1, elem2};
    Expr *array_expr = ast_create_array_expr(&arena, elements, 2, &elem1_tok);
    array_expr->expr_type = str_array_type;

    Expr *args[1] = {array_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_STRING);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.choice(double[]) returns double */
static void test_random_choice_double_array_returns_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);

    /* Create a double[] array */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *double_array_type = ast_create_array_type(&arena, double_type);

    Token elem1_tok;
    setup_token(&elem1_tok, TOKEN_DOUBLE_LITERAL, "1.5", 1, "test.sn", &arena);
    LiteralValue elem1_val;
    elem1_val.double_value = 1.5;
    Expr *elem1 = ast_create_literal_expr(&arena, elem1_val, double_type, false, &elem1_tok);

    Expr *elements[1] = {elem1};
    Expr *array_expr = ast_create_array_expr(&arena, elements, 1, &elem1_tok);
    array_expr->expr_type = double_array_type;

    Expr *args[1] = {array_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_DOUBLE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.choice(bool[]) returns bool */
static void test_random_choice_bool_array_returns_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);

    /* Create a bool[] array */
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *bool_array_type = ast_create_array_type(&arena, bool_type);

    Token elem1_tok;
    setup_token(&elem1_tok, TOKEN_BOOL_LITERAL, "true", 1, "test.sn", &arena);
    LiteralValue elem1_val;
    elem1_val.bool_value = true;
    Expr *elem1 = ast_create_literal_expr(&arena, elem1_val, bool_type, false, &elem1_tok);

    Expr *elements[1] = {elem1};
    Expr *array_expr = ast_create_array_expr(&arena, elements, 1, &elem1_tok);
    array_expr->expr_type = bool_array_type;

    Expr *args[1] = {array_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_BOOL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.choice() with non-array argument reports error */
static void test_random_choice_non_array_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);

    /* Pass an int instead of an array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token int_tok;
    setup_token(&int_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue int_val;
    int_val.int_value = 42;
    Expr *int_expr = ast_create_literal_expr(&arena, int_val, int_type, false, &int_tok);

    Expr *args[1] = {int_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.choice() with string argument reports error */
static void test_random_choice_string_arg_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);

    /* Pass a string instead of an array */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Token str_tok;
    setup_token(&str_tok, TOKEN_STRING_LITERAL, "\"hello\"", 1, "test.sn", &arena);
    LiteralValue str_val;
    str_val.string_value = "hello";
    Expr *str_expr = ast_create_literal_expr(&arena, str_val, str_type, false, &str_tok);

    Expr *args[1] = {str_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.choice() with wrong argument count reports error */
static void test_random_choice_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);

    /* Pass no arguments when 1 is required */
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.choice() with too many arguments reports error */
static void test_random_choice_too_many_args_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);

    /* Create two arrays when only 1 is expected */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token elem_tok;
    setup_token(&elem_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue elem_val;
    elem_val.int_value = 1;
    Expr *elem = ast_create_literal_expr(&arena, elem_val, int_type, false, &elem_tok);

    Expr *elements[1] = {elem};
    Expr *array1 = ast_create_array_expr(&arena, elements, 1, &elem_tok);
    array1->expr_type = int_array_type;
    Expr *array2 = ast_create_array_expr(&arena, elements, 1, &elem_tok);
    array2->expr_type = int_array_type;

    Expr *args[2] = {array1, array2};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Tests for Random.weightedChoice() collection operation
 * ============================================================================ */

/* Test Random.weightedChoice(int[], double[]) returns int */
static void test_random_weightedChoice_int_array_returns_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "weightedChoice", 1, "test.sn", &arena);

    /* Create int[] items array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token int_elem_tok;
    setup_token(&int_elem_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue int_elem_val;
    int_elem_val.int_value = 1;
    Expr *int_elem = ast_create_literal_expr(&arena, int_elem_val, int_type, false, &int_elem_tok);

    Expr *int_elements[1] = {int_elem};
    Expr *items_expr = ast_create_array_expr(&arena, int_elements, 1, &int_elem_tok);
    items_expr->expr_type = int_array_type;

    /* Create double[] weights array */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *double_array_type = ast_create_array_type(&arena, double_type);

    Token double_elem_tok;
    setup_token(&double_elem_tok, TOKEN_DOUBLE_LITERAL, "1.0", 1, "test.sn", &arena);
    LiteralValue double_elem_val;
    double_elem_val.double_value = 1.0;
    Expr *double_elem = ast_create_literal_expr(&arena, double_elem_val, double_type, false, &double_elem_tok);

    Expr *double_elements[1] = {double_elem};
    Expr *weights_expr = ast_create_array_expr(&arena, double_elements, 1, &double_elem_tok);
    weights_expr->expr_type = double_array_type;

    Expr *args[2] = {items_expr, weights_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.weightedChoice(str[], double[]) returns str */
static void test_random_weightedChoice_str_array_returns_str(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "weightedChoice", 1, "test.sn", &arena);

    /* Create str[] items array */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *str_array_type = ast_create_array_type(&arena, str_type);

    Token str_elem_tok;
    setup_token(&str_elem_tok, TOKEN_STRING_LITERAL, "\"hello\"", 1, "test.sn", &arena);
    LiteralValue str_elem_val;
    str_elem_val.string_value = "hello";
    Expr *str_elem = ast_create_literal_expr(&arena, str_elem_val, str_type, false, &str_elem_tok);

    Expr *str_elements[1] = {str_elem};
    Expr *items_expr = ast_create_array_expr(&arena, str_elements, 1, &str_elem_tok);
    items_expr->expr_type = str_array_type;

    /* Create double[] weights array */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *double_array_type = ast_create_array_type(&arena, double_type);

    Token double_elem_tok;
    setup_token(&double_elem_tok, TOKEN_DOUBLE_LITERAL, "1.0", 1, "test.sn", &arena);
    LiteralValue double_elem_val;
    double_elem_val.double_value = 1.0;
    Expr *double_elem = ast_create_literal_expr(&arena, double_elem_val, double_type, false, &double_elem_tok);

    Expr *double_elements[1] = {double_elem};
    Expr *weights_expr = ast_create_array_expr(&arena, double_elements, 1, &double_elem_tok);
    weights_expr->expr_type = double_array_type;

    Expr *args[2] = {items_expr, weights_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_STRING);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.weightedChoice() with non-array first arg reports error */
static void test_random_weightedChoice_non_array_first_arg_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "weightedChoice", 1, "test.sn", &arena);

    /* Pass an int instead of an array for items */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token int_tok;
    setup_token(&int_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue int_val;
    int_val.int_value = 42;
    Expr *int_expr = ast_create_literal_expr(&arena, int_val, int_type, false, &int_tok);

    /* Create double[] weights array */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *double_array_type = ast_create_array_type(&arena, double_type);

    Token double_elem_tok;
    setup_token(&double_elem_tok, TOKEN_DOUBLE_LITERAL, "1.0", 1, "test.sn", &arena);
    LiteralValue double_elem_val;
    double_elem_val.double_value = 1.0;
    Expr *double_elem = ast_create_literal_expr(&arena, double_elem_val, double_type, false, &double_elem_tok);

    Expr *double_elements[1] = {double_elem};
    Expr *weights_expr = ast_create_array_expr(&arena, double_elements, 1, &double_elem_tok);
    weights_expr->expr_type = double_array_type;

    Expr *args[2] = {int_expr, weights_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.weightedChoice() with non-double[] second arg reports error */
static void test_random_weightedChoice_non_double_array_second_arg_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "weightedChoice", 1, "test.sn", &arena);

    /* Create int[] items array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token int_elem_tok;
    setup_token(&int_elem_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue int_elem_val;
    int_elem_val.int_value = 1;
    Expr *int_elem = ast_create_literal_expr(&arena, int_elem_val, int_type, false, &int_elem_tok);

    Expr *int_elements[1] = {int_elem};
    Expr *items_expr = ast_create_array_expr(&arena, int_elements, 1, &int_elem_tok);
    items_expr->expr_type = int_array_type;

    /* Create int[] instead of double[] for weights (wrong type) */
    Token int_weight_tok;
    setup_token(&int_weight_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue int_weight_val;
    int_weight_val.int_value = 1;
    Expr *int_weight = ast_create_literal_expr(&arena, int_weight_val, int_type, false, &int_weight_tok);

    Expr *int_weight_elements[1] = {int_weight};
    Expr *weights_expr = ast_create_array_expr(&arena, int_weight_elements, 1, &int_weight_tok);
    weights_expr->expr_type = int_array_type;  /* int[] instead of double[] */

    Expr *args[2] = {items_expr, weights_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.weightedChoice() with wrong argument count reports error */
static void test_random_weightedChoice_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "weightedChoice", 1, "test.sn", &arena);

    /* Create only one array argument when 2 are required */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token int_elem_tok;
    setup_token(&int_elem_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue int_elem_val;
    int_elem_val.int_value = 1;
    Expr *int_elem = ast_create_literal_expr(&arena, int_elem_val, int_type, false, &int_elem_tok);

    Expr *int_elements[1] = {int_elem};
    Expr *items_expr = ast_create_array_expr(&arena, int_elements, 1, &int_elem_tok);
    items_expr->expr_type = int_array_type;

    Expr *args[1] = {items_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.weightedChoice() with no arguments reports error */
static void test_random_weightedChoice_no_args_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "weightedChoice", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Tests for Random INSTANCE choice/weightedChoice methods
 * Note: Uses create_random_variable() from type_checker_tests_random_basic.c
 * ============================================================================ */

/* Test rng.choice(int[]) instance method returns int */
static void test_random_instance_choice_int_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "choice", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    /* Create int[] array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token elem_tok;
    setup_token(&elem_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue elem_val;
    elem_val.int_value = 42;
    Expr *elem_expr = ast_create_literal_expr(&arena, elem_val, int_type, false, &elem_tok);

    Expr *elements[1] = {elem_expr};
    Expr *array_expr = ast_create_array_expr(&arena, elements, 1, &elem_tok);
    array_expr->expr_type = int_array_type;

    Expr *args[1] = {array_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 1, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.weightedChoice(int[], double[]) instance method returns int */
static void test_random_instance_weightedChoice_int_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "weightedChoice", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    /* Create int[] items array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token item_tok;
    setup_token(&item_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue item_val;
    item_val.int_value = 10;
    Expr *item_expr = ast_create_literal_expr(&arena, item_val, int_type, false, &item_tok);

    Expr *items[1] = {item_expr};
    Expr *items_array = ast_create_array_expr(&arena, items, 1, &item_tok);
    items_array->expr_type = int_array_type;

    /* Create double[] weights array */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *double_array_type = ast_create_array_type(&arena, double_type);

    Token weight_tok;
    setup_token(&weight_tok, TOKEN_DOUBLE_LITERAL, "1.0", 1, "test.sn", &arena);
    LiteralValue weight_val;
    weight_val.double_value = 1.0;
    Expr *weight_expr = ast_create_literal_expr(&arena, weight_val, double_type, false, &weight_tok);

    Expr *weights[1] = {weight_expr};
    Expr *weights_array = ast_create_array_expr(&arena, weights, 1, &weight_tok);
    weights_array->expr_type = double_array_type;

    Expr *args[2] = {items_array, weights_array};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 2, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Main test runner
 * ============================================================================ */

static void test_tc_random_choice_main(void)
{
    /* choice() static method tests */
    TEST_RUN("choice_int_array_returns_int", test_random_choice_int_array_returns_int);
    TEST_RUN("choice_str_array_returns_str", test_random_choice_str_array_returns_str);
    TEST_RUN("choice_double_array_returns_double", test_random_choice_double_array_returns_double);
    TEST_RUN("choice_bool_array_returns_bool", test_random_choice_bool_array_returns_bool);
    TEST_RUN("choice_non_array_error", test_random_choice_non_array_error);
    TEST_RUN("choice_string_arg_error", test_random_choice_string_arg_error);
    TEST_RUN("choice_wrong_arg_count_error", test_random_choice_wrong_arg_count_error);
    TEST_RUN("choice_too_many_args_error", test_random_choice_too_many_args_error);

    /* weightedChoice() static method tests */
    TEST_RUN("weightedChoice_int_array_returns_int", test_random_weightedChoice_int_array_returns_int);
    TEST_RUN("weightedChoice_str_array_returns_str", test_random_weightedChoice_str_array_returns_str);
    TEST_RUN("weightedChoice_non_array_first_arg_error", test_random_weightedChoice_non_array_first_arg_error);
    TEST_RUN("weightedChoice_non_double_second_arg_error", test_random_weightedChoice_non_double_array_second_arg_error);
    TEST_RUN("weightedChoice_wrong_arg_count_error", test_random_weightedChoice_wrong_arg_count_error);
    TEST_RUN("weightedChoice_no_args_error", test_random_weightedChoice_no_args_error);

    /* Instance method tests */
    TEST_RUN("instance_choice_int_array", test_random_instance_choice_int_array);
    TEST_RUN("instance_weightedChoice_int_array", test_random_instance_weightedChoice_int_array);
}
