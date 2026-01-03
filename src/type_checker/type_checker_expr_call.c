/* ============================================================================
 * type_checker_expr_call.c - Call Expression Type Checking
 * ============================================================================
 * Type checking for function calls, including built-in functions (len),
 * regular user-defined function calls, and lambda argument type inference.
 * Extracted from type_checker_expr.c for modularity.
 * ============================================================================ */

#include "type_checker/type_checker_expr_call.h"
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

    // Note: Other array operations are method-style only:
    //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)

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
    if (callee_type->as.function.param_count != expr->as.call.arg_count)
    {
        argument_count_error(expr->token, func_name,
                            callee_type->as.function.param_count,
                            expr->as.call.arg_count);
        return NULL;
    }
    for (int i = 0; i < expr->as.call.arg_count; i++)
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
    DEBUG_VERBOSE("Returning function return type: %d", callee_type->as.function.return_type->kind);
    return callee_type->as.function.return_type;
}

/* ============================================================================
 * Array Method Type Checking
 * ============================================================================
 * Handles type checking for array method access (not calls).
 * Returns the function type for the method, or NULL if not an array method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

Type *type_check_array_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle array types */
    if (object_type->kind != TYPE_ARRAY)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* array.length property - returns int directly */
    if (strcmp(name, "length") == 0)
    {
        DEBUG_VERBOSE("Returning INT type for array length access");
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }

    /* array.push(elem) -> void */
    if (strcmp(name, "push") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array push method");
        return ast_create_function_type(table->arena, void_type, param_types, 1);
    }

    /* array.pop() -> element_type */
    if (strcmp(name, "pop") == 0)
    {
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array pop method");
        return ast_create_function_type(table->arena, object_type->as.array.element_type, param_types, 0);
    }

    /* array.clear() -> void */
    if (strcmp(name, "clear") == 0)
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array clear method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* array.concat(other_array) -> array */
    if (strcmp(name, "concat") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *param_array_type = ast_create_array_type(table->arena, element_type);
        Type *param_types[1] = {param_array_type};
        DEBUG_VERBOSE("Returning function type for array concat method");
        return ast_create_function_type(table->arena, object_type, param_types, 1);
    }

    /* array.indexOf(elem) -> int */
    if (strcmp(name, "indexOf") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array indexOf method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* array.contains(elem) -> bool */
    if (strcmp(name, "contains") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {element_type};
        DEBUG_VERBOSE("Returning function type for array contains method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* array.clone() -> array */
    if (strcmp(name, "clone") == 0)
    {
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array clone method");
        return ast_create_function_type(table->arena, object_type, param_types, 0);
    }

    /* array.join(separator) -> str */
    if (strcmp(name, "join") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for array join method");
        return ast_create_function_type(table->arena, string_type, param_types, 1);
    }

    /* array.reverse() -> void */
    if (strcmp(name, "reverse") == 0)
    {
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for array reverse method");
        return ast_create_function_type(table->arena, void_type, param_types, 0);
    }

    /* array.insert(elem, index) -> void */
    if (strcmp(name, "insert") == 0)
    {
        Type *element_type = object_type->as.array.element_type;
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *void_type = ast_create_primitive_type(table->arena, TYPE_VOID);
        Type *param_types[2] = {element_type, int_type};
        DEBUG_VERBOSE("Returning function type for array insert method");
        return ast_create_function_type(table->arena, void_type, param_types, 2);
    }

    /* array.remove(index) -> element_type */
    if (strcmp(name, "remove") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *element_type = object_type->as.array.element_type;
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for array remove method");
        return ast_create_function_type(table->arena, element_type, param_types, 1);
    }

    /* Byte array extension methods - only available on byte[] */
    if (object_type->as.array.element_type->kind == TYPE_BYTE)
    {
        /* byte[].toString() -> str */
        if (strcmp(name, "toString") == 0)
        {
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *param_types[] = {NULL};
            DEBUG_VERBOSE("Returning function type for byte array toString method");
            return ast_create_function_type(table->arena, string_type, param_types, 0);
        }

        /* byte[].toStringLatin1() -> str */
        if (strcmp(name, "toStringLatin1") == 0)
        {
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *param_types[] = {NULL};
            DEBUG_VERBOSE("Returning function type for byte array toStringLatin1 method");
            return ast_create_function_type(table->arena, string_type, param_types, 0);
        }

        /* byte[].toHex() -> str */
        if (strcmp(name, "toHex") == 0)
        {
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *param_types[] = {NULL};
            DEBUG_VERBOSE("Returning function type for byte array toHex method");
            return ast_create_function_type(table->arena, string_type, param_types, 0);
        }

        /* byte[].toBase64() -> str */
        if (strcmp(name, "toBase64") == 0)
        {
            Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
            Type *param_types[] = {NULL};
            DEBUG_VERBOSE("Returning function type for byte array toBase64 method");
            return ast_create_function_type(table->arena, string_type, param_types, 0);
        }
    }

    /* Not an array method */
    return NULL;
}

/* ============================================================================
 * String Method Type Checking
 * ============================================================================
 * Handles type checking for string method access (not calls).
 * Returns the function type for the method, or NULL if not a string method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

Type *type_check_string_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle string types */
    if (object_type->kind != TYPE_STRING)
    {
        return NULL;
    }

    const char *name = member_name.start;

    /* string.length property - returns int directly */
    if (strcmp(name, "length") == 0)
    {
        DEBUG_VERBOSE("Returning INT type for string length access");
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }

    /* string.substring(start, end) -> str */
    if (strcmp(name, "substring") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[2] = {int_type, int_type};
        DEBUG_VERBOSE("Returning function type for string substring method");
        return ast_create_function_type(table->arena, string_type, param_types, 2);
    }

    /* string.regionEquals(start, length, other) -> bool */
    if (strcmp(name, "regionEquals") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[3] = {int_type, int_type, string_type};
        DEBUG_VERBOSE("Returning function type for string regionEquals method");
        return ast_create_function_type(table->arena, bool_type, param_types, 3);
    }

    /* string.indexOf(substr) -> int */
    if (strcmp(name, "indexOf") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string indexOf method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* string.split(delimiter) -> str[] */
    if (strcmp(name, "split") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string split method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 1);
    }

    /* string.trim() -> str */
    if (strcmp(name, "trim") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string trim method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* string.toUpper() -> str */
    if (strcmp(name, "toUpper") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toUpper method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* string.toLower() -> str */
    if (strcmp(name, "toLower") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toLower method");
        return ast_create_function_type(table->arena, string_type, param_types, 0);
    }

    /* string.startsWith(prefix) -> bool */
    if (strcmp(name, "startsWith") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string startsWith method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* string.endsWith(suffix) -> bool */
    if (strcmp(name, "endsWith") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string endsWith method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* string.contains(substr) -> bool */
    if (strcmp(name, "contains") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string contains method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* string.replace(old, new) -> str */
    if (strcmp(name, "replace") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[2] = {string_type, string_type};
        DEBUG_VERBOSE("Returning function type for string replace method");
        return ast_create_function_type(table->arena, string_type, param_types, 2);
    }

    /* string.charAt(index) -> char */
    if (strcmp(name, "charAt") == 0)
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *char_type = ast_create_primitive_type(table->arena, TYPE_CHAR);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for string charAt method");
        return ast_create_function_type(table->arena, char_type, param_types, 1);
    }

    /* string.toBytes() -> byte[] (UTF-8 encoding) */
    if (strcmp(name, "toBytes") == 0)
    {
        Type *byte_type = ast_create_primitive_type(table->arena, TYPE_BYTE);
        Type *byte_array_type = ast_create_array_type(table->arena, byte_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string toBytes method");
        return ast_create_function_type(table->arena, byte_array_type, param_types, 0);
    }

    /* string.splitWhitespace() -> str[] */
    if (strcmp(name, "splitWhitespace") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string splitWhitespace method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 0);
    }

    /* string.splitLines() -> str[] */
    if (strcmp(name, "splitLines") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *str_array_type = ast_create_array_type(table->arena, string_type);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string splitLines method");
        return ast_create_function_type(table->arena, str_array_type, param_types, 0);
    }

    /* string.isBlank() -> bool */
    if (strcmp(name, "isBlank") == 0)
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for string isBlank method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* string.append(other) -> str */
    if (strcmp(name, "append") == 0)
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for string append method");
        return ast_create_function_type(table->arena, string_type, param_types, 1);
    }

    /* Not a string method */
    return NULL;
}

/* ============================================================================
 * TextFile Method Type Checking
 * ============================================================================
 * Handles type checking for TextFile method access (not calls).
 * Returns the function type for the method, or NULL if not a TextFile method.
 * Caller should handle errors for invalid members.
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
 * ============================================================================
 * Handles type checking for BinaryFile method access (not calls).
 * Returns the function type for the method, or NULL if not a BinaryFile method.
 * Caller should handle errors for invalid members.
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

/* ============================================================================
 * Time Method Type Checking
 * ============================================================================
 * Handles type checking for Time method access (not calls).
 * Returns the function type for the method, or NULL if not a Time method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

Type *type_check_time_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle Time types */
    if (object_type->kind != TYPE_TIME)
    {
        return NULL;
    }

    /* Time epoch getter methods */

    /* time.millis() -> int */
    if (token_equals(member_name, "millis"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time millis method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.seconds() -> int */
    if (token_equals(member_name, "seconds"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time seconds method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* Time date component getter methods */

    /* time.year() -> int */
    if (token_equals(member_name, "year"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time year method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.month() -> int */
    if (token_equals(member_name, "month"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time month method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.day() -> int */
    if (token_equals(member_name, "day"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time day method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* Time time component getter methods */

    /* time.hour() -> int */
    if (token_equals(member_name, "hour"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time hour method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.minute() -> int */
    if (token_equals(member_name, "minute"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time minute method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.second() -> int */
    if (token_equals(member_name, "second"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time second method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* time.weekday() -> int */
    if (token_equals(member_name, "weekday"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time weekday method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* Time formatting methods */

    /* time.format(pattern) -> str */
    if (token_equals(member_name, "format"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for Time format method");
        return ast_create_function_type(table->arena, str_type, param_types, 1);
    }

    /* time.toIso() -> str */
    if (token_equals(member_name, "toIso"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toIso method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* time.toDate() -> Date */
    if (token_equals(member_name, "toDate"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toDate method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* time.toTime() -> str */
    if (token_equals(member_name, "toTime"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Time toTime method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* Time arithmetic methods */

    /* time.add(millis) -> Time */
    if (token_equals(member_name, "add"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time add method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.addSeconds(seconds) -> Time */
    if (token_equals(member_name, "addSeconds"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time addSeconds method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.addMinutes(minutes) -> Time */
    if (token_equals(member_name, "addMinutes"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time addMinutes method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.addHours(hours) -> Time */
    if (token_equals(member_name, "addHours"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time addHours method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.addDays(days) -> Time */
    if (token_equals(member_name, "addDays"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Time addDays method");
        return ast_create_function_type(table->arena, time_type, param_types, 1);
    }

    /* time.diff(other) -> int */
    if (token_equals(member_name, "diff"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[1] = {time_type};
        DEBUG_VERBOSE("Returning function type for Time diff method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* Time comparison methods */

    /* time.isBefore(other) -> bool */
    if (token_equals(member_name, "isBefore"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[1] = {time_type};
        DEBUG_VERBOSE("Returning function type for Time isBefore method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* time.isAfter(other) -> bool */
    if (token_equals(member_name, "isAfter"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[1] = {time_type};
        DEBUG_VERBOSE("Returning function type for Time isAfter method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* time.equals(other) -> bool */
    if (token_equals(member_name, "equals"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[1] = {time_type};
        DEBUG_VERBOSE("Returning function type for Time equals method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* Not a Time method */
    return NULL;
}

/* ============================================================================
 * Date Method Type Checking
 * ============================================================================
 * Handles type checking for Date method access (not calls).
 * Returns the function type for the method, or NULL if not a Date method.
 * Caller should handle errors for invalid members.
 * ============================================================================ */

Type *type_check_date_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle Date types */
    if (object_type->kind != TYPE_DATE)
    {
        return NULL;
    }

    /* Date getter methods returning int */

    /* date.year() -> int */
    if (token_equals(member_name, "year"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date year method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.month() -> int */
    if (token_equals(member_name, "month"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date month method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.day() -> int */
    if (token_equals(member_name, "day"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date day method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.weekday() -> int */
    if (token_equals(member_name, "weekday"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date weekday method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.dayOfYear() -> int */
    if (token_equals(member_name, "dayOfYear"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date dayOfYear method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.epochDays() -> int */
    if (token_equals(member_name, "epochDays"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date epochDays method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* date.daysInMonth() -> int */
    if (token_equals(member_name, "daysInMonth"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date daysInMonth method");
        return ast_create_function_type(table->arena, int_type, param_types, 0);
    }

    /* Date getter methods returning bool */

    /* date.isLeapYear() -> bool */
    if (token_equals(member_name, "isLeapYear"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date isLeapYear method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* date.isWeekend() -> bool */
    if (token_equals(member_name, "isWeekend"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date isWeekend method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* date.isWeekday() -> bool */
    if (token_equals(member_name, "isWeekday"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date isWeekday method");
        return ast_create_function_type(table->arena, bool_type, param_types, 0);
    }

    /* Date formatting methods */

    /* date.format(pattern) -> str */
    if (token_equals(member_name, "format"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[1] = {string_type};
        DEBUG_VERBOSE("Returning function type for Date format method");
        return ast_create_function_type(table->arena, str_type, param_types, 1);
    }

    /* date.toIso() -> str */
    if (token_equals(member_name, "toIso"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date toIso method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* date.toString() -> str */
    if (token_equals(member_name, "toString"))
    {
        Type *str_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date toString method");
        return ast_create_function_type(table->arena, str_type, param_types, 0);
    }

    /* Date arithmetic methods returning Date */

    /* date.addDays(days) -> Date */
    if (token_equals(member_name, "addDays"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Date addDays method");
        return ast_create_function_type(table->arena, date_type, param_types, 1);
    }

    /* date.addWeeks(weeks) -> Date */
    if (token_equals(member_name, "addWeeks"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Date addWeeks method");
        return ast_create_function_type(table->arena, date_type, param_types, 1);
    }

    /* date.addMonths(months) -> Date */
    if (token_equals(member_name, "addMonths"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Date addMonths method");
        return ast_create_function_type(table->arena, date_type, param_types, 1);
    }

    /* date.addYears(years) -> Date */
    if (token_equals(member_name, "addYears"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *param_types[1] = {int_type};
        DEBUG_VERBOSE("Returning function type for Date addYears method");
        return ast_create_function_type(table->arena, date_type, param_types, 1);
    }

    /* date.diffDays(other) -> int */
    if (token_equals(member_name, "diffDays"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[1] = {date_type};
        DEBUG_VERBOSE("Returning function type for Date diffDays method");
        return ast_create_function_type(table->arena, int_type, param_types, 1);
    }

    /* Date boundary methods returning Date */

    /* date.startOfMonth() -> Date */
    if (token_equals(member_name, "startOfMonth"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date startOfMonth method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* date.endOfMonth() -> Date */
    if (token_equals(member_name, "endOfMonth"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date endOfMonth method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* date.startOfYear() -> Date */
    if (token_equals(member_name, "startOfYear"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date startOfYear method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* date.endOfYear() -> Date */
    if (token_equals(member_name, "endOfYear"))
    {
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date endOfYear method");
        return ast_create_function_type(table->arena, date_type, param_types, 0);
    }

    /* Date comparison methods */

    /* date.isBefore(other) -> bool */
    if (token_equals(member_name, "isBefore"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[1] = {date_type};
        DEBUG_VERBOSE("Returning function type for Date isBefore method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* date.isAfter(other) -> bool */
    if (token_equals(member_name, "isAfter"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[1] = {date_type};
        DEBUG_VERBOSE("Returning function type for Date isAfter method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* date.equals(other) -> bool */
    if (token_equals(member_name, "equals"))
    {
        Type *bool_type = ast_create_primitive_type(table->arena, TYPE_BOOL);
        Type *date_type = ast_create_primitive_type(table->arena, TYPE_DATE);
        Type *param_types[1] = {date_type};
        DEBUG_VERBOSE("Returning function type for Date equals method");
        return ast_create_function_type(table->arena, bool_type, param_types, 1);
    }

    /* Date/Time conversion */

    /* date.toTime() -> Time */
    if (token_equals(member_name, "toTime"))
    {
        Type *time_type = ast_create_primitive_type(table->arena, TYPE_TIME);
        Type *param_types[] = {NULL};
        DEBUG_VERBOSE("Returning function type for Date toTime method");
        return ast_create_function_type(table->arena, time_type, param_types, 0);
    }

    /* Not a Date method */
    return NULL;
}

/* ============================================================================
 * Process Property Type Checking
 * ============================================================================
 * Handles type checking for Process property access (not calls).
 * Process has three properties: exitCode (int), stdout (str), stderr (str).
 * Returns the property type, or NULL if not a Process property.
 * ============================================================================ */

Type *type_check_process_method(Expr *expr, Type *object_type, Token member_name, SymbolTable *table)
{
    (void)expr; /* Reserved for future use (e.g., error location) */

    /* Only handle Process types */
    if (object_type->kind != TYPE_PROCESS)
    {
        return NULL;
    }

    /* Process properties (accessed as member, return value directly) */

    /* process.exitCode -> int */
    if (token_equals(member_name, "exitCode"))
    {
        Type *int_type = ast_create_primitive_type(table->arena, TYPE_INT);
        DEBUG_VERBOSE("Returning int type for Process exitCode property");
        return int_type;
    }

    /* process.stdout -> str */
    if (token_equals(member_name, "stdout"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for Process stdout property");
        return string_type;
    }

    /* process.stderr -> str */
    if (token_equals(member_name, "stderr"))
    {
        Type *string_type = ast_create_primitive_type(table->arena, TYPE_STRING);
        DEBUG_VERBOSE("Returning string type for Process stderr property");
        return string_type;
    }

    /* Not a Process property */
    return NULL;
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
                if (args_type == NULL || args_type->kind != TYPE_ARRAY ||
                    args_type->as.array.element_type == NULL ||
                    args_type->as.array.element_type->kind != TYPE_STRING)
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

    type_error(&type_name, "Unknown static type");
    return NULL;
}
