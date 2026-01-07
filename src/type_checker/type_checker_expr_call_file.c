/* ============================================================================
 * type_checker_expr_call_file.c - File Method Type Checking
 * ============================================================================
 * Type checking for TextFile and BinaryFile method access (not calls).
 * Returns the function type for the method, or NULL if not a file method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

#include "type_checker/type_checker_expr_call_file.h"
#include "type_checker/type_checker_expr_call.h"
#include "debug.h"

/* ============================================================================
 * TextFile Method Type Checking
 * ============================================================================ */

Type *type_check_text_file_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle TextFile types */
    if (object_type->kind != TYPE_TEXT_FILE)
    {
        return NULL;
    }

    /* TextFile instance reading methods */

    /* file.readChar() -> int */
    if (token_equals(member_name, "readChar"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readChar method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* file.readWord() -> str */
    if (token_equals(member_name, "readWord"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readWord method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* file.readLine() -> str */
    if (token_equals(member_name, "readLine"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readLine method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* file.readAll() -> str */
    if (token_equals(member_name, "readAll"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readAll method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* file.readLines() -> str[] */
    if (token_equals(member_name, "readLines"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile readLines method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 0);
    }

    /* file.readInto(buffer) -> int */
    if (token_equals(member_name, "readInto"))
    {
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *char_array_type = ast_create_array_type(table->arena, char_type);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {char_array_type};
        DEBUG_VERBOSE("Returning function type for TextFile readInto method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* file.close() -> void */
    if (token_equals(member_name, "close"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile close method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* TextFile instance writing methods */

    /* file.writeChar(c) -> void */
    if (token_equals(member_name, "writeChar"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[1] = {char_type};
        DEBUG_VERBOSE("Returning function type for TextFile writeChar method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* file.write(s) -> void */
    if (token_equals(member_name, "write"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TextFile write method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* file.writeLine(s) -> void */
    if (token_equals(member_name, "writeLine"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TextFile writeLine method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* file.print(s) -> void */
    if (token_equals(member_name, "print"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TextFile print method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* file.println(s) -> void */
    if (token_equals(member_name, "println"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for TextFile println method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* TextFile state query methods */

    /* file.hasChars() -> bool */
    if (token_equals(member_name, "hasChars"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile hasChars method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* file.hasWords() -> bool */
    if (token_equals(member_name, "hasWords"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile hasWords method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* file.hasLines() -> bool */
    if (token_equals(member_name, "hasLines"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile hasLines method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* file.isEof() -> bool */
    if (token_equals(member_name, "isEof"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile isEof method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* TextFile position manipulation methods */

    /* file.position() -> int */
    if (token_equals(member_name, "position"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile position method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* file.seek(pos) -> void */
    if (token_equals(member_name, "seek"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for TextFile seek method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* file.rewind() -> void */
    if (token_equals(member_name, "rewind"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile rewind method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* file.flush() -> void */
    if (token_equals(member_name, "flush"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for TextFile flush method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* TextFile properties (accessed as member, return value directly) */

    /* file.path -> str */
    if (token_equals(member_name, "path"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for TextFile path property");
        return string_type;
    }

    /* file.name -> str */
    if (token_equals(member_name, "name"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for TextFile name property");
        return string_type;
    }

    /* file.size -> int */
    if (token_equals(member_name, "size"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning int type for TextFile size property");
        return int_type;
    }

    /* Not a TextFile method */
    return NULL;
}

/* ============================================================================
 * BinaryFile Method Type Checking
 * ============================================================================ */

Type *type_check_binary_file_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle BinaryFile types */
    if (object_type->kind != TYPE_BINARY_FILE)
    {
        return NULL;
    }

    /* BinaryFile instance reading methods */

    /* file.readByte() -> int */
    if (token_equals(member_name, "readByte"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile readByte method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* file.readBytes(count) -> byte[] */
    if (token_equals(member_name, "readBytes"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile readBytes method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 1);
    }

    /* file.readAll() -> byte[] */
    if (token_equals(member_name, "readAll"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile readAll method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 0);
    }

    /* file.readInto(buffer) -> int */
    if (token_equals(member_name, "readInto"))
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {byte_array_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile readInto method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* BinaryFile instance writing methods */

    /* file.writeByte(b) -> void */
    if (token_equals(member_name, "writeByte"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile writeByte method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* file.writeBytes(bytes) -> void */
    if (token_equals(member_name, "writeBytes"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[1] = {byte_array_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile writeBytes method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* BinaryFile state query methods */

    /* file.hasBytes() -> bool */
    if (token_equals(member_name, "hasBytes"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile hasBytes method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* file.isEof() -> bool */
    if (token_equals(member_name, "isEof"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile isEof method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* BinaryFile position manipulation methods */

    /* file.position() -> int */
    if (token_equals(member_name, "position"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile position method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* file.seek(pos) -> void */
    if (token_equals(member_name, "seek"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for BinaryFile seek method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* file.rewind() -> void */
    if (token_equals(member_name, "rewind"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile rewind method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* file.flush() -> void */
    if (token_equals(member_name, "flush"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile flush method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* file.close() -> void */
    if (token_equals(member_name, "close"))
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for BinaryFile close method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* BinaryFile properties (accessed as member, return value directly) */

    /* file.path -> str */
    if (token_equals(member_name, "path"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for BinaryFile path property");
        return string_type;
    }

    /* file.name -> str */
    if (token_equals(member_name, "name"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for BinaryFile name property");
        return string_type;
    }

    /* file.size -> int */
    if (token_equals(member_name, "size"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning int type for BinaryFile size property");
        return int_type;
    }

    /* Not a BinaryFile method */
    return NULL;
}
