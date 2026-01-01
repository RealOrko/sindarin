#include "code_gen_expr.h"
#include "code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Helper to compare Token with C string */
static bool codegen_token_equals(Token tok, const char *str)
{
    size_t len = strlen(str);
    return tok.length == (int)len && strncmp(tok.start, str, len) == 0;
}

char *code_gen_static_call_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_static_call_expression");
    StaticCallExpr *call = &expr->as.static_call;
    Token type_name = call->type_name;
    Token method_name = call->method_name;

    /* Generate argument expressions */
    char *arg0 = call->arg_count > 0 ? code_gen_expression(gen, call->arguments[0]) : NULL;
    char *arg1 = call->arg_count > 1 ? code_gen_expression(gen, call->arguments[1]) : NULL;

    /* TextFile static methods - also in code_gen_expr_call_file.c */
    if (codegen_token_equals(type_name, "TextFile"))
    {
        if (codegen_token_equals(method_name, "open"))
        {
            /* TextFile.open(path) -> rt_text_file_open(arena, path) */
            return arena_sprintf(gen->arena, "rt_text_file_open(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "exists"))
        {
            /* TextFile.exists(path) -> rt_text_file_exists(path) */
            return arena_sprintf(gen->arena, "rt_text_file_exists(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "readAll"))
        {
            /* TextFile.readAll(path) -> rt_text_file_read_all(arena, path) */
            return arena_sprintf(gen->arena, "rt_text_file_read_all(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "writeAll"))
        {
            /* TextFile.writeAll(path, content) -> rt_text_file_write_all(path, content) */
            return arena_sprintf(gen->arena, "rt_text_file_write_all(%s, %s)", arg0, arg1);
        }
        else if (codegen_token_equals(method_name, "delete"))
        {
            /* TextFile.delete(path) -> rt_text_file_delete(path) */
            return arena_sprintf(gen->arena, "rt_text_file_delete(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "copy"))
        {
            /* TextFile.copy(src, dst) -> rt_text_file_copy(src, dst) */
            return arena_sprintf(gen->arena, "rt_text_file_copy(%s, %s)", arg0, arg1);
        }
        else if (codegen_token_equals(method_name, "move"))
        {
            /* TextFile.move(src, dst) -> rt_text_file_move(src, dst) */
            return arena_sprintf(gen->arena, "rt_text_file_move(%s, %s)", arg0, arg1);
        }
    }

    /* BinaryFile static methods - also in code_gen_expr_call_file.c */
    if (codegen_token_equals(type_name, "BinaryFile"))
    {
        if (codegen_token_equals(method_name, "open"))
        {
            /* BinaryFile.open(path) -> rt_binary_file_open(arena, path) */
            return arena_sprintf(gen->arena, "rt_binary_file_open(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "exists"))
        {
            /* BinaryFile.exists(path) -> rt_binary_file_exists(path) */
            return arena_sprintf(gen->arena, "rt_binary_file_exists(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "readAll"))
        {
            /* BinaryFile.readAll(path) -> rt_binary_file_read_all(arena, path) */
            return arena_sprintf(gen->arena, "rt_binary_file_read_all(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "writeAll"))
        {
            /* BinaryFile.writeAll(path, data) -> rt_binary_file_write_all(path, data) */
            return arena_sprintf(gen->arena, "rt_binary_file_write_all(%s, %s)", arg0, arg1);
        }
        else if (codegen_token_equals(method_name, "delete"))
        {
            /* BinaryFile.delete(path) -> rt_binary_file_delete(path) */
            return arena_sprintf(gen->arena, "rt_binary_file_delete(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "copy"))
        {
            /* BinaryFile.copy(src, dst) -> rt_binary_file_copy(src, dst) */
            return arena_sprintf(gen->arena, "rt_binary_file_copy(%s, %s)", arg0, arg1);
        }
        else if (codegen_token_equals(method_name, "move"))
        {
            /* BinaryFile.move(src, dst) -> rt_binary_file_move(src, dst) */
            return arena_sprintf(gen->arena, "rt_binary_file_move(%s, %s)", arg0, arg1);
        }
    }

    /* Stdin static methods */
    if (codegen_token_equals(type_name, "Stdin"))
    {
        if (codegen_token_equals(method_name, "readLine"))
        {
            return arena_sprintf(gen->arena, "rt_stdin_read_line(%s)", ARENA_VAR(gen));
        }
        else if (codegen_token_equals(method_name, "readChar"))
        {
            return arena_sprintf(gen->arena, "rt_stdin_read_char()");
        }
        else if (codegen_token_equals(method_name, "readWord"))
        {
            return arena_sprintf(gen->arena, "rt_stdin_read_word(%s)", ARENA_VAR(gen));
        }
        else if (codegen_token_equals(method_name, "hasChars"))
        {
            return arena_sprintf(gen->arena, "rt_stdin_has_chars()");
        }
        else if (codegen_token_equals(method_name, "hasLines"))
        {
            return arena_sprintf(gen->arena, "rt_stdin_has_lines()");
        }
        else if (codegen_token_equals(method_name, "isEof"))
        {
            return arena_sprintf(gen->arena, "rt_stdin_is_eof()");
        }
    }

    /* Stdout static methods */
    if (codegen_token_equals(type_name, "Stdout"))
    {
        if (codegen_token_equals(method_name, "write"))
        {
            return arena_sprintf(gen->arena, "rt_stdout_write(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "writeLine"))
        {
            return arena_sprintf(gen->arena, "rt_stdout_write_line(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "flush"))
        {
            return arena_sprintf(gen->arena, "rt_stdout_flush()");
        }
    }

    /* Stderr static methods */
    if (codegen_token_equals(type_name, "Stderr"))
    {
        if (codegen_token_equals(method_name, "write"))
        {
            return arena_sprintf(gen->arena, "rt_stderr_write(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "writeLine"))
        {
            return arena_sprintf(gen->arena, "rt_stderr_write_line(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "flush"))
        {
            return arena_sprintf(gen->arena, "rt_stderr_flush()");
        }
    }

    /* Bytes static methods */
    if (codegen_token_equals(type_name, "Bytes"))
    {
        if (codegen_token_equals(method_name, "fromHex"))
        {
            /* Bytes.fromHex(hex) -> rt_bytes_from_hex(arena, hex) */
            return arena_sprintf(gen->arena, "rt_bytes_from_hex(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "fromBase64"))
        {
            /* Bytes.fromBase64(b64) -> rt_bytes_from_base64(arena, b64) */
            return arena_sprintf(gen->arena, "rt_bytes_from_base64(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
    }

    /* Path static methods */
    if (codegen_token_equals(type_name, "Path"))
    {
        if (codegen_token_equals(method_name, "directory"))
        {
            /* Path.directory(path) -> rt_path_directory(arena, path) */
            return arena_sprintf(gen->arena, "rt_path_directory(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "filename"))
        {
            /* Path.filename(path) -> rt_path_filename(arena, path) */
            return arena_sprintf(gen->arena, "rt_path_filename(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "extension"))
        {
            /* Path.extension(path) -> rt_path_extension(arena, path) */
            return arena_sprintf(gen->arena, "rt_path_extension(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "join"))
        {
            /* Path.join handles 2 or 3 arguments */
            if (call->arg_count == 2)
            {
                return arena_sprintf(gen->arena, "rt_path_join2(%s, %s, %s)",
                                     ARENA_VAR(gen), arg0, arg1);
            }
            else if (call->arg_count == 3)
            {
                char *arg2 = code_gen_expression(gen, call->arguments[2]);
                return arena_sprintf(gen->arena, "rt_path_join3(%s, %s, %s, %s)",
                                     ARENA_VAR(gen), arg0, arg1, arg2);
            }
            else
            {
                /* For more than 3 arguments, chain the joins */
                char *result = arena_sprintf(gen->arena, "rt_path_join2(%s, %s, %s)",
                                            ARENA_VAR(gen), arg0, arg1);
                for (int i = 2; i < call->arg_count; i++)
                {
                    char *next_arg = code_gen_expression(gen, call->arguments[i]);
                    result = arena_sprintf(gen->arena, "rt_path_join2(%s, %s, %s)",
                                          ARENA_VAR(gen), result, next_arg);
                }
                return result;
            }
        }
        else if (codegen_token_equals(method_name, "absolute"))
        {
            /* Path.absolute(path) -> rt_path_absolute(arena, path) */
            return arena_sprintf(gen->arena, "rt_path_absolute(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "exists"))
        {
            /* Path.exists(path) -> rt_path_exists(path) */
            return arena_sprintf(gen->arena, "rt_path_exists(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "isFile"))
        {
            /* Path.isFile(path) -> rt_path_is_file(path) */
            return arena_sprintf(gen->arena, "rt_path_is_file(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "isDirectory"))
        {
            /* Path.isDirectory(path) -> rt_path_is_directory(path) */
            return arena_sprintf(gen->arena, "rt_path_is_directory(%s)", arg0);
        }
    }

    /* Directory static methods */
    if (codegen_token_equals(type_name, "Directory"))
    {
        if (codegen_token_equals(method_name, "list"))
        {
            /* Directory.list(path) -> rt_directory_list(arena, path) */
            return arena_sprintf(gen->arena, "rt_directory_list(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "listRecursive"))
        {
            /* Directory.listRecursive(path) -> rt_directory_list_recursive(arena, path) */
            return arena_sprintf(gen->arena, "rt_directory_list_recursive(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "create"))
        {
            /* Directory.create(path) -> rt_directory_create(path) */
            return arena_sprintf(gen->arena, "rt_directory_create(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "delete"))
        {
            /* Directory.delete(path) -> rt_directory_delete(path) */
            return arena_sprintf(gen->arena, "rt_directory_delete(%s)", arg0);
        }
        else if (codegen_token_equals(method_name, "deleteRecursive"))
        {
            /* Directory.deleteRecursive(path) -> rt_directory_delete_recursive(path) */
            return arena_sprintf(gen->arena, "rt_directory_delete_recursive(%s)", arg0);
        }
    }

    /* Time static methods */
    if (codegen_token_equals(type_name, "Time"))
    {
        if (codegen_token_equals(method_name, "now"))
        {
            /* Time.now() -> rt_time_now(arena) */
            return arena_sprintf(gen->arena, "rt_time_now(%s)", ARENA_VAR(gen));
        }
        else if (codegen_token_equals(method_name, "utc"))
        {
            /* Time.utc() -> rt_time_utc(arena) */
            return arena_sprintf(gen->arena, "rt_time_utc(%s)", ARENA_VAR(gen));
        }
        else if (codegen_token_equals(method_name, "fromMillis"))
        {
            /* Time.fromMillis(ms) -> rt_time_from_millis(arena, ms) */
            return arena_sprintf(gen->arena, "rt_time_from_millis(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "fromSeconds"))
        {
            /* Time.fromSeconds(s) -> rt_time_from_seconds(arena, s) */
            return arena_sprintf(gen->arena, "rt_time_from_seconds(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "sleep"))
        {
            /* Time.sleep(ms) -> rt_time_sleep(ms) */
            return arena_sprintf(gen->arena, "rt_time_sleep(%s)", arg0);
        }
    }

    /* Fallback for unimplemented static methods */
    return arena_sprintf(gen->arena,
        "(fprintf(stderr, \"Static method call not yet implemented: %.*s.%.*s\\n\"), exit(1), (void *)0)",
        type_name.length, type_name.start,
        method_name.length, method_name.start);
}
