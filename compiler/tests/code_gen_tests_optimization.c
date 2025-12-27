// tests/code_gen_tests_optimization.c
// Comprehensive tests for code generation optimizations
// Tests: constant folding, native operators, edge cases, and behavior verification

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include "../arena.h"
#include "../ast.h"
#include "../code_gen.h"
#include "../code_gen_util.h"
#include "../symbol_table.h"

/* Helper to set up a token */
static void init_token(Token *tok, TokenType type, const char *lexeme)
{
    tok->type = type;
    tok->start = lexeme;
    tok->length = strlen(lexeme);
    tok->line = 1;
    tok->filename = "test.sn";
}

/* Helper to create an int literal expression */
static Expr *make_int_literal(Arena *arena, int64_t value)
{
    Token tok;
    init_token(&tok, TOKEN_INT_LITERAL, "0");

    LiteralValue lit_val;
    lit_val.int_value = value;
    Type *int_type = ast_create_primitive_type(arena, TYPE_INT);
    return ast_create_literal_expr(arena, lit_val, int_type, false, &tok);
}

/* Helper to create a long literal expression */
static Expr *make_long_literal(Arena *arena, int64_t value)
{
    Token tok;
    init_token(&tok, TOKEN_LONG_LITERAL, "0L");

    LiteralValue lit_val;
    lit_val.int_value = value;
    Type *long_type = ast_create_primitive_type(arena, TYPE_LONG);
    return ast_create_literal_expr(arena, lit_val, long_type, false, &tok);
}

/* Helper to create a double literal expression */
static Expr *make_double_literal(Arena *arena, double value)
{
    Token tok;
    init_token(&tok, TOKEN_DOUBLE_LITERAL, "0.0");

    LiteralValue lit_val;
    lit_val.double_value = value;
    Type *double_type = ast_create_primitive_type(arena, TYPE_DOUBLE);
    return ast_create_literal_expr(arena, lit_val, double_type, false, &tok);
}

/* Helper to create a bool literal expression */
static Expr *make_bool_literal(Arena *arena, bool value)
{
    Token tok;
    init_token(&tok, TOKEN_BOOL_LITERAL, value ? "true" : "false");

    LiteralValue lit_val;
    lit_val.bool_value = value ? 1 : 0;
    Type *bool_type = ast_create_primitive_type(arena, TYPE_BOOL);
    return ast_create_literal_expr(arena, lit_val, bool_type, false, &tok);
}

/* Helper to create a binary expression */
static Expr *make_binary_expr(Arena *arena, Expr *left, TokenType op, Expr *right)
{
    Token tok;
    init_token(&tok, op, "+");
    return ast_create_binary_expr(arena, left, op, right, &tok);
}

/* Helper to create a unary expression */
static Expr *make_unary_expr(Arena *arena, TokenType op, Expr *operand)
{
    Token tok;
    init_token(&tok, op, "-");
    return ast_create_unary_expr(arena, op, operand, &tok);
}

/* ============================================================================
 * CONSTANT FOLDING EDGE CASE TESTS
 * ============================================================================ */

/* Test integer overflow cases */
static void test_constant_fold_int_overflow(void)
{
    printf("Testing constant folding with integer overflow...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Create MAX + 1 - this will wrap around in C */
    Expr *left = make_long_literal(&arena, LONG_MAX);
    Expr *right = make_long_literal(&arena, 1);
    Expr *add = make_binary_expr(&arena, left, TOKEN_PLUS, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    /* Constant folding should succeed (C's undefined behavior is our undefined behavior) */
    bool success = try_fold_constant(add, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    /* Result wraps around in two's complement (this is implementation-defined) */
    assert(int_result == LONG_MIN);  /* Wrapped around */

    arena_free(&arena);
}

/* Test integer underflow cases */
static void test_constant_fold_int_underflow(void)
{
    printf("Testing constant folding with integer underflow...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Create MIN - 1 - this will wrap around in C */
    Expr *left = make_long_literal(&arena, LONG_MIN);
    Expr *right = make_long_literal(&arena, 1);
    Expr *sub = make_binary_expr(&arena, left, TOKEN_MINUS, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(sub, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    /* Result wraps around in two's complement */
    assert(int_result == LONG_MAX);  /* Wrapped around */

    arena_free(&arena);
}

/* Test multiplication overflow */
static void test_constant_fold_mul_overflow(void)
{
    printf("Testing constant folding with multiplication overflow...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Create LONG_MAX * 2 - will overflow */
    Expr *left = make_long_literal(&arena, LONG_MAX);
    Expr *right = make_long_literal(&arena, 2);
    Expr *mul = make_binary_expr(&arena, left, TOKEN_STAR, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(mul, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    /* Result will have wrapped around */
    assert(int_result == -2);  /* LONG_MAX * 2 overflows to -2 in two's complement */

    arena_free(&arena);
}

/* Test division by zero is NOT folded */
static void test_constant_fold_div_by_zero_int(void)
{
    printf("Testing constant folding rejects integer division by zero...\n");
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = make_int_literal(&arena, 10);
    Expr *right = make_int_literal(&arena, 0);
    Expr *div = make_binary_expr(&arena, left, TOKEN_SLASH, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    /* Division by zero should NOT be folded */
    bool success = try_fold_constant(div, &int_result, &double_result, &is_double);
    assert(success == false);

    arena_free(&arena);
}

/* Test modulo by zero is NOT folded */
static void test_constant_fold_mod_by_zero(void)
{
    printf("Testing constant folding rejects modulo by zero...\n");
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = make_int_literal(&arena, 10);
    Expr *right = make_int_literal(&arena, 0);
    Expr *mod = make_binary_expr(&arena, left, TOKEN_MODULO, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    /* Modulo by zero should NOT be folded */
    bool success = try_fold_constant(mod, &int_result, &double_result, &is_double);
    assert(success == false);

    arena_free(&arena);
}

/* Test double division by zero is NOT folded */
static void test_constant_fold_div_by_zero_double(void)
{
    printf("Testing constant folding rejects double division by zero...\n");
    Arena arena;
    arena_init(&arena, 4096);

    Expr *left = make_double_literal(&arena, 10.0);
    Expr *right = make_double_literal(&arena, 0.0);
    Expr *div = make_binary_expr(&arena, left, TOKEN_SLASH, right);

    int64_t int_result;
    double double_result;
    bool is_double;

    /* Division by zero should NOT be folded (even for doubles which would produce inf) */
    bool success = try_fold_constant(div, &int_result, &double_result, &is_double);
    assert(success == false);

    arena_free(&arena);
}

/* Test double edge cases */
static void test_constant_fold_double_edge_cases(void)
{
    printf("Testing constant folding with double edge cases...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Test with DBL_MAX */
    Expr *max = make_double_literal(&arena, DBL_MAX);
    Expr *one = make_double_literal(&arena, 1.0);
    Expr *add = make_binary_expr(&arena, max, TOKEN_PLUS, one);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(add, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    assert(double_result == DBL_MAX);  /* Adding 1 to DBL_MAX rounds back */

    /* Test with very small numbers */
    Expr *tiny = make_double_literal(&arena, DBL_MIN);
    Expr *two = make_double_literal(&arena, 2.0);
    Expr *div = make_binary_expr(&arena, tiny, TOKEN_SLASH, two);

    success = try_fold_constant(div, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    assert(double_result == DBL_MIN / 2.0);

    arena_free(&arena);
}

/* Test negative zero handling */
static void test_constant_fold_negative_zero(void)
{
    printf("Testing constant folding with negative zero...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* -0.0 * positive = -0.0 */
    Expr *neg_zero = make_double_literal(&arena, -0.0);
    Expr *pos = make_double_literal(&arena, 5.0);
    Expr *mul = make_binary_expr(&arena, neg_zero, TOKEN_STAR, pos);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(mul, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    /* Result should be -0.0, which equals 0.0 numerically */
    assert(double_result == 0.0);

    arena_free(&arena);
}

/* Test deeply nested constant expressions */
static void test_constant_fold_deep_nesting(void)
{
    printf("Testing constant folding with deeply nested expressions...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Build ((((1 + 2) * 3) - 4) / 2) = ((3 * 3) - 4) / 2 = (9 - 4) / 2 = 5 / 2 = 2 */
    Expr *one = make_int_literal(&arena, 1);
    Expr *two = make_int_literal(&arena, 2);
    Expr *three = make_int_literal(&arena, 3);
    Expr *four = make_int_literal(&arena, 4);
    Expr *two2 = make_int_literal(&arena, 2);

    Expr *add = make_binary_expr(&arena, one, TOKEN_PLUS, two);       /* 1 + 2 = 3 */
    Expr *mul = make_binary_expr(&arena, add, TOKEN_STAR, three);     /* 3 * 3 = 9 */
    Expr *sub = make_binary_expr(&arena, mul, TOKEN_MINUS, four);     /* 9 - 4 = 5 */
    Expr *div = make_binary_expr(&arena, sub, TOKEN_SLASH, two2);     /* 5 / 2 = 2 */

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(div, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    assert(int_result == 2);

    arena_free(&arena);
}

/* Test logical operators in constant folding */
static void test_constant_fold_logical_operators(void)
{
    printf("Testing constant folding with logical operators...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Test true && true = true */
    Expr *t1 = make_bool_literal(&arena, true);
    Expr *t2 = make_bool_literal(&arena, true);
    Expr *and_tt = make_binary_expr(&arena, t1, TOKEN_AND, t2);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(and_tt, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    assert(int_result == 1);

    /* Test true && false = false */
    Expr *f1 = make_bool_literal(&arena, false);
    Expr *and_tf = make_binary_expr(&arena, t1, TOKEN_AND, f1);

    success = try_fold_constant(and_tf, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(int_result == 0);

    /* Test false || true = true */
    Expr *or_ft = make_binary_expr(&arena, f1, TOKEN_OR, t1);
    success = try_fold_constant(or_ft, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(int_result == 1);

    /* Test false || false = false */
    Expr *f2 = make_bool_literal(&arena, false);
    Expr *or_ff = make_binary_expr(&arena, f1, TOKEN_OR, f2);
    success = try_fold_constant(or_ff, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(int_result == 0);

    arena_free(&arena);
}

/* Test unary negation edge cases */
static void test_constant_fold_unary_negation_edge(void)
{
    printf("Testing constant folding with unary negation edge cases...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Test -LONG_MIN wraps to LONG_MIN (undefined behavior in C, but we fold it) */
    Expr *min = make_long_literal(&arena, LONG_MIN);
    Expr *neg = make_unary_expr(&arena, TOKEN_MINUS, min);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(neg, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == false);
    /* -LONG_MIN overflows back to LONG_MIN in two's complement */
    assert(int_result == LONG_MIN);

    /* Test double negation */
    Expr *dbl = make_double_literal(&arena, -3.14);
    Expr *neg_dbl = make_unary_expr(&arena, TOKEN_MINUS, dbl);

    success = try_fold_constant(neg_dbl, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    assert(double_result == 3.14);

    arena_free(&arena);
}

/* Test comparison operators */
static void test_constant_fold_comparisons(void)
{
    printf("Testing constant folding with all comparison operators...\n");
    Arena arena;
    arena_init(&arena, 4096);

    Expr *five = make_int_literal(&arena, 5);
    Expr *ten = make_int_literal(&arena, 10);
    Expr *five2 = make_int_literal(&arena, 5);

    int64_t int_result;
    double double_result;
    bool is_double;
    bool success;

    /* 5 < 10 = true */
    Expr *lt = make_binary_expr(&arena, five, TOKEN_LESS, ten);
    success = try_fold_constant(lt, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    /* 5 <= 5 = true */
    Expr *le = make_binary_expr(&arena, five, TOKEN_LESS_EQUAL, five2);
    success = try_fold_constant(le, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    /* 10 > 5 = true */
    Expr *gt = make_binary_expr(&arena, ten, TOKEN_GREATER, five);
    success = try_fold_constant(gt, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    /* 5 >= 10 = false */
    Expr *ge = make_binary_expr(&arena, five, TOKEN_GREATER_EQUAL, ten);
    success = try_fold_constant(ge, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 0);

    /* 5 == 5 = true */
    Expr *eq = make_binary_expr(&arena, five, TOKEN_EQUAL_EQUAL, five2);
    success = try_fold_constant(eq, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    /* 5 != 10 = true */
    Expr *ne = make_binary_expr(&arena, five, TOKEN_BANG_EQUAL, ten);
    success = try_fold_constant(ne, &int_result, &double_result, &is_double);
    assert(success && !is_double && int_result == 1);

    arena_free(&arena);
}

/* Test double comparisons with precision issues */
static void test_constant_fold_double_comparison_precision(void)
{
    printf("Testing constant folding with double comparison precision...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* 0.1 + 0.2 should be close to but not exactly 0.3 due to floating point */
    Expr *pt1 = make_double_literal(&arena, 0.1);
    Expr *pt2 = make_double_literal(&arena, 0.2);
    Expr *sum = make_binary_expr(&arena, pt1, TOKEN_PLUS, pt2);

    int64_t int_result;
    double double_result;
    bool is_double;

    bool success = try_fold_constant(sum, &int_result, &double_result, &is_double);
    assert(success == true);
    assert(is_double == true);
    /* The result is not exactly 0.3 due to IEEE 754 representation */
    assert(double_result > 0.29 && double_result < 0.31);

    arena_free(&arena);
}

/* ============================================================================
 * NATIVE OPERATOR TESTS
 * ============================================================================ */

/* Test native operator availability */
static void test_can_use_native_operator(void)
{
    printf("Testing can_use_native_operator for all operators...\n");

    /* Operators that can use native C */
    assert(can_use_native_operator(TOKEN_PLUS) == true);
    assert(can_use_native_operator(TOKEN_MINUS) == true);
    assert(can_use_native_operator(TOKEN_STAR) == true);
    assert(can_use_native_operator(TOKEN_EQUAL_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_BANG_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_LESS) == true);
    assert(can_use_native_operator(TOKEN_LESS_EQUAL) == true);
    assert(can_use_native_operator(TOKEN_GREATER) == true);
    assert(can_use_native_operator(TOKEN_GREATER_EQUAL) == true);

    /* Operators that need runtime (division/modulo for zero check) */
    assert(can_use_native_operator(TOKEN_SLASH) == false);
    assert(can_use_native_operator(TOKEN_MODULO) == false);

    /* Unknown operators */
    assert(can_use_native_operator(TOKEN_DOT) == false);
    assert(can_use_native_operator(TOKEN_COMMA) == false);
}

/* Test get_native_c_operator returns correct strings */
static void test_get_native_c_operator(void)
{
    printf("Testing get_native_c_operator returns correct strings...\n");

    assert(strcmp(get_native_c_operator(TOKEN_PLUS), "+") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_MINUS), "-") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_STAR), "*") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_SLASH), "/") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_MODULO), "%") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_EQUAL_EQUAL), "==") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_BANG_EQUAL), "!=") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_LESS), "<") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_LESS_EQUAL), "<=") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_GREATER), ">") == 0);
    assert(strcmp(get_native_c_operator(TOKEN_GREATER_EQUAL), ">=") == 0);

    /* Unknown operators return NULL */
    assert(get_native_c_operator(TOKEN_DOT) == NULL);
}

/* Test gen_native_arithmetic in unchecked mode */
static void test_gen_native_arithmetic_unchecked(void)
{
    printf("Testing gen_native_arithmetic in unchecked mode...\n");
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, "/dev/null");
    gen.arithmetic_mode = ARITH_UNCHECKED;

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);

    /* Test integer addition */
    char *result = gen_native_arithmetic(&gen, "5L", "3L", TOKEN_PLUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "+") != NULL);

    /* Test integer subtraction */
    result = gen_native_arithmetic(&gen, "10L", "4L", TOKEN_MINUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test integer multiplication */
    result = gen_native_arithmetic(&gen, "7L", "6L", TOKEN_STAR, int_type);
    assert(result != NULL);
    assert(strstr(result, "*") != NULL);

    /* Test division should return NULL (needs runtime for zero check) */
    result = gen_native_arithmetic(&gen, "20L", "4L", TOKEN_SLASH, int_type);
    assert(result == NULL);

    /* Test double addition */
    result = gen_native_arithmetic(&gen, "3.14", "2.0", TOKEN_PLUS, double_type);
    assert(result != NULL);
    assert(strstr(result, "+") != NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test gen_native_arithmetic in checked mode returns NULL */
static void test_gen_native_arithmetic_checked(void)
{
    printf("Testing gen_native_arithmetic in checked mode returns NULL...\n");
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, "/dev/null");
    gen.arithmetic_mode = ARITH_CHECKED;  /* Default mode */

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* In checked mode, all operations should return NULL */
    char *result = gen_native_arithmetic(&gen, "5L", "3L", TOKEN_PLUS, int_type);
    assert(result == NULL);

    result = gen_native_arithmetic(&gen, "5L", "3L", TOKEN_MINUS, int_type);
    assert(result == NULL);

    result = gen_native_arithmetic(&gen, "5L", "3L", TOKEN_STAR, int_type);
    assert(result == NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test gen_native_unary */
static void test_gen_native_unary(void)
{
    printf("Testing gen_native_unary...\n");
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, "/dev/null");
    gen.arithmetic_mode = ARITH_UNCHECKED;

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);

    /* Test integer negation */
    char *result = gen_native_unary(&gen, "42L", TOKEN_MINUS, int_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test double negation */
    result = gen_native_unary(&gen, "3.14", TOKEN_MINUS, double_type);
    assert(result != NULL);
    assert(strstr(result, "-") != NULL);

    /* Test logical not */
    result = gen_native_unary(&gen, "true", TOKEN_BANG, bool_type);
    assert(result != NULL);
    assert(strstr(result, "!") != NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* ============================================================================
 * ARENA REQUIREMENT ANALYSIS TESTS
 * ============================================================================ */

/* Test type_needs_arena through function_needs_arena */
static void test_function_needs_arena_primitives_only(void)
{
    printf("Testing function_needs_arena with primitives only...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Create a simple function with only int parameters and return */
    Parameter params[2];
    Token param_name1, param_name2;
    init_token(&param_name1, TOKEN_IDENTIFIER, "a");
    init_token(&param_name2, TOKEN_IDENTIFIER, "b");
    params[0].name = param_name1;
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[0].mem_qualifier = MEM_DEFAULT;
    params[1].name = param_name2;
    params[1].type = ast_create_primitive_type(&arena, TYPE_INT);
    params[1].mem_qualifier = MEM_DEFAULT;

    Token fn_name;
    init_token(&fn_name, TOKEN_IDENTIFIER, "add");

    /* Simple return a + b */
    Token ret_tok;
    init_token(&ret_tok, TOKEN_RETURN, "return");

    Expr *a_var = arena_alloc(&arena, sizeof(Expr));
    a_var->type = EXPR_VARIABLE;
    a_var->as.variable.name = param_name1;
    a_var->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Expr *b_var = arena_alloc(&arena, sizeof(Expr));
    b_var->type = EXPR_VARIABLE;
    b_var->as.variable.name = param_name2;
    b_var->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Expr *add_expr = make_binary_expr(&arena, a_var, TOKEN_PLUS, b_var);
    add_expr->expr_type = ast_create_primitive_type(&arena, TYPE_INT);

    Stmt *ret_stmt = arena_alloc(&arena, sizeof(Stmt));
    ret_stmt->type = STMT_RETURN;
    ret_stmt->as.return_stmt.keyword = ret_tok;
    ret_stmt->as.return_stmt.value = add_expr;

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = ret_stmt;

    FunctionStmt fn = {
        .name = fn_name,
        .params = params,
        .param_count = 2,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT),
        .body = body,
        .body_count = 1,
        .modifier = FUNC_DEFAULT
    };

    /* Function with only primitives should NOT need arena */
    assert(function_needs_arena(&fn) == false);

    arena_free(&arena);
}

/* Test function_needs_arena with string return type */
static void test_function_needs_arena_string_return(void)
{
    printf("Testing function_needs_arena with string return type...\n");
    Arena arena;
    arena_init(&arena, 4096);

    Token fn_name;
    init_token(&fn_name, TOKEN_IDENTIFIER, "get_string");

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));

    /* return "hello" */
    Token ret_tok;
    init_token(&ret_tok, TOKEN_RETURN, "return");

    Token str_tok;
    init_token(&str_tok, TOKEN_STRING_LITERAL, "\"hello\"");
    LiteralValue lit_val;
    lit_val.string_value = "hello";
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Expr *str_lit = ast_create_literal_expr(&arena, lit_val, str_type, false, &str_tok);

    Stmt *ret_stmt = arena_alloc(&arena, sizeof(Stmt));
    ret_stmt->type = STMT_RETURN;
    ret_stmt->as.return_stmt.keyword = ret_tok;
    ret_stmt->as.return_stmt.value = str_lit;
    body[0] = ret_stmt;

    FunctionStmt fn = {
        .name = fn_name,
        .params = NULL,
        .param_count = 0,
        .return_type = ast_create_primitive_type(&arena, TYPE_STRING),
        .body = body,
        .body_count = 1,
        .modifier = FUNC_DEFAULT
    };

    /* Function returning string should need arena */
    assert(function_needs_arena(&fn) == true);

    arena_free(&arena);
}

/* Test expr_needs_arena for various expression types */
static void test_expr_needs_arena_types(void)
{
    printf("Testing expr_needs_arena for various expression types...\n");
    Arena arena;
    arena_init(&arena, 4096);

    /* Literals don't need arena */
    Expr *int_lit = make_int_literal(&arena, 42);
    assert(expr_needs_arena(int_lit) == false);

    /* Variables don't need arena */
    Token var_name;
    init_token(&var_name, TOKEN_IDENTIFIER, "x");
    Expr *var_expr = ast_create_variable_expr(&arena, var_name, &var_name);
    assert(expr_needs_arena(var_expr) == false);

    /* Array literals need arena */
    Expr *arr = arena_alloc(&arena, sizeof(Expr));
    arr->type = EXPR_ARRAY;
    arr->as.array.elements = NULL;
    arr->as.array.element_count = 0;
    assert(expr_needs_arena(arr) == true);

    /* Interpolated strings need arena */
    Expr *interp = arena_alloc(&arena, sizeof(Expr));
    interp->type = EXPR_INTERPOLATED;
    interp->as.interpol.parts = NULL;
    interp->as.interpol.part_count = 0;
    assert(expr_needs_arena(interp) == true);

    /* Array slices need arena */
    Expr *slice = arena_alloc(&arena, sizeof(Expr));
    slice->type = EXPR_ARRAY_SLICE;
    assert(expr_needs_arena(slice) == true);

    /* Lambda expressions need arena */
    Expr *lambda = arena_alloc(&arena, sizeof(Expr));
    lambda->type = EXPR_LAMBDA;
    assert(expr_needs_arena(lambda) == true);

    arena_free(&arena);
}

/* ============================================================================
 * TAIL CALL MARKING VERIFICATION
 * ============================================================================ */

/* Test function_has_marked_tail_calls */
static void test_function_has_marked_tail_calls_detection(void)
{
    printf("Testing function_has_marked_tail_calls detection...\n");
    Arena arena;
    arena_init(&arena, 4096);

    Token fn_name;
    init_token(&fn_name, TOKEN_IDENTIFIER, "factorial");

    /* Create return factorial(n-1) with is_tail_call = true */
    Token var_tok;
    init_token(&var_tok, TOKEN_IDENTIFIER, "factorial");

    Expr *callee = arena_alloc(&arena, sizeof(Expr));
    callee->type = EXPR_VARIABLE;
    callee->as.variable.name = var_tok;

    Expr *call = arena_alloc(&arena, sizeof(Expr));
    call->type = EXPR_CALL;
    call->as.call.callee = callee;
    call->as.call.arguments = NULL;
    call->as.call.arg_count = 0;
    call->as.call.is_tail_call = true;  /* Marked as tail call */

    Token ret_tok;
    init_token(&ret_tok, TOKEN_RETURN, "return");

    Stmt *ret_stmt = arena_alloc(&arena, sizeof(Stmt));
    ret_stmt->type = STMT_RETURN;
    ret_stmt->as.return_stmt.keyword = ret_tok;
    ret_stmt->as.return_stmt.value = call;

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *));
    body[0] = ret_stmt;

    FunctionStmt fn = {
        .name = fn_name,
        .params = NULL,
        .param_count = 0,
        .return_type = ast_create_primitive_type(&arena, TYPE_INT),
        .body = body,
        .body_count = 1,
        .modifier = FUNC_DEFAULT
    };

    /* Should detect the marked tail call */
    assert(function_has_marked_tail_calls(&fn) == true);

    /* Now test without the mark */
    call->as.call.is_tail_call = false;
    assert(function_has_marked_tail_calls(&fn) == false);

    arena_free(&arena);
}

/* ============================================================================
 * CONSTANT FOLDING CODE GENERATION TESTS
 * ============================================================================ */

/* Test try_constant_fold_binary generates correct literals */
static void test_try_constant_fold_binary_output(void)
{
    printf("Testing try_constant_fold_binary generates correct literals...\n");
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, "/dev/null");

    /* Test integer folding produces correct literal */
    Expr *left = make_int_literal(&arena, 5);
    Expr *right = make_int_literal(&arena, 3);

    BinaryExpr bin_expr;
    bin_expr.left = left;
    bin_expr.right = right;
    bin_expr.operator = TOKEN_PLUS;

    char *result = try_constant_fold_binary(&gen, &bin_expr);
    assert(result != NULL);
    assert(strcmp(result, "8L") == 0);

    /* Test multiplication */
    bin_expr.operator = TOKEN_STAR;
    result = try_constant_fold_binary(&gen, &bin_expr);
    assert(result != NULL);
    assert(strcmp(result, "15L") == 0);

    /* Test double folding */
    Expr *d_left = make_double_literal(&arena, 2.5);
    Expr *d_right = make_double_literal(&arena, 4.0);
    bin_expr.left = d_left;
    bin_expr.right = d_right;
    bin_expr.operator = TOKEN_STAR;

    result = try_constant_fold_binary(&gen, &bin_expr);
    assert(result != NULL);
    /* Should produce a double literal (10.0) */
    assert(strstr(result, "10") != NULL);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* Test try_constant_fold_unary generates correct literals */
static void test_try_constant_fold_unary_output(void)
{
    printf("Testing try_constant_fold_unary generates correct literals...\n");
    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, "/dev/null");

    /* Test integer negation */
    Expr *operand = make_int_literal(&arena, 42);

    UnaryExpr unary_expr;
    unary_expr.operand = operand;
    unary_expr.operator = TOKEN_MINUS;

    char *result = try_constant_fold_unary(&gen, &unary_expr);
    assert(result != NULL);
    assert(strcmp(result, "-42L") == 0);

    /* Test logical not on true */
    Expr *bool_operand = make_bool_literal(&arena, true);
    unary_expr.operand = bool_operand;
    unary_expr.operator = TOKEN_BANG;

    result = try_constant_fold_unary(&gen, &unary_expr);
    assert(result != NULL);
    assert(strcmp(result, "0L") == 0);

    /* Test logical not on false */
    bool_operand = make_bool_literal(&arena, false);
    unary_expr.operand = bool_operand;

    result = try_constant_fold_unary(&gen, &unary_expr);
    assert(result != NULL);
    assert(strcmp(result, "1L") == 0);

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);
    arena_free(&arena);
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

void test_code_gen_optimization_main(void)
{
    printf("\n=== Running Code Generation Optimization Tests ===\n\n");

    /* Constant folding edge case tests */
    test_constant_fold_int_overflow();
    test_constant_fold_int_underflow();
    test_constant_fold_mul_overflow();
    test_constant_fold_div_by_zero_int();
    test_constant_fold_mod_by_zero();
    test_constant_fold_div_by_zero_double();
    test_constant_fold_double_edge_cases();
    test_constant_fold_negative_zero();
    test_constant_fold_deep_nesting();
    test_constant_fold_logical_operators();
    test_constant_fold_unary_negation_edge();
    test_constant_fold_comparisons();
    test_constant_fold_double_comparison_precision();

    /* Native operator tests */
    test_can_use_native_operator();
    test_get_native_c_operator();
    test_gen_native_arithmetic_unchecked();
    test_gen_native_arithmetic_checked();
    test_gen_native_unary();

    /* Arena requirement tests */
    test_function_needs_arena_primitives_only();
    test_function_needs_arena_string_return();
    test_expr_needs_arena_types();

    /* Tail call marking tests */
    test_function_has_marked_tail_calls_detection();

    /* Constant folding code generation tests */
    test_try_constant_fold_binary_output();
    test_try_constant_fold_unary_output();

    printf("\n=== All Code Generation Optimization Tests Passed ===\n");
}
