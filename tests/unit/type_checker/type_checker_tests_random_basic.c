// tests/unit/type_checker/type_checker_tests_random_basic.c
// Tests for Random type checking: factory methods and basic value generation
// Split from type_checker_tests_random.c

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"
#include "../ast/ast_expr.h"

/* ============================================================================
 * Tests for Random.create() and Random.createWithSeed() factory methods
 * ============================================================================ */

/* Test Random.create() returns TYPE_RANDOM with no arguments */
static void test_random_create_returns_random_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Random.create() call with no arguments */
    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "create", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    /* Type check should return TYPE_RANDOM */
    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_RANDOM);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.createWithSeed(seed) returns TYPE_RANDOM with long argument */
static void test_random_create_with_seed_returns_random_type(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Random.createWithSeed(42L) call */
    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "createWithSeed", 1, "test.sn", &arena);

    /* Create a long literal as seed argument */
    Token seed_tok;
    setup_token(&seed_tok, TOKEN_LONG_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue seed_val;
    seed_val.int_value = 42L;
    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    Expr *seed_expr = ast_create_literal_expr(&arena, seed_val, long_type, false, &seed_tok);

    Expr *args[1] = {seed_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    /* Type check should return TYPE_RANDOM */
    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_RANDOM);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.create() with wrong argument count reports error */
static void test_random_create_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Random.create(42L) call - wrong: should have no args */
    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "create", 1, "test.sn", &arena);

    /* Create a long literal as argument (but create() takes no args) */
    Token arg_tok;
    setup_token(&arg_tok, TOKEN_LONG_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue arg_val;
    arg_val.int_value = 42L;
    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, long_type, false, &arg_tok);

    Expr *args[1] = {arg_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.createWithSeed() with wrong argument count reports error (no args) */
static void test_random_create_with_seed_no_args_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Random.createWithSeed() call - wrong: needs 1 arg */
    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "createWithSeed", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.createWithSeed() with wrong argument type reports error */
static void test_random_create_with_seed_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Random.createWithSeed("hello") call - wrong: needs long, not string */
    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "createWithSeed", 1, "test.sn", &arena);

    /* Create a string literal as argument (but createWithSeed() needs long) */
    Token str_tok;
    setup_token(&str_tok, TOKEN_STRING_LITERAL, "\"hello\"", 1, "test.sn", &arena);
    LiteralValue str_val;
    str_val.string_value = "hello";
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Expr *str_expr = ast_create_literal_expr(&arena, str_val, str_type, false, &str_tok);

    Expr *args[1] = {str_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.createWithSeed() with int argument reports error (needs long) */
static void test_random_create_with_seed_int_arg_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Random.createWithSeed(42) call - wrong: needs long, not int */
    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "createWithSeed", 1, "test.sn", &arena);

    /* Create an int literal as argument (but createWithSeed() needs long) */
    Token int_tok;
    setup_token(&int_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue int_val;
    int_val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *int_expr = ast_create_literal_expr(&arena, int_val, int_type, false, &int_tok);

    Expr *args[1] = {int_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.createWithSeed() with too many arguments reports error */
static void test_random_create_with_seed_too_many_args_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Random.createWithSeed(42L, 100L) call - wrong: needs 1 arg */
    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "createWithSeed", 1, "test.sn", &arena);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);

    Token seed1_tok;
    setup_token(&seed1_tok, TOKEN_LONG_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue seed1_val;
    seed1_val.int_value = 42L;
    Expr *seed1_expr = ast_create_literal_expr(&arena, seed1_val, long_type, false, &seed1_tok);

    Token seed2_tok;
    setup_token(&seed2_tok, TOKEN_LONG_LITERAL, "100", 1, "test.sn", &arena);
    LiteralValue seed2_val;
    seed2_val.int_value = 100L;
    Expr *seed2_expr = ast_create_literal_expr(&arena, seed2_val, long_type, false, &seed2_tok);

    Expr *args[2] = {seed1_expr, seed2_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.unknownMethod() reports unknown method error */
static void test_random_unknown_method_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create Random.unknownMethod() call */
    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "unknownMethod", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Tests for Random static value generation methods
 * ============================================================================ */

/* Test Random.int(min, max) validates int parameters and returns int */
static void test_random_int_returns_int(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "int", 1, "test.sn", &arena);

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

    Expr *args[2] = {min_expr, max_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.int() with wrong argument count reports error */
static void test_random_int_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "int", 1, "test.sn", &arena);

    /* Only one argument instead of two */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token min_tok;
    setup_token(&min_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.int_value = 1;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, int_type, false, &min_tok);

    Expr *args[1] = {min_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.int() with wrong argument type reports error */
static void test_random_int_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "int", 1, "test.sn", &arena);

    /* Double arguments instead of int */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Token min_tok;
    setup_token(&min_tok, TOKEN_DOUBLE_LITERAL, "1.0", 1, "test.sn", &arena);
    LiteralValue min_val;
    min_val.double_value = 1.0;
    Expr *min_expr = ast_create_literal_expr(&arena, min_val, double_type, false, &min_tok);

    Token max_tok;
    setup_token(&max_tok, TOKEN_DOUBLE_LITERAL, "100.0", 1, "test.sn", &arena);
    LiteralValue max_val;
    max_val.double_value = 100.0;
    Expr *max_expr = ast_create_literal_expr(&arena, max_val, double_type, false, &max_tok);

    Expr *args[2] = {min_expr, max_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.long(min, max) validates long parameters and returns long */
static void test_random_long_returns_long(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "long", 1, "test.sn", &arena);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);

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

    Expr *args[2] = {min_expr, max_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_LONG);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.long() with wrong argument type reports error */
static void test_random_long_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "long", 1, "test.sn", &arena);

    /* Int arguments instead of long */
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

    Expr *args[2] = {min_expr, max_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.double(min, max) validates double parameters and returns double */
static void test_random_double_returns_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "double", 1, "test.sn", &arena);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

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

    Expr *args[2] = {min_expr, max_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_DOUBLE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.double() with wrong argument type reports error */
static void test_random_double_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "double", 1, "test.sn", &arena);

    /* Int arguments instead of double */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
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

    Expr *args[2] = {min_expr, max_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.bool() returns bool with no parameters */
static void test_random_bool_returns_bool(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "bool", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_BOOL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.bool() with arguments reports error */
static void test_random_bool_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "bool", 1, "test.sn", &arena);

    /* Pass an argument when none expected */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token arg_tok;
    setup_token(&arg_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue arg_val;
    arg_val.int_value = 1;
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, int_type, false, &arg_tok);

    Expr *args[1] = {arg_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.byte() returns byte with no parameters */
static void test_random_byte_returns_byte(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "byte", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_BYTE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.byte() with arguments reports error */
static void test_random_byte_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "byte", 1, "test.sn", &arena);

    /* Pass an argument when none expected */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token arg_tok;
    setup_token(&arg_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    LiteralValue arg_val;
    arg_val.int_value = 1;
    Expr *arg_expr = ast_create_literal_expr(&arena, arg_val, int_type, false, &arg_tok);

    Expr *args[1] = {arg_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.bytes(count) validates int parameter and returns byte[] */
static void test_random_bytes_returns_byte_array(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "bytes", 1, "test.sn", &arena);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "32", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 32;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[1] = {count_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_BYTE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.bytes() with wrong argument count reports error */
static void test_random_bytes_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "bytes", 1, "test.sn", &arena);

    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, NULL, 0, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.bytes() with wrong argument type reports error */
static void test_random_bytes_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "bytes", 1, "test.sn", &arena);

    /* String argument instead of int */
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Token count_tok;
    setup_token(&count_tok, TOKEN_STRING_LITERAL, "\"32\"", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.string_value = "32";
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

/* Test Random.gaussian(mean, stddev) validates double parameters and returns double */
static void test_random_gaussian_returns_double(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "gaussian", 1, "test.sn", &arena);

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

    Expr *args[2] = {mean_expr, stddev_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_DOUBLE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.gaussian() with wrong argument count reports error */
static void test_random_gaussian_wrong_arg_count_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "gaussian", 1, "test.sn", &arena);

    /* Only one argument instead of two */
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Token mean_tok;
    setup_token(&mean_tok, TOKEN_DOUBLE_LITERAL, "170.0", 1, "test.sn", &arena);
    LiteralValue mean_val;
    mean_val.double_value = 170.0;
    Expr *mean_expr = ast_create_literal_expr(&arena, mean_val, double_type, false, &mean_tok);

    Expr *args[1] = {mean_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 1, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test Random.gaussian() with wrong argument type reports error */
static void test_random_gaussian_wrong_type_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token type_tok;
    setup_token(&type_tok, TOKEN_IDENTIFIER, "Random", 1, "test.sn", &arena);
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "gaussian", 1, "test.sn", &arena);

    /* Int arguments instead of double */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
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

    Expr *args[2] = {mean_expr, stddev_expr};
    Expr *static_call = ast_create_static_call_expr(&arena, type_tok, method_tok, args, 2, &type_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(static_call, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Tests for Random INSTANCE methods (rng.method() syntax)
 * These test calling methods on a Random variable, not static Random.method()
 * ============================================================================ */

/* Helper to create a Random variable expression */
static Expr *create_random_variable(Arena *arena, SymbolTable *table)
{
    /* Add rng: Random to symbol table */
    Type *random_type = ast_create_primitive_type(arena, TYPE_RANDOM);
    Token rng_tok;
    setup_token(&rng_tok, TOKEN_IDENTIFIER, "rng", 1, "test.sn", arena);
    symbol_table_add_symbol(table, rng_tok, random_type);

    /* Create variable expression */
    return ast_create_variable_expr(arena, rng_tok, NULL);
}

/* Test rng.int(min, max) returns int */
static void test_random_instance_int_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create rng variable of type Random */
    Expr *rng_var = create_random_variable(&arena, &table);

    /* Create member access: rng.int */
    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "int", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    /* Create arguments */
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

    /* Create call: rng.int(1, 100) */
    Expr *args[2] = {min_expr, max_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 2, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.long(min, max) returns long */
static void test_random_instance_long_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "long", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Type *long_type = ast_create_primitive_type(&arena, TYPE_LONG);
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

    Expr *args[2] = {min_expr, max_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 2, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_LONG);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.double(min, max) returns double */
static void test_random_instance_double_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "double", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
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

    Expr *args[2] = {min_expr, max_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 2, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_DOUBLE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.bool() returns bool */
static void test_random_instance_bool_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "bool", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Expr *call_expr = ast_create_call_expr(&arena, member_expr, NULL, 0, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_BOOL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.byte() returns byte */
static void test_random_instance_byte_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "byte", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Expr *call_expr = ast_create_call_expr(&arena, member_expr, NULL, 0, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_BYTE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.bytes(count) returns byte[] */
static void test_random_instance_bytes_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "bytes", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token count_tok;
    setup_token(&count_tok, TOKEN_INT_LITERAL, "32", 1, "test.sn", &arena);
    LiteralValue count_val;
    count_val.int_value = 32;
    Expr *count_expr = ast_create_literal_expr(&arena, count_val, int_type, false, &count_tok);

    Expr *args[1] = {count_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 1, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(result->as.array.element_type->kind == TYPE_BYTE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.gaussian(mean, stddev) returns double */
static void test_random_instance_gaussian_method(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "gaussian", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Token mean_tok;
    setup_token(&mean_tok, TOKEN_DOUBLE_LITERAL, "0.0", 1, "test.sn", &arena);
    LiteralValue mean_val;
    mean_val.double_value = 0.0;
    Expr *mean_expr = ast_create_literal_expr(&arena, mean_val, double_type, false, &mean_tok);

    Token stddev_tok;
    setup_token(&stddev_tok, TOKEN_DOUBLE_LITERAL, "1.0", 1, "test.sn", &arena);
    LiteralValue stddev_val;
    stddev_val.double_value = 1.0;
    Expr *stddev_expr = ast_create_literal_expr(&arena, stddev_val, double_type, false, &stddev_tok);

    Expr *args[2] = {mean_expr, stddev_expr};
    Expr *call_expr = ast_create_call_expr(&arena, member_expr, args, 2, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_DOUBLE);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test rng.invalidMethod() reports error */
static void test_random_instance_invalid_method_error(void)
{
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Expr *rng_var = create_random_variable(&arena, &table);

    Token method_tok;
    setup_token(&method_tok, TOKEN_IDENTIFIER, "invalidMethod", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, rng_var, method_tok, NULL);

    Expr *call_expr = ast_create_call_expr(&arena, member_expr, NULL, 0, &method_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(call_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ============================================================================
 * Main test runner
 * ============================================================================ */

static void test_tc_random_basic_main(void)
{
    /* Factory method tests */
    TEST_RUN("create_returns_random_type", test_random_create_returns_random_type);
    TEST_RUN("create_with_seed_returns_random_type", test_random_create_with_seed_returns_random_type);
    TEST_RUN("create_wrong_arg_count_error", test_random_create_wrong_arg_count_error);
    TEST_RUN("create_with_seed_no_args_error", test_random_create_with_seed_no_args_error);
    TEST_RUN("create_with_seed_wrong_type_error", test_random_create_with_seed_wrong_type_error);
    TEST_RUN("create_with_seed_int_arg_error", test_random_create_with_seed_int_arg_error);
    TEST_RUN("create_with_seed_too_many_args_error", test_random_create_with_seed_too_many_args_error);
    TEST_RUN("unknown_method_error", test_random_unknown_method_error);

    /* Value generation method tests */
    TEST_RUN("int_returns_int", test_random_int_returns_int);
    TEST_RUN("int_wrong_arg_count_error", test_random_int_wrong_arg_count_error);
    TEST_RUN("int_wrong_type_error", test_random_int_wrong_type_error);
    TEST_RUN("long_returns_long", test_random_long_returns_long);
    TEST_RUN("long_wrong_type_error", test_random_long_wrong_type_error);
    TEST_RUN("double_returns_double", test_random_double_returns_double);
    TEST_RUN("double_wrong_type_error", test_random_double_wrong_type_error);
    TEST_RUN("bool_returns_bool", test_random_bool_returns_bool);
    TEST_RUN("bool_wrong_arg_count_error", test_random_bool_wrong_arg_count_error);
    TEST_RUN("byte_returns_byte", test_random_byte_returns_byte);
    TEST_RUN("byte_wrong_arg_count_error", test_random_byte_wrong_arg_count_error);
    TEST_RUN("bytes_returns_byte_array", test_random_bytes_returns_byte_array);
    TEST_RUN("bytes_wrong_arg_count_error", test_random_bytes_wrong_arg_count_error);
    TEST_RUN("bytes_wrong_type_error", test_random_bytes_wrong_type_error);
    TEST_RUN("gaussian_returns_double", test_random_gaussian_returns_double);
    TEST_RUN("gaussian_wrong_arg_count_error", test_random_gaussian_wrong_arg_count_error);
    TEST_RUN("gaussian_wrong_type_error", test_random_gaussian_wrong_type_error);

    /* Instance method tests */
    TEST_RUN("instance_int_method", test_random_instance_int_method);
    TEST_RUN("instance_long_method", test_random_instance_long_method);
    TEST_RUN("instance_double_method", test_random_instance_double_method);
    TEST_RUN("instance_bool_method", test_random_instance_bool_method);
    TEST_RUN("instance_byte_method", test_random_instance_byte_method);
    TEST_RUN("instance_bytes_method", test_random_instance_bytes_method);
    TEST_RUN("instance_gaussian_method", test_random_instance_gaussian_method);
    TEST_RUN("instance_invalid_method_error", test_random_instance_invalid_method_error);
}
