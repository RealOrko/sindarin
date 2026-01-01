#ifndef CODE_GEN_EXPR_STATIC_H
#define CODE_GEN_EXPR_STATIC_H

#include "code_gen.h"

/* Static call expression code generation for built-in types:
 * - TextFile static methods (open, exists, readAll, writeAll, delete, copy, move)
 * - BinaryFile static methods (open, exists, readAll, writeAll, delete, copy, move)
 * - Stdin static methods (readLine, readChar, readWord, hasChars, hasLines, isEof)
 * - Stdout static methods (write, writeLine, flush)
 * - Stderr static methods (write, writeLine, flush)
 * - Bytes static methods (fromHex, fromBase64)
 * - Path static methods (directory, filename, extension, join, absolute, exists, isFile, isDirectory)
 * - Directory static methods (list, listRecursive, create, delete, deleteRecursive)
 * - Time static methods (now, utc, fromMillis, fromSeconds, sleep)
 */
char *code_gen_static_call_expression(CodeGen *gen, Expr *expr);

#endif /* CODE_GEN_EXPR_STATIC_H */
