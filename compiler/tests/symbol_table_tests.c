// tests/symbol_table_tests.c

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../arena.h"
#include "../debug.h"
#include "../symbol_table.h"
#include "../ast.h"

// Helper macros and constants
#define TEST_ARENA_SIZE 4096
#define TOKEN_LITERAL(str) ((Token){ .start = str, .length = sizeof(str) - 1, .line = 1, .type = TOKEN_IDENTIFIER })
#define TOKEN_PTR(str, len) ((Token){ .start = (const char*)str, .length = len, .line = 1, .type = TOKEN_IDENTIFIER })

// Helper to create a simple int type
static Type *create_int_type(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_INT);
}

// Helper to create a simple string type (pointer-sized)
static Type *create_string_type(Arena *arena) {
    return ast_create_primitive_type(arena, TYPE_STRING);
}

// Test initialization with NULL
void test_symbol_table_init_null_arena() {
    DEBUG_INFO("Starting test_symbol_table_init_null_arena");
    printf("Testing symbol_table_init with NULL arena...\n");

    SymbolTable table = {0};  // Zero-initialize to check it stays zeroed
    symbol_table_init(NULL, &table);
    // Should not crash, and table remains zeroed since arena is NULL
    assert(table.scopes == NULL);
    assert(table.scopes_count == 0);
    assert(table.current == NULL);

    DEBUG_INFO("Finished test_symbol_table_init_null_arena");
}

// Test initialization and basic global scope
void test_symbol_table_init_basic() {
    DEBUG_INFO("Starting test_symbol_table_init_basic");
    printf("Testing symbol_table_init basic...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    assert(table.arena == &arena);
    assert(table.scopes != NULL);
    assert(table.scopes_count == 1);
    assert(table.current != NULL);
    assert(table.global_scope == table.current);
    assert(table.current->symbols == NULL);
    assert(table.current->enclosing == NULL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_init_basic");
}

// Test cleanup on empty table
void test_symbol_table_cleanup_empty() {
    DEBUG_INFO("Starting test_symbol_table_cleanup_empty");
    printf("Testing symbol_table_cleanup on empty table...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    symbol_table_cleanup(&table);  // Should handle empty scopes
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_cleanup_empty");
}

// Test pushing a single scope
void test_symbol_table_push_scope_single() {
    DEBUG_INFO("Starting test_symbol_table_push_scope_single");
    printf("Testing symbol_table_push_scope single...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    int initial_count = table.scopes_count;
    symbol_table_push_scope(&table);
    assert(table.scopes_count == initial_count + 1);
    assert(table.current->enclosing == table.global_scope);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET);
    assert(table.current->symbols == NULL);

    symbol_table_pop_scope(&table);  // Restore
    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_push_scope_single");
}

// Test pushing multiple scopes and nesting
void test_symbol_table_push_scope_nested() {
    DEBUG_INFO("Starting test_symbol_table_push_scope_nested");
    printf("Testing symbol_table_push_scope nested...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    symbol_table_push_scope(&table);  // Scope 1
    Scope *scope1 = table.current;
    symbol_table_push_scope(&table);  // Scope 2
    Scope *scope2 = table.current;
    assert(scope2->enclosing == scope1);

    symbol_table_pop_scope(&table);  // Back to scope1
    assert(table.current == scope1);
    symbol_table_pop_scope(&table);  // Back to global
    assert(table.current == table.global_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_push_scope_nested");
}

// Test scope expansion (when capacity exceeded)
void test_symbol_table_push_scope_expand() {
    DEBUG_INFO("Starting test_symbol_table_push_scope_expand");
    printf("Testing symbol_table_push_scope with array expansion...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 2);  // Larger for multiple reallocs
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Push until expansion (start with capacity 8, push 9th total)
    for (int i = 0; i < 8; i++) {
        symbol_table_push_scope(&table);
    }
    assert(table.scopes_count == 9);
    assert(table.scopes_capacity >= 16);  // Doubled from 8

    // Pop all added scopes (back to global)
    for (int i = 0; i < 8; i++) {
        symbol_table_pop_scope(&table);
    }
    assert(table.current == table.global_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_push_scope_expand");
}

// Test popping beyond global (should do nothing)
void test_symbol_table_pop_scope_beyond_global() {
    DEBUG_INFO("Starting test_symbol_table_pop_scope_beyond_global");
    printf("Testing symbol_table_pop_scope beyond global...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    symbol_table_pop_scope(&table);  // Should do nothing
    assert(table.current == table.global_scope);

    // Pop after pushing one
    symbol_table_push_scope(&table);
    symbol_table_pop_scope(&table);
    symbol_table_pop_scope(&table);  // Now beyond
    assert(table.current == table.global_scope);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_pop_scope_beyond_global");
}

// Test offset propagation on pop (max of child and parent)
void test_symbol_table_pop_scope_offset_propagation() {
    DEBUG_INFO("Starting test_symbol_table_pop_scope_offset_propagation");
    printf("Testing symbol_table_pop_scope offset propagation...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Global offsets at base
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    symbol_table_push_scope(&table);  // Child1, add local to increase
    Type *int_type = create_int_type(&arena);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("var1"), int_type);
    (void)table.current->next_local_offset;  // Increased by 8 (int size aligned)

    symbol_table_push_scope(&table);  // Child2, smaller increase
    symbol_table_add_symbol(&table, TOKEN_LITERAL("var2"), int_type);
    int child2_local = table.current->next_local_offset;  // Increased by another 8

    symbol_table_pop_scope(&table);  // Back to child1, should take max (child2_local > child1_local)
    assert(table.current->next_local_offset == child2_local);

    symbol_table_pop_scope(&table);  // Back to global, take max
    assert(table.global_scope->next_local_offset == child2_local);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_pop_scope_offset_propagation");
}

// Test begin_function_scope (push + reset offsets)
void test_symbol_table_begin_function_scope() {
    DEBUG_INFO("Starting test_symbol_table_begin_function_scope");
    printf("Testing symbol_table_begin_function_scope...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Add something to global to increase offsets
    Type *int_type = create_int_type(&arena);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("global_var"), int_type);
    int global_offset = table.global_scope->next_local_offset;

    symbol_table_begin_function_scope(&table);  // Pushes and resets
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET);
    assert(table.current->enclosing == table.global_scope);

    symbol_table_pop_scope(&table);  // Offsets should propagate max to global
    assert(table.global_scope->next_local_offset == global_offset);  // Unchanged, since func reset to base < global

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_begin_function_scope");
}

// Test adding local symbol basic
void test_symbol_table_add_symbol_local_basic() {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_local_basic");
    printf("Testing symbol_table_add_symbol local basic...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("test_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol_current(&table, name);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_LOCAL);
    assert(sym->type->kind == TYPE_INT);
    assert(sym->offset == -LOCAL_BASE_OFFSET);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 8);  // int size 8, aligned

    // Check duplicated name (should update type, but since same, ok; code updates type)
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    symbol_table_add_symbol(&table, name, double_type);
    assert(sym->type->kind == TYPE_DOUBLE);  // Updated

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_local_basic");
}

void test_symbol_table_add_symbol_param() {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_param");
    printf("Testing symbol_table_add_symbol_with_kind param...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);  // Fix: Initialize the symbol table with the arena
    symbol_table_begin_function_scope(&table);  // Function scope

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("param1");
    symbol_table_add_symbol_with_kind(&table, name, int_type, SYMBOL_PARAM);

    Symbol *sym = symbol_table_lookup_symbol_current(&table, name);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_PARAM);
    assert(sym->offset == -PARAM_BASE_OFFSET);
    assert(table.current->next_param_offset == PARAM_BASE_OFFSET + 8);

    // Add another param, offset accumulates negatively
    Token name2 = TOKEN_LITERAL("param2");
    symbol_table_add_symbol_with_kind(&table, name2, int_type, SYMBOL_PARAM);
    Symbol *sym2 = symbol_table_lookup_symbol_current(&table, name2);
    assert(sym2->offset == -(PARAM_BASE_OFFSET + 8));

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_param");
}

// Test adding global (kind default, offset 0)
void test_symbol_table_add_symbol_global() {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_global");
    printf("Testing symbol_table_add_symbol global (offset 0)...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("global_sym");
    symbol_table_add_symbol_with_kind(&table, name, int_type, SYMBOL_GLOBAL);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym != NULL);
    assert(sym->kind == SYMBOL_GLOBAL);
    assert(sym->offset == 0);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_global");
}

// Test adding in no current scope (should error, do nothing)
void test_symbol_table_add_symbol_no_scope() {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_no_scope");
    printf("Testing symbol_table_add_symbol with no current scope...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table = {0};  // Uninitialized, current NULL

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("no_scope_var");
    symbol_table_add_symbol(&table, name, int_type);  // Should do nothing

    assert(table.current == NULL);  // Unchanged

    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_no_scope");
}

// Test lookup current scope basic
void test_symbol_table_lookup_current_basic() {
    DEBUG_INFO("Starting test_symbol_table_lookup_current_basic");
    printf("Testing symbol_table_lookup_symbol_current basic...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("local_var");
    symbol_table_add_symbol(&table, name, int_type);

    Symbol *sym = symbol_table_lookup_symbol_current(&table, name);
    assert(sym != NULL);
    assert(strcmp(sym->name.start, "local_var") == 0);

    // Non-existent
    Token bad_name = TOKEN_LITERAL("bad_var");
    sym = symbol_table_lookup_symbol_current(&table, bad_name);
    assert(sym == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_current_basic");
}

// Test lookup all scopes (enclosing)
void test_symbol_table_lookup_enclosing() {
    DEBUG_INFO("Starting test_symbol_table_lookup_enclosing");
    printf("Testing symbol_table_lookup_symbol with enclosing scopes...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Global
    Type *int_type = create_int_type(&arena);
    Token global_name = TOKEN_LITERAL("global_var");
    symbol_table_add_symbol(&table, global_name, int_type);

    symbol_table_push_scope(&table);  // Inner
    Token inner_name = TOKEN_LITERAL("inner_var");
    symbol_table_add_symbol(&table, inner_name, int_type);

    // Lookup inner (current)
    Symbol *sym = symbol_table_lookup_symbol(&table, inner_name);
    assert(sym != NULL);
    assert(sym->name.length == 9);  // "inner_var"

    // Lookup global (enclosing)
    sym = symbol_table_lookup_symbol(&table, global_name);
    assert(sym != NULL);
    assert(strcmp(sym->name.start, "global_var") == 0);

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_enclosing");
}

// Test shadowing (inner hides outer)
void test_symbol_table_lookup_shadowing() {
    DEBUG_INFO("Starting test_symbol_table_lookup_shadowing");
    printf("Testing symbol_table_lookup_symbol with shadowing...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("shadow_var");

    // Global
    symbol_table_add_symbol(&table, name, int_type);
    Symbol *global_sym = symbol_table_lookup_symbol(&table, name);
    assert(global_sym != NULL);
    assert(global_sym->offset == -LOCAL_BASE_OFFSET);  // Local in global scope?

    symbol_table_push_scope(&table);  // Inner, add same name
    symbol_table_add_symbol(&table, name, create_string_type(&arena));  // Different type

    // Lookup should find inner
    Symbol *inner_sym = symbol_table_lookup_symbol(&table, name);
    assert(inner_sym != NULL);
    assert(inner_sym->type->kind == TYPE_STRING);
    assert(inner_sym != global_sym);  // Different symbol

    symbol_table_pop_scope(&table);
    // Now back, should find global
    Symbol *back_sym = symbol_table_lookup_symbol(&table, name);
    assert(back_sym == global_sym);
    assert(back_sym->type->kind == TYPE_INT);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_shadowing");
}

// Test token equality variations (pointer match, content match, different lengths)
void test_symbol_table_lookup_token_variations() {
    DEBUG_INFO("Starting test_symbol_table_lookup_token_variations");
    printf("Testing symbol_table_lookup_symbol with token variations...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    const char *name_str = "test123";
    Token orig_name = TOKEN_PTR(name_str, 7);
    symbol_table_add_symbol(&table, orig_name, int_type);

    // Same pointer
    Token same_ptr = TOKEN_PTR(name_str, 7);  // Same start addr
    Symbol *sym1 = symbol_table_lookup_symbol(&table, same_ptr);
    assert(sym1 != NULL);

    // Different pointer, same content (e.g., duplicated string)
    char dup_str[] = "test123";
    Token diff_ptr = TOKEN_PTR(dup_str, 7);
    Symbol *sym2 = symbol_table_lookup_symbol(&table, diff_ptr);
    assert(sym2 != NULL);
    assert(sym2 == sym1);  // Same symbol

    // Mismatch length
    Token short_name = TOKEN_PTR(name_str, 6);  // "test12"
    Symbol *sym3 = symbol_table_lookup_symbol(&table, short_name);
    assert(sym3 == NULL);

    // Mismatch content
    char diff_str[] = "test124";
    Token diff_content = TOKEN_PTR(diff_str, 7);
    Symbol *sym4 = symbol_table_lookup_symbol(&table, diff_content);
    assert(sym4 == NULL);

    // Case sensitive? Assuming yes, as memcmp
    char upper_str[] = "TEST123";
    Token upper = TOKEN_PTR(upper_str, 7);
    Symbol *sym5 = symbol_table_lookup_symbol(&table, upper);
    assert(sym5 == NULL);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_token_variations");
}

// Test lookup with NULL table or current
void test_symbol_table_lookup_nulls() {
    DEBUG_INFO("Starting test_symbol_table_lookup_nulls");
    printf("Testing symbol_table_lookup_symbol with NULLs...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Token name = TOKEN_LITERAL("any_var");
    Symbol *sym = symbol_table_lookup_symbol(NULL, name);
    assert(sym == NULL);

    sym = symbol_table_lookup_symbol(&table, name);  // Not added yet
    assert(sym == NULL);

    // Set current to NULL manually (edge)
    table.current = NULL;
    sym = symbol_table_lookup_symbol(&table, name);
    assert(sym == NULL);

    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_lookup_nulls");
}

// Test get_symbol_offset basic and not found
void test_symbol_table_get_symbol_offset() {
    DEBUG_INFO("Starting test_symbol_table_get_symbol_offset");
    printf("Testing symbol_table_get_symbol_offset...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("offset_var");
    symbol_table_add_symbol(&table, name, int_type);

    int offset = symbol_table_get_symbol_offset(&table, name);
    assert(offset == -LOCAL_BASE_OFFSET);

    // Not found
    Token bad_name = TOKEN_LITERAL("bad_offset");
    offset = symbol_table_get_symbol_offset(&table, bad_name);
    assert(offset == -1);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_get_symbol_offset");
}

// Test offset alignment for different type sizes (char=1, int=8, etc.)
void test_symbol_table_offsets_alignment() {
    DEBUG_INFO("Starting test_symbol_table_offsets_alignment");
    printf("Testing symbol_table offsets with alignment...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);
    symbol_table_begin_function_scope(&table);  // For params/locals

    // Char (1 byte, align to 8 -> 8)
    Type *char_type = ast_create_primitive_type(&arena, TYPE_CHAR);
    Token char_name = TOKEN_LITERAL("ch");
    symbol_table_add_symbol_with_kind(&table, char_name, char_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 8);

    // Int (8 bytes, align 8)
    Type *int_type = create_int_type(&arena);
    Token int_name = TOKEN_LITERAL("i");
    symbol_table_add_symbol_with_kind(&table, int_name, int_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 16);

    // Bool (1 byte, align 8)
    Type *bool_type = ast_create_primitive_type(&arena, TYPE_BOOL);
    Token bool_name = TOKEN_LITERAL("b");
    symbol_table_add_symbol_with_kind(&table, bool_name, bool_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 24);

    // Double (8)
    Type *double_type = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Token double_name = TOKEN_LITERAL("d");
    symbol_table_add_symbol_with_kind(&table, double_name, double_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 32);

    // String (8)
    Type *str_type = create_string_type(&arena);
    Token str_name = TOKEN_LITERAL("s");
    symbol_table_add_symbol_with_kind(&table, str_name, str_type, SYMBOL_LOCAL);
    assert(table.current->next_local_offset == LOCAL_BASE_OFFSET + 40);

    // Check actual offsets negative
    Symbol *sym = symbol_table_lookup_symbol(&table, char_name);
    assert(sym->offset == -LOCAL_BASE_OFFSET);
    sym = symbol_table_lookup_symbol(&table, int_name);
    assert(sym->offset == -(LOCAL_BASE_OFFSET + 8));
    sym = symbol_table_lookup_symbol(&table, bool_name);
    assert(sym->offset == -(LOCAL_BASE_OFFSET + 16));
    sym = symbol_table_lookup_symbol(&table, double_name);
    assert(sym->offset == -(LOCAL_BASE_OFFSET + 24));
    sym = symbol_table_lookup_symbol(&table, str_name);
    assert(sym->offset == -(LOCAL_BASE_OFFSET + 32));

    symbol_table_pop_scope(&table);
    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_offsets_alignment");
}

// Test type cloning on add (since add clones type)
void test_symbol_table_add_symbol_type_clone() {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_type_clone");
    printf("Testing symbol_table_add_symbol type cloning...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *orig_type = create_int_type(&arena);
    Token name = TOKEN_LITERAL("clone_var");
    symbol_table_add_symbol(&table, name, orig_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, name);
    assert(sym->type != orig_type);  // Cloned
    assert(ast_type_equals(sym->type, orig_type));  // But equal

    // Modify orig (if possible), but since primitive, just check equality

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_type_clone");
}

// Test arena exhaustion (simulate by small arena, but hard; test alloc failures)
void test_symbol_table_add_symbol_arena_exhaust() {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_arena_exhaust");
    printf("Testing symbol_table_add_symbol with arena exhaustion (simulated)...\n");

    // Note: Hard to simulate exact OOM in unit test without mocking arena.
    // Assume code handles NULL from arena_alloc gracefully (DEBUG_ERROR, return early)

    Arena small_arena;
    arena_init(&small_arena, 64);  // Very small
    SymbolTable table;
    symbol_table_init(&small_arena, &table);

    // This may fail, but test that it doesn't crash
    Type *int_type = create_int_type(&small_arena);  // May fail if arena too small
    if (int_type == NULL) {
        printf("Arena too small, skipping add test.\n");
    } else {
        Token name = TOKEN_LITERAL("oom_var");
        symbol_table_add_symbol(&table, name, int_type);
        // If added, ok; if not, current->symbols may be NULL
    }

    symbol_table_cleanup(&table);
    arena_free(&small_arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_arena_exhaust");
}

// Test adding many symbols (stress offsets)
void test_symbol_table_add_many_symbols() {
    DEBUG_INFO("Starting test_symbol_table_add_many_symbols");
    printf("Testing symbol_table_add_symbol many times (offset accumulation)...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE * 4);  // Larger for many
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    int expected_offset = LOCAL_BASE_OFFSET;
    for (int i = 0; i < 100; i++) {  // 100 locals, each 8 bytes
        char name_buf[16];
        snprintf(name_buf, sizeof(name_buf), "var_%d", i);
        Token name = TOKEN_PTR(name_buf, strlen(name_buf));
        symbol_table_add_symbol(&table, name, int_type);

        Symbol *sym = symbol_table_lookup_symbol_current(&table, name);
        assert(sym != NULL);
        assert(sym->offset == -expected_offset);
        expected_offset += 8;
    }
    assert(table.current->next_local_offset == expected_offset);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_many_symbols");
}

// Test token duplication on add (arena_strndup)
void test_symbol_table_add_symbol_token_dup() {
    DEBUG_INFO("Starting test_symbol_table_add_symbol_token_dup");
    printf("Testing symbol_table_add_symbol token duplication...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    const char *orig_str = "dup_test";
    Token orig_token = TOKEN_PTR(orig_str, 8);
    Type *int_type = create_int_type(&arena);
    symbol_table_add_symbol(&table, orig_token, int_type);

    Symbol *sym = symbol_table_lookup_symbol(&table, orig_token);
    assert(sym != NULL);
    assert(sym->name.start != orig_str);  // Duplicated
    assert(memcmp(sym->name.start, orig_str, 8) == 0);
    assert(sym->name.length == 8);
    assert(sym->name.line == 1);
    assert(sym->name.type == TOKEN_IDENTIFIER);

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_symbol_token_dup");
}

// Test complex types (array, function) cloning and size
void test_symbol_table_add_complex_types() {
    DEBUG_INFO("Starting test_symbol_table_add_complex_types");
    printf("Testing symbol_table_add_symbol with complex types (array, function)...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    // Array type
    Type *int_type = create_int_type(&arena);
    Type *arr_type = ast_create_array_type(&arena, int_type);
    Token arr_name = TOKEN_LITERAL("arr_sym");
    symbol_table_add_symbol(&table, arr_name, arr_type);
    Symbol *arr_sym = symbol_table_lookup_symbol(&table, arr_name);
    assert(arr_sym->type->kind == TYPE_ARRAY);
    assert(get_type_size(arr_sym->type) == 8);  // Default size 8 for pointer?

    // Function type
    Type *void_type = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *param_types[] = {int_type};
    Type *func_type = ast_create_function_type(&arena, void_type, param_types, 1);
    Token func_name = TOKEN_LITERAL("func_sym");
    symbol_table_add_symbol(&table, func_name, func_type);
    Symbol *func_sym = symbol_table_lookup_symbol(&table, func_name);
    assert(func_sym->type->kind == TYPE_FUNCTION);
    assert(get_type_size(func_sym->type) == 8);  // Default

    // Check equality after clone
    assert(ast_type_equals(arr_sym->type, arr_type));
    assert(ast_type_equals(func_sym->type, func_type));

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_add_complex_types");
}

// Test print function (basic output verification via printf, but since DEBUG, assume it works)
void test_symbol_table_print() {
    DEBUG_INFO("Starting test_symbol_table_print");
    printf("Testing symbol_table_print...\n");

    Arena arena;
    arena_init(&arena, TEST_ARENA_SIZE);
    SymbolTable table;
    symbol_table_init(&arena, &table);

    Type *int_type = create_int_type(&arena);
    symbol_table_add_symbol(&table, TOKEN_LITERAL("print_var"), int_type);

    // Print should output without crash
    symbol_table_print(&table, "test_print");

    symbol_table_cleanup(&table);
    arena_free(&arena);

    DEBUG_INFO("Finished test_symbol_table_print");
}

void test_symbol_table_main() 
{
    test_symbol_table_init_null_arena();
    test_symbol_table_init_basic();
    test_symbol_table_cleanup_empty();
    test_symbol_table_push_scope_single();
    test_symbol_table_push_scope_nested();
    test_symbol_table_push_scope_expand();
    test_symbol_table_pop_scope_beyond_global();
    test_symbol_table_pop_scope_offset_propagation();
    test_symbol_table_begin_function_scope();
    test_symbol_table_add_symbol_local_basic();
    test_symbol_table_add_symbol_param();
    test_symbol_table_add_symbol_global();
    test_symbol_table_add_symbol_no_scope();
    test_symbol_table_lookup_current_basic();
    test_symbol_table_lookup_enclosing();
    test_symbol_table_lookup_shadowing();
    test_symbol_table_lookup_token_variations();
    test_symbol_table_lookup_nulls();
    test_symbol_table_get_symbol_offset();
    test_symbol_table_offsets_alignment();
    test_symbol_table_add_symbol_type_clone();
    test_symbol_table_add_symbol_arena_exhaust();
    test_symbol_table_add_many_symbols();
    test_symbol_table_add_symbol_token_dup();
    test_symbol_table_add_complex_types();
    test_symbol_table_print();
}
