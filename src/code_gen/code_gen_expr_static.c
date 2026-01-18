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
