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
void type_mismatch_error(Token *token, Type *expected, Type *actual, const char *context);

/* Type predicates */
bool is_numeric_type(Type *type);
bool is_comparison_operator(TokenType op);
bool is_arithmetic_operator(TokenType op);
bool is_printable_type(Type *type);

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
