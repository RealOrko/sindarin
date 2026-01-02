#ifndef TYPE_CHECKER_EXPR_CALL_H
#define TYPE_CHECKER_EXPR_CALL_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/* ============================================================================
 * Call Expression Type Checking
 * ============================================================================
 * Type checking for function calls, method calls, static method calls,
 * and built-in functions. This module handles all call-related type checking
 * extracted from type_checker_expr.c.
 * ============================================================================ */

/* Main call expression type checker
 * Dispatches to appropriate handler based on call type:
 * - Built-in functions (len)
 * - User-defined function calls
 * - Method calls on objects
 * - Static method calls (e.g., TextFile.open)
 */
Type *type_check_call_expression(Expr *expr, SymbolTable *table);

/* Helper to check if callee matches a built-in function name */
bool is_builtin_name(Expr *callee, const char *name);

/* Helper to compare a token's text against a string */
bool token_equals(Token tok, const char *str);

/* Static method type checking for built-in types
 * Handles: TextFile, BinaryFile, Time, Stdin, Stdout, Stderr, Bytes, Path, Directory
 */
Type *type_check_static_method_call(Expr *expr, SymbolTable *table);

/* Type-specific method call type checkers
 * Each returns the result type of the method call, or NULL on error.
 */
Type *type_check_text_file_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);
Type *type_check_binary_file_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);
Type *type_check_time_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);
Type *type_check_date_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);
Type *type_check_array_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);
Type *type_check_string_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_H */
