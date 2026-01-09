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
bool is_comparison_operator(SnTokenType op);
bool is_arithmetic_operator(SnTokenType op);
bool is_printable_type(Type *type);
bool is_variadic_compatible_type(Type *type);

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

/* Module symbol extraction for namespaced imports.
 * Walks the imported module AST to find all function definitions
 * and extracts their names and types.
 *
 * @param imported_module The module containing imported statements
 * @param table Symbol table for type construction (uses arena)
 * @param symbols_out Output: array of symbol name tokens
 * @param types_out Output: array of function types
 * @param count_out Output: number of extracted symbols
 *
 * Memory is allocated from the arena in the symbol table.
 * If no symbols are found, *count_out is 0 and output arrays are NULL.
 */
void get_module_symbols(Module *imported_module, SymbolTable *table,
                        Token ***symbols_out, Type ***types_out, int *count_out);

/* Native function context tracking.
 * Used to enforce restrictions on pointer types in non-native functions.
 * Native functions can declare pointer variables and work with raw pointers,
 * while regular functions must immediately unwrap pointers with 'as val'.
 */
void native_context_enter(void);
void native_context_exit(void);
bool native_context_is_active(void);

/* 'as val' operand context tracking.
 * Used to allow pointer slices inside 'as val' expressions in non-native functions.
 * ptr[0..len] as val is OK even in regular functions because as val wraps the slice.
 */
void as_val_context_enter(void);
void as_val_context_exit(void);
bool as_val_context_is_active(void);

/* C-compatible type checking.
 * Native callback types can only use C-compatible types in their signatures.
 * C-compatible types are: primitives (int, long, double, float, char, byte, bool, void),
 * interop types (int32, uint32, uint, float), pointers, and opaque types.
 * Non-C-compatible types are: str (Sindarin strings), arrays, function types (closures).
 */
bool is_c_compatible_type(Type *type);

#endif /* TYPE_CHECKER_UTIL_H */
