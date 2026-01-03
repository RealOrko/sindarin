// tests/type_checker_tests_thread.c
// Tests for thread spawn and sync type checking

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../type_checker/type_checker_expr.h"
#include "../type_checker/type_checker_stmt.h"

/* Test spawn with non-call expression reports error */
static void test_thread_spawn_non_call_error(void)
{
    printf("Testing thread spawn with non-call expression reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a thread spawn expression with a literal instead of a call */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);

    /* Create a literal expression (not a call) */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *literal_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &spawn_tok);

    /* Create thread spawn with literal (invalid) */
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, literal_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test spawn with non-function callee reports error */
static void test_thread_spawn_non_function_error(void)
{
    printf("Testing thread spawn with non-function callee reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a non-function variable to symbol table */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "x", 1, "test.sn", &arena);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Create a call expression to the non-function variable */
    Expr *callee = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &var_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return NULL */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test void spawn assignment reports error */
static void test_void_spawn_assignment_error(void)
{
    printf("Testing void spawn assignment reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a void function type */
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *func_type = ast_create_function_type(&arena, void_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "doWork", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create a call expression to the void function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* The spawn expression itself should type-check to void */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);

    /* Now create a var declaration trying to assign the void spawn */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "result", 2, "test.sn", &arena);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement - should report error */
    type_checker_reset_error();
    type_check_stmt(var_decl, &table, void_type);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test valid non-void spawn returns correct type */
static void test_valid_spawn_returns_correct_type(void)
{
    printf("Testing valid non-void spawn returns correct type...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a function returning int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func_type = ast_create_function_type(&arena, int_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return int */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test pending state is marked on result variable */
static void test_pending_state_marked_on_spawn_assignment(void)
{
    printf("Testing pending state is marked on result variable...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a function returning int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func_type = ast_create_function_type(&arena, int_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration: var r: int = &compute() */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(!type_checker_had_error());

    /* Look up the result variable and verify it's pending */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_name_tok);
    assert(sym != NULL);
    assert(symbol_table_is_pending(sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array argument is frozen after spawn */
static void test_array_arg_frozen_after_spawn(void)
{
    printf("Testing array argument is frozen after spawn...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create types */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Create an array variable that will be passed to the function */
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "myData", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_tok);
    assert(!symbol_table_is_frozen(arr_sym));

    /* Create a function that takes an array parameter and returns int */
    Type **param_types = (Type **)arena_alloc(&arena, sizeof(Type *));
    param_types[0] = array_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 1);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "processData", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create call expression with the array as argument */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr **args = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    args[0] = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, args, 1, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration: var r: int = &processData(myData) */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(!type_checker_had_error());

    /* The array argument should now be frozen */
    assert(symbol_table_is_frozen(arr_sym));
    assert(symbol_table_get_freeze_count(arr_sym) == 1);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test frozen args stored in pending variable symbol after spawn */
static void test_frozen_args_stored_in_pending_symbol(void)
{
    printf("Testing frozen args stored in pending variable symbol...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create types */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Create two array variables that will be passed to the function */
    Token arr1_tok, arr2_tok;
    setup_token(&arr1_tok, TOKEN_IDENTIFIER, "data1", 1, "test.sn", &arena);
    setup_token(&arr2_tok, TOKEN_IDENTIFIER, "data2", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr1_tok, array_type);
    symbol_table_add_symbol(&table, arr2_tok, array_type);

    Symbol *arr1_sym = symbol_table_lookup_symbol(&table, arr1_tok);
    Symbol *arr2_sym = symbol_table_lookup_symbol(&table, arr2_tok);

    /* Create a function that takes two array parameters and returns int */
    Type **param_types = (Type **)arena_alloc(&arena, sizeof(Type *) * 2);
    param_types[0] = array_type;
    param_types[1] = array_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 2);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "combine", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create call expression with both arrays as arguments */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr **args = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 2);
    args[0] = ast_create_variable_expr(&arena, arr1_tok, &arr1_tok);
    args[1] = ast_create_variable_expr(&arena, arr2_tok, &arr2_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, args, 2, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration: var r: int = &combine(data1, data2) */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(!type_checker_had_error());

    /* Look up the result variable */
    Symbol *result_sym = symbol_table_lookup_symbol(&table, var_name_tok);
    assert(result_sym != NULL);
    assert(symbol_table_is_pending(result_sym));

    /* Verify frozen_args are stored in the pending symbol */
    assert(result_sym->frozen_args != NULL);
    assert(result_sym->frozen_args_count == 2);

    /* Verify both arrays are in the frozen_args */
    bool found_arr1 = false, found_arr2 = false;
    for (int i = 0; i < result_sym->frozen_args_count; i++)
    {
        if (result_sym->frozen_args[i] == arr1_sym) found_arr1 = true;
        if (result_sym->frozen_args[i] == arr2_sym) found_arr2 = true;
    }
    assert(found_arr1);
    assert(found_arr2);

    /* Both arrays should be frozen */
    assert(symbol_table_is_frozen(arr1_sym));
    assert(symbol_table_is_frozen(arr2_sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test 'as ref' primitive is frozen after spawn */
static void test_as_ref_primitive_frozen_after_spawn(void)
{
    printf("Testing 'as ref' primitive is frozen after spawn...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create types */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create an int variable that will be passed as 'as ref' */
    Token counter_tok;
    setup_token(&counter_tok, TOKEN_IDENTIFIER, "counter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, counter_tok, int_type);

    Symbol *counter_sym = symbol_table_lookup_symbol(&table, counter_tok);
    assert(!symbol_table_is_frozen(counter_sym));

    /* Create a function that takes an int 'as ref' parameter and returns int */
    Type **param_types = (Type **)arena_alloc(&arena, sizeof(Type *));
    param_types[0] = int_type;
    Type *func_type = ast_create_function_type(&arena, int_type, param_types, 1);

    /* Set param_mem_quals to indicate 'as ref' for the first parameter */
    MemoryQualifier *quals = (MemoryQualifier *)arena_alloc(&arena, sizeof(MemoryQualifier));
    quals[0] = MEM_AS_REF;
    func_type->as.function.param_mem_quals = quals;

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "incrementCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create call expression with the counter as argument */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr **args = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    args[0] = ast_create_variable_expr(&arena, counter_tok, &counter_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, args, 1, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration: var r: int = &incrementCounter(counter) */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);
    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check the statement */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(!type_checker_had_error());

    /* The 'as ref' primitive argument should now be frozen */
    assert(symbol_table_is_frozen(counter_sym));
    assert(symbol_table_get_freeze_count(counter_sym) == 1);

    /* Look up the result variable and verify frozen_args contains the primitive */
    Symbol *result_sym = symbol_table_lookup_symbol(&table, var_name_tok);
    assert(result_sym != NULL);
    assert(result_sym->frozen_args != NULL);
    assert(result_sym->frozen_args_count == 1);
    assert(result_sym->frozen_args[0] == counter_sym);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test spawn with wrong return type for variable declaration */
static void test_spawn_type_mismatch_error(void)
{
    printf("Testing spawn return type mismatch with variable reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a function returning string */
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *func_type = ast_create_function_type(&arena, string_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getString", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Create var declaration with wrong type: var r: int = &getString() */
    Token var_name_tok;
    setup_token(&var_name_tok, TOKEN_IDENTIFIER, "r", 2, "test.sn", &arena);
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    Stmt *var_decl = ast_create_var_decl_stmt(&arena, var_name_tok, int_type, spawn_expr, &var_name_tok);

    /* Type check should report error */
    type_checker_reset_error();
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    type_check_stmt(var_decl, &table, void_type);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test sync on non-variable expression reports error */
static void test_sync_non_variable_error(void)
{
    printf("Testing sync on non-variable expression reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a literal expression (not a variable) */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *literal_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);

    /* Create thread sync with literal (invalid - not a variable or spawn) */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, literal_expr, false, &sync_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test sync on unknown variable reports error */
static void test_sync_unknown_variable_error(void)
{
    printf("Testing sync on unknown variable reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create variable expression for unknown variable */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "unknownVar", 1, "test.sn", &arena);
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create thread sync with unknown variable */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, false, &sync_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test sync on non-pending variable reports error */
static void test_sync_non_pending_variable_error(void)
{
    printf("Testing sync on non-pending variable reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal (non-pending) variable */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalVar", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Verify variable is NOT pending */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(!symbol_table_is_pending(sym));

    /* Create variable expression */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create thread sync on non-pending variable */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, false, &sync_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test valid sync on pending variable returns correct type */
static void test_valid_sync_returns_correct_type(void)
{
    printf("Testing valid sync on pending variable returns correct type...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a function returning int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func_type = ast_create_function_type(&arena, int_type, NULL, 0);

    /* Add the function to symbol table */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "compute", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);

    /* Add a pending variable (simulating result of spawn assignment) */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "result", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Mark the variable as pending */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);

    /* Create variable expression */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create thread sync */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, false, &sync_tok);

    /* Type check should return int type */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that sync transitions symbol from PENDING to SYNCHRONIZED state */
static void test_sync_state_transition(void)
{
    printf("Testing sync transitions from PENDING to SYNCHRONIZED state...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable with int type */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "threadResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Mark the variable as pending (simulating spawn assignment) */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    assert(sym->thread_state == THREAD_STATE_PENDING);

    /* Create variable expression for sync */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create thread sync expression */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, false, &sync_tok);

    /* Type check the sync - should transition state */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    /* Verify state transitioned to SYNCHRONIZED */
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Verify subsequent access to the variable is allowed */
    type_checker_reset_error();
    Type *access_result = type_check_expr(var_expr, &table);
    assert(access_result != NULL);
    assert(access_result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that sync unfreezes captured arguments */
static void test_sync_unfreezes_arguments(void)
{
    printf("Testing sync unfreezes captured arguments...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create an array type (arrays are frozen when passed to threads) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Add an array variable */
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "myArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    /* Freeze the array (simulating spawn capturing it) */
    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_tok);
    symbol_table_freeze_symbol(arr_sym);
    assert(symbol_table_is_frozen(arr_sym));

    /* Create a pending thread handle with frozen_args tracking */
    Token handle_tok;
    setup_token(&handle_tok, TOKEN_IDENTIFIER, "threadHandle", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle_tok, int_type);

    Symbol *handle_sym = symbol_table_lookup_symbol(&table, handle_tok);
    symbol_table_mark_pending(handle_sym);

    /* Set frozen args on the pending symbol */
    Symbol **frozen_args = (Symbol **)arena_alloc(&arena, sizeof(Symbol *) * 1);
    frozen_args[0] = arr_sym;
    symbol_table_set_frozen_args(handle_sym, frozen_args, 1);

    /* Create sync expression for the handle */
    Expr *handle_expr = ast_create_variable_expr(&arena, handle_tok, &handle_tok);
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, handle_expr, false, &sync_tok);

    /* Type check the sync - should unfreeze the array */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(!type_checker_had_error());

    /* Verify the array is now unfrozen */
    assert(!symbol_table_is_frozen(arr_sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that frozen argument becomes writable after sync */
static void test_frozen_arg_writable_after_sync(void)
{
    printf("Testing frozen arg becomes writable after sync...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create an array type (arrays are frozen when passed to threads) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Add an array variable */
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "myArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    /* Freeze the array (simulating spawn capturing it) */
    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_tok);
    symbol_table_freeze_symbol(arr_sym);
    assert(symbol_table_is_frozen(arr_sym));

    /* Verify array cannot be modified while frozen - create push member access */
    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token push_tok;
    setup_token(&push_tok, TOKEN_IDENTIFIER, "push", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);

    /* Type check push on frozen array should fail */
    type_checker_reset_error();
    Type *frozen_result = type_check_expr(member_expr, &table);
    assert(frozen_result == NULL);
    assert(type_checker_had_error());

    /* Now create a pending thread handle with frozen_args tracking */
    Token handle_tok;
    setup_token(&handle_tok, TOKEN_IDENTIFIER, "threadHandle", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle_tok, int_type);

    Symbol *handle_sym = symbol_table_lookup_symbol(&table, handle_tok);
    symbol_table_mark_pending(handle_sym);

    /* Set frozen args on the pending symbol */
    Symbol **frozen_args = (Symbol **)arena_alloc(&arena, sizeof(Symbol *) * 1);
    frozen_args[0] = arr_sym;
    symbol_table_set_frozen_args(handle_sym, frozen_args, 1);

    /* Create and type check sync expression */
    Expr *handle_expr = ast_create_variable_expr(&arena, handle_tok, &handle_tok);
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, handle_expr, false, &sync_tok);

    type_checker_reset_error();
    Type *sync_result = type_check_expr(sync_expr, &table);
    assert(sync_result != NULL);
    assert(!type_checker_had_error());

    /* Verify array is now unfrozen */
    assert(!symbol_table_is_frozen(arr_sym));

    /* Now verify we can access push on the unfrozen array - should succeed */
    Expr *arr_var2 = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token push_tok2;
    setup_token(&push_tok2, TOKEN_IDENTIFIER, "push", 3, "test.sn", &arena);
    Expr *member_expr2 = ast_create_member_expr(&arena, arr_var2, push_tok2, &push_tok2);

    type_checker_reset_error();
    Type *unfrozen_result = type_check_expr(member_expr2, &table);
    assert(unfrozen_result != NULL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test sync handles case with no frozen arguments */
static void test_sync_handles_no_frozen_args(void)
{
    printf("Testing sync handles no frozen arguments...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a pending thread handle with no frozen args */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token handle_tok;
    setup_token(&handle_tok, TOKEN_IDENTIFIER, "threadHandle", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle_tok, int_type);

    Symbol *handle_sym = symbol_table_lookup_symbol(&table, handle_tok);
    symbol_table_mark_pending(handle_sym);
    /* frozen_args is NULL by default, frozen_args_count is 0 */
    assert(handle_sym->frozen_args == NULL);
    assert(handle_sym->frozen_args_count == 0);

    /* Create sync expression */
    Expr *handle_expr = ast_create_variable_expr(&arena, handle_tok, &handle_tok);
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, handle_expr, false, &sync_tok);

    /* Type check should succeed even with no frozen args */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that multiple freezes are decremented correctly */
static void test_sync_multiple_freezes_decremented(void)
{
    printf("Testing multiple freezes are decremented correctly...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create an array */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "sharedArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_tok);

    /* Freeze twice (simulating two threads capturing the same array) */
    symbol_table_freeze_symbol(arr_sym);
    symbol_table_freeze_symbol(arr_sym);
    assert(symbol_table_get_freeze_count(arr_sym) == 2);
    assert(symbol_table_is_frozen(arr_sym));

    /* First sync unfreezes once */
    Token handle1_tok;
    setup_token(&handle1_tok, TOKEN_IDENTIFIER, "thread1", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle1_tok, int_type);
    Symbol *handle1_sym = symbol_table_lookup_symbol(&table, handle1_tok);
    symbol_table_mark_pending(handle1_sym);
    Symbol **frozen1 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen1[0] = arr_sym;
    symbol_table_set_frozen_args(handle1_sym, frozen1, 1);

    Expr *handle1_expr = ast_create_variable_expr(&arena, handle1_tok, &handle1_tok);
    Token sync1_tok;
    setup_token(&sync1_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync1_expr = ast_create_thread_sync_expr(&arena, handle1_expr, false, &sync1_tok);

    type_checker_reset_error();
    type_check_expr(sync1_expr, &table);
    assert(!type_checker_had_error());

    /* After first sync, still frozen (freeze_count = 1) */
    assert(symbol_table_get_freeze_count(arr_sym) == 1);
    assert(symbol_table_is_frozen(arr_sym));

    /* Second sync unfreezes completely */
    Token handle2_tok;
    setup_token(&handle2_tok, TOKEN_IDENTIFIER, "thread2", 3, "test.sn", &arena);
    symbol_table_add_symbol(&table, handle2_tok, int_type);
    Symbol *handle2_sym = symbol_table_lookup_symbol(&table, handle2_tok);
    symbol_table_mark_pending(handle2_sym);
    Symbol **frozen2 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen2[0] = arr_sym;
    symbol_table_set_frozen_args(handle2_sym, frozen2, 1);

    Expr *handle2_expr = ast_create_variable_expr(&arena, handle2_tok, &handle2_tok);
    Token sync2_tok;
    setup_token(&sync2_tok, TOKEN_BANG, "!", 3, "test.sn", &arena);
    Expr *sync2_expr = ast_create_thread_sync_expr(&arena, handle2_expr, false, &sync2_tok);

    type_checker_reset_error();
    type_check_expr(sync2_expr, &table);
    assert(!type_checker_had_error());

    /* After second sync, completely unfrozen */
    assert(symbol_table_get_freeze_count(arr_sym) == 0);
    assert(!symbol_table_is_frozen(arr_sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with is_array flag true validates array handle */
static void test_array_sync_validates_array_handle(void)
{
    printf("Testing array sync validates array handle...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create two pending thread handle variables */
    Token h1_tok, h2_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);

    /* Create array of variable expressions */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 2);
    elements[0] = v1;
    elements[1] = v2;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *array_expr = ast_create_array_expr(&arena, elements, 2, &arr_tok);

    /* Create array sync expression with is_array = true */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, array_expr, true, &sync_tok);

    /* Type check should succeed and return void */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    /* Verify both variables are now synchronized */
    assert(h1_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with non-array expression reports error */
static void test_array_sync_non_array_error(void)
{
    printf("Testing array sync with non-array expression reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create a variable (not an array) */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Create array sync expression with is_array = true but handle is not EXPR_ARRAY */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, var_expr, true, &sync_tok);

    /* Type check should fail */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with non-variable element reports error */
static void test_array_sync_non_variable_element_error(void)
{
    printf("Testing array sync with non-variable element reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create array with literal element (not variable) */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *literal = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);

    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    elements[0] = literal;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *array_expr = ast_create_array_expr(&arena, elements, 1, &arr_tok);

    /* Create array sync */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, array_expr, true, &sync_tok);

    /* Type check should fail - element is not a variable */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with non-pending element reports error */
static void test_array_sync_non_pending_element_error(void)
{
    printf("Testing array sync with non-pending element reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create variable that is NOT pending */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalVar", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);
    /* Don't mark as pending */

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    elements[0] = var_expr;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *array_expr = ast_create_array_expr(&arena, elements, 1, &arr_tok);

    /* Create array sync */
    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, array_expr, true, &sync_tok);

    /* Type check should fail - element is not pending */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync returns void type */
static void test_array_sync_returns_void(void)
{
    printf("Testing array sync returns void type...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create single pending variable in array */
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);

    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *));
    elements[0] = var_expr;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *array_expr = ast_create_array_expr(&arena, elements, 1, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, array_expr, true, &sync_tok);

    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync handles mixed states (some pending, some synchronized) */
static void test_array_sync_mixed_states(void)
{
    printf("Testing array sync handles mixed states gracefully...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create three thread handles */
    Token h1_tok, h2_tok, h3_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 1, "test.sn", &arena);
    setup_token(&h3_tok, TOKEN_IDENTIFIER, "t3", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);
    symbol_table_add_symbol(&table, h3_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    Symbol *h3_sym = symbol_table_lookup_symbol(&table, h3_tok);

    /* t1 is pending, t2 is already synchronized, t3 is pending */
    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);
    symbol_table_mark_synchronized(h2_sym); /* Already done */
    symbol_table_mark_pending(h3_sym);

    assert(h1_sym->thread_state == THREAD_STATE_PENDING);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h3_sym->thread_state == THREAD_STATE_PENDING);

    /* Create array sync with all three */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr *v3 = ast_create_variable_expr(&arena, h3_tok, &h3_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 3);
    elements[0] = v1;
    elements[1] = v2;
    elements[2] = v3;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *array_expr = ast_create_array_expr(&arena, elements, 3, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, array_expr, true, &sync_tok);

    /* Type check should succeed - mixed states handled gracefully */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    /* All should now be synchronized */
    assert(h1_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h3_sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync unfreezes arguments for all synced threads */
static void test_array_sync_unfreezes_all_arguments(void)
{
    printf("Testing array sync unfreezes all arguments...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Create shared arrays that will be frozen */
    Token arr1_tok, arr2_tok;
    setup_token(&arr1_tok, TOKEN_IDENTIFIER, "sharedArr1", 1, "test.sn", &arena);
    setup_token(&arr2_tok, TOKEN_IDENTIFIER, "sharedArr2", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr1_tok, array_type);
    symbol_table_add_symbol(&table, arr2_tok, array_type);

    Symbol *arr1_sym = symbol_table_lookup_symbol(&table, arr1_tok);
    Symbol *arr2_sym = symbol_table_lookup_symbol(&table, arr2_tok);

    /* Freeze both arrays */
    symbol_table_freeze_symbol(arr1_sym);
    symbol_table_freeze_symbol(arr2_sym);
    assert(symbol_table_is_frozen(arr1_sym));
    assert(symbol_table_is_frozen(arr2_sym));

    /* Create two pending thread handles with frozen args */
    Token h1_tok, h2_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 2, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);

    /* Set frozen args on thread handles */
    Symbol **frozen1 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen1[0] = arr1_sym;
    symbol_table_set_frozen_args(h1_sym, frozen1, 1);

    Symbol **frozen2 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen2[0] = arr2_sym;
    symbol_table_set_frozen_args(h2_sym, frozen2, 1);

    /* Create array sync */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 2);
    elements[0] = v1;
    elements[1] = v2;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *array_expr = ast_create_array_expr(&arena, elements, 2, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, array_expr, true, &sync_tok);

    /* Type check */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(!type_checker_had_error());

    /* Both shared arrays should be unfrozen */
    assert(!symbol_table_is_frozen(arr1_sym));
    assert(!symbol_table_is_frozen(arr2_sym));

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test array sync with same variable frozen by multiple threads */
static void test_array_sync_shared_frozen_variable(void)
{
    printf("Testing array sync with shared frozen variable...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);

    /* Create a shared array that will be frozen by BOTH threads */
    Token shared_arr_tok;
    setup_token(&shared_arr_tok, TOKEN_IDENTIFIER, "sharedData", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, shared_arr_tok, array_type);

    Symbol *shared_arr_sym = symbol_table_lookup_symbol(&table, shared_arr_tok);

    /* Freeze the array TWICE (simulating two threads capturing the same array) */
    symbol_table_freeze_symbol(shared_arr_sym);
    symbol_table_freeze_symbol(shared_arr_sym);
    assert(symbol_table_get_freeze_count(shared_arr_sym) == 2);
    assert(symbol_table_is_frozen(shared_arr_sym));

    /* Create two pending thread handles, BOTH referencing the same frozen array */
    Token h1_tok, h2_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 2, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 2, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);

    /* Both thread handles reference the SAME frozen array */
    Symbol **frozen1 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen1[0] = shared_arr_sym;
    symbol_table_set_frozen_args(h1_sym, frozen1, 1);

    Symbol **frozen2 = (Symbol **)arena_alloc(&arena, sizeof(Symbol *));
    frozen2[0] = shared_arr_sym;
    symbol_table_set_frozen_args(h2_sym, frozen2, 1);

    /* Create array sync [t1, t2]! */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 2);
    elements[0] = v1;
    elements[1] = v2;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 2, "test.sn", &arena);
    Expr *array_expr = ast_create_array_expr(&arena, elements, 2, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 2, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, array_expr, true, &sync_tok);

    /* Type check - should sync both and decrement freeze_count twice */
    type_checker_reset_error();
    Type *result = type_check_expr(sync_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    /* After syncing both threads, freeze_count should be 0 and array unfrozen */
    assert(symbol_table_get_freeze_count(shared_arr_sym) == 0);
    assert(!symbol_table_is_frozen(shared_arr_sym));

    /* Both thread handles should be synchronized */
    assert(h1_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Verify the array is now writable - test push method access */
    Expr *arr_var = ast_create_variable_expr(&arena, shared_arr_tok, &shared_arr_tok);
    Token push_tok;
    setup_token(&push_tok, TOKEN_IDENTIFIER, "push", 3, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);

    type_checker_reset_error();
    Type *push_result = type_check_expr(member_expr, &table);
    assert(push_result != NULL);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test accessing a pending variable reports error */
static void test_pending_variable_access_error(void)
{
    printf("Testing accessing pending variable reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and mark it pending (simulating spawn assignment) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "pendingResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    assert(symbol_table_is_pending(sym));

    /* Create variable expression to access the pending variable */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(var_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test accessing a synchronized variable is allowed */
static void test_synchronized_variable_access_allowed(void)
{
    printf("Testing accessing synchronized variable is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and mark it synchronized */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "syncedResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    symbol_table_mark_synchronized(sym);
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Create variable expression to access the synchronized variable */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(var_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test accessing a normal (non-thread) variable is allowed */
static void test_normal_variable_access_allowed(void)
{
    printf("Testing accessing normal variable is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal variable (not a thread) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalVar", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Verify variable is NORMAL state (default) */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(sym->thread_state == THREAD_STATE_NORMAL);

    /* Create variable expression */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(var_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test all array elements become accessible after sync */
static void test_array_sync_all_elements_accessible(void)
{
    printf("Testing all array elements accessible after sync...\n");
    Arena arena;
    arena_init(&arena, 8192);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);

    /* Create three pending thread handles */
    Token h1_tok, h2_tok, h3_tok;
    setup_token(&h1_tok, TOKEN_IDENTIFIER, "t1", 1, "test.sn", &arena);
    setup_token(&h2_tok, TOKEN_IDENTIFIER, "t2", 1, "test.sn", &arena);
    setup_token(&h3_tok, TOKEN_IDENTIFIER, "t3", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, h1_tok, int_type);
    symbol_table_add_symbol(&table, h2_tok, int_type);
    symbol_table_add_symbol(&table, h3_tok, int_type);

    Symbol *h1_sym = symbol_table_lookup_symbol(&table, h1_tok);
    Symbol *h2_sym = symbol_table_lookup_symbol(&table, h2_tok);
    Symbol *h3_sym = symbol_table_lookup_symbol(&table, h3_tok);

    symbol_table_mark_pending(h1_sym);
    symbol_table_mark_pending(h2_sym);
    symbol_table_mark_pending(h3_sym);

    /* Create array sync */
    Expr *v1 = ast_create_variable_expr(&arena, h1_tok, &h1_tok);
    Expr *v2 = ast_create_variable_expr(&arena, h2_tok, &h2_tok);
    Expr *v3 = ast_create_variable_expr(&arena, h3_tok, &h3_tok);
    Expr **elements = (Expr **)arena_alloc(&arena, sizeof(Expr *) * 3);
    elements[0] = v1;
    elements[1] = v2;
    elements[2] = v3;

    Token arr_tok;
    setup_token(&arr_tok, TOKEN_LEFT_BRACKET, "[", 1, "test.sn", &arena);
    Expr *array_expr = ast_create_array_expr(&arena, elements, 3, &arr_tok);

    Token sync_tok;
    setup_token(&sync_tok, TOKEN_BANG, "!", 1, "test.sn", &arena);
    Expr *sync_expr = ast_create_thread_sync_expr(&arena, array_expr, true, &sync_tok);

    /* Sync all */
    type_checker_reset_error();
    type_check_expr(sync_expr, &table);
    assert(!type_checker_had_error());

    /* All should be synchronized (accessible) */
    assert(h1_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h2_sym->thread_state == THREAD_STATE_SYNCHRONIZED);
    assert(h3_sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Verify we can access each variable (type check should succeed) */
    type_checker_reset_error();
    Type *r1 = type_check_expr(v1, &table);
    assert(r1 != NULL);
    assert(r1->kind == TYPE_INT);
    assert(!type_checker_had_error());

    type_checker_reset_error();
    Type *r2 = type_check_expr(v2, &table);
    assert(r2 != NULL);
    assert(r2->kind == TYPE_INT);
    assert(!type_checker_had_error());

    type_checker_reset_error();
    Type *r3 = type_check_expr(v3, &table);
    assert(r3 != NULL);
    assert(r3->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test reassigning a pending variable reports error */
static void test_pending_variable_reassign_error(void)
{
    printf("Testing reassigning pending variable reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and mark it pending (simulating spawn assignment) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "pendingResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    assert(symbol_table_is_pending(sym));

    /* Create assignment expression: pendingResult = 42 */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *value_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, var_tok, value_expr, &var_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(assign_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test reassigning a synchronized variable is allowed */
static void test_synchronized_variable_reassign_allowed(void)
{
    printf("Testing reassigning synchronized variable is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and mark it synchronized */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "syncedResult", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_mark_pending(sym);
    symbol_table_mark_synchronized(sym);
    assert(sym->thread_state == THREAD_STATE_SYNCHRONIZED);

    /* Create assignment expression: syncedResult = 42 */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *value_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, var_tok, value_expr, &var_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(assign_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test reassigning a normal (non-thread) variable is allowed */
static void test_normal_variable_reassign_allowed(void)
{
    printf("Testing reassigning normal variable is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal variable (not a thread) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalVar", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    /* Verify variable is NORMAL state (default) */
    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(sym->thread_state == THREAD_STATE_NORMAL);

    /* Create assignment expression: normalVar = 42 */
    LiteralValue lit_val;
    lit_val.int_value = 42;
    Token lit_tok;
    setup_token(&lit_tok, TOKEN_INT_LITERAL, "42", 1, "test.sn", &arena);
    Expr *value_expr = ast_create_literal_expr(&arena, lit_val, int_type, false, &lit_tok);
    Expr *assign_expr = ast_create_assign_expr(&arena, var_tok, value_expr, &var_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(assign_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that mutating methods on frozen arrays report error */
static void test_frozen_array_mutating_method_error(void)
{
    printf("Testing mutating method on frozen array reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add an array variable and freeze it */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "frozenArr", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, arr_tok);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym));

    /* Create member expression: frozenArr.push */
    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token push_tok;
    setup_token(&push_tok, TOKEN_IDENTIFIER, "push", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, arr_var, push_tok, &push_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(member_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that read-only methods on frozen arrays are allowed */
static void test_frozen_array_readonly_method_allowed(void)
{
    printf("Testing read-only method on frozen array is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add an array variable and freeze it */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Token arr_tok;
    setup_token(&arr_tok, TOKEN_IDENTIFIER, "frozenArr", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, arr_tok, array_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, arr_tok);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym));

    /* Create member expression: frozenArr.length */
    Expr *arr_var = ast_create_variable_expr(&arena, arr_tok, &arr_tok);
    Token length_tok;
    setup_token(&length_tok, TOKEN_IDENTIFIER, "length", 1, "test.sn", &arena);
    Expr *member_expr = ast_create_member_expr(&arena, arr_var, length_tok, &length_tok);

    /* Type check should succeed - length is read-only */
    type_checker_reset_error();
    Type *result = type_check_expr(member_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that incrementing a frozen variable reports error */
static void test_frozen_variable_increment_error(void)
{
    printf("Testing incrementing frozen variable reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and freeze it */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "frozenCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym));

    /* Create increment expression: frozenCounter++ */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Token inc_tok;
    setup_token(&inc_tok, TOKEN_PLUS_PLUS, "++", 1, "test.sn", &arena);
    Expr *inc_expr = ast_create_increment_expr(&arena, var_expr, &inc_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(inc_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that decrementing a frozen variable reports error */
static void test_frozen_variable_decrement_error(void)
{
    printf("Testing decrementing frozen variable reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a variable and freeze it */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "frozenCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    symbol_table_freeze_symbol(sym);
    assert(symbol_table_is_frozen(sym));

    /* Create decrement expression: frozenCounter-- */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Token dec_tok;
    setup_token(&dec_tok, TOKEN_MINUS_MINUS, "--", 1, "test.sn", &arena);
    Expr *dec_expr = ast_create_decrement_expr(&arena, var_expr, &dec_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(dec_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that incrementing a normal variable is allowed */
static void test_normal_variable_increment_allowed(void)
{
    printf("Testing incrementing normal variable is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal variable (not frozen) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(!symbol_table_is_frozen(sym));

    /* Create increment expression: normalCounter++ */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Token inc_tok;
    setup_token(&inc_tok, TOKEN_PLUS_PLUS, "++", 1, "test.sn", &arena);
    Expr *inc_expr = ast_create_increment_expr(&arena, var_expr, &inc_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(inc_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test that decrementing a normal variable is allowed */
static void test_normal_variable_decrement_allowed(void)
{
    printf("Testing decrementing normal variable is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Add a normal variable (not frozen) */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Token var_tok;
    setup_token(&var_tok, TOKEN_IDENTIFIER, "normalCounter", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, var_tok, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, var_tok);
    assert(!symbol_table_is_frozen(sym));

    /* Create decrement expression: normalCounter-- */
    Expr *var_expr = ast_create_variable_expr(&arena, var_tok, &var_tok);
    Token dec_tok;
    setup_token(&dec_tok, TOKEN_MINUS_MINUS, "--", 1, "test.sn", &arena);
    Expr *dec_expr = ast_create_decrement_expr(&arena, var_expr, &dec_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(dec_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test private function returning array type reports error */
static void test_private_function_array_return_error(void)
{
    printf("Testing private function returning array type reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a private function returning int[] */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Type *func_type = ast_create_function_type(&arena, array_type, NULL, 0);

    /* Add the function to symbol table with FUNC_PRIVATE modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_PRIVATE;
    func_sym->declared_func_mod = FUNC_PRIVATE;

    /* Create a call expression to the private function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test private function returning string type reports error */
static void test_private_function_string_return_error(void)
{
    printf("Testing private function returning string type reports error...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a private function returning str */
    Type *string_type = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *func_type = ast_create_function_type(&arena, string_type, NULL, 0);

    /* Add the function to symbol table with FUNC_PRIVATE modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getString", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_PRIVATE;
    func_sym->declared_func_mod = FUNC_PRIVATE;

    /* Create a call expression to the private function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should return NULL and set error */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result == NULL);
    assert(type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test private function returning primitive int is allowed */
static void test_private_function_int_return_allowed(void)
{
    printf("Testing private function returning int is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a private function returning int */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *func_type = ast_create_function_type(&arena, int_type, NULL, 0);

    /* Add the function to symbol table with FUNC_PRIVATE modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getInt", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_PRIVATE;
    func_sym->declared_func_mod = FUNC_PRIVATE;

    /* Create a call expression to the private function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_INT);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test private function returning void is allowed */
static void test_private_function_void_return_allowed(void)
{
    printf("Testing private function returning void is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a private function returning void */
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *func_type = ast_create_function_type(&arena, void_type, NULL, 0);

    /* Add the function to symbol table with FUNC_PRIVATE modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "doWork", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_PRIVATE;
    func_sym->declared_func_mod = FUNC_PRIVATE;

    /* Create a call expression to the private function */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should succeed */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_VOID);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test default (non-private) function returning array is allowed */
static void test_default_function_array_return_allowed(void)
{
    printf("Testing default function returning array is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a default function returning int[] */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Type *func_type = ast_create_function_type(&arena, array_type, NULL, 0);

    /* Add the function to symbol table with FUNC_DEFAULT modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_DEFAULT;

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should succeed - default modifier allows any return type */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

/* Test shared function returning array is allowed */
static void test_shared_function_array_return_allowed(void)
{
    printf("Testing shared function returning array is allowed...\n");
    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    /* Create a shared function returning int[] */
    Type *int_type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *array_type = ast_create_array_type(&arena, int_type);
    Type *func_type = ast_create_function_type(&arena, array_type, NULL, 0);

    /* Add the function to symbol table with FUNC_SHARED modifier */
    Token func_tok;
    setup_token(&func_tok, TOKEN_IDENTIFIER, "getArray", 1, "test.sn", &arena);
    symbol_table_add_symbol(&table, func_tok, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_tok);
    func_sym->is_function = true;
    func_sym->func_mod = FUNC_SHARED;

    /* Create a call expression */
    Expr *callee = ast_create_variable_expr(&arena, func_tok, &func_tok);
    Expr *call_expr = ast_create_call_expr(&arena, callee, NULL, 0, &func_tok);

    /* Create thread spawn */
    Token spawn_tok;
    setup_token(&spawn_tok, TOKEN_AMPERSAND, "&", 1, "test.sn", &arena);
    Expr *spawn_expr = ast_create_thread_spawn_expr(&arena, call_expr, FUNC_DEFAULT, &spawn_tok);

    /* Type check should succeed - shared modifier allows any return type */
    type_checker_reset_error();
    Type *result = type_check_expr(spawn_expr, &table);
    assert(result != NULL);
    assert(result->kind == TYPE_ARRAY);
    assert(!type_checker_had_error());

    symbol_table_cleanup(&table);
    arena_free(&arena);
}

void test_type_checker_thread_main(void)
{
    printf("\n=== Running Thread Type Checker Tests ===\n\n");

    test_thread_spawn_non_call_error();
    test_thread_spawn_non_function_error();
    test_void_spawn_assignment_error();
    test_valid_spawn_returns_correct_type();
    test_pending_state_marked_on_spawn_assignment();
    test_array_arg_frozen_after_spawn();
    test_frozen_args_stored_in_pending_symbol();
    test_as_ref_primitive_frozen_after_spawn();
    test_spawn_type_mismatch_error();
    test_sync_non_variable_error();
    test_sync_unknown_variable_error();
    test_sync_non_pending_variable_error();
    test_valid_sync_returns_correct_type();
    test_sync_state_transition();
    test_sync_unfreezes_arguments();
    test_frozen_arg_writable_after_sync();
    test_sync_handles_no_frozen_args();
    test_sync_multiple_freezes_decremented();
    test_array_sync_validates_array_handle();
    test_array_sync_non_array_error();
    test_array_sync_non_variable_element_error();
    test_array_sync_non_pending_element_error();
    test_array_sync_returns_void();
    test_array_sync_mixed_states();
    test_array_sync_unfreezes_all_arguments();
    test_array_sync_shared_frozen_variable();
    test_array_sync_all_elements_accessible();
    test_pending_variable_access_error();
    test_synchronized_variable_access_allowed();
    test_normal_variable_access_allowed();
    test_pending_variable_reassign_error();
    test_synchronized_variable_reassign_allowed();
    test_normal_variable_reassign_allowed();
    test_frozen_array_mutating_method_error();
    test_frozen_array_readonly_method_allowed();
    test_frozen_variable_increment_error();
    test_frozen_variable_decrement_error();
    test_normal_variable_increment_allowed();
    test_normal_variable_decrement_allowed();
    test_private_function_array_return_error();
    test_private_function_string_return_error();
    test_private_function_int_return_allowed();
    test_private_function_void_return_allowed();
    test_default_function_array_return_allowed();
    test_shared_function_array_return_allowed();

    printf("\n=== All Thread Type Checker Tests Passed ===\n\n");
}
