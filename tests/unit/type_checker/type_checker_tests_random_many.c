// tests/unit/type_checker/type_checker_tests_random_many.c
// Tests for Random type checking: batch generation methods (*Many)
// Split from type_checker_tests_random.c

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"
#include "../ast/ast_expr.h"

/* ============================================================================
 * Tests for Random static batch generation methods
 * ============================================================================ */

/* Test Random.intMany(min, max, count) returns int[] */
static void test_random_intMany_returns_int_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "intMany", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token min_tok;
    setup_token(&min_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.int_value = 1;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, int_type, false, &min_tok);

    Token max_tok;
    setup_token(&max_tok, TOKEN_INT_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue max_val;
    max_val.int_value = 100;
    Expr *max_expr = ast_create_literal_expr(&arena, max_val, int_type, false, &max_tok);

    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 10;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[3] = {min_expr, max_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 3, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.intMany() with wrong argument count reports error */
static void test_random_intMany_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "intMany", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token min_tok;
    setup_token(&min_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.int_value = 1;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, int_type, false, &min_tok);

    Token max_tok;
    setup_token(&max_tok, TOKEN_INT_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue max_val;
    max_val.int_value = 100;
    Expr *max_expr = ast_create_literal_expr(&arena, max_val, int_type, false, &max_tok);

    /* Only 2 arguments instead of 3 */
    Expr *args[2] = {min_expr, max_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.longMany(min, max, count) returns long[] */
static void test_random_longMany_returns_long_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "longMany", 1, "test.sn", &arena);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token min_tok;
    setup_token(&min_tok, TOKEN_LONG_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.int_value = 1L;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, long_type, false, &min_tok);

    Token max_tok;
    setup_token(&max_tok, TOKEN_LONG_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue max_val;
    max_val.int_value = 100L;
    Expr *max_expr = ast_create_literal_expr(&arena, max_val, long_type, false, &max_tok);

    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 10;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[3] = {min_expr, max_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 3, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_LONG);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.longMany() with wrong argument type reports error */
static void test_random_longMany_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "longMany", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Using int arguments instead of long for min/max */
    Token min_tok;
    setup_token(&min_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.int_value = 1;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, int_type, false, &min_tok);

    Token max_tok;
    setup_token(&max_tok, TOKEN_INT_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue max_val;
    max_val.int_value = 100;
    Expr *max_expr = ast_create_literal_expr(&arena, max_val, int_type, false, &max_tok);

    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 10;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[3] = {min_expr, max_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 3, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.doubleMany(min, max, count) returns double[] */
static void test_random_doubleMany_returns_double_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "doubleMany", 1, "test.sn", &arena);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token min_tok;
    setup_token(&min_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.double_value = 0.0;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, double_type, false, &min_tok);

    Token max_tok;
    setup_token(&max_tok, TOKEN_DOUBLE_LITERAL, "1.0", 1, "test.sn", &arena);
    LiteralValue max_val;
    max_val.double_value = 1.0;
    Expr *max_expr = ast_create_literal_expr(&arena, max_val, double_type, false, &max_tok);

    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 10;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[3] = {min_expr, max_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 3, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_DOUBLE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.doubleMany() with wrong argument type reports error */
static void test_random_doubleMany_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "doubleMany", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Using int arguments instead of double for min/max */
    Token min_tok;
    setup_token(&min_tok, TOKEN_INT_LITERAL, "0", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.int_value = 0;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, int_type, false, &min_tok);

    Token max_tok;
    setup_token(&max_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue max_val;
    max_val.int_value = 1;
    Expr *max_expr = ast_create_literal_expr(&arena, max_val, int_type, false, &max_tok);

    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 10;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[3] = {min_expr, max_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 3, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.boolMany(count) returns bool[] */
static void test_random_boolMany_returns_bool_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "boolMany", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 10;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[1] = {count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_BOOL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.boolMany() with wrong argument count reports error */
static void test_random_boolMany_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "boolMany", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.boolMany() with wrong argument type reports error */
static void test_random_boolMany_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "boolMany", 1, "test.sn", &arena);

    /* String argument instead of int */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Token count_tok;
    setup_token(&count_tok, TOKEN_STRING_LITERAL, "\"10\"", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.string_value = "10";
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, str_type, false, &count_tok);

    Expr *args[1] = {count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.gaussianMany(mean, stddev, count) returns double[] */
static void test_random_gaussianMany_returns_double_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "gaussianMany", 1, "test.sn", &arena);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Token mean_tok;
    setup_token(&mean_tok, TOKEN_DOUBLE_LITERAL, "170.0", 1, "test.sn", &arena);
    LiteralValue mean_val;
    mean_val.double_value = 170.0;
    Expr *mean_expr = ast_create_literal_expr(&arena, mean_val, double_type, false, &mean_tok);

    Token stddev_tok;
    setup_token(&stddev_tok, TOKEN_DOUBLE_LITERAL, "10.0", 1, "test.sn", &arena);
    LiteralValue stddev_val;
    stddev_val.double_value = 10.0;
    Expr *stddev_expr = ast_create_literal_expr(&arena, stddev_val, double_type, false, &stddev_tok);

    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 100;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[3] = {mean_expr, stddev_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 3, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_DOUBLE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.gaussianMany() with wrong argument count reports error */
static void test_random_gaussianMany_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "gaussianMany", 1, "test.sn", &arena);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    Token mean_tok;
    setup_token(&mean_tok, TOKEN_DOUBLE_LITERAL, "170.0", 1, "test.sn", &arena);
    LiteralValue mean_val;
    mean_val.double_value = 170.0;
    Expr *mean_expr = ast_create_literal_expr(&arena, mean_val, double_type, false, &mean_tok);

    Token stddev_tok;
    setup_token(&stddev_tok, TOKEN_DOUBLE_LITERAL, "10.0", 1, "test.sn", &arena);
    LiteralValue stddev_val;
    stddev_val.double_value = 10.0;
    Expr *stddev_expr = ast_create_literal_expr(&arena, stddev_val, double_type, false, &stddev_tok);

    /* Only 2 arguments instead of 3 */
    Expr *args[2] = {mean_expr, stddev_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.gaussianMany() with wrong argument type reports error */
static void test_random_gaussianMany_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "gaussianMany", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Using int arguments instead of double for mean/stddev */
    Token mean_tok;
    setup_token(&mean_tok, TOKEN_INT_LITERAL, "170", 1, "test.sn", &arena);
    LiteralValue mean_val;
    mean_val.int_value = 170;
    Expr *mean_expr = ast_create_literal_expr(&arena, mean_val, int_type, false, &mean_tok);

    Token stddev_tok;
    setup_token(&stddev_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue stddev_val;
    stddev_val.int_value = 10;
    Expr *stddev_expr = ast_create_literal_expr(&arena, stddev_val, int_type, false, &stddev_tok);

    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 100;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[3] = {mean_expr, stddev_expr, count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 3, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Tests for Random INSTANCE many methods (rng.method() syntax)
 * Note: Uses create_random_variable() from type_checker_tests_random_basic.c
 * ============================================================================ */

/* Test rng.intMany(min, max, count) returns int[] */
static void test_random_instance_intMany_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "intMany", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token min_tok;
    setup_token(&min_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.int_value = 1;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, int_type, false, &min_tok);

    Token max_tok;
    setup_token(&max_tok, TOKEN_INT_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue max_val;
    max_val.int_value = 100;
    Expr *max_expr = ast_create_literal_expr(&arena, max_val, int_type, false, &max_tok);

    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 10;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[3] = {min_expr, max_expr, count_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 3, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.boolMany(count) returns bool[] */
static void test_random_instance_boolMany_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "boolMany", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "5", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 5;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[1] = {count_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 1, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_BOOL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Main test runner
 * ============================================================================ */

static void test_tc_random_many_main(void)
{
    /* Static batch generation method tests */
    TEST_RUN("intMany_returns_int_array", test_random_intMany_returns_int_array);
    TEST_RUN("intMany_wrong_arg_count_error", test_random_intMany_wrong_arg_count_error);
    TEST_RUN("longMany_returns_long_array", test_random_longMany_returns_long_array);
    TEST_RUN("longMany_wrong_type_error", test_random_longMany_wrong_type_error);
    TEST_RUN("doubleMany_returns_double_array", test_random_doubleMany_returns_double_array);
    TEST_RUN("doubleMany_wrong_type_error", test_random_doubleMany_wrong_type_error);
    TEST_RUN("boolMany_returns_bool_array", test_random_boolMany_returns_bool_array);
    TEST_RUN("boolMany_wrong_arg_count_error", test_random_boolMany_wrong_arg_count_error);
    TEST_RUN("boolMany_wrong_type_error", test_random_boolMany_wrong_type_error);
    TEST_RUN("gaussianMany_returns_double_array", test_random_gaussianMany_returns_double_array);
    TEST_RUN("gaussianMany_wrong_arg_count_error", test_random_gaussianMany_wrong_arg_count_error);
    TEST_RUN("gaussianMany_wrong_type_error", test_random_gaussianMany_wrong_type_error);

    /* Instance many method tests */
    TEST_RUN("instance_intMany_method", test_random_instance_intMany_method);
    TEST_RUN("instance_boolMany_method", test_random_instance_boolMany_method);
}
