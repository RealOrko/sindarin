#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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

    /* Process static methods */
    if (codegen_token_equals(type_name, "Process"))
    {
        if (codegen_token_equals(method_name, "run"))
        {
            if (call->arg_count == 1)
            {
                /* Process.run(cmd) -> rt_process_run(arena, cmd) */
                return arena_sprintf(gen->arena, "rt_process_run(%s, %s)",
                                     ARENA_VAR(gen), arg0);
            }
            else if (call->arg_count == 2)
            {
                /* Process.run(cmd, args) -> rt_process_run_with_args(arena, cmd, args) */
                return arena_sprintf(gen->arena, "rt_process_run_with_args(%s, %s, %s)",
                                     ARENA_VAR(gen), arg0, arg1);
            }
        }
    }

    /* TcpListener static methods */
    if (codegen_token_equals(type_name, "TcpListener"))
    {
        if (codegen_token_equals(method_name, "bind"))
        {
            /* TcpListener.bind(address) -> rt_tcp_listener_bind(arena, address) */
            return arena_sprintf(gen->arena, "rt_tcp_listener_bind(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
    }

    /* TcpStream static methods */
    if (codegen_token_equals(type_name, "TcpStream"))
    {
        if (codegen_token_equals(method_name, "connect"))
        {
            /* TcpStream.connect(address) -> rt_tcp_stream_connect(arena, address) */
            return arena_sprintf(gen->arena, "rt_tcp_stream_connect(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
    }

    /* UdpSocket static methods */
    if (codegen_token_equals(type_name, "UdpSocket"))
    {
        if (codegen_token_equals(method_name, "bind"))
        {
            /* UdpSocket.bind(address) -> rt_udp_socket_bind(arena, address) */
            return arena_sprintf(gen->arena, "rt_udp_socket_bind(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
    }

    /* Random static methods */
    if (codegen_token_equals(type_name, "Random"))
    {
        /* Factory methods */
        if (codegen_token_equals(method_name, "create"))
        {
            return arena_sprintf(gen->arena, "rt_random_create(%s)", ARENA_VAR(gen));
        }
        else if (codegen_token_equals(method_name, "createWithSeed"))
        {
            return arena_sprintf(gen->arena, "rt_random_create_with_seed(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        /* Value generation methods */
        else if (codegen_token_equals(method_name, "int"))
        {
            return arena_sprintf(gen->arena, "rt_random_static_int(%s, %s)", arg0, arg1);
        }
        else if (codegen_token_equals(method_name, "long"))
        {
            return arena_sprintf(gen->arena, "rt_random_static_long(%s, %s)", arg0, arg1);
        }
        else if (codegen_token_equals(method_name, "double"))
        {
            return arena_sprintf(gen->arena, "rt_random_static_double(%s, %s)", arg0, arg1);
        }
        else if (codegen_token_equals(method_name, "bool"))
        {
            return arena_sprintf(gen->arena, "rt_random_static_bool()");
        }
        else if (codegen_token_equals(method_name, "byte"))
        {
            return arena_sprintf(gen->arena, "rt_random_static_byte()");
        }
        else if (codegen_token_equals(method_name, "bytes"))
        {
            return arena_sprintf(gen->arena, "rt_random_static_bytes(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "gaussian"))
        {
            return arena_sprintf(gen->arena, "rt_random_static_gaussian(%s, %s)", arg0, arg1);
        }
        /* Batch generation methods */
        else if (codegen_token_equals(method_name, "intMany"))
        {
            char *arg2 = code_gen_expression(gen, call->arguments[2]);
            return arena_sprintf(gen->arena, "rt_random_static_int_many(%s, %s, %s, %s)",
                                 ARENA_VAR(gen), arg0, arg1, arg2);
        }
        else if (codegen_token_equals(method_name, "longMany"))
        {
            char *arg2 = code_gen_expression(gen, call->arguments[2]);
            return arena_sprintf(gen->arena, "rt_random_static_long_many(%s, %s, %s, %s)",
                                 ARENA_VAR(gen), arg0, arg1, arg2);
        }
        else if (codegen_token_equals(method_name, "doubleMany"))
        {
            char *arg2 = code_gen_expression(gen, call->arguments[2]);
            return arena_sprintf(gen->arena, "rt_random_static_double_many(%s, %s, %s, %s)",
                                 ARENA_VAR(gen), arg0, arg1, arg2);
        }
        else if (codegen_token_equals(method_name, "boolMany"))
        {
            return arena_sprintf(gen->arena, "rt_random_static_bool_many(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        else if (codegen_token_equals(method_name, "gaussianMany"))
        {
            char *arg2 = code_gen_expression(gen, call->arguments[2]);
            return arena_sprintf(gen->arena, "rt_random_static_gaussian_many(%s, %s, %s, %s)",
                                 ARENA_VAR(gen), arg0, arg1, arg2);
        }
        /* Collection operations */
        else if (codegen_token_equals(method_name, "choice"))
        {
            /* Determine element type from the array argument to call the correct function */
            Type *arr_type = call->arguments[0]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *type_suffix;
            switch (elem_type->kind)
            {
            case TYPE_INT:
                type_suffix = "long";
                break;
            case TYPE_LONG:
                type_suffix = "long";
                break;
            case TYPE_DOUBLE:
                type_suffix = "double";
                break;
            case TYPE_STRING:
                type_suffix = "string";
                break;
            case TYPE_BOOL:
                type_suffix = "bool";
                break;
            case TYPE_BYTE:
                type_suffix = "byte";
                break;
            default:
                type_suffix = "long";
                break;
            }
            return arena_sprintf(gen->arena, "rt_random_static_choice_%s(%s, rt_array_length(%s))",
                                 type_suffix, arg0, arg0);
        }
        else if (codegen_token_equals(method_name, "shuffle"))
        {
            /* Determine element type from the array argument to call the correct function */
            Type *arr_type = call->arguments[0]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *type_suffix;
            switch (elem_type->kind)
            {
            case TYPE_INT:
                type_suffix = "long";
                break;
            case TYPE_LONG:
                type_suffix = "long";
                break;
            case TYPE_DOUBLE:
                type_suffix = "double";
                break;
            case TYPE_STRING:
                type_suffix = "string";
                break;
            case TYPE_BOOL:
                type_suffix = "bool";
                break;
            case TYPE_BYTE:
                type_suffix = "byte";
                break;
            default:
                type_suffix = "long";
                break;
            }
            return arena_sprintf(gen->arena, "rt_random_static_shuffle_%s(%s)",
                                 type_suffix, arg0);
        }
        else if (codegen_token_equals(method_name, "weightedChoice"))
        {
            /* Determine element type from the items array argument to call the correct function */
            Type *arr_type = call->arguments[0]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *type_suffix;
            switch (elem_type->kind)
            {
            case TYPE_INT:
                type_suffix = "long";
                break;
            case TYPE_LONG:
                type_suffix = "long";
                break;
            case TYPE_DOUBLE:
                type_suffix = "double";
                break;
            case TYPE_STRING:
                type_suffix = "string";
                break;
            default:
                type_suffix = "long";
                break;
            }
            return arena_sprintf(gen->arena, "rt_random_static_weighted_choice_%s(%s, %s)",
                                 type_suffix, arg0, arg1);
        }
        else if (codegen_token_equals(method_name, "sample"))
        {
            /* Determine element type from the array argument to call the correct function */
            Type *arr_type = call->arguments[0]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *type_suffix;
            switch (elem_type->kind)
            {
            case TYPE_INT:
                type_suffix = "long";
                break;
            case TYPE_LONG:
                type_suffix = "long";
                break;
            case TYPE_DOUBLE:
                type_suffix = "double";
                break;
            case TYPE_STRING:
                type_suffix = "string";
                break;
            default:
                type_suffix = "long";
                break;
            }
            return arena_sprintf(gen->arena, "rt_random_static_sample_%s(%s, %s, %s)",
                                 type_suffix, ARENA_VAR(gen), arg0, arg1);
        }
    }

    /* UUID static methods */
    if (codegen_token_equals(type_name, "UUID"))
    {
        /* UUID.create() or UUID.new() -> rt_uuid_create(arena) */
        if (codegen_token_equals(method_name, "create") || codegen_token_equals(method_name, "new"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_create(%s)", ARENA_VAR(gen));
        }
        /* UUID.v7() -> rt_uuid_v7(arena) */
        else if (codegen_token_equals(method_name, "v7"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_v7(%s)", ARENA_VAR(gen));
        }
        /* UUID.v4() -> rt_uuid_v4(arena) */
        else if (codegen_token_equals(method_name, "v4"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_v4(%s)", ARENA_VAR(gen));
        }
        /* UUID.v5(namespace, name) -> rt_uuid_v5(arena, namespace, name) */
        else if (codegen_token_equals(method_name, "v5"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_v5(%s, %s, %s)",
                                 ARENA_VAR(gen), arg0, arg1);
        }
        /* UUID.fromString(str) -> rt_uuid_from_string(arena, str) */
        else if (codegen_token_equals(method_name, "fromString"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_from_string(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        /* UUID.fromHex(str) -> rt_uuid_from_hex(arena, str) */
        else if (codegen_token_equals(method_name, "fromHex"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_from_hex(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        /* UUID.fromBase64(str) -> rt_uuid_from_base64(arena, str) */
        else if (codegen_token_equals(method_name, "fromBase64"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_from_base64(%s, %s)",
                                 ARENA_VAR(gen), arg0);
        }
        /* UUID.fromBytes(bytes) -> rt_uuid_from_bytes(arena, bytes->data) */
        else if (codegen_token_equals(method_name, "fromBytes"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_from_bytes(%s, (unsigned char *)%s->data)",
                                 ARENA_VAR(gen), arg0);
        }
        /* UUID.zero() -> rt_uuid_nil(arena) */
        else if (codegen_token_equals(method_name, "zero"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_nil(%s)", ARENA_VAR(gen));
        }
        /* UUID.max() -> rt_uuid_max(arena) */
        else if (codegen_token_equals(method_name, "max"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_max(%s)", ARENA_VAR(gen));
        }
        /* UUID.namespaceDns() -> rt_uuid_namespace_dns(arena) */
        else if (codegen_token_equals(method_name, "namespaceDns"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_namespace_dns(%s)", ARENA_VAR(gen));
        }
        /* UUID.namespaceUrl() -> rt_uuid_namespace_url(arena) */
        else if (codegen_token_equals(method_name, "namespaceUrl"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_namespace_url(%s)", ARENA_VAR(gen));
        }
        /* UUID.namespaceOid() -> rt_uuid_namespace_oid(arena) */
        else if (codegen_token_equals(method_name, "namespaceOid"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_namespace_oid(%s)", ARENA_VAR(gen));
        }
        /* UUID.namespaceX500() -> rt_uuid_namespace_x500(arena) */
        else if (codegen_token_equals(method_name, "namespaceX500"))
        {
            return arena_sprintf(gen->arena, "rt_uuid_namespace_x500(%s)", ARENA_VAR(gen));
        }
    }

    /* Environment static methods */
    if (codegen_token_equals(type_name, "Environment"))
    {
        /* Environment.get(key) -> rt_env_get(arena, key)
         * Environment.get(key, default) -> rt_env_get_default(arena, key, default) */
        if (codegen_token_equals(method_name, "get"))
        {
            if (call->arg_count == 1)
            {
                return arena_sprintf(gen->arena, "rt_env_get(%s, %s)",
                                     ARENA_VAR(gen), arg0);
            }
            else
            {
                return arena_sprintf(gen->arena, "rt_env_get_default(%s, %s, %s)",
                                     ARENA_VAR(gen), arg0, arg1);
            }
        }
        /* Environment.set(key, value) -> rt_env_set(key, value) */
        else if (codegen_token_equals(method_name, "set"))
        {
            return arena_sprintf(gen->arena, "(void)rt_env_set(%s, %s)", arg0, arg1);
        }
        /* Environment.has(key) -> rt_env_has(key) */
        else if (codegen_token_equals(method_name, "has"))
        {
            return arena_sprintf(gen->arena, "rt_env_has(%s)", arg0);
        }
        /* Environment.remove(key) -> rt_env_remove(key) */
        else if (codegen_token_equals(method_name, "remove"))
        {
            return arena_sprintf(gen->arena, "rt_env_remove(%s)", arg0);
        }
        /* Environment.getInt(key) -> rt_env_get_int(key, &success) with error handling
         * Environment.getInt(key, default) -> rt_env_get_int_default(key, default) */
        else if (codegen_token_equals(method_name, "getInt"))
        {
            if (call->arg_count == 1)
            {
                return arena_sprintf(gen->arena,
                    "({ int __success = 0; long __val = rt_env_get_int(%s, &__success); "
                    "if (!__success) { fprintf(stderr, \"RuntimeError: Environment variable '%%s' not set or invalid int\\n\", %s); exit(1); } __val; })",
                    arg0, arg0);
            }
            else
            {
                return arena_sprintf(gen->arena, "rt_env_get_int_default(%s, %s)", arg0, arg1);
            }
        }
        /* Environment.getLong(key) -> rt_env_get_long(key, &success) with error handling
         * Environment.getLong(key, default) -> rt_env_get_long_default(key, default) */
        else if (codegen_token_equals(method_name, "getLong"))
        {
            if (call->arg_count == 1)
            {
                return arena_sprintf(gen->arena,
                    "({ int __success = 0; long long __val = rt_env_get_long(%s, &__success); "
                    "if (!__success) { fprintf(stderr, \"RuntimeError: Environment variable '%%s' not set or invalid long\\n\", %s); exit(1); } __val; })",
                    arg0, arg0);
            }
            else
            {
                return arena_sprintf(gen->arena, "rt_env_get_long_default(%s, %s)", arg0, arg1);
            }
        }
        /* Environment.getDouble(key) -> rt_env_get_double(key, &success) with error handling
         * Environment.getDouble(key, default) -> rt_env_get_double_default(key, default) */
        else if (codegen_token_equals(method_name, "getDouble"))
        {
            if (call->arg_count == 1)
            {
                return arena_sprintf(gen->arena,
                    "({ int __success = 0; double __val = rt_env_get_double(%s, &__success); "
                    "if (!__success) { fprintf(stderr, \"RuntimeError: Environment variable '%%s' not set or invalid double\\n\", %s); exit(1); } __val; })",
                    arg0, arg0);
            }
            else
            {
                return arena_sprintf(gen->arena, "rt_env_get_double_default(%s, %s)", arg0, arg1);
            }
        }
        /* Environment.getBool(key) -> rt_env_get_bool(key, &success) with error handling
         * Environment.getBool(key, default) -> rt_env_get_bool_default(key, default) */
        else if (codegen_token_equals(method_name, "getBool"))
        {
            if (call->arg_count == 1)
            {
                return arena_sprintf(gen->arena,
                    "({ int __success = 0; int __val = rt_env_get_bool(%s, &__success); "
                    "if (!__success) { fprintf(stderr, \"RuntimeError: Environment variable '%%s' not set or invalid bool\\n\", %s); exit(1); } __val; })",
                    arg0, arg0);
            }
            else
            {
                return arena_sprintf(gen->arena, "rt_env_get_bool_default(%s, %s)", arg0, arg1);
            }
        }
        /* Environment.list() -> rt_env_list(arena) */
        else if (codegen_token_equals(method_name, "list"))
        {
            return arena_sprintf(gen->arena, "rt_env_list(%s)", ARENA_VAR(gen));
        }
        /* Environment.names() -> rt_env_names(arena) */
        else if (codegen_token_equals(method_name, "names"))
        {
            return arena_sprintf(gen->arena, "rt_env_names(%s)", ARENA_VAR(gen));
        }
        /* Environment.all() -> rt_env_names(arena) (backward compatibility alias) */
        else if (codegen_token_equals(method_name, "all"))
        {
            return arena_sprintf(gen->arena, "rt_env_names(%s)", ARENA_VAR(gen));
        }
    }

    /* Interceptor static methods */
    if (codegen_token_equals(type_name, "Interceptor"))
    {
        /* Interceptor.register(handler) -> rt_interceptor_register((RtInterceptHandler)handler) */
        if (codegen_token_equals(method_name, "register"))
        {
            return arena_sprintf(gen->arena, "(rt_interceptor_register((RtInterceptHandler)%s), (void)0)", arg0);
        }
        /* Interceptor.registerWhere(handler, pattern) -> rt_interceptor_register_where((RtInterceptHandler)handler, pattern) */
        else if (codegen_token_equals(method_name, "registerWhere"))
        {
            return arena_sprintf(gen->arena, "(rt_interceptor_register_where((RtInterceptHandler)%s, %s), (void)0)", arg0, arg1);
        }
        /* Interceptor.clearAll() -> rt_interceptor_clear_all() */
        else if (codegen_token_equals(method_name, "clearAll"))
        {
            return arena_sprintf(gen->arena, "(rt_interceptor_clear_all(), (void)0)");
        }
        /* Interceptor.isActive() -> rt_interceptor_is_active() */
        else if (codegen_token_equals(method_name, "isActive"))
        {
            return arena_sprintf(gen->arena, "rt_interceptor_is_active()");
        }
        /* Interceptor.count() -> rt_interceptor_count() */
        else if (codegen_token_equals(method_name, "count"))
        {
            return arena_sprintf(gen->arena, "rt_interceptor_count()");
        }
    }

    /* Check for user-defined struct static methods */
    if (call->resolved_method != NULL && call->resolved_struct_type != NULL)
    {
        StructMethod *method = call->resolved_method;
        Type *struct_type = call->resolved_struct_type;
        const char *struct_name = struct_type->as.struct_type.name;

        if (method->is_native)
        {
            /* Native static method - use c_alias if present, else use naming convention */
            const char *func_name;
            if (method->c_alias != NULL)
            {
                /* Use explicit c_alias from #pragma alias */
                func_name = method->c_alias;
            }
            else
            {
                /* Create lowercase struct name for native method naming */
                char *struct_name_lower = arena_strdup(gen->arena, struct_name);
                for (char *p = struct_name_lower; *p; p++)
                {
                    *p = (char)tolower((unsigned char)*p);
                }
                func_name = arena_sprintf(gen->arena, "rt_%s_%s", struct_name_lower, method->name);
            }

            /* Build args list - NO arena for native methods */
            char *args_list = arena_strdup(gen->arena, "");
            for (int i = 0; i < call->arg_count; i++)
            {
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                if (i > 0)
                {
                    args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
                }
                else
                {
                    args_list = arg_str;
                }
            }

            return arena_sprintf(gen->arena, "%s(%s)", func_name, args_list);
        }
        else
        {
            /* Non-native static method: StructName_methodName(arena, args) */
            char *args_list = arena_strdup(gen->arena, ARENA_VAR(gen));

            for (int i = 0; i < call->arg_count; i++)
            {
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
            }

            return arena_sprintf(gen->arena, "%s_%s(%s)",
                                 struct_name, method->name, args_list);
        }
    }

    /* Fallback for unimplemented static methods */
    return arena_sprintf(gen->arena,
        "(fprintf(stderr, \"Static method call not yet implemented: %.*s.%.*s\\n\"), exit(1), (void *)0)",
        type_name.length, type_name.start,
        method_name.length, method_name.start);
}
