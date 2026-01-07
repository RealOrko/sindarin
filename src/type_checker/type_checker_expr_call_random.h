#ifndef TYPE_CHECKER_EXPR_CALL_RANDOM_H
#define TYPE_CHECKER_EXPR_CALL_RANDOM_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Random/UUID/Process Type Method Type Checking
 * ============================================================================
 * Type checking for Random, UUID, and Process method access.
 * Returns the function type for the method, or NULL if not a valid method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

/* Type check Process methods
 * Handles: exitCode, stdout, stderr
 */
Type *type_check_process_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check Random methods
 * Handles: int, long, double, bool, byte, bytes, gaussian,
 *          intMany, longMany, doubleMany, boolMany, gaussianMany
 */
Type *type_check_random_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check UUID methods
 * Handles: toString, toHex, toBase64, toBytes, version, variant,
 *          isNil, timestamp, time, equals
 */
Type *type_check_uuid_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_RANDOM_H */
