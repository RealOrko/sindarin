/* ============================================================================
 * type_checker_expr_call_core.c - Core Call Expression Type Checking
 * ============================================================================
 * Type checking for function calls, including built-in functions (len),
 * regular user-defined function calls, static method calls, and lambda
 * argument type inference.
 *
 * This is the core module that contains the main dispatchers and helpers.
 * Type-specific method checking is delegated to specialized modules:
 * - type_checker_expr_call_array.c for array methods
 * - type_checker_expr_call_string.c for string methods
 * - type_checker_expr_call_file.c for TextFile/BinaryFile methods
 * - type_checker_expr_call_time.c for Time/Date methods
 * - type_checker_expr_call_net.c for TcpListener/TcpStream/UdpSocket methods
 * - type_checker_expr_call_random.c for Random/UUID/Process methods
 * ============================================================================ */

#include "type_checker/type_checker_expr_call_core.h"
#include "type_checker/type_checker_expr_call.h"
#include "type_checker/type_checker_expr_call_array.h"
#include "type_checker/type_checker_expr_call_string.h"
#include "type_checker/type_checker_expr_call_file.h"
#include "type_checker/type_checker_expr_call_time.h"
#include "type_checker/type_checker_expr_call_net.h"
#include "type_checker/type_checker_expr_call_random.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Check if callee matches a built-in function name */
bool is_builtin_name(Expr *callee, const char *name)
{
    if (callee->type != EXPR_VARIABLE) return false;
    Token tok = callee->as.variable.name;
    size_t len = strlen(name);
    return tok.length == (int)len && strncmp(tok.start, name, len) == 0;
}

/* Compare a token's text against a string */
bool token_equals(Token tok, const char *str)
{
    size_t len = strlen(str);
    return tok.length == (int)len && strncmp(tok.start, str, len) == 0;
}

/* ============================================================================
 * Call Expression Type Checking
 * ============================================================================ */

Type *type_check_call_expression(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function call with %d arguments", expr->as.call.arg_count);

    // Handle array built-in functions specially
    Expr *callee = expr->as.call.callee;

    // len(arr) -> int (works on arrays and strings)
    if (is_builtin_name(callee, "len") && expr->as.call.arg_count == 1)
    {
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL) return NULL;
        if (arg_type->kind != TYPE_ARRAY && arg_type->kind != TYPE_STRING)
        {
            type_error(expr->token, "len() requires array or string argument");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }

    // exit(code: int) -> void
    if (is_builtin_name(callee, "exit") && expr->as.call.arg_count == 1)
    {
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL) return NULL;
        if (arg_type->kind != TYPE_INT)
        {
            type_error(expr->token, "exit() requires int argument");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_VOID);
    }

    // assert(condition: bool, message: str) -> void
    if (is_builtin_name(callee, "assert") && expr->as.call.arg_count == 2)
    {
        Type *cond_type = type_check_expr(expr->as.call.arguments[0], table);
        if (cond_type == NULL) return NULL;
        if (cond_type->kind != TYPE_BOOL)
        {
            type_error(expr->token, "assert() first argument must be bool");
            return NULL;
        }
        Type *msg_type = type_check_expr(expr->as.call.arguments[1], table);
        if (msg_type == NULL) return NULL;
        if (msg_type->kind != TYPE_STRING)
        {
            type_error(expr->token, "assert() second argument must be str");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_VOID);
    }

    // Note: Other array operations are method-style only:
    //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)

    /* ========================================================================
     * Random instance collection methods: choice, shuffle, weightedChoice, sample
     * These need special handling because return type depends on argument type
     * ======================================================================== */
    if (callee->type == EXPR_MEMBER)
    {
        Expr *object = callee->as.member.object;
        Token method_name = callee->as.member.member_name;

        /* Skip namespace member access - namespaces can't be Random types
         * and type_check_expr on a namespace variable will emit an error */
        bool is_namespace_access = (object->type == EXPR_VARIABLE &&
                                    symbol_table_is_namespace(table, object->as.variable.name));

        /* Type check the object first (if not a namespace) */
        Type *object_type = is_namespace_access ? NULL : type_check_expr(object, table);
        if (object_type != NULL && object_type->kind == TYPE_RANDOM)
        {
            /* rng.choice(array: T[]): T */
            if (token_equals(method_name, "choice"))
            {
                if (expr->as.call.arg_count != 1)
                {
                    type_error(&method_name, "rng.choice requires exactly 1 argument (array)");
                    return NULL;
                }
                Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
                if (arg_type == NULL)
                {
                    return NULL;
                }
                if (arg_type->kind != TYPE_ARRAY)
                {
                    type_error(&method_name, "rng.choice requires an array argument");
                    return NULL;
                }
                /* Return the element type of the array */
                return arg_type->as.array.element_type;
            }

            /* rng.shuffle(array: T[]): void */
            if (token_equals(method_name, "shuffle"))
            {
                if (expr->as.call.arg_count != 1)
                {
                    type_error(&method_name, "rng.shuffle requires exactly 1 argument (array)");
                    return NULL;
                }
                Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
                if (arg_type == NULL)
                {
                    return NULL;
                }
                if (arg_type->kind != TYPE_ARRAY)
                {
                    type_error(&method_name, "rng.shuffle requires an array argument");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_VOID);
            }

            /* rng.weightedChoice(items: T[], weights: double[]): T */
            if (token_equals(method_name, "weightedChoice"))
            {
                if (expr->as.call.arg_count != 2)
                {
                    type_error(&method_name, "rng.weightedChoice requires exactly 2 arguments (items, weights)");
                    return NULL;
                }
                Type *items_type = type_check_expr(expr->as.call.arguments[0], table);
                if (items_type == NULL)
                {
                    return NULL;
                }
                if (items_type->kind != TYPE_ARRAY)
                {
                    type_error(&method_name, "rng.weightedChoice first argument (items) must be an array");
                    return NULL;
                }
                Type *weights_type = type_check_expr(expr->as.call.arguments[1], table);
                if (weights_type == NULL)
                {
                    return NULL;
                }
                if (weights_type->kind != TYPE_ARRAY ||
                    weights_type->as.array.element_type->kind != TYPE_DOUBLE)
                {
                    type_error(&method_name, "rng.weightedChoice second argument (weights) must be double[]");
                    return NULL;
                }
                /* Return the element type of the items array */
                return items_type->as.array.element_type;
            }

            /* rng.sample(array: T[], count: int): T[] */
            if (token_equals(method_name, "sample"))
            {
                if (expr->as.call.arg_count != 2)
                {
                    type_error(&method_name, "rng.sample requires exactly 2 arguments (array, count)");
                    return NULL;
                }
                Type *array_type = type_check_expr(expr->as.call.arguments[0], table);
                if (array_type == NULL)
                {
                    return NULL;
                }
                if (array_type->kind != TYPE_ARRAY)
                {
                    type_error(&method_name, "rng.sample first argument (array) must be an array");
                    return NULL;
                }
                Type *count_type = type_check_expr(expr->as.call.arguments[1], table);
                if (count_type == NULL)
                {
                    return NULL;
                }
                if (count_type->kind != TYPE_INT)
                {
                    type_error(&method_name, "rng.sample second argument (count) must be int");
                    return NULL;
                }
                /* Return an array of the same type */
                return array_type;
            }
        }
    }

    // Standard function call handling
    Type *callee_type = type_check_expr(expr->as.call.callee, table);

    /* Get function name for error messages */
    char func_name[128] = "<anonymous>";
    if (expr->as.call.callee->type == EXPR_VARIABLE)
    {
        int name_len = expr->as.call.callee->as.variable.name.length;
        int copy_len = name_len < 127 ? name_len : 127;
        memcpy(func_name, expr->as.call.callee->as.variable.name.start, copy_len);
        func_name[copy_len] = '\0';
    }

    if (callee_type == NULL)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Invalid callee '%s' in function call", func_name);
        type_error(expr->token, msg);
        return NULL;
    }
    if (callee_type->kind != TYPE_FUNCTION)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "'%s' is of type '%s', cannot call non-function",
                 func_name, type_name(callee_type));
        type_error(expr->token, msg);
        return NULL;
    }
    int expected_params = callee_type->as.function.param_count;
    bool is_variadic = callee_type->as.function.is_variadic;

    /* For variadic functions, we need at least the fixed parameters.
     * For non-variadic functions, exact count is required. */
    if (is_variadic)
    {
        if (expr->as.call.arg_count < expected_params)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Function '%s' requires at least %d argument(s), got %d",
                     func_name, expected_params, expr->as.call.arg_count);
            type_error(expr->token, msg);
            return NULL;
        }
    }
    else
    {
        if (expected_params != expr->as.call.arg_count)
        {
            argument_count_error(expr->token, func_name,
                                expected_params,
                                expr->as.call.arg_count);
            return NULL;
        }
    }

    /* Type check the fixed parameters */
    for (int i = 0; i < expected_params; i++)
    {
        Expr *arg_expr = expr->as.call.arguments[i];
        Type *param_type = callee_type->as.function.param_types[i];

        /* If argument is a lambda with missing types, infer from parameter type */
        if (arg_expr != NULL && arg_expr->type == EXPR_LAMBDA &&
            param_type != NULL && param_type->kind == TYPE_FUNCTION)
        {
            LambdaExpr *lambda = &arg_expr->as.lambda;
            Type *func_type = param_type;

            /* Check parameter count matches */
            if (lambda->param_count == func_type->as.function.param_count)
            {
                /* Infer missing parameter types */
                for (int j = 0; j < lambda->param_count; j++)
                {
                    if (lambda->params[j].type == NULL)
                    {
                        lambda->params[j].type = func_type->as.function.param_types[j];
                        DEBUG_VERBOSE("Inferred call argument lambda param %d type", j);
                    }
                }

                /* Infer missing return type */
                if (lambda->return_type == NULL)
                {
                    lambda->return_type = func_type->as.function.return_type;
                    DEBUG_VERBOSE("Inferred call argument lambda return type");
                }
            }
        }

        Type *arg_type = type_check_expr(arg_expr, table);
        if (arg_type == NULL)
        {
            type_error(expr->token, "Invalid argument in function call");
            return NULL;
        }
        if (param_type->kind == TYPE_ANY)
        {
            if (!is_printable_type(arg_type))
            {
                type_error(expr->token, "Unsupported type for built-in function");
                return NULL;
            }
        }
        else
        {
            if (!ast_type_equals(arg_type, param_type))
            {
                argument_type_error(expr->token, func_name, i, param_type, arg_type);
                return NULL;
            }
        }
    }

    /* Type check variadic arguments - must be primitives, str, or pointers (not arrays) */
    if (is_variadic)
    {
        for (int i = expected_params; i < expr->as.call.arg_count; i++)
        {
            Expr *arg_expr = expr->as.call.arguments[i];
            Type *arg_type = type_check_expr(arg_expr, table);
            if (arg_type == NULL)
            {
                type_error(expr->token, "Invalid argument in function call");
                return NULL;
            }
            if (!is_variadic_compatible_type(arg_type))
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Variadic argument %d has type '%s', but only primitives, str, and pointers are allowed",
                         i + 1, type_name(arg_type));
                type_error(expr->token, msg);
                return NULL;
            }
        }
    }

    DEBUG_VERBOSE("Returning function return type: %d", callee_type->as.function.return_type->kind);
    return callee_type->as.function.return_type;
}

/* ============================================================================
 * Static Method Call Type Checking
 * ============================================================================
 * Handles type checking for static method calls like TextFile.open(),
 * BinaryFile.readAll(), Time.now(), Path.join(), Directory.list(), etc.
 * ============================================================================ */

Type *type_check_static_method_call(Expr *expr, SymbolTable *table)
{
    StaticCallExpr *call = &expr->as.static_call;
    Token type_name = call->type_name;
    Token method_name = call->method_name;

    /* Type check all arguments first */
    for (int i = 0; i < call->arg_count; i++)
    {
        Type *arg_type = type_check_expr(call->arguments[i], table);
        if (arg_type == NULL)
        {
            return NULL;
        }
    }

    /* TextFile static methods */
    if (token_equals(type_name, "TextFile"))
    {
        if (token_equals(method_name, "open"))
        {
            /* TextFile.open(path: str): TextFile */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TextFile.open requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.open requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TEXT_FILE);
        }
        else if (token_equals(method_name, "exists"))
        {
            /* TextFile.exists(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TextFile.exists requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.exists requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "readAll"))
        {
            /* TextFile.readAll(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TextFile.readAll requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.readAll requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "writeAll"))
        {
            /* TextFile.writeAll(path: str, content: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "TextFile.writeAll requires exactly 2 arguments (path, content)");
                return NULL;
            }
            Type *path_type = call->arguments[0]->expr_type;
            Type *content_type = call->arguments[1]->expr_type;
            if (path_type == NULL || path_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.writeAll first argument must be a string path");
                return NULL;
            }
            if (content_type == NULL || content_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.writeAll second argument must be a string content");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "delete"))
        {
            /* TextFile.delete(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TextFile.delete requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.delete requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "copy"))
        {
            /* TextFile.copy(src: str, dst: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "TextFile.copy requires exactly 2 arguments (src, dst)");
                return NULL;
            }
            Type *src_type = call->arguments[0]->expr_type;
            Type *dst_type = call->arguments[1]->expr_type;
            if (src_type == NULL || src_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.copy first argument must be a string source path");
                return NULL;
            }
            if (dst_type == NULL || dst_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.copy second argument must be a string destination path");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "move"))
        {
            /* TextFile.move(src: str, dst: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "TextFile.move requires exactly 2 arguments (src, dst)");
                return NULL;
            }
            Type *src_type = call->arguments[0]->expr_type;
            Type *dst_type = call->arguments[1]->expr_type;
            if (src_type == NULL || src_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.move first argument must be a string source path");
                return NULL;
            }
            if (dst_type == NULL || dst_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TextFile.move second argument must be a string destination path");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown TextFile static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* BinaryFile static methods */
    if (token_equals(type_name, "BinaryFile"))
    {
        if (token_equals(method_name, "open"))
        {
            /* BinaryFile.open(path: str): BinaryFile */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "BinaryFile.open requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.open requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BINARY_FILE);
        }
        else if (token_equals(method_name, "exists"))
        {
            /* BinaryFile.exists(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "BinaryFile.exists requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.exists requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "readAll"))
        {
            /* BinaryFile.readAll(path: str): byte[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "BinaryFile.readAll requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.readAll requires a string argument");
                return NULL;
            }
            Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
            return ast_create_array_type(table->arena, byte_type);
        }
        else if (token_equals(method_name, "writeAll"))
        {
            /* BinaryFile.writeAll(path: str, data: byte[]): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "BinaryFile.writeAll requires exactly 2 arguments (path, data)");
                return NULL;
            }
            Type *path_type = call->arguments[0]->expr_type;
            Type *data_type = call->arguments[1]->expr_type;
            if (path_type == NULL || path_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.writeAll first argument must be a string path");
                return NULL;
            }
            if (data_type == NULL || data_type->kind != TYPE_ARRAY ||
                data_type->as.array.element_type->kind != TYPE_BYTE)
            {
                type_error(&method_name, "BinaryFile.writeAll second argument must be a byte array");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "delete"))
        {
            /* BinaryFile.delete(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "BinaryFile.delete requires exactly 1 argument (path)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.delete requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "copy"))
        {
            /* BinaryFile.copy(src: str, dst: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "BinaryFile.copy requires exactly 2 arguments (src, dst)");
                return NULL;
            }
            Type *src_type = call->arguments[0]->expr_type;
            Type *dst_type = call->arguments[1]->expr_type;
            if (src_type == NULL || src_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.copy first argument must be a string source path");
                return NULL;
            }
            if (dst_type == NULL || dst_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.copy second argument must be a string destination path");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "move"))
        {
            /* BinaryFile.move(src: str, dst: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "BinaryFile.move requires exactly 2 arguments (src, dst)");
                return NULL;
            }
            Type *src_type = call->arguments[0]->expr_type;
            Type *dst_type = call->arguments[1]->expr_type;
            if (src_type == NULL || src_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.move first argument must be a string source path");
                return NULL;
            }
            if (dst_type == NULL || dst_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "BinaryFile.move second argument must be a string destination path");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown BinaryFile static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Time static methods */
    if (token_equals(type_name, "Time"))
    {
        if (token_equals(method_name, "now"))
        {
            /* Time.now(): Time */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Time.now takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TIME);
        }
        else if (token_equals(method_name, "utc"))
        {
            /* Time.utc(): Time */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Time.utc takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TIME);
        }
        else if (token_equals(method_name, "fromMillis"))
        {
            /* Time.fromMillis(ms: int): Time */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Time.fromMillis requires exactly 1 argument (ms)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Time.fromMillis requires an int argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TIME);
        }
        else if (token_equals(method_name, "fromSeconds"))
        {
            /* Time.fromSeconds(s: int): Time */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Time.fromSeconds requires exactly 1 argument (s)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Time.fromSeconds requires an int argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TIME);
        }
        else if (token_equals(method_name, "sleep"))
        {
            /* Time.sleep(ms: int): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Time.sleep requires exactly 1 argument (ms)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Time.sleep requires an int argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Time static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Date static methods */
    if (token_equals(type_name, "Date"))
    {
        if (token_equals(method_name, "today"))
        {
            /* Date.today(): Date */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Date.today takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_DATE);
        }
        else if (token_equals(method_name, "fromYmd"))
        {
            /* Date.fromYmd(year, month, day: int): Date */
            if (call->arg_count != 3)
            {
                type_error(&method_name, "Date.fromYmd requires exactly 3 arguments (year, month, day)");
                return NULL;
            }
            for (int i = 0; i < 3; i++)
            {
                Type *arg_type = call->arguments[i]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_INT)
                {
                    type_error(&method_name, "Date.fromYmd requires int arguments");
                    return NULL;
                }
            }
            return ast_create_primitive_type(table->arena, TYPE_DATE);
        }
        else if (token_equals(method_name, "fromString"))
        {
            /* Date.fromString(str: str): Date */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Date.fromString requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Date.fromString requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_DATE);
        }
        else if (token_equals(method_name, "fromEpochDays"))
        {
            /* Date.fromEpochDays(days: int): Date */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Date.fromEpochDays requires exactly 1 argument (days)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Date.fromEpochDays requires an int argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_DATE);
        }
        else if (token_equals(method_name, "isLeapYear"))
        {
            /* Date.isLeapYear(year: int): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Date.isLeapYear requires exactly 1 argument (year)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Date.isLeapYear requires an int argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "daysInMonth"))
        {
            /* Date.daysInMonth(year: int, month: int): int */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Date.daysInMonth requires exactly 2 arguments (year, month)");
                return NULL;
            }
            for (int i = 0; i < 2; i++)
            {
                Type *arg_type = call->arguments[i]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_INT)
                {
                    type_error(&method_name, "Date.daysInMonth requires int arguments");
                    return NULL;
                }
            }
            return ast_create_primitive_type(table->arena, TYPE_INT);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Date static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Stdin static methods - console input */
    if (token_equals(type_name, "Stdin"))
    {
        if (token_equals(method_name, "readLine"))
        {
            /* Stdin.readLine(): str */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readLine takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "readChar"))
        {
            /* Stdin.readChar(): int */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readChar takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_INT);
        }
        else if (token_equals(method_name, "readWord"))
        {
            /* Stdin.readWord(): str */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readWord takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "hasChars"))
        {
            /* Stdin.hasChars(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.hasChars takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "hasLines"))
        {
            /* Stdin.hasLines(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.hasLines takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "isEof"))
        {
            /* Stdin.isEof(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.isEof takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stdin method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Stdout static methods - console output */
    if (token_equals(type_name, "Stdout"))
    {
        if (token_equals(method_name, "write"))
        {
            /* Stdout.write(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stdout.write requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stdout.write requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "writeLine"))
        {
            /* Stdout.writeLine(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stdout.writeLine requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stdout.writeLine requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "flush"))
        {
            /* Stdout.flush(): void */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdout.flush takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stdout method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Stderr static methods - error output */
    if (token_equals(type_name, "Stderr"))
    {
        if (token_equals(method_name, "write"))
        {
            /* Stderr.write(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stderr.write requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stderr.write requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "writeLine"))
        {
            /* Stderr.writeLine(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stderr.writeLine requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stderr.writeLine requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "flush"))
        {
            /* Stderr.flush(): void */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stderr.flush takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stderr method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Bytes static methods - byte array decoding utilities */
    if (token_equals(type_name, "Bytes"))
    {
        if (token_equals(method_name, "fromHex"))
        {
            /* Bytes.fromHex(hex: str): byte[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Bytes.fromHex requires exactly 1 argument (hex string)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Bytes.fromHex requires a string argument");
                return NULL;
            }
            Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
            return ast_create_array_type(table->arena, byte_type);
        }
        else if (token_equals(method_name, "fromBase64"))
        {
            /* Bytes.fromBase64(b64: str): byte[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Bytes.fromBase64 requires exactly 1 argument (Base64 string)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Bytes.fromBase64 requires a string argument");
                return NULL;
            }
            Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
            return ast_create_array_type(table->arena, byte_type);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Bytes static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Path static methods - path manipulation utilities */
    if (token_equals(type_name, "Path"))
    {
        if (token_equals(method_name, "directory"))
        {
            /* Path.directory(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.directory requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.directory requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "filename"))
        {
            /* Path.filename(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.filename requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.filename requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "extension"))
        {
            /* Path.extension(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.extension requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.extension requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "join"))
        {
            /* Path.join(paths...: str): str - variable arguments, at least 2 */
            if (call->arg_count < 2)
            {
                type_error(&method_name, "Path.join requires at least 2 arguments");
                return NULL;
            }
            for (int i = 0; i < call->arg_count; i++)
            {
                Type *arg_type = call->arguments[i]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Path.join requires all arguments to be strings");
                    return NULL;
                }
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "absolute"))
        {
            /* Path.absolute(path: str): str */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.absolute requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.absolute requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "exists"))
        {
            /* Path.exists(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.exists requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.exists requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "isFile"))
        {
            /* Path.isFile(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.isFile requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.isFile requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "isDirectory"))
        {
            /* Path.isDirectory(path: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Path.isDirectory requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Path.isDirectory requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Path static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Directory static methods - directory operations */
    if (token_equals(type_name, "Directory"))
    {
        if (token_equals(method_name, "list"))
        {
            /* Directory.list(path: str): str[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.list requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.list requires a string argument");
                return NULL;
            }
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            return ast_create_array_type(table->arena, string_type);
        }
        else if (token_equals(method_name, "listRecursive"))
        {
            /* Directory.listRecursive(path: str): str[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.listRecursive requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.listRecursive requires a string argument");
                return NULL;
            }
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            return ast_create_array_type(table->arena, string_type);
        }
        else if (token_equals(method_name, "create"))
        {
            /* Directory.create(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.create requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.create requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "delete"))
        {
            /* Directory.delete(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.delete requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.delete requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "deleteRecursive"))
        {
            /* Directory.deleteRecursive(path: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Directory.deleteRecursive requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Directory.deleteRecursive requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Directory static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Process static methods - process execution */
    if (token_equals(type_name, "Process"))
    {
        if (token_equals(method_name, "run"))
        {
            /* Process.run(cmd: str): Process
             * Process.run(cmd: str, args: str[]): Process */
            if (call->arg_count == 0 || call->arg_count > 2)
            {
                type_error(&method_name, "Process.run requires 1 or 2 arguments (cmd, optional args)");
                return NULL;
            }
            /* First argument must be a string (command) */
            Type *cmd_type = call->arguments[0]->expr_type;
            if (cmd_type == NULL || cmd_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Process.run requires a string command as first argument");
                return NULL;
            }
            /* Second argument (if present) must be a string array (args) */
            if (call->arg_count == 2)
            {
                Type *args_type = call->arguments[1]->expr_type;
                /* Allow str[] (TYPE_STRING) or empty array {} (TYPE_NIL) */
                if (args_type == NULL || args_type->kind != TYPE_ARRAY ||
                    args_type->as.array.element_type == NULL ||
                    (args_type->as.array.element_type->kind != TYPE_STRING &&
                     args_type->as.array.element_type->kind != TYPE_NIL))
                {
                    type_error(&method_name, "Process.run requires a str[] as second argument");
                    return NULL;
                }
            }
            return ast_create_primitive_type(table->arena, TYPE_PROCESS);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Process static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* TcpListener static methods - TCP server creation */
    if (token_equals(type_name, "TcpListener"))
    {
        if (token_equals(method_name, "bind"))
        {
            /* TcpListener.bind(address: str): TcpListener */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TcpListener.bind requires exactly 1 argument (address)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TcpListener.bind requires a string address argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TCP_LISTENER);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown TcpListener static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* TcpStream static methods - TCP client creation */
    if (token_equals(type_name, "TcpStream"))
    {
        if (token_equals(method_name, "connect"))
        {
            /* TcpStream.connect(address: str): TcpStream */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "TcpStream.connect requires exactly 1 argument (address)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "TcpStream.connect requires a string address argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_TCP_STREAM);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown TcpStream static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* UdpSocket static methods - UDP socket creation */
    if (token_equals(type_name, "UdpSocket"))
    {
        if (token_equals(method_name, "bind"))
        {
            /* UdpSocket.bind(address: str): UdpSocket */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "UdpSocket.bind requires exactly 1 argument (address)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "UdpSocket.bind requires a string address argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UDP_SOCKET);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown UdpSocket static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Random static methods - random number generation */
    if (token_equals(type_name, "Random"))
    {
        if (token_equals(method_name, "create"))
        {
            /* Random.create(): Random */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Random.create takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_RANDOM);
        }
        else if (token_equals(method_name, "createWithSeed"))
        {
            /* Random.createWithSeed(seed: long): Random */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Random.createWithSeed requires exactly 1 argument (seed)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_LONG)
            {
                type_error(&method_name, "Random.createWithSeed requires a long argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_RANDOM);
        }
        else if (token_equals(method_name, "int"))
        {
            /* Random.int(min: int, max: int): int */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Random.int requires exactly 2 arguments (min, max)");
                return NULL;
            }
            Type *min_type = call->arguments[0]->expr_type;
            Type *max_type = call->arguments[1]->expr_type;
            if (min_type == NULL || min_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.int first argument (min) must be int");
                return NULL;
            }
            if (max_type == NULL || max_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.int second argument (max) must be int");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_INT);
        }
        else if (token_equals(method_name, "long"))
        {
            /* Random.long(min: long, max: long): long */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Random.long requires exactly 2 arguments (min, max)");
                return NULL;
            }
            Type *min_type = call->arguments[0]->expr_type;
            Type *max_type = call->arguments[1]->expr_type;
            if (min_type == NULL || min_type->kind != TYPE_LONG)
            {
                type_error(&method_name, "Random.long first argument (min) must be long");
                return NULL;
            }
            if (max_type == NULL || max_type->kind != TYPE_LONG)
            {
                type_error(&method_name, "Random.long second argument (max) must be long");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_LONG);
        }
        else if (token_equals(method_name, "double"))
        {
            /* Random.double(min: double, max: double): double */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Random.double requires exactly 2 arguments (min, max)");
                return NULL;
            }
            Type *min_type = call->arguments[0]->expr_type;
            Type *max_type = call->arguments[1]->expr_type;
            if (min_type == NULL || min_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.double first argument (min) must be double");
                return NULL;
            }
            if (max_type == NULL || max_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.double second argument (max) must be double");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_DOUBLE);
        }
        else if (token_equals(method_name, "bool"))
        {
            /* Random.bool(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Random.bool takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "byte"))
        {
            /* Random.byte(): byte */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Random.byte takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BYTE);
        }
        else if (token_equals(method_name, "bytes"))
        {
            /* Random.bytes(count: int): byte[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Random.bytes requires exactly 1 argument (count)");
                return NULL;
            }
            Type *count_type = call->arguments[0]->expr_type;
            if (count_type == NULL || count_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.bytes requires an int argument");
                return NULL;
            }
            Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
            return ast_create_array_type(table->arena, byte_type);
        }
        else if (token_equals(method_name, "gaussian"))
        {
            /* Random.gaussian(mean: double, stddev: double): double */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Random.gaussian requires exactly 2 arguments (mean, stddev)");
                return NULL;
            }
            Type *mean_type = call->arguments[0]->expr_type;
            Type *stddev_type = call->arguments[1]->expr_type;
            if (mean_type == NULL || mean_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.gaussian first argument (mean) must be double");
                return NULL;
            }
            if (stddev_type == NULL || stddev_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.gaussian second argument (stddev) must be double");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_DOUBLE);
        }
        else if (token_equals(method_name, "intMany"))
        {
            /* Random.intMany(min: int, max: int, count: int): int[] */
            if (call->arg_count != 3)
            {
                type_error(&method_name, "Random.intMany requires exactly 3 arguments (min, max, count)");
                return NULL;
            }
            Type *min_type = call->arguments[0]->expr_type;
            Type *max_type = call->arguments[1]->expr_type;
            Type *count_type = call->arguments[2]->expr_type;
            if (min_type == NULL || min_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.intMany first argument (min) must be int");
                return NULL;
            }
            if (max_type == NULL || max_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.intMany second argument (max) must be int");
                return NULL;
            }
            if (count_type == NULL || count_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.intMany third argument (count) must be int");
                return NULL;
            }
            Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
            return ast_create_array_type(table->arena, int_type);
        }
        else if (token_equals(method_name, "longMany"))
        {
            /* Random.longMany(min: long, max: long, count: int): long[] */
            if (call->arg_count != 3)
            {
                type_error(&method_name, "Random.longMany requires exactly 3 arguments (min, max, count)");
                return NULL;
            }
            Type *min_type = call->arguments[0]->expr_type;
            Type *max_type = call->arguments[1]->expr_type;
            Type *count_type = call->arguments[2]->expr_type;
            if (min_type == NULL || min_type->kind != TYPE_LONG)
            {
                type_error(&method_name, "Random.longMany first argument (min) must be long");
                return NULL;
            }
            if (max_type == NULL || max_type->kind != TYPE_LONG)
            {
                type_error(&method_name, "Random.longMany second argument (max) must be long");
                return NULL;
            }
            if (count_type == NULL || count_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.longMany third argument (count) must be int");
                return NULL;
            }
            Type *long_type = ast_create_primitive_type(table->arena, TYPE_LONG);
            return ast_create_array_type(table->arena, long_type);
        }
        else if (token_equals(method_name, "doubleMany"))
        {
            /* Random.doubleMany(min: double, max: double, count: int): double[] */
            if (call->arg_count != 3)
            {
                type_error(&method_name, "Random.doubleMany requires exactly 3 arguments (min, max, count)");
                return NULL;
            }
            Type *min_type = call->arguments[0]->expr_type;
            Type *max_type = call->arguments[1]->expr_type;
            Type *count_type = call->arguments[2]->expr_type;
            if (min_type == NULL || min_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.doubleMany first argument (min) must be double");
                return NULL;
            }
            if (max_type == NULL || max_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.doubleMany second argument (max) must be double");
                return NULL;
            }
            if (count_type == NULL || count_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.doubleMany third argument (count) must be int");
                return NULL;
            }
            Type *double_type = ast_create_primitive_type(table->arena, TYPE_DOUBLE);
            return ast_create_array_type(table->arena, double_type);
        }
        else if (token_equals(method_name, "boolMany"))
        {
            /* Random.boolMany(count: int): bool[] */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Random.boolMany requires exactly 1 argument (count)");
                return NULL;
            }
            Type *count_type = call->arguments[0]->expr_type;
            if (count_type == NULL || count_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.boolMany requires an int argument");
                return NULL;
            }
            Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
            return ast_create_array_type(table->arena, bool_type);
        }
        else if (token_equals(method_name, "gaussianMany"))
        {
            /* Random.gaussianMany(mean: double, stddev: double, count: int): double[] */
            if (call->arg_count != 3)
            {
                type_error(&method_name, "Random.gaussianMany requires exactly 3 arguments (mean, stddev, count)");
                return NULL;
            }
            Type *mean_type = call->arguments[0]->expr_type;
            Type *stddev_type = call->arguments[1]->expr_type;
            Type *count_type = call->arguments[2]->expr_type;
            if (mean_type == NULL || mean_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.gaussianMany first argument (mean) must be double");
                return NULL;
            }
            if (stddev_type == NULL || stddev_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.gaussianMany second argument (stddev) must be double");
                return NULL;
            }
            if (count_type == NULL || count_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.gaussianMany third argument (count) must be int");
                return NULL;
            }
            Type *double_type = ast_create_primitive_type(table->arena, TYPE_DOUBLE);
            return ast_create_array_type(table->arena, double_type);
        }
        else if (token_equals(method_name, "choice"))
        {
            /* Random.choice(array: T[]): T - returns element type of array */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Random.choice requires exactly 1 argument (array)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_ARRAY)
            {
                type_error(&method_name, "Random.choice requires an array argument");
                return NULL;
            }
            /* Return the element type of the array */
            return arg_type->as.array.element_type;
        }
        else if (token_equals(method_name, "shuffle"))
        {
            /* Random.shuffle(array: T[]): void - shuffles array in place */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Random.shuffle requires exactly 1 argument (array)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_ARRAY)
            {
                type_error(&method_name, "Random.shuffle requires an array argument");
                return NULL;
            }
            /* Shuffle modifies in place, returns void */
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "weightedChoice"))
        {
            /* Random.weightedChoice(items: T[], weights: double[]): T */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Random.weightedChoice requires exactly 2 arguments (items, weights)");
                return NULL;
            }
            Type *items_type = call->arguments[0]->expr_type;
            if (items_type == NULL || items_type->kind != TYPE_ARRAY)
            {
                type_error(&method_name, "Random.weightedChoice first argument (items) must be an array");
                return NULL;
            }
            Type *weights_type = call->arguments[1]->expr_type;
            if (weights_type == NULL || weights_type->kind != TYPE_ARRAY ||
                weights_type->as.array.element_type->kind != TYPE_DOUBLE)
            {
                type_error(&method_name, "Random.weightedChoice second argument (weights) must be double[]");
                return NULL;
            }
            /* Return the element type of the items array */
            return items_type->as.array.element_type;
        }
        else if (token_equals(method_name, "sample"))
        {
            /* Random.sample(array: T[], count: int): T[] */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Random.sample requires exactly 2 arguments (array, count)");
                return NULL;
            }
            Type *array_type = call->arguments[0]->expr_type;
            if (array_type == NULL || array_type->kind != TYPE_ARRAY)
            {
                type_error(&method_name, "Random.sample first argument (array) must be an array");
                return NULL;
            }
            Type *count_type = call->arguments[1]->expr_type;
            if (count_type == NULL || count_type->kind != TYPE_INT)
            {
                type_error(&method_name, "Random.sample second argument (count) must be int");
                return NULL;
            }
            /* Return an array of the same element type */
            return array_type;
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Random static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* UUID static methods - universally unique identifier generation */
    if (token_equals(type_name, "UUID"))
    {
        if (token_equals(method_name, "create") || token_equals(method_name, "new"))
        {
            /* UUID.create(): UUID - Generate UUIDv7 (recommended default) */
            /* UUID.new(): UUID - Alias for create() */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.create takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "v7"))
        {
            /* UUID.v7(): UUID - Generate UUIDv7 (time-ordered) */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.v7 takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "v4"))
        {
            /* UUID.v4(): UUID - Generate UUIDv4 (random) */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.v4 takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "v5"))
        {
            /* UUID.v5(namespace: UUID, name: str): UUID - Deterministic UUID from namespace + name */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "UUID.v5 requires exactly 2 arguments (namespace, name)");
                return NULL;
            }
            Type *ns_type = call->arguments[0]->expr_type;
            Type *name_type = call->arguments[1]->expr_type;
            if (ns_type == NULL || ns_type->kind != TYPE_UUID)
            {
                type_error(&method_name, "UUID.v5 first argument (namespace) must be UUID");
                return NULL;
            }
            if (name_type == NULL || name_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "UUID.v5 second argument (name) must be str");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "fromString"))
        {
            /* UUID.fromString(str): UUID - Parse standard 36-char format */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "UUID.fromString requires exactly 1 argument (str)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "UUID.fromString requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "fromHex"))
        {
            /* UUID.fromHex(str): UUID - Parse 32-char hex format */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "UUID.fromHex requires exactly 1 argument (str)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "UUID.fromHex requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "fromBase64"))
        {
            /* UUID.fromBase64(str): UUID - Parse 22-char base64 format */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "UUID.fromBase64 requires exactly 1 argument (str)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "UUID.fromBase64 requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "fromBytes"))
        {
            /* UUID.fromBytes(bytes: byte[]): UUID - Create from 16-byte array */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "UUID.fromBytes requires exactly 1 argument (byte[])");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_ARRAY ||
                arg_type->as.array.element_type == NULL ||
                arg_type->as.array.element_type->kind != TYPE_BYTE)
            {
                type_error(&method_name, "UUID.fromBytes requires a byte[] argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "zero"))
        {
            /* UUID.zero(): UUID - All zeros UUID (nil UUID) */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.zero takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "max"))
        {
            /* UUID.max(): UUID - All ones UUID */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.max takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "namespaceDns"))
        {
            /* UUID.namespaceDns(): UUID - DNS namespace constant */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.namespaceDns takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "namespaceUrl"))
        {
            /* UUID.namespaceUrl(): UUID - URL namespace constant */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.namespaceUrl takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "namespaceOid"))
        {
            /* UUID.namespaceOid(): UUID - OID namespace constant */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.namespaceOid takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else if (token_equals(method_name, "namespaceX500"))
        {
            /* UUID.namespaceX500(): UUID - X.500 DN namespace constant */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "UUID.namespaceX500 takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_UUID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown UUID static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Environment static methods - access environment variables */
    if (token_equals(type_name, "Environment"))
    {
        if (token_equals(method_name, "get"))
        {
            /* Environment.get(key: str): str - Get environment variable value
             * Environment.get(key: str, default: str): str - Get with default */
            if (call->arg_count == 1)
            {
                Type *arg_type = call->arguments[0]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.get requires a string argument for key");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_STRING);
            }
            else if (call->arg_count == 2)
            {
                Type *key_type = call->arguments[0]->expr_type;
                Type *default_type = call->arguments[1]->expr_type;
                if (key_type == NULL || key_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.get requires a string argument for key");
                    return NULL;
                }
                if (default_type == NULL || default_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.get requires a string argument for default");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_STRING);
            }
            else
            {
                type_error(&method_name, "Environment.get requires 1 or 2 arguments");
                return NULL;
            }
        }
        else if (token_equals(method_name, "set"))
        {
            /* Environment.set(key: str, value: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Environment.set requires exactly 2 arguments (key, value)");
                return NULL;
            }
            Type *key_type = call->arguments[0]->expr_type;
            Type *value_type = call->arguments[1]->expr_type;
            if (key_type == NULL || key_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Environment.set requires a string argument for key");
                return NULL;
            }
            if (value_type == NULL || value_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Environment.set requires a string argument for value");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "has"))
        {
            /* Environment.has(key: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Environment.has requires exactly 1 argument (key)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Environment.has requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "remove"))
        {
            /* Environment.remove(key: str): bool */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Environment.remove requires exactly 1 argument (key)");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Environment.remove requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "getInt"))
        {
            /* Environment.getInt(key: str): int
             * Environment.getInt(key: str, default: int): int */
            if (call->arg_count == 1)
            {
                Type *arg_type = call->arguments[0]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.getInt requires a string argument for key");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_INT);
            }
            else if (call->arg_count == 2)
            {
                Type *key_type = call->arguments[0]->expr_type;
                Type *default_type = call->arguments[1]->expr_type;
                if (key_type == NULL || key_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.getInt requires a string argument for key");
                    return NULL;
                }
                if (default_type == NULL || default_type->kind != TYPE_INT)
                {
                    type_error(&method_name, "Environment.getInt requires an int argument for default");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_INT);
            }
            else
            {
                type_error(&method_name, "Environment.getInt requires 1 or 2 arguments");
                return NULL;
            }
        }
        else if (token_equals(method_name, "getLong"))
        {
            /* Environment.getLong(key: str): long
             * Environment.getLong(key: str, default: long): long */
            if (call->arg_count == 1)
            {
                Type *arg_type = call->arguments[0]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.getLong requires a string argument for key");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_LONG);
            }
            else if (call->arg_count == 2)
            {
                Type *key_type = call->arguments[0]->expr_type;
                Type *default_type = call->arguments[1]->expr_type;
                if (key_type == NULL || key_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.getLong requires a string argument for key");
                    return NULL;
                }
                if (default_type == NULL || default_type->kind != TYPE_LONG)
                {
                    type_error(&method_name, "Environment.getLong requires a long argument for default");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_LONG);
            }
            else
            {
                type_error(&method_name, "Environment.getLong requires 1 or 2 arguments");
                return NULL;
            }
        }
        else if (token_equals(method_name, "getDouble"))
        {
            /* Environment.getDouble(key: str): double
             * Environment.getDouble(key: str, default: double): double */
            if (call->arg_count == 1)
            {
                Type *arg_type = call->arguments[0]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.getDouble requires a string argument for key");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_DOUBLE);
            }
            else if (call->arg_count == 2)
            {
                Type *key_type = call->arguments[0]->expr_type;
                Type *default_type = call->arguments[1]->expr_type;
                if (key_type == NULL || key_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.getDouble requires a string argument for key");
                    return NULL;
                }
                if (default_type == NULL || default_type->kind != TYPE_DOUBLE)
                {
                    type_error(&method_name, "Environment.getDouble requires a double argument for default");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_DOUBLE);
            }
            else
            {
                type_error(&method_name, "Environment.getDouble requires 1 or 2 arguments");
                return NULL;
            }
        }
        else if (token_equals(method_name, "getBool"))
        {
            /* Environment.getBool(key: str): bool
             * Environment.getBool(key: str, default: bool): bool */
            if (call->arg_count == 1)
            {
                Type *arg_type = call->arguments[0]->expr_type;
                if (arg_type == NULL || arg_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.getBool requires a string argument for key");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_BOOL);
            }
            else if (call->arg_count == 2)
            {
                Type *key_type = call->arguments[0]->expr_type;
                Type *default_type = call->arguments[1]->expr_type;
                if (key_type == NULL || key_type->kind != TYPE_STRING)
                {
                    type_error(&method_name, "Environment.getBool requires a string argument for key");
                    return NULL;
                }
                if (default_type == NULL || default_type->kind != TYPE_BOOL)
                {
                    type_error(&method_name, "Environment.getBool requires a bool argument for default");
                    return NULL;
                }
                return ast_create_primitive_type(table->arena, TYPE_BOOL);
            }
            else
            {
                type_error(&method_name, "Environment.getBool requires 1 or 2 arguments");
                return NULL;
            }
        }
        else if (token_equals(method_name, "list"))
        {
            /* Environment.list(): str[][] - Get all as [name, value] pairs */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Environment.list takes no arguments");
                return NULL;
            }
            Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *pair_type = ast_create_array_type(table->arena, str_type);
            return ast_create_array_type(table->arena, pair_type);
        }
        else if (token_equals(method_name, "names"))
        {
            /* Environment.names(): str[] - Get all variable names */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Environment.names takes no arguments");
                return NULL;
            }
            Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            return ast_create_array_type(table->arena, str_type);
        }
        else if (token_equals(method_name, "all"))
        {
            /* Environment.all(): str[] - Alias for names() for backward compatibility */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Environment.all takes no arguments");
                return NULL;
            }
            Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            return ast_create_array_type(table->arena, str_type);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Environment static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Interceptor static methods - function interception for debugging/mocking */
    if (token_equals(type_name, "Interceptor"))
    {
        if (token_equals(method_name, "register"))
        {
            /* Interceptor.register(handler: fn(str, any[], fn(): any): any): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Interceptor.register requires exactly 1 argument (handler function)");
                return NULL;
            }
            Type *handler_type = call->arguments[0]->expr_type;
            if (handler_type == NULL || handler_type->kind != TYPE_FUNCTION)
            {
                type_error(&method_name, "Interceptor.register requires a function argument");
                return NULL;
            }
            /* TODO: Validate handler signature is fn(str, any[], fn(): any): any */
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "registerWhere"))
        {
            /* Interceptor.registerWhere(handler: fn(str, any[], fn(): any): any, pattern: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Interceptor.registerWhere requires exactly 2 arguments (handler, pattern)");
                return NULL;
            }
            Type *handler_type = call->arguments[0]->expr_type;
            Type *pattern_type = call->arguments[1]->expr_type;
            if (handler_type == NULL || handler_type->kind != TYPE_FUNCTION)
            {
                type_error(&method_name, "Interceptor.registerWhere first argument must be a function");
                return NULL;
            }
            if (pattern_type == NULL || pattern_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Interceptor.registerWhere second argument must be a pattern string");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "clearAll"))
        {
            /* Interceptor.clearAll(): void */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Interceptor.clearAll takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "isActive"))
        {
            /* Interceptor.isActive(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Interceptor.isActive takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "count"))
        {
            /* Interceptor.count(): int */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Interceptor.count takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_INT);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Interceptor static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    type_error(&type_name, "Unknown static type");
    return NULL;
}
