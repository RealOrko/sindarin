// tests/type_checker_tests_native.c
// Tests for native function context tracking and pointer variable restrictions

#include "../type_checker/type_checker_util.h"
#include "../type_checker.h"
#include "../ast.h"
#include "../ast/ast_expr.h"
#include "../ast/ast_stmt.h"
#include "../symbol_table.h"
#include "../arena.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Helper to set up a token for testing */
static void setup_test_token(Token *tok, TokenType type, const char *lexeme, int line, const char *filename, Arena *arena)
{
    tok->type = type;
    tok->line = line;
    size_t lex_len = strlen(lexeme);
    char *allocated_lexeme = (char *)arena_alloc(arena, lex_len + 1);
    memcpy(allocated_lexeme, lexeme, lex_len + 1);
    tok->start = allocated_lexeme;
    tok->length = (int)lex_len;
    tok->filename = filename;
}

/* Test that native_context_is_active returns false by default */
static void test_native_context_default_inactive(void)
{
    printf("Testing native_context_is_active default state...\n");
    /* Ensure we're starting fresh - exit any leftover context */
    while (native_context_is_active())
    {
        native_context_exit();
    }
    assert(native_context_is_active() == false);
}

/* Test that native_context_enter activates the context */
static void test_native_context_enter(void)
{
    printf("Testing native_context_enter...\n");
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }
    assert(native_context_is_active() == false);

    native_context_enter();
    assert(native_context_is_active() == true);

    /* Cleanup */
    native_context_exit();
    assert(native_context_is_active() == false);
}

/* Test that native_context_exit deactivates the context */
static void test_native_context_exit(void)
{
    printf("Testing native_context_exit...\n");
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }

    native_context_enter();
    assert(native_context_is_active() == true);

    native_context_exit();
    assert(native_context_is_active() == false);
}

/* Test nested native contexts (native function calling another native function) */
static void test_native_context_nesting(void)
{
    printf("Testing native_context nesting...\n");
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }

    /* Enter outer native function */
    native_context_enter();
    assert(native_context_is_active() == true);

    /* Enter inner native function (nested) */
    native_context_enter();
    assert(native_context_is_active() == true);

    /* Exit inner native function */
    native_context_exit();
    assert(native_context_is_active() == true);  /* Still in outer */

    /* Exit outer native function */
    native_context_exit();
    assert(native_context_is_active() == false);  /* Now inactive */
}

/* Test that excessive exits don't go negative */
static void test_native_context_excessive_exit(void)
{
    printf("Testing native_context excessive exit safety...\n");
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }

    /* Try to exit when not active - should be safe */
    native_context_exit();
    native_context_exit();
    native_context_exit();
    assert(native_context_is_active() == false);

    /* Should still work after excessive exits */
    native_context_enter();
    assert(native_context_is_active() == true);
    native_context_exit();
    assert(native_context_is_active() == false);
}

/* Test multiple enter/exit cycles */
static void test_native_context_multiple_cycles(void)
{
    printf("Testing native_context multiple enter/exit cycles...\n");
    /* Start from inactive state */
    while (native_context_is_active())
    {
        native_context_exit();
    }

    for (int i = 0; i < 5; i++)
    {
        assert(native_context_is_active() == false);
        native_context_enter();
        assert(native_context_is_active() == true);
        native_context_exit();
        assert(native_context_is_active() == false);
    }
}

/* Test that pointer variables are REJECTED in regular (non-native) functions */
static void test_pointer_var_rejected_in_regular_function(void)
{
    printf("Testing pointer variable rejected in regular function...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Wrap in a REGULAR function (not native) */
    Stmt *body[1] = {p_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "regular_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;  /* Regular function */

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - pointer vars not allowed in regular functions */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer variables are ACCEPTED in native functions */
static void test_pointer_var_accepted_in_native_function(void)
{
    printf("Testing pointer variable accepted in native function...\n");

    Arena arena;
    arena_init(&arena, 4096);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Wrap in a NATIVE function */
    Stmt *body[1] = {p_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "native_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = true;  /* Native function */

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should PASS - pointer vars allowed in native functions */

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test helper: create a binary expression with pointer and int */
static Stmt *create_pointer_arithmetic_stmt(Arena *arena, Type *ptr_type, Type *int_type, TokenType op)
{
    /* Create a pointer variable reference */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", arena);
    Expr *p_ref = ast_create_variable_expr(arena, p_tok, &p_tok);
    p_ref->expr_type = ptr_type;

    /* Create int literal 1 */
    Token lit_tok;
    setup_test_token(&lit_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", arena);
    LiteralValue val = {.int_value = 1};
    Expr *lit = ast_create_literal_expr(arena, val, int_type, false, &lit_tok);

    /* Create binary expression: p + 1 */
    Token op_tok;
    setup_test_token(&op_tok, TOKEN_PLUS, "+", 1, "test.sn", arena);
    Expr *binary = ast_create_binary_expr(arena, p_ref, op, lit, &op_tok);

    /* Wrap in expression statement */
    return ast_create_expr_stmt(arena, binary, &op_tok);
}

/* Test that pointer arithmetic is REJECTED for all operators (+, -, *, /, %) */
static void test_pointer_arithmetic_rejected(void)
{
    printf("Testing pointer arithmetic rejected for all operators...\n");

    /* Test each arithmetic operator */
    TokenType operators[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MODULO};
    const char *op_names[] = {"+", "-", "*", "/", "%%"};
    int num_ops = sizeof(operators) / sizeof(operators[0]);

    for (int i = 0; i < num_ops; i++)
    {
        Arena arena;
        arena_init(&arena, 8192);

        SymbolTable table;
        symbol_table_init(&arena, &table);

        Module module;
        ast_init_module(&arena, &module, "test.sn");

        Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
        Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
        Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
        Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

        /* Create: var p: *int = nil */
        Token p_tok;
        setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
        Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

        /* Create: p + 1 (or p - 1, etc.) */
        Stmt *arith_stmt = create_pointer_arithmetic_stmt(&arena, ptr_int_type, int_type, operators[i]);

        /* Wrap in a native function (to allow pointer var declaration) */
        Stmt *body[2] = {p_decl, arith_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        if (no_error != 0)
        {
            printf("FAIL: Pointer arithmetic with '%s' should be rejected but passed\n", op_names[i]);
            assert(no_error == 0);
        }

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

    printf("  All pointer arithmetic operators correctly rejected\n");
}

/* Test helper: create a comparison expression with two pointers */
static Stmt *create_pointer_comparison_stmt(Arena *arena, Type *ptr_type, TokenType op, bool use_nil_as_right)
{
    /* Create a pointer variable reference */
    Token p1_tok;
    setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", arena);
    Expr *p1_ref = ast_create_variable_expr(arena, p1_tok, &p1_tok);
    p1_ref->expr_type = ptr_type;

    Expr *right_operand;
    if (use_nil_as_right)
    {
        /* Create nil literal */
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", arena);
        Type *nil_type = ast_create_primitive_type(arena, TYPE_NIL);
        LiteralValue nil_val = {.int_value = 0};
        right_operand = ast_create_literal_expr(arena, nil_val, nil_type, false, &nil_tok);
    }
    else
    {
        /* Create second pointer variable reference */
        Token p2_tok;
        setup_test_token(&p2_tok, TOKEN_IDENTIFIER, "p2", 1, "test.sn", arena);
        right_operand = ast_create_variable_expr(arena, p2_tok, &p2_tok);
        right_operand->expr_type = ptr_type;
    }

    /* Create binary expression: p1 == p2 or p1 == nil */
    Token op_tok;
    setup_test_token(&op_tok, op == TOKEN_EQUAL_EQUAL ? TOKEN_EQUAL_EQUAL : TOKEN_BANG_EQUAL,
                     op == TOKEN_EQUAL_EQUAL ? "==" : "!=", 1, "test.sn", arena);
    Expr *binary = ast_create_binary_expr(arena, p1_ref, op, right_operand, &op_tok);

    /* Wrap in expression statement */
    return ast_create_expr_stmt(arena, binary, &op_tok);
}

/* Test that pointer equality (==, !=) with nil is ALLOWED */
static void test_pointer_nil_comparison_allowed(void)
{
    printf("Testing pointer nil comparison (== and !=) allowed...\n");

    TokenType operators[] = {TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL};
    const char *op_names[] = {"==", "!="};
    int num_ops = sizeof(operators) / sizeof(operators[0]);

    for (int i = 0; i < num_ops; i++)
    {
        Arena arena;
        arena_init(&arena, 8192);

        SymbolTable table;
        symbol_table_init(&arena, &table);

        Module module;
        ast_init_module(&arena, &module, "test.sn");

        Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
        Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
        Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
        Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

        /* Create: var p1: *int = nil */
        Token p1_tok;
        setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", &arena);
        Token nil_tok;
        setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
        Stmt *p1_decl = ast_create_var_decl_stmt(&arena, p1_tok, ptr_int_type, nil_lit, NULL);

        /* Create: p1 == nil or p1 != nil */
        Stmt *compare_stmt = create_pointer_comparison_stmt(&arena, ptr_int_type, operators[i], true);

        /* Wrap in a native function */
        Stmt *body[2] = {p1_decl, compare_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        if (no_error != 1)
        {
            printf("FAIL: Pointer nil comparison with '%s' should be allowed but failed\n", op_names[i]);
            assert(no_error == 1);
        }

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

    printf("  Pointer nil comparison correctly allowed\n");
}

/* Test that pointer-to-pointer equality (==, !=) is ALLOWED */
static void test_pointer_pointer_comparison_allowed(void)
{
    printf("Testing pointer-to-pointer comparison (== and !=) allowed...\n");

    TokenType operators[] = {TOKEN_EQUAL_EQUAL, TOKEN_BANG_EQUAL};
    const char *op_names[] = {"==", "!="};
    int num_ops = sizeof(operators) / sizeof(operators[0]);

    for (int i = 0; i < num_ops; i++)
    {
        Arena arena;
        arena_init(&arena, 8192);

        SymbolTable table;
        symbol_table_init(&arena, &table);

        Module module;
        ast_init_module(&arena, &module, "test.sn");

        Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
        Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
        Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
        Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

        /* Create: var p1: *int = nil */
        Token p1_tok;
        setup_test_token(&p1_tok, TOKEN_IDENTIFIER, "p1", 1, "test.sn", &arena);
        Token nil_tok1;
        setup_test_token(&nil_tok1, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        LiteralValue nil_val = {.int_value = 0};
        Expr *nil_lit1 = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok1);
        Stmt *p1_decl = ast_create_var_decl_stmt(&arena, p1_tok, ptr_int_type, nil_lit1, NULL);

        /* Create: var p2: *int = nil */
        Token p2_tok;
        setup_test_token(&p2_tok, TOKEN_IDENTIFIER, "p2", 1, "test.sn", &arena);
        Token nil_tok2;
        setup_test_token(&nil_tok2, TOKEN_NIL, "nil", 1, "test.sn", &arena);
        Expr *nil_lit2 = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok2);
        Stmt *p2_decl = ast_create_var_decl_stmt(&arena, p2_tok, ptr_int_type, nil_lit2, NULL);

        /* Create: p1 == p2 or p1 != p2 */
        Stmt *compare_stmt = create_pointer_comparison_stmt(&arena, ptr_int_type, operators[i], false);

        /* Wrap in a native function */
        Stmt *body[3] = {p1_decl, p2_decl, compare_stmt};
        Token func_name_tok;
        setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
        Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 3, &func_name_tok);
        func_decl->as.function.is_native = true;

        ast_module_add_statement(&arena, &module, func_decl);

        int no_error = type_check_module(&module, &table);
        if (no_error != 1)
        {
            printf("FAIL: Pointer-to-pointer comparison with '%s' should be allowed but failed\n", op_names[i]);
            assert(no_error == 1);
        }

        symbol_table_cleanup(&table);
        arena_free(&arena);
    }

    printf("  Pointer-to-pointer comparison correctly allowed\n");
}

/* Test that inline pointer passing (e.g., use_ptr(get_ptr())) is allowed */
static void test_inline_pointer_passing_allowed(void)
{
    printf("Testing inline pointer passing (use_ptr(get_ptr())) allowed...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: native fn use_ptr(ptr: *int): void (forward declaration) */
    Token use_ptr_tok;
    setup_test_token(&use_ptr_tok, TOKEN_IDENTIFIER, "use_ptr", 2, "test.sn", &arena);
    Token ptr_param_tok;
    setup_test_token(&ptr_param_tok, TOKEN_IDENTIFIER, "ptr", 2, "test.sn", &arena);
    Parameter use_ptr_params[1];
    use_ptr_params[0].name = ptr_param_tok;
    use_ptr_params[0].type = ptr_int_type;
    Stmt *use_ptr_decl = ast_create_function_stmt(&arena, use_ptr_tok, use_ptr_params, 1, void_type, NULL, 0, &use_ptr_tok);
    use_ptr_decl->as.function.is_native = true;

    /* Create call: get_ptr() */
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);

    /* Create call: use_ptr(get_ptr()) - inline pointer passing */
    Token use_ptr_call_tok;
    setup_test_token(&use_ptr_call_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Expr *use_ptr_callee = ast_create_variable_expr(&arena, use_ptr_call_tok, &use_ptr_call_tok);
    Expr *args[1] = {get_ptr_call};
    Expr *inline_call = ast_create_call_expr(&arena, use_ptr_callee, args, 1, &use_ptr_call_tok);

    /* Wrap in expression statement */
    Stmt *call_stmt = ast_create_expr_stmt(&arena, inline_call, &use_ptr_call_tok);

    /* Wrap in main function */
    Stmt *main_body[1] = {call_stmt};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;  /* Regular function doing inline call */

    /* Add all to module */
    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, use_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    if (no_error != 1)
    {
        printf("FAIL: Inline pointer passing use_ptr(get_ptr()) should be allowed\n");
        assert(no_error == 1);
    }

    printf("  Inline pointer passing correctly allowed\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test inline pointer passing with nil is allowed */
static void test_inline_nil_passing_allowed(void)
{
    printf("Testing inline nil passing (use_ptr(nil)) allowed...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn use_ptr(ptr: *int): void (forward declaration) */
    Token use_ptr_tok;
    setup_test_token(&use_ptr_tok, TOKEN_IDENTIFIER, "use_ptr", 1, "test.sn", &arena);
    Token ptr_param_tok;
    setup_test_token(&ptr_param_tok, TOKEN_IDENTIFIER, "ptr", 1, "test.sn", &arena);
    Parameter use_ptr_params[1];
    use_ptr_params[0].name = ptr_param_tok;
    use_ptr_params[0].type = ptr_int_type;
    Stmt *use_ptr_decl = ast_create_function_stmt(&arena, use_ptr_tok, use_ptr_params, 1, void_type, NULL, 0, &use_ptr_tok);
    use_ptr_decl->as.function.is_native = true;

    /* Create nil literal */
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 5, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);

    /* Create call: use_ptr(nil) */
    Token use_ptr_call_tok;
    setup_test_token(&use_ptr_call_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Expr *use_ptr_callee = ast_create_variable_expr(&arena, use_ptr_call_tok, &use_ptr_call_tok);
    Expr *args[1] = {nil_lit};
    Expr *nil_call = ast_create_call_expr(&arena, use_ptr_callee, args, 1, &use_ptr_call_tok);

    /* Wrap in expression statement */
    Stmt *call_stmt = ast_create_expr_stmt(&arena, nil_call, &use_ptr_call_tok);

    /* Wrap in main function */
    Stmt *main_body[1] = {call_stmt};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    /* Add all to module */
    ast_module_add_statement(&arena, &module, use_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    if (no_error != 1)
    {
        printf("FAIL: Inline nil passing use_ptr(nil) should be allowed\n");
        assert(no_error == 1);
    }

    printf("  Inline nil passing correctly allowed\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *int to int */
static void test_as_val_unwraps_pointer_int(void)
{
    printf("Testing 'as val' unwraps *int to int...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Create: var x: int = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *int as val => int */

    /* Verify the expression type is int */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_INT);

    printf("  '*int as val' correctly typed as int\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *double to double */
static void test_as_val_unwraps_pointer_double(void)
{
    printf("Testing 'as val' unwraps *double to double...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_double_type = ast_create_pointer_type(&arena, double_type);

    /* Create: var p: *double = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_double_type, nil_lit, NULL);

    /* Create: var x: double = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, double_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *double as val => double */

    /* Verify the expression type is double */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_DOUBLE);

    printf("  '*double as val' correctly typed as double\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' rejects non-pointer operand (int as val should error) */
static void test_as_val_rejects_non_pointer(void)
{
    printf("Testing 'as val' rejects non-pointer operand (int as val)...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: var n: int = 42 */
    Token n_tok;
    setup_test_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token lit_tok;
    setup_test_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, int_type, lit, NULL);

    /* Create: var x: int = n as val (THIS SHOULD FAIL - n is int, not *int) */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token n_ref_tok;
    setup_test_token(&n_ref_tok, TOKEN_IDENTIFIER, "n", 2, "test.sn", &arena);
    Expr *n_ref = ast_create_variable_expr(&arena, n_ref_tok, &n_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, n_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {n_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: int as val is not allowed */

    printf("  'int as val' correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' correctly unwraps *float to float */
static void test_as_val_unwraps_pointer_float(void)
{
    printf("Testing 'as val' unwraps *float to float...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_float_type = ast_create_pointer_type(&arena, float_type);

    /* Create: var p: *float = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_float_type, nil_lit, NULL);

    /* Create: var x: float = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, float_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *float as val => float */

    /* Verify the expression type is float */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_FLOAT);

    printf("  '*float as val' correctly typed as float\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test: *char as val converts to str (null-terminated string) */
void test_as_val_char_pointer_to_str(void)
{
    printf("Testing: *char as val => str (null-terminated string)...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_char_type = ast_create_pointer_type(&arena, char_type);

    /* Create: var p: *char = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_char_type, nil_lit, NULL);

    /* Create: var s: str = p as val */
    Token s_tok;
    setup_test_token(&s_tok, TOKEN_IDENTIFIER, "s", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Stmt *s_decl = ast_create_var_decl_stmt(&arena, s_tok, str_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, s_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *char as val => str */

    /* Verify the expression type is str */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_STRING);

    /* Verify the metadata flag is set */
    assert(as_val_expr->as.as_val.is_cstr_to_str == true);

    printf("  '*char as val' correctly typed as str with is_cstr_to_str=true\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test: *int as val does NOT set is_cstr_to_str flag */
void test_as_val_int_pointer_no_cstr_flag(void)
{
    printf("Testing: *int as val does NOT set is_cstr_to_str...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Create: var x: int = p as val */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 2, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, x_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *int as val => int */

    /* Verify the expression type is int */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_INT);

    /* Verify the metadata flag is NOT set */
    assert(as_val_expr->as.as_val.is_cstr_to_str == false);

    printf("  '*int as val' correctly typed as int with is_cstr_to_str=false\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer return from native fn WITHOUT 'as val' fails in regular function */
static void test_pointer_return_without_as_val_fails_in_regular_fn(void)
{
    printf("Testing pointer return without 'as val' fails in regular function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var x: int = get_ptr() -- missing 'as val', should fail */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, get_ptr_call, NULL);

    /* Wrap in regular (non-native) function */
    Stmt *main_body[1] = {x_decl};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: pointer return without as val in regular function */

    printf("  Pointer return without 'as val' correctly rejected in regular function\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer return from native fn WITH 'as val' succeeds in regular function */
static void test_pointer_return_with_as_val_succeeds_in_regular_fn(void)
{
    printf("Testing pointer return with 'as val' succeeds in regular function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var x: int = get_ptr() as val -- with 'as val', should succeed */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 5, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, get_ptr_call, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, as_val_expr, NULL);

    /* Wrap in regular (non-native) function */
    Stmt *main_body[1] = {x_decl};
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_func = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, main_body, 1, &main_tok);
    main_func->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, main_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED: pointer return with as val in regular function */

    printf("  Pointer return with 'as val' correctly allowed in regular function\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native functions can store pointer return values without 'as val' */
static void test_native_fn_can_store_pointer_return(void)
{
    printf("Testing native function can store pointer return without 'as val'...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);

    /* Create: native fn get_ptr(): *int (forward declaration) */
    Token get_ptr_tok;
    setup_test_token(&get_ptr_tok, TOKEN_IDENTIFIER, "get_ptr", 1, "test.sn", &arena);
    Stmt *get_ptr_decl = ast_create_function_stmt(&arena, get_ptr_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ptr_tok);
    get_ptr_decl->as.function.is_native = true;

    /* Create: var p: *int = get_ptr() -- allowed in native function */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 5, "test.sn", &arena);
    Token get_ptr_call_tok;
    setup_test_token(&get_ptr_call_tok, TOKEN_IDENTIFIER, "get_ptr", 5, "test.sn", &arena);
    Expr *get_ptr_callee = ast_create_variable_expr(&arena, get_ptr_call_tok, &get_ptr_call_tok);
    Expr *get_ptr_call = ast_create_call_expr(&arena, get_ptr_callee, NULL, 0, &get_ptr_call_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, get_ptr_call, NULL);

    /* Wrap in native function */
    Stmt *native_body[1] = {p_decl};
    Token native_tok;
    setup_test_token(&native_tok, TOKEN_IDENTIFIER, "use_ptr", 5, "test.sn", &arena);
    Stmt *native_func = ast_create_function_stmt(&arena, native_tok, NULL, 0, void_type, native_body, 1, &native_tok);
    native_func->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, get_ptr_decl);
    ast_module_add_statement(&arena, &module, native_func);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED: native function can store pointer returns */

    printf("  Native function can correctly store pointer return values\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer slice *byte[0..10] produces byte[] */
static void test_pointer_slice_byte_to_byte_array(void)
{
    printf("Testing pointer slice *byte[0..10] => byte[]...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: var p: *byte = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_byte_type, nil_lit, NULL);

    /* Create slice expression: p[0..10] */
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 10};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, p_ref, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var data: byte[] = p[0..10] */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, slice_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *byte[0..10] => byte[] */

    /* Verify the slice expression type is byte[] */
    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_BYTE);

    printf("  '*byte[0..10]' correctly typed as byte[]\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer slice *int[0..5] produces int[] */
static void test_pointer_slice_int_to_int_array(void)
{
    printf("Testing pointer slice *int[0..5] => int[]...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: var p: *int = nil */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 1, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 1, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_int_type, nil_lit, NULL);

    /* Create slice expression: p[0..5] */
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 2, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "5", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 5};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, p_ref, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var data: int[] = p[0..5] */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, int_array_type, slice_expr, NULL);

    /* Wrap in a native function */
    Stmt *body[2] = {p_decl, data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: *int[0..5] => int[] */

    /* Verify the slice expression type is int[] */
    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    printf("  '*int[0..5]' correctly typed as int[]\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that slicing a non-array, non-pointer type fails (e.g., int[0..5]) */
static void test_slice_non_array_non_pointer_fails(void)
{
    printf("Testing slice on non-array, non-pointer type fails (int[0..5])...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: var n: int = 42 */
    Token n_tok;
    setup_test_token(&n_tok, TOKEN_IDENTIFIER, "n", 1, "test.sn", &arena);
    Token lit_tok;
    setup_test_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    LiteralValue val = {.int_value = 42};
    Expr *lit = ast_create_literal_expr(&arena, val, int_type, false, &lit_tok);
    Stmt *n_decl = ast_create_var_decl_stmt(&arena, n_tok, int_type, lit, NULL);

    /* Create slice expression: n[0..5] - THIS SHOULD FAIL, n is int not array/pointer */
    Token n_ref_tok;
    setup_test_token(&n_ref_tok, TOKEN_IDENTIFIER, "n", 2, "test.sn", &arena);
    Expr *n_ref = ast_create_variable_expr(&arena, n_ref_tok, &n_ref_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "5", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 5};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, n_ref, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var data: int[] = n[0..5] */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, int_array_type, slice_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {n_decl, data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: cannot slice int */

    printf("  Slice on int correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that array slicing still works correctly (regression test) */
static void test_array_slice_still_works(void)
{
    printf("Testing array slice still works (regression test)...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: var arr: int[] = {1, 2, 3, 4, 5} */
    Token arr_tok;
    setup_test_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    LiteralValue v1 = {.int_value = 1};
    LiteralValue v2 = {.int_value = 2};
    LiteralValue v3 = {.int_value = 3};
    Token lit1_tok; setup_test_token(&lit1_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    Token lit2_tok; setup_test_token(&lit2_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    Token lit3_tok; setup_test_token(&lit3_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    Expr *e1 = ast_create_literal_expr(&arena, v1, int_type, false, &lit1_tok);
    Expr *e2 = ast_create_literal_expr(&arena, v2, int_type, false, &lit2_tok);
    Expr *e3 = ast_create_literal_expr(&arena, v3, int_type, false, &lit3_tok);
    Expr *elements[3] = {e1, e2, e3};

    Token brace_tok;
    setup_test_token(&brace_tok, TOKEN_LEFT_BRACE, "{", 1, "test.sn", &arena);
    Expr *arr_lit = ast_create_array_expr(&arena, elements, 3, &brace_tok);

    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, int_array_type, arr_lit, NULL);

    /* Create slice expression: arr[1..3] */
    Token arr_ref_tok;
    setup_test_token(&arr_ref_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);
    Expr *arr_ref = ast_create_variable_expr(&arena, arr_ref_tok, &arr_ref_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "1", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 1};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "3", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 3};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, arr_ref, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var slice: int[] = arr[1..3] */
    Token slice_tok;
    setup_test_token(&slice_tok, TOKEN_IDENTIFIER, "slice", 2, "test.sn", &arena);
    Stmt *slice_decl = ast_create_var_decl_stmt(&arena, slice_tok, int_array_type, slice_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {arr_decl, slice_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: array slicing still works */

    /* Verify the slice expression type is int[] */
    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    printf("  Array slice still correctly typed as int[]\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that as_val context tracking functions work */
static void test_as_val_context_tracking(void)
{
    printf("Testing as_val context tracking...\n");

    /* Default: not active */
    assert(as_val_context_is_active() == false);

    /* Enter: active */
    as_val_context_enter();
    assert(as_val_context_is_active() == true);

    /* Nesting: still active */
    as_val_context_enter();
    assert(as_val_context_is_active() == true);

    /* Exit once: still active (nested) */
    as_val_context_exit();
    assert(as_val_context_is_active() == true);

    /* Exit again: inactive */
    as_val_context_exit();
    assert(as_val_context_is_active() == false);

    printf("  as_val context tracking works correctly\n");
}

/* Test that pointer slice with 'as val' works in regular function */
static void test_pointer_slice_with_as_val_in_regular_fn(void)
{
    printf("Testing pointer slice with 'as val' in regular function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: native fn get_data(): *byte (forward declaration) */
    Token get_data_tok;
    setup_test_token(&get_data_tok, TOKEN_IDENTIFIER, "get_data", 1, "test.sn", &arena);
    Stmt *get_data_decl = ast_create_function_stmt(&arena, get_data_tok, NULL, 0, ptr_byte_type, NULL, 0, &get_data_tok);
    get_data_decl->as.function.is_native = true;

    /* Create slice expression: get_data()[0..10] */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_data", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 10};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, NULL, &bracket_tok);

    /* Wrap slice in 'as val': get_data()[0..10] as val */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var data: byte[] = get_data()[0..10] as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;  /* REGULAR function */

    ast_module_add_statement(&arena, &module, get_data_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED: ptr[0..10] as val is OK in regular fn */

    /* Verify the as_val expression type is byte[] */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_ARRAY);
    assert(as_val_expr->expr_type->as.array.element_type->kind == TYPE_BYTE);

    /* Verify is_noop is true (slice already produces array type) */
    assert(as_val_expr->as.as_val.is_noop == true);
    assert(as_val_expr->as.as_val.is_cstr_to_str == false);

    /* Verify is_from_pointer is true on the inner slice expression */
    assert(slice_expr->as.array_slice.is_from_pointer == true);

    printf("  Pointer slice with 'as val' correctly allowed in regular function\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer slice WITHOUT 'as val' fails in regular function */
static void test_pointer_slice_without_as_val_in_regular_fn_fails(void)
{
    printf("Testing pointer slice without 'as val' fails in regular function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: native fn get_data(): *byte (forward declaration) */
    Token get_data_tok;
    setup_test_token(&get_data_tok, TOKEN_IDENTIFIER, "get_data", 1, "test.sn", &arena);
    Stmt *get_data_decl = ast_create_function_stmt(&arena, get_data_tok, NULL, 0, ptr_byte_type, NULL, 0, &get_data_tok);
    get_data_decl->as.function.is_native = true;

    /* Create slice expression: get_data()[0..10] -- WITHOUT as val */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_data", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 10};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, NULL, &bracket_tok);

    /* Create: var data: byte[] = get_data()[0..10] -- NO as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, slice_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;  /* REGULAR function */

    ast_module_add_statement(&arena, &module, get_data_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: ptr[0..10] without as val not allowed in regular fn */

    printf("  Pointer slice without 'as val' correctly rejected in regular function\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as val' on array types works (no-op) */
static void test_as_val_on_array_type_is_noop(void)
{
    printf("Testing 'as val' on array type is no-op...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: var arr: int[] = {1, 2, 3} */
    Token arr_tok;
    setup_test_token(&arr_tok, TOKEN_IDENTIFIER, "arr", 1, "test.sn", &arena);

    Token one_tok, two_tok, three_tok;
    setup_test_token(&one_tok, TOKEN_INT_LITERAL, "1", 1, "test.sn", &arena);
    setup_test_token(&two_tok, TOKEN_INT_LITERAL, "2", 1, "test.sn", &arena);
    setup_test_token(&three_tok, TOKEN_INT_LITERAL, "3", 1, "test.sn", &arena);
    LiteralValue one_val = {.int_value = 1};
    LiteralValue two_val = {.int_value = 2};
    LiteralValue three_val = {.int_value = 3};
    Expr *one_lit = ast_create_literal_expr(&arena, one_val, int_type, false, &one_tok);
    Expr *two_lit = ast_create_literal_expr(&arena, two_val, int_type, false, &two_tok);
    Expr *three_lit = ast_create_literal_expr(&arena, three_val, int_type, false, &three_tok);
    Expr **elements = arena_alloc(&arena, sizeof(Expr *) * 3);
    elements[0] = one_lit;
    elements[1] = two_lit;
    elements[2] = three_lit;
    Expr *array_expr = ast_create_array_expr(&arena, elements, 3, &arr_tok);
    Stmt *arr_decl = ast_create_var_decl_stmt(&arena, arr_tok, int_array_type, array_expr, NULL);

    /* Create: arr as val */
    Token arr_ref_tok;
    setup_test_token(&arr_ref_tok, TOKEN_IDENTIFIER, "arr", 2, "test.sn", &arena);
    Expr *arr_ref = ast_create_variable_expr(&arena, arr_ref_tok, &arr_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, arr_ref, &as_tok);

    /* Create: var copy: int[] = arr as val */
    Token copy_tok;
    setup_test_token(&copy_tok, TOKEN_IDENTIFIER, "copy", 2, "test.sn", &arena);
    Stmt *copy_decl = ast_create_var_decl_stmt(&arena, copy_tok, int_array_type, as_val_expr, NULL);

    /* Wrap in a function */
    Stmt *body[2] = {arr_decl, copy_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: 'as val' on array is no-op */

    /* Verify the as_val expression type is int[] */
    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_ARRAY);
    assert(as_val_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    printf("  'as val' on array type correctly returns same array type\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that get_buffer()[0..len] as val correctly infers byte[] from *byte */
static void test_get_buffer_slice_as_val_type_inference(void)
{
    printf("Testing 'get_buffer()[0..len] as val' type inference...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: native fn get_buffer(): *byte (forward declaration) */
    Token get_buffer_tok;
    setup_test_token(&get_buffer_tok, TOKEN_IDENTIFIER, "get_buffer", 1, "test.sn", &arena);
    Stmt *get_buffer_decl = ast_create_function_stmt(&arena, get_buffer_tok, NULL, 0, ptr_byte_type, NULL, 0, &get_buffer_tok);
    get_buffer_decl->as.function.is_native = true;

    /* Create call expression: get_buffer() */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_buffer", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    /* Create slice bounds: 0 and len (a variable) */
    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token len_tok;
    setup_test_token(&len_tok, TOKEN_IDENTIFIER, "len", 2, "test.sn", &arena);
    Expr *len_expr = ast_create_variable_expr(&arena, len_tok, &len_tok);

    /* Create slice expression: get_buffer()[0..len] */
    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, len_expr, NULL, &bracket_tok);

    /* Wrap slice in 'as val': get_buffer()[0..len] as val */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var len: int = 10 (needed for type checking len variable) */
    Token len_decl_tok;
    setup_test_token(&len_decl_tok, TOKEN_IDENTIFIER, "len", 1, "test.sn", &arena);
    Token ten_tok;
    setup_test_token(&ten_tok, TOKEN_INT_LITERAL, "10", 1, "test.sn", &arena);
    LiteralValue ten_val = {.int_value = 10};
    Expr *ten_expr = ast_create_literal_expr(&arena, ten_val, int_type, false, &ten_tok);
    Stmt *len_decl = ast_create_var_decl_stmt(&arena, len_decl_tok, int_type, ten_expr, NULL);

    /* Create: var data: byte[] = get_buffer()[0..len] as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[2] = {len_decl, data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 2, &func_name_tok);
    func_decl->as.function.is_native = false;  /* REGULAR function */

    ast_module_add_statement(&arena, &module, get_buffer_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED */

    /* Verify type inference:
     * - call_expr should be *byte
     * - slice_expr should be byte[] (slice extracts element type from pointer base)
     * - as_val_expr should be byte[] (as val on array is no-op)
     */
    assert(call_expr->expr_type != NULL);
    assert(call_expr->expr_type->kind == TYPE_POINTER);
    assert(call_expr->expr_type->as.pointer.base_type->kind == TYPE_BYTE);

    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_BYTE);

    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_ARRAY);
    assert(as_val_expr->expr_type->as.array.element_type->kind == TYPE_BYTE);

    printf("  'get_buffer()[0..len] as val' correctly infers byte[]\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that slicing a non-pointer/non-array type produces error */
static void test_slice_invalid_type_error(void)
{
    printf("Testing slice of invalid type produces error...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: native fn get_int(): int (forward declaration) */
    Token get_int_tok;
    setup_test_token(&get_int_tok, TOKEN_IDENTIFIER, "get_int", 1, "test.sn", &arena);
    Stmt *get_int_decl = ast_create_function_stmt(&arena, get_int_tok, NULL, 0, int_type, NULL, 0, &get_int_tok);
    get_int_decl->as.function.is_native = true;

    /* Create call expression: get_int() */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_int", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    /* Create slice bounds */
    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 10};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    /* Create slice expression: get_int()[0..10] -- INVALID: int is not sliceable! */
    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, NULL, &bracket_tok);

    /* Wrap slice in 'as val' */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var data: byte[] = get_int()[0..10] as val -- SHOULD FAIL */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_int_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: cannot slice an int */

    printf("  Slicing invalid type (int) correctly produces error\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that *int slice produces int[] */
static void test_int_pointer_slice_as_val_type_inference(void)
{
    printf("Testing '*int slice as val' produces int[]...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_int_type = ast_create_pointer_type(&arena, int_type);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: native fn get_ints(): *int (forward declaration) */
    Token get_ints_tok;
    setup_test_token(&get_ints_tok, TOKEN_IDENTIFIER, "get_ints", 1, "test.sn", &arena);
    Stmt *get_ints_decl = ast_create_function_stmt(&arena, get_ints_tok, NULL, 0, ptr_int_type, NULL, 0, &get_ints_tok);
    get_ints_decl->as.function.is_native = true;

    /* Create call expression: get_ints() */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_ints", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    /* Create slice bounds */
    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "5", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 5};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    /* Create slice expression: get_ints()[0..5] */
    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, NULL, &bracket_tok);

    /* Wrap slice in 'as val': get_ints()[0..5] as val */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var data: int[] = get_ints()[0..5] as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, int_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_ints_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED */

    /* Verify type inference */
    assert(call_expr->expr_type != NULL);
    assert(call_expr->expr_type->kind == TYPE_POINTER);
    assert(call_expr->expr_type->as.pointer.base_type->kind == TYPE_INT);

    assert(slice_expr->expr_type != NULL);
    assert(slice_expr->expr_type->kind == TYPE_ARRAY);
    assert(slice_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    assert(as_val_expr->expr_type != NULL);
    assert(as_val_expr->expr_type->kind == TYPE_ARRAY);
    assert(as_val_expr->expr_type->as.array.element_type->kind == TYPE_INT);

    printf("  '*int slice as val' correctly infers int[]\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that pointer slice with step parameter is rejected */
static void test_pointer_slice_with_step_fails(void)
{
    printf("Testing pointer slice with step parameter fails...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *byte_type = ast_create_primitive_type(&arena, TYPE_BYTE);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_byte_type = ast_create_pointer_type(&arena, byte_type);
    Type *byte_array_type = ast_create_array_type(&arena, byte_type);

    /* Create: native fn get_data(): *byte (forward declaration) */
    Token get_data_tok;
    setup_test_token(&get_data_tok, TOKEN_IDENTIFIER, "get_data", 1, "test.sn", &arena);
    Stmt *get_data_decl = ast_create_function_stmt(&arena, get_data_tok, NULL, 0, ptr_byte_type, NULL, 0, &get_data_tok);
    get_data_decl->as.function.is_native = true;

    /* Create slice expression: get_data()[0..10:2] -- with step parameter */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "get_data", 2, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);

    Token start_tok;
    setup_test_token(&start_tok, TOKEN_INT_LITERAL, "0", 2, "test.sn", &arena);
    LiteralValue start_val = {.int_value = 0};
    Expr *start_expr = ast_create_literal_expr(&arena, start_val, int_type, false, &start_tok);

    Token end_tok;
    setup_test_token(&end_tok, TOKEN_INT_LITERAL, "10", 2, "test.sn", &arena);
    LiteralValue end_val = {.int_value = 10};
    Expr *end_expr = ast_create_literal_expr(&arena, end_val, int_type, false, &end_tok);

    Token step_tok;
    setup_test_token(&step_tok, TOKEN_INT_LITERAL, "2", 2, "test.sn", &arena);
    LiteralValue step_val = {.int_value = 2};
    Expr *step_expr = ast_create_literal_expr(&arena, step_val, int_type, false, &step_tok);

    Token bracket_tok;
    setup_test_token(&bracket_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    /* Create slice with step: ptr[0..10:2] */
    Expr *slice_expr = ast_create_array_slice_expr(&arena, call_expr, start_expr, end_expr, step_expr, &bracket_tok);

    /* Wrap in as val */
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 2, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, slice_expr, &as_tok);

    /* Create: var data: byte[] = get_data()[0..10:2] as val */
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 2, "test.sn", &arena);
    Stmt *data_decl = ast_create_var_decl_stmt(&arena, data_tok, byte_array_type, as_val_expr, NULL);

    /* Wrap in a REGULAR function */
    Stmt *body[1] = {data_decl};
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "test_func", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, NULL, 0, void_type, body, 1, &func_name_tok);
    func_decl->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, get_data_decl);
    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: ptr[0..10:2] with step not allowed */

    printf("  Pointer slice with step parameter correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as ref' parameter on primitive types in native functions is valid */
static void test_as_ref_primitive_param_in_native_fn(void)
{
    printf("Testing 'as ref' primitive parameter in native function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_dimensions(width: int as ref, height: int as ref): void */
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "get_dimensions", 1, "test.sn", &arena);

    Token width_tok;
    setup_test_token(&width_tok, TOKEN_IDENTIFIER, "width", 1, "test.sn", &arena);

    Token height_tok;
    setup_test_token(&height_tok, TOKEN_IDENTIFIER, "height", 1, "test.sn", &arena);

    /* Create params array with 'as ref' qualifier */
    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 2);
    params[0].name = width_tok;
    params[0].type = int_type;
    params[0].mem_qualifier = MEM_AS_REF;
    params[1].name = height_tok;
    params[1].type = int_type;
    params[1].mem_qualifier = MEM_AS_REF;

    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 2, void_type, NULL, 0, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass: as ref on int is valid */

    printf("  'as ref' primitive parameters in native function correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that 'as ref' on array parameter (non-primitive) is rejected */
static void test_as_ref_array_param_rejected(void)
{
    printf("Testing 'as ref' on array parameter is rejected...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: native fn process(data: int[] as ref): void -- this should fail */
    Token func_name_tok;
    setup_test_token(&func_name_tok, TOKEN_IDENTIFIER, "process", 1, "test.sn", &arena);

    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 1, "test.sn", &arena);

    /* Create param with 'as ref' on array (invalid) */
    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 1);
    params[0].name = data_tok;
    params[0].type = int_array_type;
    params[0].mem_qualifier = MEM_AS_REF;

    Stmt *func_decl = ast_create_function_stmt(&arena, func_name_tok, params, 1, void_type, NULL, 0, &func_name_tok);
    func_decl->as.function.is_native = true;

    ast_module_add_statement(&arena, &module, func_decl);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL: as ref only applies to primitives */

    printf("  'as ref' on array parameter correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that calling a native function with 'as ref' params works with regular vars */
static void test_as_ref_param_call_with_vars(void)
{
    printf("Testing call to native function with 'as ref' params...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn set_value(out: int as ref): void */
    Token set_val_tok;
    setup_test_token(&set_val_tok, TOKEN_IDENTIFIER, "set_value", 1, "test.sn", &arena);

    Token out_tok;
    setup_test_token(&out_tok, TOKEN_IDENTIFIER, "out", 1, "test.sn", &arena);

    Parameter *native_params = arena_alloc(&arena, sizeof(Parameter) * 1);
    native_params[0].name = out_tok;
    native_params[0].type = int_type;
    native_params[0].mem_qualifier = MEM_AS_REF;

    /* Create the param_mem_quals array for the function type */
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 1);
    param_types[0] = int_type;
    Type *func_type = ast_create_function_type(&arena, void_type, param_types, 1);
    func_type->as.function.param_mem_quals = arena_alloc(&arena, sizeof(MemoryQualifier) * 1);
    func_type->as.function.param_mem_quals[0] = MEM_AS_REF;

    Stmt *native_decl = ast_create_function_stmt(&arena, set_val_tok, native_params, 1, void_type, NULL, 0, &set_val_tok);
    native_decl->as.function.is_native = true;

    /* Create a regular function that calls set_value(x) */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 2, "test.sn", &arena);

    /* var x: int = 0 */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 3, "test.sn", &arena);
    Token zero_tok;
    setup_test_token(&zero_tok, TOKEN_INT_LITERAL, "0", 3, "test.sn", &arena);
    LiteralValue zero_val = {.int_value = 0};
    Expr *zero_lit = ast_create_literal_expr(&arena, zero_val, int_type, false, &zero_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, int_type, zero_lit, NULL);

    /* set_value(x) */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "set_value", 4, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);

    Token x_arg_tok;
    setup_test_token(&x_arg_tok, TOKEN_IDENTIFIER, "x", 4, "test.sn", &arena);
    Expr *x_arg = ast_create_variable_expr(&arena, x_arg_tok, &x_arg_tok);

    Expr **args = arena_alloc(&arena, sizeof(Expr *) * 1);
    args[0] = x_arg;
    Expr *call = ast_create_call_expr(&arena, callee, args, 1, &call_tok);
    Stmt *call_stmt = ast_create_expr_stmt(&arena, call, &call_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    body[0] = x_decl;
    body[1] = call_stmt;
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, body, 2, &main_tok);
    main_fn->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, native_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    printf("  Calling native function with 'as ref' params correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ==========================================================================
 * Variadic Function Tests
 * ========================================================================== */

void test_variadic_function_accepts_extra_args(void)
{
    printf("Testing variadic function accepts extra arguments...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create module */
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn printf(format: str, ...): int */
    Token printf_tok;
    setup_test_token(&printf_tok, TOKEN_IDENTIFIER, "printf", 1, "test.sn", &arena);

    Token format_tok;
    setup_test_token(&format_tok, TOKEN_IDENTIFIER, "format", 1, "test.sn", &arena);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 1);
    params[0].name = format_tok;
    params[0].type = str_type;
    params[0].mem_qualifier = MEM_DEFAULT;

    Stmt *printf_decl = ast_create_function_stmt(&arena, printf_tok, params, 1, int_type, NULL, 0, &printf_tok);
    printf_decl->as.function.is_native = true;
    printf_decl->as.function.is_variadic = true;

    /* Create main function that calls printf with extra args */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 2, "test.sn", &arena);

    /* printf("Hello %d", 42) */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "printf", 3, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);

    Token str_tok;
    setup_test_token(&str_tok, TOKEN_STRING_LITERAL, "Hello %d", 3, "test.sn", &arena);
    LiteralValue str_val = {.string_value = "Hello %d"};
    Expr *format_lit = ast_create_literal_expr(&arena, str_val, str_type, false, &str_tok);

    Token int_tok;
    setup_test_token(&int_tok, TOKEN_INT_LITERAL, "42", 3, "test.sn", &arena);
    LiteralValue int_val = {.int_value = 42};
    Expr *int_lit = ast_create_literal_expr(&arena, int_val, int_type, false, &int_tok);

    Expr **args = arena_alloc(&arena, sizeof(Expr *) * 2);
    args[0] = format_lit;
    args[1] = int_lit;
    Expr *call = ast_create_call_expr(&arena, callee, args, 2, &call_tok);
    Stmt *call_stmt = ast_create_expr_stmt(&arena, call, &call_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 1);
    body[0] = call_stmt;
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, body, 1, &main_tok);
    main_fn->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, printf_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass - variadic accepts extra args */

    printf("  Variadic function correctly accepts extra arguments\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_variadic_function_rejects_too_few_args(void)
{
    printf("Testing variadic function rejects too few arguments...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create module */
    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *str_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn printf(format: str, ...): int */
    Token printf_tok;
    setup_test_token(&printf_tok, TOKEN_IDENTIFIER, "printf", 1, "test.sn", &arena);

    Token format_tok;
    setup_test_token(&format_tok, TOKEN_IDENTIFIER, "format", 1, "test.sn", &arena);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter) * 1);
    params[0].name = format_tok;
    params[0].type = str_type;
    params[0].mem_qualifier = MEM_DEFAULT;

    Stmt *printf_decl = ast_create_function_stmt(&arena, printf_tok, params, 1, int_type, NULL, 0, &printf_tok);
    printf_decl->as.function.is_native = true;
    printf_decl->as.function.is_variadic = true;

    /* Create main function that calls printf with NO args (missing format) */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 2, "test.sn", &arena);

    /* printf() - missing required format param */
    Token call_tok;
    setup_test_token(&call_tok, TOKEN_IDENTIFIER, "printf", 3, "test.sn", &arena);
    Expr *callee = ast_create_variable_expr(&arena, call_tok, &call_tok);

    Expr *call = ast_create_call_expr(&arena, callee, NULL, 0, &call_tok);
    Stmt *call_stmt = ast_create_expr_stmt(&arena, call, &call_tok);

    Stmt **body = arena_alloc(&arena, sizeof(Stmt *) * 1);
    body[0] = call_stmt;
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, body, 1, &main_tok);
    main_fn->as.function.is_native = false;

    ast_module_add_statement(&arena, &module, printf_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should fail - missing required param */

    printf("  Variadic function correctly rejects missing required params\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native callback type alias with C-compatible types succeeds */
void test_native_callback_type_alias_c_compatible(void)
{
    printf("Testing native callback type alias with C-compatible types...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_void_type = ast_create_pointer_type(&arena, void_type);

    /* Create: type Comparator = native fn(a: *void, b: *void): int */
    Token comparator_tok;
    setup_test_token(&comparator_tok, TOKEN_IDENTIFIER, "Comparator", 1, "test.sn", &arena);

    /* Build parameter types: *void, *void */
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = ptr_void_type;
    param_types[1] = ptr_void_type;

    Type *callback_type = ast_create_function_type(&arena, int_type, param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, comparator_tok, callback_type, &comparator_tok);

    /* Add a simple main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should succeed - all types are C-compatible */

    printf("  Native callback with C-compatible types correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native callback type alias with array parameter fails */
void test_native_callback_type_alias_array_param_fails(void)
{
    printf("Testing native callback type alias with array param fails...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: type BadCallback = native fn(arr: int[]): void */
    Token badcb_tok;
    setup_test_token(&badcb_tok, TOKEN_IDENTIFIER, "BadCallback", 1, "test.sn", &arena);

    /* Build parameter types: int[] (array - NOT C-compatible) */
    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 1);
    param_types[0] = int_array_type;

    Type *callback_type = ast_create_function_type(&arena, void_type, param_types, 1);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, badcb_tok, callback_type, &badcb_tok);

    /* Add a simple main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should fail - array type is not C-compatible */

    printf("  Native callback with array param correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native callback type alias with array return fails */
void test_native_callback_type_alias_array_return_fails(void)
{
    printf("Testing native callback type alias with array return fails...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *int_array_type = ast_create_array_type(&arena, int_type);

    /* Create: type BadCallback = native fn(): int[] */
    Token badcb_tok;
    setup_test_token(&badcb_tok, TOKEN_IDENTIFIER, "BadCallback", 1, "test.sn", &arena);

    /* No params, return type is int[] (array - NOT C-compatible) */
    Type *callback_type = ast_create_function_type(&arena, int_array_type, NULL, 0);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, badcb_tok, callback_type, &badcb_tok);

    /* Add a simple main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should fail - array return type is not C-compatible */

    printf("  Native callback with array return correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native callback type can be used as parameter in native function */
void test_native_callback_as_function_param(void)
{
    printf("Testing native callback type as function parameter...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_void_type = ast_create_pointer_type(&arena, void_type);

    /* Create: type Comparator = native fn(a: *void, b: *void): int */
    Token comparator_tok;
    setup_test_token(&comparator_tok, TOKEN_IDENTIFIER, "Comparator", 1, "test.sn", &arena);

    Type **cb_param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    cb_param_types[0] = ptr_void_type;
    cb_param_types[1] = ptr_void_type;

    Type *callback_type = ast_create_function_type(&arena, int_type, cb_param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, comparator_tok, callback_type, &comparator_tok);

    /* Register the type in symbol table (simulating what parser does) */
    symbol_table_add_type(&table, comparator_tok, callback_type);

    /* Create: native fn qsort(base: *void, count: int, size: int, cmp: Comparator): void */
    Token qsort_tok;
    setup_test_token(&qsort_tok, TOKEN_IDENTIFIER, "qsort", 3, "test.sn", &arena);

    Parameter *qsort_params = arena_alloc(&arena, sizeof(Parameter) * 4);
    Token base_tok; setup_test_token(&base_tok, TOKEN_IDENTIFIER, "base", 3, "test.sn", &arena);
    Token count_tok; setup_test_token(&count_tok, TOKEN_IDENTIFIER, "count", 3, "test.sn", &arena);
    Token size_tok; setup_test_token(&size_tok, TOKEN_IDENTIFIER, "size", 3, "test.sn", &arena);
    Token cmp_tok; setup_test_token(&cmp_tok, TOKEN_IDENTIFIER, "cmp", 3, "test.sn", &arena);

    qsort_params[0].name = base_tok;
    qsort_params[0].type = ptr_void_type;
    qsort_params[0].mem_qualifier = MEM_DEFAULT;
    qsort_params[1].name = count_tok;
    qsort_params[1].type = int_type;
    qsort_params[1].mem_qualifier = MEM_DEFAULT;
    qsort_params[2].name = size_tok;
    qsort_params[2].type = int_type;
    qsort_params[2].mem_qualifier = MEM_DEFAULT;
    qsort_params[3].name = cmp_tok;
    qsort_params[3].type = callback_type;  /* Callback type as parameter! */
    qsort_params[3].mem_qualifier = MEM_DEFAULT;

    Stmt *qsort_decl = ast_create_function_stmt(&arena, qsort_tok, qsort_params, 4, void_type, NULL, 0, &qsort_tok);
    qsort_decl->as.function.is_native = true;

    /* Add a simple main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 5, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, qsort_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should succeed - callback type can be used as param */

    printf("  Native callback type as function parameter correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native lambda capturing a variable from enclosing scope produces an error */
void test_native_lambda_capture_rejected(void)
{
    printf("Testing native lambda capturing variable is rejected...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_void_type = ast_create_pointer_type(&arena, void_type);

    /* Create: type Callback = native fn(data: *void): void */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "Callback", 1, "test.sn", &arena);

    Type **cb_param_types = arena_alloc(&arena, sizeof(Type *));
    cb_param_types[0] = ptr_void_type;

    Type *callback_type = ast_create_function_type(&arena, void_type, cb_param_types, 1);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);
    symbol_table_add_type(&table, callback_tok, callback_type);

    /* Create:
     * native fn setup(): void =>
     *     var counter: int = 0
     *     var handler: Callback = fn(data: *void): void =>
     *         counter = counter + 1  // ERROR: capture
     */
    Token setup_tok;
    setup_test_token(&setup_tok, TOKEN_IDENTIFIER, "setup", 2, "test.sn", &arena);

    /* Create: var counter: int = 0 */
    Token counter_tok;
    setup_test_token(&counter_tok, TOKEN_IDENTIFIER, "counter", 3, "test.sn", &arena);
    Token zero_tok;
    setup_test_token(&zero_tok, TOKEN_INT_LITERAL, "0", 3, "test.sn", &arena);
    LiteralValue zero_val = {.int_value = 0};
    Expr *zero_lit = ast_create_literal_expr(&arena, zero_val, int_type, false, &zero_tok);
    Stmt *counter_decl = ast_create_var_decl_stmt(&arena, counter_tok, int_type, zero_lit, NULL);

    /* Create the native lambda body: counter = counter + 1
     * This references 'counter' from enclosing scope = capture! */
    Token counter_ref_tok;
    setup_test_token(&counter_ref_tok, TOKEN_IDENTIFIER, "counter", 5, "test.sn", &arena);
    Expr *counter_ref = ast_create_variable_expr(&arena, counter_ref_tok, &counter_ref_tok);
    Token one_tok;
    setup_test_token(&one_tok, TOKEN_INT_LITERAL, "1", 5, "test.sn", &arena);
    LiteralValue one_val = {.int_value = 1};
    Expr *one_lit = ast_create_literal_expr(&arena, one_val, int_type, false, &one_tok);
    Expr *add_expr = ast_create_binary_expr(&arena, counter_ref, TOKEN_PLUS, one_lit, &counter_ref_tok);

    Token assign_tok;
    setup_test_token(&assign_tok, TOKEN_IDENTIFIER, "counter", 5, "test.sn", &arena);
    Expr *assign_expr = ast_create_assign_expr(&arena, assign_tok, add_expr, &assign_tok);
    Stmt *assign_stmt = ast_create_expr_stmt(&arena, assign_expr, &assign_tok);

    /* Create native lambda with statement body */
    Parameter *lambda_params = arena_alloc(&arena, sizeof(Parameter));
    Token data_tok;
    setup_test_token(&data_tok, TOKEN_IDENTIFIER, "data", 4, "test.sn", &arena);
    lambda_params[0].name = data_tok;
    lambda_params[0].type = ptr_void_type;
    lambda_params[0].mem_qualifier = MEM_DEFAULT;

    Token fn_tok;
    setup_test_token(&fn_tok, TOKEN_FN, "fn", 4, "test.sn", &arena);

    Stmt **lambda_body_stmts = arena_alloc(&arena, sizeof(Stmt *));
    lambda_body_stmts[0] = assign_stmt;

    Expr *native_lambda = ast_create_lambda_stmt_expr(&arena, lambda_params, 1,
                                                       void_type, lambda_body_stmts, 1,
                                                       FUNC_DEFAULT, true, &fn_tok);  /* is_native = true */

    /* Create: var handler: Callback = <lambda> */
    Token handler_tok;
    setup_test_token(&handler_tok, TOKEN_IDENTIFIER, "handler", 4, "test.sn", &arena);
    Stmt *handler_decl = ast_create_var_decl_stmt(&arena, handler_tok, callback_type, native_lambda, NULL);

    /* Create setup function body */
    Stmt **setup_body = arena_alloc(&arena, sizeof(Stmt *) * 2);
    setup_body[0] = counter_decl;
    setup_body[1] = handler_decl;

    Stmt *setup_fn = ast_create_function_stmt(&arena, setup_tok, NULL, 0, void_type, setup_body, 2, &setup_tok);
    setup_fn->as.function.is_native = true;

    /* Add a main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, setup_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - native lambda captures 'counter' */

    printf("  Native lambda capturing variable correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native lambda using only its own parameters succeeds */
void test_native_lambda_params_only_succeeds(void)
{
    printf("Testing native lambda using only parameters succeeds...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *ptr_void_type = ast_create_pointer_type(&arena, void_type);

    /* Create: type Comparator = native fn(a: *void, b: *void): int */
    Token comparator_tok;
    setup_test_token(&comparator_tok, TOKEN_IDENTIFIER, "Comparator", 1, "test.sn", &arena);

    Type **cmp_param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    cmp_param_types[0] = ptr_void_type;
    cmp_param_types[1] = ptr_void_type;

    Type *comparator_type = ast_create_function_type(&arena, int_type, cmp_param_types, 2);
    comparator_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, comparator_tok, comparator_type, &comparator_tok);
    symbol_table_add_type(&table, comparator_tok, comparator_type);

    /* Create:
     * native fn setup(): void =>
     *     var cmp: Comparator = fn(a: *void, b: *void): int =>
     *         return 0  // Only uses parameters and literals - OK!
     */
    Token setup_tok;
    setup_test_token(&setup_tok, TOKEN_IDENTIFIER, "setup", 2, "test.sn", &arena);

    /* Create the native lambda body: return 0 */
    Token return_tok;
    setup_test_token(&return_tok, TOKEN_RETURN, "return", 4, "test.sn", &arena);
    Token zero_tok;
    setup_test_token(&zero_tok, TOKEN_INT_LITERAL, "0", 4, "test.sn", &arena);
    LiteralValue zero_val = {.int_value = 0};
    Expr *zero_lit = ast_create_literal_expr(&arena, zero_val, int_type, false, &zero_tok);
    Stmt *return_stmt = ast_create_return_stmt(&arena, return_tok, zero_lit, &return_tok);

    /* Create native lambda with parameters a and b */
    Parameter *lambda_params = arena_alloc(&arena, sizeof(Parameter) * 2);
    Token a_tok;
    setup_test_token(&a_tok, TOKEN_IDENTIFIER, "a", 3, "test.sn", &arena);
    lambda_params[0].name = a_tok;
    lambda_params[0].type = ptr_void_type;
    lambda_params[0].mem_qualifier = MEM_DEFAULT;
    Token b_tok;
    setup_test_token(&b_tok, TOKEN_IDENTIFIER, "b", 3, "test.sn", &arena);
    lambda_params[1].name = b_tok;
    lambda_params[1].type = ptr_void_type;
    lambda_params[1].mem_qualifier = MEM_DEFAULT;

    Token fn_tok;
    setup_test_token(&fn_tok, TOKEN_FN, "fn", 3, "test.sn", &arena);

    Stmt **lambda_body_stmts = arena_alloc(&arena, sizeof(Stmt *));
    lambda_body_stmts[0] = return_stmt;

    Expr *native_lambda = ast_create_lambda_stmt_expr(&arena, lambda_params, 2,
                                                       int_type, lambda_body_stmts, 1,
                                                       FUNC_DEFAULT, true, &fn_tok);  /* is_native = true */

    /* Create: var cmp: Comparator = <lambda> */
    Token cmp_tok;
    setup_test_token(&cmp_tok, TOKEN_IDENTIFIER, "cmp", 3, "test.sn", &arena);
    Stmt *cmp_decl = ast_create_var_decl_stmt(&arena, cmp_tok, comparator_type, native_lambda, NULL);

    /* Create setup function body */
    Stmt **setup_body = arena_alloc(&arena, sizeof(Stmt *));
    setup_body[0] = cmp_decl;

    Stmt *setup_fn = ast_create_function_stmt(&arena, setup_tok, NULL, 0, void_type, setup_body, 1, &setup_tok);
    setup_fn->as.function.is_native = true;

    /* Add a main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, setup_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED - lambda only uses its own parameters */

    printf("  Native lambda using only parameters correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native lambda with mismatched parameter count produces error */
void test_native_lambda_param_count_mismatch(void)
{
    printf("Testing native lambda with mismatched parameter count...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: type Callback = native fn(a: int, b: int): int */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "Callback", 1, "test.sn", &arena);

    Type **cb_param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    cb_param_types[0] = int_type;
    cb_param_types[1] = int_type;

    Type *callback_type = ast_create_function_type(&arena, int_type, cb_param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);
    symbol_table_add_type(&table, callback_tok, callback_type);

    /* Create a native lambda with wrong param count: fn(a: int): int => a
     * This has 1 param but callback expects 2 */
    Parameter *lambda_params = arena_alloc(&arena, sizeof(Parameter));
    Token a_tok;
    setup_test_token(&a_tok, TOKEN_IDENTIFIER, "a", 3, "test.sn", &arena);
    lambda_params[0].name = a_tok;
    lambda_params[0].type = int_type;
    lambda_params[0].mem_qualifier = MEM_DEFAULT;

    Token fn_tok;
    setup_test_token(&fn_tok, TOKEN_FN, "fn", 3, "test.sn", &arena);

    /* Body: just return a */
    Expr *a_ref = ast_create_variable_expr(&arena, a_tok, &a_tok);

    Expr *native_lambda = ast_create_lambda_expr(&arena, lambda_params, 1,
                                                  int_type, a_ref, FUNC_DEFAULT,
                                                  false, &fn_tok);  /* is_native will be inferred */

    /* Create: var cmp: Callback = <lambda> */
    Token cmp_tok;
    setup_test_token(&cmp_tok, TOKEN_IDENTIFIER, "cmp", 3, "test.sn", &arena);
    Stmt *cmp_decl = ast_create_var_decl_stmt(&arena, cmp_tok, callback_type, native_lambda, NULL);

    /* Create setup function body */
    Token setup_tok;
    setup_test_token(&setup_tok, TOKEN_IDENTIFIER, "setup", 2, "test.sn", &arena);
    Stmt **setup_body = arena_alloc(&arena, sizeof(Stmt *));
    setup_body[0] = cmp_decl;

    Stmt *setup_fn = ast_create_function_stmt(&arena, setup_tok, NULL, 0, void_type, setup_body, 1, &setup_tok);
    setup_fn->as.function.is_native = true;

    /* Add a main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, setup_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - parameter count mismatch */

    printf("  Native lambda with mismatched parameter count correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ==========================================================================
 * Opaque Type Tests
 * ========================================================================== */

/* Test that opaque type declaration is valid */
void test_opaque_type_declaration(void)
{
    printf("Testing opaque type declaration...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: type FILE = opaque */
    Token file_tok;
    setup_test_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);

    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    Stmt *type_decl = ast_create_type_decl_stmt(&arena, file_tok, opaque_type, &file_tok);

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    printf("  Opaque type declaration correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that opaque pointer type is valid in native function */
void test_opaque_pointer_in_native_function(void)
{
    printf("Testing opaque pointer type in native function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create opaque type: type FILE = opaque */
    Token file_tok;
    setup_test_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);
    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    Stmt *type_decl = ast_create_type_decl_stmt(&arena, file_tok, opaque_type, &file_tok);

    /* Register the type in the symbol table */
    symbol_table_add_type(&table, file_tok, opaque_type);

    /* Create pointer to opaque type: *FILE */
    Type *ptr_file_type = ast_create_pointer_type(&arena, opaque_type);

    /* Create: native fn fclose(f: *FILE): int */
    Token fclose_tok;
    setup_test_token(&fclose_tok, TOKEN_IDENTIFIER, "fclose", 2, "test.sn", &arena);
    Token f_param_tok;
    setup_test_token(&f_param_tok, TOKEN_IDENTIFIER, "f", 2, "test.sn", &arena);

    Parameter *params = arena_alloc(&arena, sizeof(Parameter));
    params[0].name = f_param_tok;
    params[0].type = ptr_file_type;
    params[0].mem_qualifier = MEM_DEFAULT;

    Stmt *fclose_decl = ast_create_function_stmt(&arena, fclose_tok, params, 1, int_type, NULL, 0, &fclose_tok);
    fclose_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, fclose_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    printf("  Opaque pointer type in native function correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that dereferencing opaque pointer is rejected */
void test_opaque_dereference_rejected(void)
{
    printf("Testing opaque pointer dereference is rejected...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *nil_type = ast_create_primitive_type(&arena, TYPE_NIL);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create opaque type: type FILE = opaque */
    Token file_tok;
    setup_test_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);
    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    Stmt *type_decl = ast_create_type_decl_stmt(&arena, file_tok, opaque_type, &file_tok);
    symbol_table_add_type(&table, file_tok, opaque_type);

    /* Create pointer to opaque type: *FILE */
    Type *ptr_file_type = ast_create_pointer_type(&arena, opaque_type);

    /* In a native function, try to dereference the pointer: var p: *FILE = nil; var x = p as val */
    Token p_tok;
    setup_test_token(&p_tok, TOKEN_IDENTIFIER, "p", 3, "test.sn", &arena);
    Token nil_tok;
    setup_test_token(&nil_tok, TOKEN_NIL, "nil", 3, "test.sn", &arena);
    LiteralValue nil_val = {.int_value = 0};
    Expr *nil_lit = ast_create_literal_expr(&arena, nil_val, nil_type, false, &nil_tok);
    Stmt *p_decl = ast_create_var_decl_stmt(&arena, p_tok, ptr_file_type, nil_lit, NULL);

    /* Create: var x = p as val -- THIS SHOULD FAIL for opaque types */
    Token x_tok;
    setup_test_token(&x_tok, TOKEN_IDENTIFIER, "x", 4, "test.sn", &arena);
    Token p_ref_tok;
    setup_test_token(&p_ref_tok, TOKEN_IDENTIFIER, "p", 4, "test.sn", &arena);
    Expr *p_ref = ast_create_variable_expr(&arena, p_ref_tok, &p_ref_tok);
    Token as_tok;
    setup_test_token(&as_tok, TOKEN_AS, "as", 4, "test.sn", &arena);
    Expr *as_val_expr = ast_create_as_val_expr(&arena, p_ref, &as_tok);
    Stmt *x_decl = ast_create_var_decl_stmt(&arena, x_tok, opaque_type, as_val_expr, NULL);

    /* Create native function body */
    Stmt *body[2] = {p_decl, x_decl};
    Token native_tok;
    setup_test_token(&native_tok, TOKEN_IDENTIFIER, "test_fn", 2, "test.sn", &arena);
    Stmt *native_fn = ast_create_function_stmt(&arena, native_tok, NULL, 0, void_type, body, 2, &native_tok);
    native_fn->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, native_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 0);  /* Should FAIL - cannot dereference opaque pointer */

    printf("  Opaque pointer dereference correctly rejected\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that opaque type is C-compatible (can be used in native callback) */
void test_opaque_type_c_compatible(void)
{
    printf("Testing opaque type is C-compatible in native callback...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create opaque type: type FILE = opaque */
    Token file_tok;
    setup_test_token(&file_tok, TOKEN_IDENTIFIER, "FILE", 1, "test.sn", &arena);
    Type *opaque_type = ast_create_opaque_type(&arena, "FILE");
    Stmt *type_decl = ast_create_type_decl_stmt(&arena, file_tok, opaque_type, &file_tok);
    symbol_table_add_type(&table, file_tok, opaque_type);

    /* Create pointer to opaque type: *FILE */
    Type *ptr_file_type = ast_create_pointer_type(&arena, opaque_type);

    /* Create: type FileCallback = native fn(f: *FILE): void */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "FileCallback", 2, "test.sn", &arena);

    Type **param_types = arena_alloc(&arena, sizeof(Type *));
    param_types[0] = ptr_file_type;

    Type *callback_type = ast_create_function_type(&arena, void_type, param_types, 1);
    callback_type->as.function.is_native = true;

    Stmt *callback_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, callback_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass - *FILE is C-compatible */

    printf("  Opaque type in native callback correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* ==========================================================================
 * Interop Primitive Type Tests
 * ========================================================================== */

/* Test int32 type in native function */
void test_int32_type_in_native_function(void)
{
    printf("Testing int32 type in native function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_int32(): int32 */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_int32", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, int32_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    printf("  int32 type in native function correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test uint type in native function */
void test_uint_type_in_native_function(void)
{
    printf("Testing uint type in native function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_uint(): uint */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_uint", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, uint_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    printf("  uint type in native function correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test uint32 type in native function */
void test_uint32_type_in_native_function(void)
{
    printf("Testing uint32 type in native function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *uint32_type = ast_create_primitive_type(&arena, TYPE_UINT32);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_uint32(): uint32 */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_uint32", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, uint32_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    printf("  uint32 type in native function correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test float type in native function */
void test_float_type_in_native_function(void)
{
    printf("Testing float type in native function...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_float(): float */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_float", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, float_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    printf("  float type in native function correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test interop types are C-compatible in native callback */
void test_interop_types_c_compatible(void)
{
    printf("Testing interop types are C-compatible in native callback...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *uint_type = ast_create_primitive_type(&arena, TYPE_UINT);
    Type *float_type = ast_create_primitive_type(&arena, TYPE_FLOAT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: type Callback = native fn(a: int32, b: uint): float */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "Callback", 1, "test.sn", &arena);

    Type **param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = int32_type;
    param_types[1] = uint_type;

    Type *callback_type = ast_create_function_type(&arena, float_type, param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *callback_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, callback_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass - int32, uint, float are C-compatible */

    printf("  Interop types in native callback correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test pointer to interop type */
void test_pointer_to_interop_type(void)
{
    printf("Testing pointer to interop type (*int32)...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int32_type = ast_create_primitive_type(&arena, TYPE_INT32);
    Type *ptr_int32_type = ast_create_pointer_type(&arena, int32_type);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: native fn get_int32_ptr(): *int32 */
    Token func_tok;
    setup_test_token(&func_tok, TOKEN_IDENTIFIER, "get_int32_ptr", 1, "test.sn", &arena);
    Stmt *func_decl = ast_create_function_stmt(&arena, func_tok, NULL, 0, ptr_int32_type, NULL, 0, &func_tok);
    func_decl->as.function.is_native = true;

    /* Add main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 3, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, func_decl);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should pass */

    printf("  Pointer to interop type correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that native lambda with matching signature succeeds */
void test_native_lambda_matching_signature(void)
{
    printf("Testing native lambda with matching signature...\n");

    Arena arena;
    arena_init(&arena, 8192);

    SymbolTable table;
    symbol_table_init(&arena, &table);

    Module module;
    ast_init_module(&arena, &module, "test.sn");

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);

    /* Create: type Callback = native fn(a: int, b: int): int */
    Token callback_tok;
    setup_test_token(&callback_tok, TOKEN_IDENTIFIER, "Callback", 1, "test.sn", &arena);

    Type **cb_param_types = arena_alloc(&arena, sizeof(Type *) * 2);
    cb_param_types[0] = int_type;
    cb_param_types[1] = int_type;

    Type *callback_type = ast_create_function_type(&arena, int_type, cb_param_types, 2);
    callback_type->as.function.is_native = true;

    Stmt *type_decl = ast_create_type_decl_stmt(&arena, callback_tok, callback_type, &callback_tok);
    symbol_table_add_type(&table, callback_tok, callback_type);

    /* Create a native lambda with matching signature: fn(a: int, b: int): int => a + b */
    Parameter *lambda_params = arena_alloc(&arena, sizeof(Parameter) * 2);
    Token a_tok;
    setup_test_token(&a_tok, TOKEN_IDENTIFIER, "a", 3, "test.sn", &arena);
    lambda_params[0].name = a_tok;
    lambda_params[0].type = int_type;
    lambda_params[0].mem_qualifier = MEM_DEFAULT;
    Token b_tok;
    setup_test_token(&b_tok, TOKEN_IDENTIFIER, "b", 3, "test.sn", &arena);
    lambda_params[1].name = b_tok;
    lambda_params[1].type = int_type;
    lambda_params[1].mem_qualifier = MEM_DEFAULT;

    Token fn_tok;
    setup_test_token(&fn_tok, TOKEN_FN, "fn", 3, "test.sn", &arena);

    /* Body: a + b */
    Expr *a_ref = ast_create_variable_expr(&arena, a_tok, &a_tok);
    Expr *b_ref = ast_create_variable_expr(&arena, b_tok, &b_tok);
    Expr *add_expr = ast_create_binary_expr(&arena, a_ref, TOKEN_PLUS, b_ref, &a_tok);

    Expr *native_lambda = ast_create_lambda_expr(&arena, lambda_params, 2,
                                                  int_type, add_expr, FUNC_DEFAULT,
                                                  false, &fn_tok);  /* is_native will be inferred */

    /* Create: var cmp: Callback = <lambda> */
    Token cmp_tok;
    setup_test_token(&cmp_tok, TOKEN_IDENTIFIER, "cmp", 3, "test.sn", &arena);
    Stmt *cmp_decl = ast_create_var_decl_stmt(&arena, cmp_tok, callback_type, native_lambda, NULL);

    /* Create setup function body */
    Token setup_tok;
    setup_test_token(&setup_tok, TOKEN_IDENTIFIER, "setup", 2, "test.sn", &arena);
    Stmt **setup_body = arena_alloc(&arena, sizeof(Stmt *));
    setup_body[0] = cmp_decl;

    Stmt *setup_fn = ast_create_function_stmt(&arena, setup_tok, NULL, 0, void_type, setup_body, 1, &setup_tok);
    setup_fn->as.function.is_native = true;

    /* Add a main function */
    Token main_tok;
    setup_test_token(&main_tok, TOKEN_IDENTIFIER, "main", 10, "test.sn", &arena);
    Stmt *main_fn = ast_create_function_stmt(&arena, main_tok, NULL, 0, void_type, NULL, 0, &main_tok);

    ast_module_add_statement(&arena, &module, type_decl);
    ast_module_add_statement(&arena, &module, setup_fn);
    ast_module_add_statement(&arena, &module, main_fn);

    int no_error = type_check_module(&module, &table);
    assert(no_error == 1);  /* Should SUCCEED - matching signature */

    printf("  Native lambda with matching signature correctly accepted\n");

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_native_main(void)
{
    test_native_context_default_inactive();
    test_native_context_enter();
    test_native_context_exit();
    test_native_context_nesting();
    test_native_context_excessive_exit();
    test_native_context_multiple_cycles();
    test_pointer_var_rejected_in_regular_function();
    test_pointer_var_accepted_in_native_function();
    test_pointer_arithmetic_rejected();
    test_pointer_nil_comparison_allowed();
    test_pointer_pointer_comparison_allowed();
    test_inline_pointer_passing_allowed();
    test_inline_nil_passing_allowed();
    test_as_val_unwraps_pointer_int();
    test_as_val_unwraps_pointer_double();
    test_as_val_unwraps_pointer_float();
    test_as_val_rejects_non_pointer();
    test_as_val_char_pointer_to_str();
    test_as_val_int_pointer_no_cstr_flag();
    test_pointer_return_without_as_val_fails_in_regular_fn();
    test_pointer_return_with_as_val_succeeds_in_regular_fn();
    test_native_fn_can_store_pointer_return();
    // Pointer slice tests
    test_pointer_slice_byte_to_byte_array();
    test_pointer_slice_int_to_int_array();
    test_slice_non_array_non_pointer_fails();
    test_array_slice_still_works();
    // Pointer slice with 'as val' tests
    test_as_val_context_tracking();
    test_pointer_slice_with_as_val_in_regular_fn();
    test_pointer_slice_without_as_val_in_regular_fn_fails();
    test_as_val_on_array_type_is_noop();
    // Type inference tests for pointer slice with 'as val'
    test_get_buffer_slice_as_val_type_inference();
    test_slice_invalid_type_error();
    test_int_pointer_slice_as_val_type_inference();
    // Edge case tests for pointer slicing
    test_pointer_slice_with_step_fails();
    // Native function 'as ref' out-parameter tests
    test_as_ref_primitive_param_in_native_fn();
    test_as_ref_array_param_rejected();
    test_as_ref_param_call_with_vars();
    // Variadic function tests
    test_variadic_function_accepts_extra_args();
    test_variadic_function_rejects_too_few_args();
    // Native callback type alias tests
    test_native_callback_type_alias_c_compatible();
    test_native_callback_type_alias_array_param_fails();
    test_native_callback_type_alias_array_return_fails();
    test_native_callback_as_function_param();
    // Native lambda capture tests
    test_native_lambda_capture_rejected();
    test_native_lambda_params_only_succeeds();
    // Native lambda signature matching tests
    test_native_lambda_param_count_mismatch();
    test_native_lambda_matching_signature();
    // Opaque type tests
    test_opaque_type_declaration();
    test_opaque_pointer_in_native_function();
    test_opaque_dereference_rejected();
    test_opaque_type_c_compatible();
    // Interop primitive type tests
    test_int32_type_in_native_function();
    test_uint_type_in_native_function();
    test_uint32_type_in_native_function();
    test_float_type_in_native_function();
    test_interop_types_c_compatible();
    test_pointer_to_interop_type();
}
