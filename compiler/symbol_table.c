#include "symbol_table.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int get_type_size(Type *type)
{
    DEBUG_VERBOSE("Entering get_type_size with type kind: %d", type->kind);
    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        DEBUG_VERBOSE("Returning size 8 for TYPE_INT or TYPE_LONG");
        return 8;
    case TYPE_DOUBLE:
        DEBUG_VERBOSE("Returning size 8 for TYPE_DOUBLE");
        return 8;
    case TYPE_CHAR:
        DEBUG_VERBOSE("Returning size 1 for TYPE_CHAR");
        return 1;
    case TYPE_BOOL:
        DEBUG_VERBOSE("Returning size 1 for TYPE_BOOL");
        return 1;
    case TYPE_STRING:
        DEBUG_VERBOSE("Returning size 8 for TYPE_STRING");
        return 8;
    default:
        DEBUG_VERBOSE("Returning default size 8 for unknown type kind: %d", type->kind);
        return 8;
    }
}

static char *token_to_string(Token token)
{
    DEBUG_VERBOSE("Converting token to string, token start: %p, length: %d", (void *)token.start, token.length);
    static char buf[256];
    int len = token.length < 255 ? token.length : 255;
    strncpy(buf, token.start, len);
    buf[len] = '\0';
    DEBUG_VERBOSE("Token converted to string: '%s'", buf);
    return buf;
}

void symbol_table_print(SymbolTable *table, const char *where)
{
    DEBUG_VERBOSE("==== SYMBOL TABLE DUMP (%s) ====", where);

    if (!table || !table->current)
    {
        DEBUG_VERBOSE("  [Empty symbol table or no current scope]");
        return;
    }

    int scope_level = 0;
    Scope *scope = table->current;

    while (scope)
    {
        DEBUG_VERBOSE("  Scope Level %d:", scope_level);
        DEBUG_VERBOSE("    next_local_offset: %d, next_param_offset: %d",
                      scope->next_local_offset, scope->next_param_offset);

        Symbol *symbol = scope->symbols;
        if (!symbol)
        {
            DEBUG_VERBOSE("    [No symbols in this scope]");
        }

        while (symbol)
        {
            DEBUG_VERBOSE("    Symbol: '%s', Type: %s, Kind: %d, Offset: %d",
                          token_to_string(symbol->name),
                          ast_type_to_string(table->arena, symbol->type),
                          symbol->kind,
                          symbol->offset);
            symbol = symbol->next;
        }

        scope = scope->enclosing;
        scope_level++;
    }

    DEBUG_VERBOSE("====================================");
}

void symbol_table_init(Arena *arena, SymbolTable *table)
{
    DEBUG_VERBOSE("Initializing symbol table with arena: %p, table: %p", (void *)arena, (void *)table);
    if (arena == NULL)
    {
        DEBUG_ERROR("NULL arena in symbol_table_init");
        return;
    }
    if (table == NULL)
    {
        DEBUG_ERROR("NULL table in symbol_table_init");
        return;
    }
    table->arena = arena;
    table->scopes = arena_alloc(arena, sizeof(Scope *) * 8);
    if (table->scopes == NULL)
    {
        DEBUG_ERROR("Out of memory creating scopes array");
    }
    table->scopes_count = 0;
    table->scopes_capacity = 8;
    table->current = NULL;
    table->current_arena_depth = 0;

    DEBUG_VERBOSE("Calling symbol_table_push_scope for initial scope");
    symbol_table_push_scope(table);
    table->global_scope = table->current;
    DEBUG_VERBOSE("Symbol table initialized, global_scope: %p", (void *)table->global_scope);
}

void free_scope(Scope *scope)
{
    DEBUG_VERBOSE("Freeing scope: %p", (void *)scope);
    if (scope == NULL)
    {
        DEBUG_VERBOSE("Scope is NULL, returning");
        return;
    }

    Symbol *symbol = scope->symbols;
    while (symbol != NULL)
    {
        DEBUG_VERBOSE("Processing symbol: %p", (void *)symbol);
        Symbol *next = symbol->next;
        symbol = next;
    }
    DEBUG_VERBOSE("Finished freeing scope: %p", (void *)scope);
}

void symbol_table_cleanup(SymbolTable *table)
{
    DEBUG_VERBOSE("Cleaning up symbol table: %p", (void *)table);
    if (table == NULL)
    {
        DEBUG_VERBOSE("Table is NULL, returning");
        return;
    }
    for (int i = 0; i < table->scopes_count; i++)
    {
        DEBUG_VERBOSE("Freeing scope %d: %p", i, (void *)table->scopes[i]);
        free_scope(table->scopes[i]);
    }
    DEBUG_VERBOSE("Finished cleaning up symbol table");
}

void symbol_table_push_scope(SymbolTable *table)
{
    DEBUG_VERBOSE("Pushing new scope for table: %p", (void *)table);
    Scope *scope = arena_alloc(table->arena, sizeof(Scope));
    if (scope == NULL)
    {
        DEBUG_ERROR("Out of memory creating scope");
        return;
    }

    scope->symbols = NULL;
    Scope *enclosing = table->current;
    scope->enclosing = enclosing;
    scope->next_local_offset = enclosing ? enclosing->next_local_offset : LOCAL_BASE_OFFSET;
    scope->next_param_offset = enclosing ? enclosing->next_param_offset : PARAM_BASE_OFFSET;
    scope->arena_depth = table->current_arena_depth;
    table->current = scope;
    DEBUG_VERBOSE("New scope created: %p, enclosing: %p, local_offset: %d, param_offset: %d, arena_depth: %d",
                  (void *)scope, (void *)enclosing, scope->next_local_offset, scope->next_param_offset, scope->arena_depth);

    if (table->scopes_count >= table->scopes_capacity)
    {
        DEBUG_VERBOSE("Expanding scopes array, current capacity: %d", table->scopes_capacity);
        int new_capacity = table->scopes_capacity * 2;
        Scope **new_scopes = arena_alloc(table->arena, sizeof(Scope *) * new_capacity);
        if (new_scopes == NULL)
        {
            DEBUG_ERROR("Out of memory expanding scopes array");
            return;
        }
        memcpy(new_scopes, table->scopes, sizeof(Scope *) * table->scopes_count);
        table->scopes = new_scopes;
        table->scopes_capacity = new_capacity;
        DEBUG_VERBOSE("Scopes array expanded to new capacity: %d", new_capacity);
    }
    table->scopes[table->scopes_count++] = scope;
    DEBUG_VERBOSE("Scope added to table, scopes_count: %d", table->scopes_count);
}

void symbol_table_begin_function_scope(SymbolTable *table)
{
    DEBUG_VERBOSE("Beginning function scope for table: %p", (void *)table);
    symbol_table_push_scope(table);
    table->current->next_local_offset = LOCAL_BASE_OFFSET;
    table->current->next_param_offset = PARAM_BASE_OFFSET;
    DEBUG_VERBOSE("Function scope set, local_offset: %d, param_offset: %d",
                  table->current->next_local_offset, table->current->next_param_offset);
}

void symbol_table_pop_scope(SymbolTable *table)
{
    DEBUG_VERBOSE("Popping scope from table: %p", (void *)table);
    if (table->current == NULL || table->current == table->global_scope)
    {
        DEBUG_VERBOSE("Current scope is NULL or global scope, returning");
        return;
    }
    Scope *to_free = table->current;
    table->current = to_free->enclosing;
    if (table->current != NULL)
    {
        table->current->next_local_offset = MAX(table->current->next_local_offset, to_free->next_local_offset);
        table->current->next_param_offset = MAX(table->current->next_param_offset, to_free->next_param_offset);
        DEBUG_VERBOSE("Updated enclosing scope offsets, local_offset: %d, param_offset: %d",
                      table->current->next_local_offset, table->current->next_param_offset);
    }
    DEBUG_VERBOSE("Scope popped, new current scope: %p", (void *)table->current);
}

static int tokens_equal(Token a, Token b)
{
    DEBUG_VERBOSE("Comparing tokens, a: %p (length: %d), b: %p (length: %d)",
                  (void *)a.start, a.length, (void *)b.start, b.length);
    if (a.length != b.length)
    {
        char a_str[256], b_str[256];
        int a_len = a.length < 255 ? a.length : 255;
        int b_len = b.length < 255 ? b.length : 255;

        strncpy(a_str, a.start, a_len);
        a_str[a_len] = '\0';
        strncpy(b_str, b.start, b_len);
        b_str[b_len] = '\0';

        DEBUG_VERBOSE("Token length mismatch: '%s'(%d) vs '%s'(%d)",
                      a_str, a.length, b_str, b.length);
        return 0;
    }

    if (a.start == b.start)
    {
        DEBUG_VERBOSE("Token address match at %p", (void *)a.start);
        return 1;
    }

    int result = memcmp(a.start, b.start, a.length);

    char a_str[256], b_str[256];
    int a_len = a.length < 255 ? a.length : 255;
    int b_len = b.length < 255 ? b.length : 255;

    strncpy(a_str, a.start, a_len);
    a_str[a_len] = '\0';
    strncpy(b_str, b.start, b_len);
    b_str[b_len] = '\0';

    if (result == 0)
    {
        DEBUG_VERBOSE("Token content match: '%s' == '%s'", a_str, b_str);
        return 1;
    }
    else
    {
        DEBUG_VERBOSE("Token content mismatch: '%s' != '%s'", a_str, b_str);
        return 0;
    }
}

void symbol_table_add_symbol_with_kind(SymbolTable *table, Token name, Type *type, SymbolKind kind)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding symbol with kind: %d, name: '%s', type: %p", kind, name_str, (void *)type);

    if (table->current == NULL)
    {
        DEBUG_ERROR("No active scope when adding symbol");
        return;
    }

    Symbol *existing = symbol_table_lookup_symbol_current(table, name);
    if (existing != NULL)
    {
        DEBUG_VERBOSE("Existing symbol found: '%s', updating type", name_str);
        existing->type = ast_clone_type(table->arena, type);
        return;
    }

    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating symbol");
        return;
    }

    symbol->name = name;
    symbol->type = ast_clone_type(table->arena, type);
    symbol->kind = kind;

    if (kind == SYMBOL_PARAM)
    {
        symbol->offset = -table->current->next_param_offset;
        int type_size = get_type_size(type);
        int aligned_size = ((type_size + OFFSET_ALIGNMENT - 1) / OFFSET_ALIGNMENT) * OFFSET_ALIGNMENT;
        table->current->next_param_offset += aligned_size;
        DEBUG_VERBOSE("Added parameter symbol: '%s', offset: %d, aligned_size: %d, new next_param_offset: %d",
                      name_str, symbol->offset, aligned_size, table->current->next_param_offset);
    }
    else if (kind == SYMBOL_LOCAL)
    {
        symbol->offset = -table->current->next_local_offset;
        int type_size = get_type_size(type);
        int aligned_size = ((type_size + OFFSET_ALIGNMENT - 1) / OFFSET_ALIGNMENT) * OFFSET_ALIGNMENT;
        table->current->next_local_offset += aligned_size;
        DEBUG_VERBOSE("Added local symbol: '%s', offset: %d, aligned_size: %d, new next_local_offset: %d",
                      name_str, symbol->offset, aligned_size, table->current->next_local_offset);
    }
    else
    {
        symbol->offset = 0;
        DEBUG_VERBOSE("Added global symbol: '%s', offset: 0", name_str);
    }

    symbol->name.start = arena_strndup(table->arena, name.start, name.length);
    if (symbol->name.start == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating token string");
        return;
    }
    symbol->name.length = name.length;
    symbol->name.line = name.line;
    symbol->name.type = name.type;
    symbol->arena_depth = table->current_arena_depth;
    symbol->mem_qual = MEM_DEFAULT;
    symbol->func_mod = FUNC_DEFAULT;
    symbol->is_function = false;
    DEBUG_VERBOSE("Symbol name duplicated: '%s', length: %d, line: %d, arena_depth: %d",
                  name_str, symbol->name.length, symbol->name.line, symbol->arena_depth);

    symbol->next = table->current->symbols;
    table->current->symbols = symbol;
    DEBUG_VERBOSE("Symbol added to current scope, new symbol: %p", (void *)symbol);
}

void symbol_table_add_symbol(SymbolTable *table, Token name, Type *type)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding symbol (default kind SYMBOL_LOCAL): '%s', type: %p", name_str, (void *)type);
    symbol_table_add_symbol_with_kind(table, name, type, SYMBOL_LOCAL);
}

Symbol *symbol_table_lookup_symbol_current(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Looking up symbol in current scope: '%s'", name_str);

    if (table->current == NULL)
    {
        DEBUG_VERBOSE("Current scope is NULL, returning NULL");
        return NULL;
    }

    Symbol *symbol = table->current->symbols;
    while (symbol != NULL)
    {
        if (tokens_equal(symbol->name, name))
        {
            char sym_name[256];
            int sym_len = symbol->name.length < 255 ? symbol->name.length : 255;
            strncpy(sym_name, symbol->name.start, sym_len);
            sym_name[sym_len] = '\0';
            DEBUG_VERBOSE("Found symbol in current scope: '%s'", sym_name);
            return symbol;
        }
        symbol = symbol->next;
    }

    DEBUG_VERBOSE("Symbol '%s' not found in current scope", name_str);
    return NULL;
}

Symbol *symbol_table_lookup_symbol(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Looking up symbol '%s' at address %p, length %d",
                  name_str, (void *)name.start, name.length);

    if (!table || !table->current)
    {
        DEBUG_VERBOSE("Null table or current scope in lookup_symbol");
        return NULL;
    }

    Scope *scope = table->current;
    int scope_level = 0;

    while (scope != NULL)
    {
        DEBUG_VERBOSE("  Checking scope level %d", scope_level);

        Symbol *symbol = scope->symbols;
        while (symbol != NULL)
        {
            char sym_name[256];
            int sym_len = symbol->name.length < 255 ? symbol->name.length : 255;
            strncpy(sym_name, symbol->name.start, sym_len);
            sym_name[sym_len] = '\0';

            DEBUG_VERBOSE("    Symbol '%s' at address %p, length %d",
                          sym_name, (void *)symbol->name.start, symbol->name.length);

            if (symbol->name.start == name.start && symbol->name.length == name.length)
            {
                DEBUG_VERBOSE("Found symbol '%s' in scope level %d (direct pointer match)",
                              sym_name, scope_level);
                return symbol;
            }

            if (symbol->name.length == name.length &&
                memcmp(symbol->name.start, name.start, name.length) == 0)
            {
                DEBUG_VERBOSE("Found symbol '%s' in scope level %d (content match)",
                              sym_name, scope_level);
                return symbol;
            }

            if (strcmp(sym_name, name_str) == 0)
            {
                DEBUG_VERBOSE("Found symbol '%s' by string comparison in scope level %d",
                              sym_name, scope_level);
                return symbol;
            }

            symbol = symbol->next;
        }

        scope = scope->enclosing;
        scope_level++;
    }

    DEBUG_VERBOSE("Symbol '%s' not found in any scope", name_str);
    return NULL;
}

int symbol_table_get_symbol_offset(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Getting offset for symbol: '%s'", name_str);

    Symbol *symbol = symbol_table_lookup_symbol(table, name);
    if (symbol == NULL)
    {
        DEBUG_ERROR("Symbol not found in get_symbol_offset: '%s'", name_str);
        return -1;
    }

    DEBUG_VERBOSE("Found symbol '%s', returning offset: %d", name_str, symbol->offset);
    return symbol->offset;
}

void symbol_table_add_symbol_full(SymbolTable *table, Token name, Type *type, SymbolKind kind, MemoryQualifier mem_qual)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding symbol with full info: '%s', kind: %d, mem_qual: %d", name_str, kind, mem_qual);

    /* First add the symbol using the standard function */
    symbol_table_add_symbol_with_kind(table, name, type, kind);

    /* Then update the memory qualifier on the newly added symbol */
    Symbol *symbol = symbol_table_lookup_symbol_current(table, name);
    if (symbol != NULL)
    {
        symbol->mem_qual = mem_qual;
        DEBUG_VERBOSE("Updated symbol '%s' mem_qual to: %d", name_str, mem_qual);
    }
}

void symbol_table_add_function(SymbolTable *table, Token name, Type *type, FunctionModifier func_mod)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding function symbol: '%s', func_mod: %d", name_str, func_mod);

    /* First add the symbol using the standard function */
    symbol_table_add_symbol_with_kind(table, name, type, SYMBOL_LOCAL);

    /* Then update the function modifier on the newly added symbol */
    Symbol *symbol = symbol_table_lookup_symbol_current(table, name);
    if (symbol != NULL)
    {
        symbol->func_mod = func_mod;
        symbol->is_function = true;
        DEBUG_VERBOSE("Updated function symbol '%s' func_mod to: %d, is_function: true", name_str, func_mod);
    }
}

void symbol_table_enter_arena(SymbolTable *table)
{
    table->current_arena_depth++;
    DEBUG_VERBOSE("Entered arena, new depth: %d", table->current_arena_depth);
}

void symbol_table_exit_arena(SymbolTable *table)
{
    if (table->current_arena_depth > 0)
    {
        table->current_arena_depth--;
    }
    DEBUG_VERBOSE("Exited arena, new depth: %d", table->current_arena_depth);
}

int symbol_table_get_arena_depth(SymbolTable *table)
{
    return table->current_arena_depth;
}