// tests/type_checker_tests_random.c
// Tests for Random.create() and Random.createWithSeed() factory method type checking

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"
#include "../ast/ast_expr.h"

/* Test Random.create() returns TYPE_RANDOM with no arguments */
static void test_random_create_returns_random_type(void)
{
    printf("Testing Random.create() returns TYPE_RANDOM...\n");
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
    printf("Testing Random.createWithSeed(long) returns TYPE_RANDOM...\n");
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
    printf("Testing Random.create() with wrong argument count reports error...\n");
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
    printf("Testing Random.createWithSeed() with no arguments reports error...\n");
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
    printf("Testing Random.createWithSeed() with wrong argument type reports error...\n");
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
    printf("Testing Random.createWithSeed() with int argument reports error...\n");
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
    printf("Testing Random.createWithSeed() with too many arguments reports error...\n");
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
    printf("Testing Random.unknownMethod() reports error...\n");
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
    printf("Testing Random.int(int, int) returns int...\n");
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
    printf("Testing Random.int() with wrong argument count reports error...\n");
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
    printf("Testing Random.int() with wrong argument type reports error...\n");
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
    printf("Testing Random.long(long, long) returns long...\n");
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
    printf("Testing Random.long() with int arguments reports error...\n");
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
    printf("Testing Random.double(double, double) returns double...\n");
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
    printf("Testing Random.double() with int arguments reports error...\n");
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
    printf("Testing Random.bool() returns bool...\n");
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
    printf("Testing Random.bool() with arguments reports error...\n");
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
    printf("Testing Random.byte() returns byte...\n");
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
    printf("Testing Random.byte() with arguments reports error...\n");
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
    printf("Testing Random.bytes(int) returns byte[]...\n");
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
    printf("Testing Random.bytes() with no arguments reports error...\n");
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
    printf("Testing Random.bytes() with string argument reports error...\n");
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
    printf("Testing Random.gaussian(double, double) returns double...\n");
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
    printf("Testing Random.gaussian() with one argument reports error...\n");
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
    printf("Testing Random.gaussian() with int arguments reports error...\n");
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
 * Tests for Random static batch generation methods
 * ============================================================================ */

/* Test Random.intMany(min, max, count) returns int[] */
static void test_random_intMany_returns_int_array(void)
{
    printf("Testing Random.intMany(int, int, int) returns int[]...\n");
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
    printf("Testing Random.intMany() with wrong argument count reports error...\n");
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
    printf("Testing Random.longMany(long, long, int) returns long[]...\n");
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
    printf("Testing Random.longMany() with int arguments reports error...\n");
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
    printf("Testing Random.doubleMany(double, double, int) returns double[]...\n");
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
    printf("Testing Random.doubleMany() with int arguments reports error...\n");
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
    printf("Testing Random.boolMany(int) returns bool[]...\n");
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
    printf("Testing Random.boolMany() with no arguments reports error...\n");
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
    printf("Testing Random.boolMany() with string argument reports error...\n");
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
    printf("Testing Random.gaussianMany(double, double, int) returns double[]...\n");
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
    printf("Testing Random.gaussianMany() with 2 arguments reports error...\n");
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
    printf("Testing Random.gaussianMany() with int arguments reports error...\n");
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
 * Tests for Random.choice() collection operation
 * ============================================================================ */

/* Test Random.choice(int[]) returns int */
static void test_random_choice_int_array_returns_int(void)
{
    printf("Testing Random.choice(int[]) returns int...\n");
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
    printf("Testing Random.choice(str[]) returns str...\n");
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
    printf("Testing Random.choice(double[]) returns double...\n");
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
    printf("Testing Random.choice(bool[]) returns bool...\n");
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
    printf("Testing Random.choice() with non-array argument reports error...\n");
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
    printf("Testing Random.choice() with string argument reports error...\n");
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
    printf("Testing Random.choice() with wrong argument count reports error...\n");
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
    printf("Testing Random.choice() with too many arguments reports error...\n");
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
 * Tests for Random.weightedChoice() collection operation
 * ============================================================================ */

/* Test Random.weightedChoice(int[], double[]) returns int */
static void test_random_weightedChoice_int_array_returns_int(void)
{
    printf("Testing Random.weightedChoice(int[], double[]) returns int...\n");
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
    printf("Testing Random.weightedChoice(str[], double[]) returns str...\n");
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
    printf("Testing Random.weightedChoice() with non-array first arg reports error...\n");
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
    printf("Testing Random.weightedChoice() with non-double[] second arg reports error...\n");
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
    printf("Testing Random.weightedChoice() with wrong argument count reports error...\n");
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
    printf("Testing Random.weightedChoice() with no arguments reports error...\n");
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
    printf("Testing rng.int(int, int) instance method returns int...\n");
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
    printf("Testing rng.long(long, long) instance method returns long...\n");
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
    printf("Testing rng.double(double, double) instance method returns double...\n");
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
    printf("Testing rng.bool() instance method returns bool...\n");
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
    printf("Testing rng.byte() instance method returns byte...\n");
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
    printf("Testing rng.bytes(int) instance method returns byte[]...\n");
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
    printf("Testing rng.gaussian(double, double) instance method returns double...\n");
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

/* Test rng.intMany(min, max, count) returns int[] */
static void test_random_instance_intMany_method(void)
{
    printf("Testing rng.intMany(int, int, int) instance method returns int[]...\n");
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
    printf("Testing rng.boolMany(int) instance method returns bool[]...\n");
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

/* Test rng.invalidMethod() reports error */
static void test_random_instance_invalid_method_error(void)
{
    printf("Testing rng.invalidMethod() instance method reports error...\n");
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

/* Test rng.choice(int[]) instance method returns int */
static void test_random_instance_choice_int_array(void)
{
    printf("Testing rng.choice(int[]) instance method returns int...\n");
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

/* Test rng.weightedChoice(int[], double[]) instance method returns int */
static void test_random_instance_weightedChoice_int_array(void)
{
    printf("Testing rng.weightedChoice(int[], double[]) instance method returns int...\n");
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

void test_type_checker_random_main(void)
{
    printf("\n=== Type Checker Random Tests ===\n");

    /* Factory method tests */
    test_random_create_returns_random_type();
    test_random_create_with_seed_returns_random_type();
    test_random_create_wrong_arg_count_error();
    test_random_create_with_seed_no_args_error();
    test_random_create_with_seed_wrong_type_error();
    test_random_create_with_seed_int_arg_error();
    test_random_create_with_seed_too_many_args_error();
    test_random_unknown_method_error();

    /* Value generation method tests */
    test_random_int_returns_int();
    test_random_int_wrong_arg_count_error();
    test_random_int_wrong_type_error();
    test_random_long_returns_long();
    test_random_long_wrong_type_error();
    test_random_double_returns_double();
    test_random_double_wrong_type_error();
    test_random_bool_returns_bool();
    test_random_bool_wrong_arg_count_error();
    test_random_byte_returns_byte();
    test_random_byte_wrong_arg_count_error();
    test_random_bytes_returns_byte_array();
    test_random_bytes_wrong_arg_count_error();
    test_random_bytes_wrong_type_error();
    test_random_gaussian_returns_double();
    test_random_gaussian_wrong_arg_count_error();
    test_random_gaussian_wrong_type_error();

    /* Batch generation method tests */
    test_random_intMany_returns_int_array();
    test_random_intMany_wrong_arg_count_error();
    test_random_longMany_returns_long_array();
    test_random_longMany_wrong_type_error();
    test_random_doubleMany_returns_double_array();
    test_random_doubleMany_wrong_type_error();
    test_random_boolMany_returns_bool_array();
    test_random_boolMany_wrong_arg_count_error();
    test_random_boolMany_wrong_type_error();
    test_random_gaussianMany_returns_double_array();
    test_random_gaussianMany_wrong_arg_count_error();
    test_random_gaussianMany_wrong_type_error();

    /* Collection operation tests: choice() */
    test_random_choice_int_array_returns_int();
    test_random_choice_str_array_returns_str();
    test_random_choice_double_array_returns_double();
    test_random_choice_bool_array_returns_bool();
    test_random_choice_non_array_error();
    test_random_choice_string_arg_error();
    test_random_choice_wrong_arg_count_error();
    test_random_choice_too_many_args_error();

    /* Collection operation tests: shuffle() */
    test_random_shuffle_int_array_returns_void();
    test_random_shuffle_str_array_returns_void();
    test_random_shuffle_double_array_returns_void();
    test_random_shuffle_non_array_error();
    test_random_shuffle_string_arg_error();
    test_random_shuffle_wrong_arg_count_error();

    /* Collection operation tests: weightedChoice() */
    test_random_weightedChoice_int_array_returns_int();
    test_random_weightedChoice_str_array_returns_str();
    test_random_weightedChoice_non_array_first_arg_error();
    test_random_weightedChoice_non_double_array_second_arg_error();
    test_random_weightedChoice_wrong_arg_count_error();
    test_random_weightedChoice_no_args_error();

    /* Collection operation tests: sample() */
    test_random_sample_int_array_returns_int_array();
    test_random_sample_str_array_returns_str_array();
    test_random_sample_non_array_first_arg_error();
    test_random_sample_non_int_second_arg_error();
    test_random_sample_wrong_arg_count_error();
    test_random_sample_no_args_error();

    /* Instance method tests */
    test_random_instance_int_method();
    test_random_instance_long_method();
    test_random_instance_double_method();
    test_random_instance_bool_method();
    test_random_instance_byte_method();
    test_random_instance_bytes_method();
    test_random_instance_gaussian_method();
    test_random_instance_intMany_method();
    test_random_instance_boolMany_method();
    test_random_instance_invalid_method_error();
    test_random_instance_choice_int_array();
    test_random_instance_shuffle_int_array();
    test_random_instance_weightedChoice_int_array();
    test_random_instance_sample_int_array();

    printf("=== All Type Checker Random Tests Passed ===\n");
}
