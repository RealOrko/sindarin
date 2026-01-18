/**
 * code_gen_expr_call.h - Code generation for call expressions
 *
 * Contains declarations for generating C code from function calls and
 * method calls on arrays, strings, and other object types.
 */

#ifndef CODE_GEN_EXPR_CALL_H
#define CODE_GEN_EXPR_CALL_H

#include "code_gen.h"
#include "ast.h"
#include <stdbool.h>

/* ============================================================================
 * Core Call Expression Code Generation (code_gen_expr_call.c)
 * ============================================================================ */

/**
 * Check if an expression produces a temporary string that needs to be freed.
 * Returns true if the expression creates a new string allocation.
 */
bool expression_produces_temp(Expr *expr);

/**
 * Generate code for call expressions (function calls and method calls).
 * This is the main dispatcher that handles:
 * - Namespace function calls (namespace.function())
 * - Method calls on objects (object.method())
 * - Static method calls (Type.method())
 * - Regular function calls (function())
 */
char *code_gen_call_expression(CodeGen *gen, Expr *expr);

/* ============================================================================
 * Array Method Code Generation (code_gen_expr_call_array.c)
 * ============================================================================ */

/**
 * Generate code for array method calls (push, clear, pop, concat, etc.)
 * Returns generated C code string, or NULL if not an array method.
 */
char *code_gen_array_method_call(CodeGen *gen, Expr *expr, const char *method_name,
                                  Expr *object, Type *element_type, int arg_count,
                                  Expr **arguments);

/* ============================================================================
 * String Method Code Generation (code_gen_expr_call_string.c)
 * ============================================================================ */

/**
 * Generate code for string instance method calls.
 * Handles: substring, regionEquals, indexOf, split, trim, toUpper, toLower,
 *          startsWith, endsWith, contains, replace, charAt, toBytes,
 *          splitWhitespace, splitLines, isBlank
 * Returns generated C code string, or NULL if not a string method.
 */
char *code_gen_string_method_call(CodeGen *gen, const char *method_name,
                                   Expr *object, bool object_is_temp,
                                   int arg_count, Expr **arguments);

/**
 * Generate code for string.length property access.
 * Returns generated C code string.
 */
char *code_gen_string_length(CodeGen *gen, Expr *object);

/* ============================================================================
 * TextFile Method Code Generation (code_gen_expr_call_file.c)
 * ============================================================================ */

/**
 * Generate code for TextFile instance method calls.
 * Handles: readChar, readWord, readLine, readAll, readLines, readInto,
 *          close, writeChar, write, writeLine, print, println,
 *          hasChars, hasWords, hasLines, isEof, position, seek, rewind, flush
 * Returns generated C code string, or NULL if not a TextFile method.
 */
char *code_gen_text_file_method_call(CodeGen *gen, const char *method_name,
                                      Expr *object, int arg_count, Expr **arguments);

/**
 * Generate code for TextFile property access (path, name, size).
 * Returns generated C code string, or NULL if not a TextFile property.
 */
char *code_gen_text_file_property(CodeGen *gen, const char *property_name,
                                   Expr *object);

/**
 * Generate code for TextFile static method calls.
 * Handles: open, exists, readAll, writeAll, delete, copy, move
 * Returns generated C code string, or NULL if not a TextFile static method.
 */
char *code_gen_text_file_static_call(CodeGen *gen, const char *method_name,
                                      int arg_count, Expr **arguments);

/* ============================================================================
 * BinaryFile Method Code Generation (code_gen_expr_call_file.c)
 * ============================================================================ */

/**
 * Generate code for BinaryFile instance method calls.
 * Handles: readByte, readBytes, readAll, readInto, close,
 *          writeByte, writeBytes, hasBytes, isEof,
 *          position, seek, rewind, flush
 * Returns generated C code string, or NULL if not a BinaryFile method.
 */
char *code_gen_binary_file_method_call(CodeGen *gen, const char *method_name,
                                        Expr *object, int arg_count, Expr **arguments);

/**
 * Generate code for BinaryFile property access (path, name, size).
 * Returns generated C code string, or NULL if not a BinaryFile property.
 */
char *code_gen_binary_file_property(CodeGen *gen, const char *property_name,
                                     Expr *object);

/**
 * Generate code for BinaryFile static method calls.
 * Handles: open, exists, readAll, writeAll, delete, copy, move
 * Returns generated C code string, or NULL if not a BinaryFile static method.
 */
char *code_gen_binary_file_static_call(CodeGen *gen, const char *method_name,
                                        int arg_count, Expr **arguments);

#endif /* CODE_GEN_EXPR_CALL_H */
