#ifndef TYPE_CHECKER_EXPR_CALL_FILE_H
#define TYPE_CHECKER_EXPR_CALL_FILE_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * File Method Type Checking
 * ============================================================================
 * Type checking for TextFile and BinaryFile method access (not calls).
 * Returns the function type for the method, or NULL if not a file method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

/* Type check TextFile methods
 * Handles: readChar, readWord, readLine, readAll, readLines, readInto, close,
 *          writeChar, write, writeLine, print, println, hasChars, hasWords,
 *          hasLines, isEof, position, seek, rewind, flush, path, name, size
 */
Type *type_check_text_file_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

/* Type check BinaryFile methods
 * Handles: readByte, readBytes, readAll, readInto, writeByte, writeBytes,
 *          hasBytes, isEof, position, seek, rewind, flush, close, path, name, size
 */
Type *type_check_binary_file_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_CALL_FILE_H */
