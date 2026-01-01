#ifndef TYPE_CHECKER_UTIL_H
#define TYPE_CHECKER_UTIL_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/* Error state management */
void type_checker_reset_error(void);
int type_checker_had_error(void);
void type_checker_set_error(void);

/* Error reporting */
const char *type_name(Type *type);
void type_error(Token *token, const char *msg);
void type_error_with_suggestion(Token *token, const char *msg, const char *suggestion);
void type_mismatch_error(Token *token, Type *expected, Type *actual, const char *context);

/* Enhanced error reporting with suggestions */
void undefined_variable_error(Token *token, SymbolTable *table);
void undefined_variable_error_for_assign(Token *token, SymbolTable *table);
void invalid_member_error(Token *token, Type *object_type, const char *member_name);
void argument_count_error(Token *token, const char *func_name, int expected, int actual);
void argument_type_error(Token *token, const char *func_name, int arg_index, Type *expected, Type *actual);

/* String similarity helpers */
int levenshtein_distance(const char *s1, int len1, const char *s2, int len2);
const char *find_similar_symbol(SymbolTable *table, const char *name, int name_len);
const char *find_similar_method(Type *type, const char *method_name);

/* Type predicates */
bool is_numeric_type(Type *type);
bool is_comparison_operator(TokenType op);
bool is_arithmetic_operator(TokenType op);
bool is_printable_type(Type *type);

/* Type promotion for numeric operations (int -> double) */
bool can_promote_numeric(Type *from, Type *to);
Type *get_promoted_type(Arena *arena, Type *left, Type *right);

/* Memory management type predicates */
bool is_primitive_type(Type *type);
bool is_reference_type(Type *type);
bool can_escape_private(Type *type);

/* Memory context for tracking private blocks/functions */
typedef struct MemoryContext {
    bool in_private_block;
    bool in_private_function;
    int private_depth;           /* Nesting depth of private blocks */
} MemoryContext;

/* Memory context management */
void memory_context_init(MemoryContext *ctx);
void memory_context_enter_private(MemoryContext *ctx);
void memory_context_exit_private(MemoryContext *ctx);
bool memory_context_is_private(MemoryContext *ctx);

#endif /* TYPE_CHECKER_UTIL_H */
