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
    FunctionModifier func_mod;  /* For function symbols: shared, private, or default */
    bool is_function;           /* True if this is a named function definition */
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
void symbol_table_add_function(SymbolTable *table, Token name, Type *type, FunctionModifier func_mod);
Symbol *symbol_table_lookup_symbol(SymbolTable *table, Token name);
Symbol *symbol_table_lookup_symbol_current(SymbolTable *table, Token name);
int symbol_table_get_symbol_offset(SymbolTable *table, Token name);

/* Arena depth management */
void symbol_table_enter_arena(SymbolTable *table);
void symbol_table_exit_arena(SymbolTable *table);
int symbol_table_get_arena_depth(SymbolTable *table);

#endif