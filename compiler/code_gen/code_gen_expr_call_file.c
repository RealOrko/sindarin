/**
 * code_gen_expr_call_file.c - Code generation for TextFile and BinaryFile method calls
 *
 * Contains implementations for generating C code from method calls on
 * TextFile and BinaryFile types, including instance methods, properties,
 * and static methods.
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * TextFile Instance Methods
 * ============================================================================ */

/* Generate code for file.readChar() - returns long (-1 on EOF) */
static char *code_gen_text_file_read_char(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_read_char(%s)", object_str);
}

/* Generate code for file.readWord() - returns string */
static char *code_gen_text_file_read_word(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_read_word(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for file.readLine() - returns string */
static char *code_gen_text_file_read_line(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_read_line(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for file.readAll() - returns string (instance method) */
static char *code_gen_text_file_instance_read_all(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_instance_read_all(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for file.readLines() - returns string[] */
static char *code_gen_text_file_read_lines(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_read_lines(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for file.readInto(buffer) - reads into existing buffer */
static char *code_gen_text_file_read_into(CodeGen *gen, const char *object_str,
                                           const char *buffer_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_read_into(%s, %s)",
                         object_str, buffer_str);
}

/* Generate code for file.close() */
static char *code_gen_text_file_close(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_close(%s)", object_str);
}

/* Generate code for file.writeChar(ch) */
static char *code_gen_text_file_write_char(CodeGen *gen, const char *object_str,
                                            const char *ch_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_write_char(%s, %s)",
                         object_str, ch_str);
}

/* Generate code for file.write(text) */
static char *code_gen_text_file_write(CodeGen *gen, const char *object_str,
                                       const char *text_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_write(%s, %s)",
                         object_str, text_str);
}

/* Generate code for file.writeLine(text) */
static char *code_gen_text_file_write_line(CodeGen *gen, const char *object_str,
                                            const char *text_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_write_line(%s, %s)",
                         object_str, text_str);
}

/* Generate code for file.print(text) */
static char *code_gen_text_file_print(CodeGen *gen, const char *object_str,
                                       const char *text_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_print(%s, %s)",
                         object_str, text_str);
}

/* Generate code for file.println(text) */
static char *code_gen_text_file_println(CodeGen *gen, const char *object_str,
                                         const char *text_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_println(%s, %s)",
                         object_str, text_str);
}

/* Generate code for file.hasChars() */
static char *code_gen_text_file_has_chars(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_has_chars(%s)", object_str);
}

/* Generate code for file.hasWords() */
static char *code_gen_text_file_has_words(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_has_words(%s)", object_str);
}

/* Generate code for file.hasLines() */
static char *code_gen_text_file_has_lines(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_has_lines(%s)", object_str);
}

/* Generate code for file.isEof() */
static char *code_gen_text_file_is_eof(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_is_eof(%s)", object_str);
}

/* Generate code for file.position() */
static char *code_gen_text_file_position(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_position(%s)", object_str);
}

/* Generate code for file.seek(pos) */
static char *code_gen_text_file_seek(CodeGen *gen, const char *object_str,
                                      const char *pos_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_seek(%s, %s)",
                         object_str, pos_str);
}

/* Generate code for file.rewind() */
static char *code_gen_text_file_rewind(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_rewind(%s)", object_str);
}

/* Generate code for file.flush() */
static char *code_gen_text_file_flush(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_text_file_flush(%s)", object_str);
}

/* ============================================================================
 * BinaryFile Instance Methods
 * ============================================================================ */

/* Generate code for file.readByte() - returns long (-1 on EOF) */
static char *code_gen_binary_file_read_byte(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_read_byte(%s)", object_str);
}

/* Generate code for file.readBytes(count) - returns byte[] */
static char *code_gen_binary_file_read_bytes(CodeGen *gen, const char *object_str,
                                              const char *count_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_read_bytes(%s, %s, %s)",
                         ARENA_VAR(gen), object_str, count_str);
}

/* Generate code for file.readAll() - returns byte[] (instance method) */
static char *code_gen_binary_file_instance_read_all(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_instance_read_all(%s, %s)",
                         ARENA_VAR(gen), object_str);
}

/* Generate code for file.readInto(buffer) - reads into existing buffer */
static char *code_gen_binary_file_read_into(CodeGen *gen, const char *object_str,
                                             const char *buffer_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_read_into(%s, %s)",
                         object_str, buffer_str);
}

/* Generate code for file.close() */
static char *code_gen_binary_file_close(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_close(%s)", object_str);
}

/* Generate code for file.writeByte(b) */
static char *code_gen_binary_file_write_byte(CodeGen *gen, const char *object_str,
                                              const char *byte_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_write_byte(%s, %s)",
                         object_str, byte_str);
}

/* Generate code for file.writeBytes(data) */
static char *code_gen_binary_file_write_bytes(CodeGen *gen, const char *object_str,
                                               const char *data_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_write_bytes(%s, %s)",
                         object_str, data_str);
}

/* Generate code for file.hasBytes() */
static char *code_gen_binary_file_has_bytes(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_has_bytes(%s)", object_str);
}

/* Generate code for file.isEof() */
static char *code_gen_binary_file_is_eof(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_is_eof(%s)", object_str);
}

/* Generate code for file.position() */
static char *code_gen_binary_file_position(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_position(%s)", object_str);
}

/* Generate code for file.seek(pos) */
static char *code_gen_binary_file_seek(CodeGen *gen, const char *object_str,
                                        const char *pos_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_seek(%s, %s)",
                         object_str, pos_str);
}

/* Generate code for file.rewind() */
static char *code_gen_binary_file_rewind(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_rewind(%s)", object_str);
}

/* Generate code for file.flush() */
static char *code_gen_binary_file_flush(CodeGen *gen, const char *object_str)
{
    return arena_sprintf(gen->arena, "rt_binary_file_flush(%s)", object_str);
}

/* ============================================================================
 * Main Dispatcher Functions
 * ============================================================================ */

/* Dispatch TextFile instance method calls */
char *code_gen_text_file_method_call(CodeGen *gen, const char *method_name,
                                      Expr *object, int arg_count, Expr **arguments)
{
    char *object_str = code_gen_expression(gen, object);

    /* Reading methods */
    if (strcmp(method_name, "readChar") == 0 && arg_count == 0) {
        return code_gen_text_file_read_char(gen, object_str);
    }
    if (strcmp(method_name, "readWord") == 0 && arg_count == 0) {
        return code_gen_text_file_read_word(gen, object_str);
    }
    if (strcmp(method_name, "readLine") == 0 && arg_count == 0) {
        return code_gen_text_file_read_line(gen, object_str);
    }
    if (strcmp(method_name, "readAll") == 0 && arg_count == 0) {
        return code_gen_text_file_instance_read_all(gen, object_str);
    }
    if (strcmp(method_name, "readLines") == 0 && arg_count == 0) {
        return code_gen_text_file_read_lines(gen, object_str);
    }
    if (strcmp(method_name, "readInto") == 0 && arg_count == 1) {
        char *buffer_str = code_gen_expression(gen, arguments[0]);
        return code_gen_text_file_read_into(gen, object_str, buffer_str);
    }

    /* Writing methods */
    if (strcmp(method_name, "writeChar") == 0 && arg_count == 1) {
        char *ch_str = code_gen_expression(gen, arguments[0]);
        return code_gen_text_file_write_char(gen, object_str, ch_str);
    }
    if (strcmp(method_name, "write") == 0 && arg_count == 1) {
        char *text_str = code_gen_expression(gen, arguments[0]);
        return code_gen_text_file_write(gen, object_str, text_str);
    }
    if (strcmp(method_name, "writeLine") == 0 && arg_count == 1) {
        char *text_str = code_gen_expression(gen, arguments[0]);
        return code_gen_text_file_write_line(gen, object_str, text_str);
    }
    if (strcmp(method_name, "print") == 0 && arg_count == 1) {
        char *text_str = code_gen_expression(gen, arguments[0]);
        return code_gen_text_file_print(gen, object_str, text_str);
    }
    if (strcmp(method_name, "println") == 0 && arg_count == 1) {
        char *text_str = code_gen_expression(gen, arguments[0]);
        return code_gen_text_file_println(gen, object_str, text_str);
    }

    /* State query methods */
    if (strcmp(method_name, "hasChars") == 0 && arg_count == 0) {
        return code_gen_text_file_has_chars(gen, object_str);
    }
    if (strcmp(method_name, "hasWords") == 0 && arg_count == 0) {
        return code_gen_text_file_has_words(gen, object_str);
    }
    if (strcmp(method_name, "hasLines") == 0 && arg_count == 0) {
        return code_gen_text_file_has_lines(gen, object_str);
    }
    if (strcmp(method_name, "isEof") == 0 && arg_count == 0) {
        return code_gen_text_file_is_eof(gen, object_str);
    }

    /* Position and control methods */
    if (strcmp(method_name, "position") == 0 && arg_count == 0) {
        return code_gen_text_file_position(gen, object_str);
    }
    if (strcmp(method_name, "seek") == 0 && arg_count == 1) {
        char *pos_str = code_gen_expression(gen, arguments[0]);
        return code_gen_text_file_seek(gen, object_str, pos_str);
    }
    if (strcmp(method_name, "rewind") == 0 && arg_count == 0) {
        return code_gen_text_file_rewind(gen, object_str);
    }
    if (strcmp(method_name, "flush") == 0 && arg_count == 0) {
        return code_gen_text_file_flush(gen, object_str);
    }
    if (strcmp(method_name, "close") == 0 && arg_count == 0) {
        return code_gen_text_file_close(gen, object_str);
    }

    /* Method not handled here */
    return NULL;
}

/* Dispatch BinaryFile instance method calls */
char *code_gen_binary_file_method_call(CodeGen *gen, const char *method_name,
                                        Expr *object, int arg_count, Expr **arguments)
{
    char *object_str = code_gen_expression(gen, object);

    /* Reading methods */
    if (strcmp(method_name, "readByte") == 0 && arg_count == 0) {
        return code_gen_binary_file_read_byte(gen, object_str);
    }
    if (strcmp(method_name, "readBytes") == 0 && arg_count == 1) {
        char *count_str = code_gen_expression(gen, arguments[0]);
        return code_gen_binary_file_read_bytes(gen, object_str, count_str);
    }
    if (strcmp(method_name, "readAll") == 0 && arg_count == 0) {
        return code_gen_binary_file_instance_read_all(gen, object_str);
    }
    if (strcmp(method_name, "readInto") == 0 && arg_count == 1) {
        char *buffer_str = code_gen_expression(gen, arguments[0]);
        return code_gen_binary_file_read_into(gen, object_str, buffer_str);
    }

    /* Writing methods */
    if (strcmp(method_name, "writeByte") == 0 && arg_count == 1) {
        char *byte_str = code_gen_expression(gen, arguments[0]);
        return code_gen_binary_file_write_byte(gen, object_str, byte_str);
    }
    if (strcmp(method_name, "writeBytes") == 0 && arg_count == 1) {
        char *data_str = code_gen_expression(gen, arguments[0]);
        return code_gen_binary_file_write_bytes(gen, object_str, data_str);
    }

    /* State query methods */
    if (strcmp(method_name, "hasBytes") == 0 && arg_count == 0) {
        return code_gen_binary_file_has_bytes(gen, object_str);
    }
    if (strcmp(method_name, "isEof") == 0 && arg_count == 0) {
        return code_gen_binary_file_is_eof(gen, object_str);
    }

    /* Position and control methods */
    if (strcmp(method_name, "position") == 0 && arg_count == 0) {
        return code_gen_binary_file_position(gen, object_str);
    }
    if (strcmp(method_name, "seek") == 0 && arg_count == 1) {
        char *pos_str = code_gen_expression(gen, arguments[0]);
        return code_gen_binary_file_seek(gen, object_str, pos_str);
    }
    if (strcmp(method_name, "rewind") == 0 && arg_count == 0) {
        return code_gen_binary_file_rewind(gen, object_str);
    }
    if (strcmp(method_name, "flush") == 0 && arg_count == 0) {
        return code_gen_binary_file_flush(gen, object_str);
    }
    if (strcmp(method_name, "close") == 0 && arg_count == 0) {
        return code_gen_binary_file_close(gen, object_str);
    }

    /* Method not handled here */
    return NULL;
}

/* ============================================================================
 * Property Accessors
 * ============================================================================ */

/* Generate code for TextFile property access (path, name, size) */
char *code_gen_text_file_property(CodeGen *gen, const char *property_name,
                                   Expr *object)
{
    char *object_str = code_gen_expression(gen, object);

    if (strcmp(property_name, "path") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_path(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }
    if (strcmp(property_name, "name") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_name(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }
    if (strcmp(property_name, "size") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_size(%s)", object_str);
    }

    return NULL;
}

/* Generate code for BinaryFile property access (path, name, size) */
char *code_gen_binary_file_property(CodeGen *gen, const char *property_name,
                                     Expr *object)
{
    char *object_str = code_gen_expression(gen, object);

    if (strcmp(property_name, "path") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_path(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }
    if (strcmp(property_name, "name") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_name(%s, %s)",
                             ARENA_VAR(gen), object_str);
    }
    if (strcmp(property_name, "size") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_size(%s)", object_str);
    }

    return NULL;
}

/* ============================================================================
 * Static Method Calls
 * ============================================================================ */

/* Generate code for TextFile static method calls */
char *code_gen_text_file_static_call(CodeGen *gen, const char *method_name,
                                      int arg_count, Expr **arguments)
{
    char *arg0 = arg_count > 0 ? code_gen_expression(gen, arguments[0]) : NULL;
    char *arg1 = arg_count > 1 ? code_gen_expression(gen, arguments[1]) : NULL;

    if (strcmp(method_name, "open") == 0 && arg_count == 1) {
        return arena_sprintf(gen->arena, "rt_text_file_open(%s, %s)",
                             ARENA_VAR(gen), arg0);
    }
    if (strcmp(method_name, "exists") == 0 && arg_count == 1) {
        return arena_sprintf(gen->arena, "rt_text_file_exists(%s)", arg0);
    }
    if (strcmp(method_name, "readAll") == 0 && arg_count == 1) {
        return arena_sprintf(gen->arena, "rt_text_file_read_all(%s, %s)",
                             ARENA_VAR(gen), arg0);
    }
    if (strcmp(method_name, "writeAll") == 0 && arg_count == 2) {
        return arena_sprintf(gen->arena, "rt_text_file_write_all(%s, %s)", arg0, arg1);
    }
    if (strcmp(method_name, "delete") == 0 && arg_count == 1) {
        return arena_sprintf(gen->arena, "rt_text_file_delete(%s)", arg0);
    }
    if (strcmp(method_name, "copy") == 0 && arg_count == 2) {
        return arena_sprintf(gen->arena, "rt_text_file_copy(%s, %s)", arg0, arg1);
    }
    if (strcmp(method_name, "move") == 0 && arg_count == 2) {
        return arena_sprintf(gen->arena, "rt_text_file_move(%s, %s)", arg0, arg1);
    }

    return NULL;
}

/* Generate code for BinaryFile static method calls */
char *code_gen_binary_file_static_call(CodeGen *gen, const char *method_name,
                                        int arg_count, Expr **arguments)
{
    char *arg0 = arg_count > 0 ? code_gen_expression(gen, arguments[0]) : NULL;
    char *arg1 = arg_count > 1 ? code_gen_expression(gen, arguments[1]) : NULL;

    if (strcmp(method_name, "open") == 0 && arg_count == 1) {
        return arena_sprintf(gen->arena, "rt_binary_file_open(%s, %s)",
                             ARENA_VAR(gen), arg0);
    }
    if (strcmp(method_name, "exists") == 0 && arg_count == 1) {
        return arena_sprintf(gen->arena, "rt_binary_file_exists(%s)", arg0);
    }
    if (strcmp(method_name, "readAll") == 0 && arg_count == 1) {
        return arena_sprintf(gen->arena, "rt_binary_file_read_all(%s, %s)",
                             ARENA_VAR(gen), arg0);
    }
    if (strcmp(method_name, "writeAll") == 0 && arg_count == 2) {
        return arena_sprintf(gen->arena, "rt_binary_file_write_all(%s, %s)", arg0, arg1);
    }
    if (strcmp(method_name, "delete") == 0 && arg_count == 1) {
        return arena_sprintf(gen->arena, "rt_binary_file_delete(%s)", arg0);
    }
    if (strcmp(method_name, "copy") == 0 && arg_count == 2) {
        return arena_sprintf(gen->arena, "rt_binary_file_copy(%s, %s)", arg0, arg1);
    }
    if (strcmp(method_name, "move") == 0 && arg_count == 2) {
        return arena_sprintf(gen->arena, "rt_binary_file_move(%s, %s)", arg0, arg1);
    }

    return NULL;
}
