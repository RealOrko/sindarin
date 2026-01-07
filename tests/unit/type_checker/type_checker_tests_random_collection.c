// tests/type_checker_tests_random_collection.c
// Tests for Random shuffle() and sample() collection operations type checking

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"
#include "../ast/ast_expr.h"

/* ============================================================================
 * Tests for Random.shuffle() collection operation
 * ============================================================================ */

/* Test Random.shuffle(int[]) returns void */
static void test_random_shuffle_int_array_returns_void(void)
{
    printf("Testing Random.shuffle(int[]) returns void...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "shuffle", 1, "test.sn", &arena);

    /* Create an int[] array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token elem1_tok;
    setup_token(&elem1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue elem1_val;
    elem1_val.int_value = 1;
    Expr *elem1 = ast_create_literal_expr(&arena, elem1_val, int_type, false, &elem1_tok);

    Expr *elements[1] = {elem1};
    Expr *array_expr = ast_create_array_expr(&arena, elements, 1, &elem1_tok);
    array_expr->expr_type = int_array_type;

    Expr *args[1] = {array_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.shuffle(str[]) returns void */
static void test_random_shuffle_str_array_returns_void(void)
{
    printf("Testing Random.shuffle(str[]) returns void...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "shuffle", 1, "test.sn", &arena);

    /* Create a str[] array */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *str_array_type = ast_create_array_type(&arena, str_type);

    Token elem1_tok;
    setup_token(&elem1_tok, TOKEN_STRING_LITERAL, "\"hello\"", 1, "test.sn", &arena);
    LiteralValue elem1_val;
    elem1_val.string_value = "hello";
    Expr *elem1 = ast_create_literal_expr(&arena, elem1_val, str_type, false, &elem1_tok);

    Expr *elements[1] = {elem1};
    Expr *array_expr = ast_create_array_expr(&arena, elements, 1, &elem1_tok);
    array_expr->expr_type = str_array_type;

    Expr *args[1] = {array_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.shuffle(double[]) returns void */
static void test_random_shuffle_double_array_returns_void(void)
{
    printf("Testing Random.shuffle(double[]) returns void...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "shuffle", 1, "test.sn", &arena);

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
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.shuffle() with non-array argument reports error */
static void test_random_shuffle_non_array_error(void)
{
    printf("Testing Random.shuffle() with non-array argument reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "shuffle", 1, "test.sn", &arena);

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

/* Test Random.shuffle() with string argument reports error */
static void test_random_shuffle_string_arg_error(void)
{
    printf("Testing Random.shuffle() with string argument reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "shuffle", 1, "test.sn", &arena);

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

/* Test Random.shuffle() with wrong argument count reports error */
static void test_random_shuffle_wrong_arg_count_error(void)
{
    printf("Testing Random.shuffle() with wrong argument count reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "shuffle", 1, "test.sn", &arena);

    /* Pass no arguments when 1 is required */
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Tests for Random.sample() collection operation
 * ============================================================================ */

/* Test Random.sample(int[], int) returns int[] */
static void test_random_sample_int_array_returns_int_array(void)
{
    printf("Testing Random.sample(int[], int) returns int[]...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "sample", 1, "test.sn", &arena);

    /* Create int[] array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token int_elem_tok;
    setup_token(&int_elem_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue int_elem_val;
    int_elem_val.int_value = 1;
    Expr *int_elem = ast_create_literal_expr(&arena, int_elem_val, int_type, false, &int_elem_tok);

    Expr *int_elements[1] = {int_elem};
    Expr *array_expr = ast_create_array_expr(&arena, int_elements, 1, &int_elem_tok);
    array_expr->expr_type = int_array_type;

    /* Create count argument */
    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 2;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[2] = {array_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.sample(str[], int) returns str[] */
static void test_random_sample_str_array_returns_str_array(void)
{
    printf("Testing Random.sample(str[], int) returns str[]...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "sample", 1, "test.sn", &arena);

    /* Create str[] array */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *str_array_type = ast_create_array_type(&arena, str_type);

    Token str_elem_tok;
    setup_token(&str_elem_tok, TOKEN_STRING_LITERAL, "\"hello\"", 1, "test.sn", &arena);
    LiteralValue str_elem_val;
    str_elem_val.string_value = "hello";
    Expr *str_elem = ast_create_literal_expr(&arena, str_elem_val, str_type, false, &str_elem_tok);

    Expr *str_elements[1] = {str_elem};
    Expr *array_expr = ast_create_array_expr(&arena, str_elements, 1, &str_elem_tok);
    array_expr->expr_type = str_array_type;

    /* Create count argument */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 2;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[2] = {array_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_STRING);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.sample() with non-array first arg reports error */
static void test_random_sample_non_array_first_arg_error(void)
{
    printf("Testing Random.sample() with non-array first arg reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "sample", 1, "test.sn", &arena);

    /* Pass an int instead of an array for array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token int_tok;
    setup_token(&int_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue int_val;
    int_val.int_value = 42;
    Expr *int_expr = ast_create_literal_expr(&arena, int_val, int_type, false, &int_tok);

    /* Create count argument */
    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 2;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[2] = {int_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.sample() with non-int second arg reports error */
static void test_random_sample_non_int_second_arg_error(void)
{
    printf("Testing Random.sample() with non-int second arg reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "sample", 1, "test.sn", &arena);

    /* Create int[] array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token int_elem_tok;
    setup_token(&int_elem_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue int_elem_val;
    int_elem_val.int_value = 1;
    Expr *int_elem = ast_create_literal_expr(&arena, int_elem_val, int_type, false, &int_elem_tok);

    Expr *int_elements[1] = {int_elem};
    Expr *array_expr = ast_create_array_expr(&arena, int_elements, 1, &int_elem_tok);
    array_expr->expr_type = int_array_type;

    /* Create string instead of int for count */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Token str_tok;
    setup_token(&str_tok, TOKEN_STRING_LITERAL, "\"2\"", 1, "test.sn", &arena);
    LiteralValue str_val;
    str_val.string_value = "2";
    Expr *str_expr = ast_create_literal_expr(&arena, str_val, str_type, false, &str_tok);

    Expr *args[2] = {array_expr, str_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.sample() with wrong argument count reports error */
static void test_random_sample_wrong_arg_count_error(void)
{
    printf("Testing Random.sample() with wrong argument count reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "sample", 1, "test.sn", &arena);

    /* Create only one array argument when 2 are required */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    Token int_elem_tok;
    setup_token(&int_elem_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue int_elem_val;
    int_elem_val.int_value = 1;
    Expr *int_elem = ast_create_literal_expr(&arena, int_elem_val, int_type, false, &int_elem_tok);

    Expr *int_elements[1] = {int_elem};
    Expr *array_expr = ast_create_array_expr(&arena, int_elements, 1, &int_elem_tok);
    array_expr->expr_type = int_array_type;

    Expr *args[1] = {array_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.sample() with no arguments reports error */
static void test_random_sample_no_args_error(void)
{
    printf("Testing Random.sample() with no arguments reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "sample", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Tests for Random INSTANCE methods - shuffle and sample
 * Note: Uses create_random_variable() from type_checker_tests_random_basic.c
 * ============================================================================ */

/* Test rng.shuffle(int[]) instance method returns void */
static void test_random_instance_shuffle_int_array(void)
{
    printf("Testing rng.shuffle(int[]) instance method returns void...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "shuffle", 1, "test.sn", &arena);
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
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.sample(int[], int) instance method returns int[] */
static void test_random_instance_sample_int_array(void)
{
    printf("Testing rng.sample(int[], int) instance method returns int[]...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "sample", 1, "test.sn", &arena);
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

    /* Create count argument */
    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 2;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[2] = {array_expr, count_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 2, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Main test runner
 * ============================================================================ */

void test_tc_random_collection_main(void)
{
    printf("\n=== Type Checker Random Collection Tests ===\n");

    /* Shuffle static tests */
    test_random_shuffle_int_array_returns_void();
    test_random_shuffle_str_array_returns_void();
    test_random_shuffle_double_array_returns_void();
    test_random_shuffle_non_array_error();
    test_random_shuffle_string_arg_error();
    test_random_shuffle_wrong_arg_count_error();

    /* Sample static tests */
    test_random_sample_int_array_returns_int_array();
    test_random_sample_str_array_returns_str_array();
    test_random_sample_non_array_first_arg_error();
    test_random_sample_non_int_second_arg_error();
    test_random_sample_wrong_arg_count_error();
    test_random_sample_no_args_error();

    /* Instance method tests */
    test_random_instance_shuffle_int_array();
    test_random_instance_sample_int_array();

    printf("=== All Type Checker Random Collection Tests Passed ===\n\n");
}
