// symbol_table.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "token.h"
#include "ast.h"
#include "arena.h"

#define OFFSET_ALIGNMENT 8
#define CALLEE_SAVED_SPACE 40 
#define LOCAL_BASE_OFFSET (8 + CALLEE_SAVED_SPACE)
#define PARAM_BASE_OFFSET LOCAL_BASE_OFFSET

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct Type Type;

/* Thread state tracking for variables that hold thread handles.
 * Used by the type checker to ensure thread results are properly synchronized. */
typedef enum
{
    THREAD_STATE_NORMAL,       /* Not a thread handle, or already synchronized */
    THREAD_STATE_PENDING,      /* Thread spawned but not yet synchronized */
    THREAD_STATE_SYNCHRONIZED  /* Thread has been synchronized (joined) */
} ThreadState;

/* Frozen state tracking for variables in thread contexts.
 * When a thread is spawned, captured variables are "frozen" to prevent
 * modification while the thread is running. */
typedef struct
{
    int freeze_count;  /* Number of pending threads that have captured this variable */
    bool frozen;       /* True if freeze_count > 0 */
} FrozenState;

typedef enum
{
    SYMBOL_GLOBAL,
    SYMBOL_LOCAL,
    SYMBOL_PARAM
} SymbolKind;

typedef struct Symbol
{
    Token name;
    Type *type;
    SymbolKind kind;
    int offset;
    struct Symbol *next;
    int arena_depth;            /* Which arena depth owns this symbol */
    MemoryQualifier mem_qual;   /* as val, as ref, or default */
    FunctionModifier func_mod;  /* For function symbols: effective modifier (shared for heap-returning) */
    FunctionModifier declared_func_mod;  /* For function symbols: original declared modifier */
    bool is_function;           /* True if this is a named function definition */
    ThreadState thread_state;   /* Thread handle state for synchronization tracking */
    FrozenState frozen_state;   /* Frozen state for thread capture tracking */
    struct Symbol **frozen_args; /* Symbols frozen by this pending thread handle */
    int frozen_args_count;      /* Number of frozen symbols */
} Symbol;

typedef struct Scope
{
    Symbol *symbols;
    struct Scope *enclosing;
    int next_local_offset;
    int next_param_offset;
    int arena_depth;            /* Arena depth level for this scope */
} Scope;

typedef struct {
    Scope *current;
    Scope *global_scope;
    Arena *arena;
    Scope **scopes;
    int scopes_count;
    int scopes_capacity;
    int current_arena_depth;    /* Current arena nesting depth */
} SymbolTable;

int get_type_size(Type *type);

void symbol_table_print(SymbolTable *table, const char *where);

void symbol_table_init(Arena *arena, SymbolTable *table);
void symbol_table_cleanup(SymbolTable *table);

void symbol_table_push_scope(SymbolTable *table);
void symbol_table_pop_scope(SymbolTable *table);
void symbol_table_begin_function_scope(SymbolTable *table);

void symbol_table_add_symbol(SymbolTable *table, Token name, Type *type);
void symbol_table_add_symbol_with_kind(SymbolTable *table, Token name, Type *type, SymbolKind kind);
void symbol_table_add_symbol_full(SymbolTable *table, Token name, Type *type, SymbolKind kind, MemoryQualifier mem_qual);
void symbol_table_add_function(SymbolTable *table, Token name, Type *type, FunctionModifier func_mod, FunctionModifier declared_func_mod);
Symbol *symbol_table_lookup_symbol(SymbolTable *table, Token name);
Symbol *symbol_table_lookup_symbol_current(SymbolTable *table, Token name);
int symbol_table_get_symbol_offset(SymbolTable *table, Token name);

/* Arena depth management */
void symbol_table_enter_arena(SymbolTable *table);
void symbol_table_exit_arena(SymbolTable *table);
int symbol_table_get_arena_depth(SymbolTable *table);

/* Thread state management */
bool symbol_table_mark_pending(Symbol *symbol);
bool symbol_table_mark_synchronized(Symbol *symbol);
bool symbol_table_is_pending(Symbol *symbol);
bool symbol_table_is_synchronized(Symbol *symbol);

/* Frozen state management for thread-captured variables */
bool symbol_table_freeze_symbol(Symbol *symbol);
bool symbol_table_unfreeze_symbol(Symbol *symbol);
bool symbol_table_is_frozen(Symbol *symbol);
int symbol_table_get_freeze_count(Symbol *symbol);
void symbol_table_set_frozen_args(Symbol *symbol, Symbol **frozen_args, int count);

/* Token-based thread state queries.
 * These convenience functions look up a variable by name and return its thread state.
 * They are useful when you have a Token (e.g., from an AST variable node) and need
 * to check thread state without first looking up the Symbol manually. */

/* Returns the thread state for a variable, or THREAD_STATE_NORMAL if not found */
ThreadState symbol_table_get_thread_state(SymbolTable *table, Token name);

/* Returns true if the variable is in THREAD_STATE_PENDING, false if not found or other state */
bool symbol_table_is_variable_pending(SymbolTable *table, Token name);

/* Returns true if the variable is frozen (captured by a pending thread), false if not found */
bool symbol_table_is_variable_frozen(SymbolTable *table, Token name);

/* Thread synchronization for ! operator.
 * Transitions a pending thread handle to synchronized state and unfreezes
 * any captured argument symbols.
 *
 * Parameters:
 *   table - The symbol table
 *   name - Token identifying the thread handle variable
 *   frozen_args - Array of Symbol pointers to unfreeze (captured arguments), or NULL
 *   frozen_count - Number of symbols in frozen_args array
 *
 * Returns:
 *   true if sync succeeded (was pending, now synchronized)
 *   false if variable not found, not pending, or already synchronized
 */
bool symbol_table_sync_variable(SymbolTable *table, Token name,
                                Symbol **frozen_args, int frozen_count);

#endif