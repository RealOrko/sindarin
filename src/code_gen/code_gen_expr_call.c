/**
 * code_gen_expr_call.c - Code generation for call expressions
 *
 * This is the main dispatcher for generating C code from function calls
 * and method calls. It delegates to specialized handlers for different
 * object types (arrays, strings, files, etc.) defined in:
 * - code_gen_expr_call_array.c
 * - code_gen_expr_call_string.c
 * - code_gen_expr_call_file.c
 * - code_gen_expr_call_time.c
 * - code_gen_expr_call_random.c
 * - code_gen_expr_call_uuid.c
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

bool expression_produces_temp(Expr *expr)
{
    DEBUG_VERBOSE("Entering expression_produces_temp");
    if (expr->expr_type->kind != TYPE_STRING)
        return false;
    switch (expr->type)
    {
    case EXPR_VARIABLE:
    case EXPR_ASSIGN:
    case EXPR_INDEX_ASSIGN:
    case EXPR_LITERAL:
        return false;
    case EXPR_BINARY:
    case EXPR_CALL:
    case EXPR_INTERPOLATED:
        return true;
    default:
        return false;
    }
}

char *code_gen_call_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_call_expression");
    CallExpr *call = &expr->as.call;

    if (call->callee->type == EXPR_MEMBER) {
        MemberExpr *member = &call->callee->as.member;
        char *member_name_str = get_var_name(gen->arena, member->member_name);
        Type *object_type = member->object->expr_type;

        /* Check for namespace function call (namespace.function).
         * If the object has no type (expr_type is NULL) and is a variable,
         * this is a namespaced function call. Type checker already validated
         * this, so we can safely emit the function call directly using the
         * member name as the function name. */
        if (object_type == NULL && member->object->type == EXPR_VARIABLE)
        {
            /* Lookup the function in the namespace to check if it's shared */
            Token ns_name = member->object->as.variable.name;
            Symbol *func_sym = symbol_table_lookup_in_namespace(gen->symbol_table, ns_name, member->member_name);
            bool callee_is_shared = (func_sym != NULL && func_sym->func_mod == FUNC_SHARED);

            /* Generate arguments */
            char **arg_strs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
            for (int i = 0; i < call->arg_count; i++)
            {
                arg_strs[i] = code_gen_expression(gen, call->arguments[i]);
            }

            /* Build args list - prepend arena if shared function */
            char *args_list;
            if (callee_is_shared && gen->current_arena_var != NULL)
            {
                args_list = arena_strdup(gen->arena, gen->current_arena_var);
            }
            else if (callee_is_shared)
            {
                args_list = arena_strdup(gen->arena, "NULL");
            }
            else
            {
                args_list = arena_strdup(gen->arena, "");
            }

            for (int i = 0; i < call->arg_count; i++)
            {
                if (args_list[0] == '\0')
                {
                    args_list = arg_strs[i];
                }
                else
                {
                    args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_strs[i]);
                }
            }

            /* Emit function call using the member name (e.g., "add" for math.add) */
            return arena_sprintf(gen->arena, "%s(%s)", member_name_str, args_list);
        }

        char *result = NULL;

        /* Dispatch to type-specific handlers (modular code generation)
         * Each handler returns NULL if it doesn't handle the method,
         * allowing fallback to the original inline implementations.
         */
        switch (object_type->kind) {
            case TYPE_ARRAY: {
                Type *element_type = object_type->as.array.element_type;
                result = code_gen_array_method_call(gen, expr, member_name_str,
                    member->object, element_type, call->arg_count, call->arguments);
                if (result) return result;
                break;
            }
            case TYPE_STRING: {
                bool object_is_temp = expression_produces_temp(member->object);
                result = code_gen_string_method_call(gen, member_name_str,
                    member->object, object_is_temp, call->arg_count, call->arguments);
                if (result) return result;
                break;
            }
            case TYPE_TEXT_FILE: {
                result = code_gen_text_file_method_call(gen, member_name_str,
                    member->object, call->arg_count, call->arguments);
                if (result) return result;
                break;
            }
            case TYPE_BINARY_FILE: {
                result = code_gen_binary_file_method_call(gen, member_name_str,
                    member->object, call->arg_count, call->arguments);
                if (result) return result;
                break;
            }
            case TYPE_TIME: {
                result = code_gen_time_method_call(gen, member_name_str,
                    member->object, call->arg_count, call->arguments);
                if (result) return result;
                break;
            }
            case TYPE_RANDOM: {
                result = code_gen_random_method_call(gen, expr, member_name_str,
                    member->object, call->arg_count, call->arguments);
                if (result) return result;
                break;
            }
            case TYPE_UUID: {
                result = code_gen_uuid_method_call(gen, expr, member_name_str,
                    member->object, call->arg_count, call->arguments);
                if (result) return result;
                break;
            }
            default:
                break;
        }

        /* Fallback to original inline implementations for methods not yet
         * handled by the modular handlers (e.g., append for strings).
         */
        if (object_type->kind == TYPE_ARRAY) {
            /* Array methods - fallback for methods not in modular handler */
            char *object_str = code_gen_expression(gen, member->object);
            Type *element_type = object_type->as.array.element_type;

            // Handle push(element)
            if (strcmp(member_name_str, "push") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                Type *arg_type = call->arguments[0]->expr_type;

                if (!ast_type_equals(element_type, arg_type)) {
                    fprintf(stderr, "Error: Argument type does not match array element type\n");
                    exit(1);
                }

                const char *push_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        push_func = "rt_array_push_long";
                        break;
                    case TYPE_DOUBLE:
                        push_func = "rt_array_push_double";
                        break;
                    case TYPE_CHAR:
                        push_func = "rt_array_push_char";
                        break;
                    case TYPE_STRING:
                        push_func = "rt_array_push_string";
                        break;
                    case TYPE_BOOL:
                        push_func = "rt_array_push_bool";
                        break;
                    case TYPE_BYTE:
                        push_func = "rt_array_push_byte";
                        break;
                    case TYPE_FUNCTION:
                    case TYPE_ARRAY:
                        push_func = "rt_array_push_ptr";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for push\n");
                        exit(1);
                }
                // push returns new array pointer, assign back to variable if object is a variable
                // For pointer types (function/array), we need to cast to void**
                if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
                    if (member->object->type == EXPR_VARIABLE) {
                        return arena_sprintf(gen->arena, "(%s = (void *)%s(%s, (void **)%s, (void *)%s))", object_str, push_func, ARENA_VAR(gen), object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena, "(void *)%s(%s, (void **)%s, (void *)%s)", push_func, ARENA_VAR(gen), object_str, arg_str);
                }
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))", object_str, push_func, ARENA_VAR(gen), object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", push_func, ARENA_VAR(gen), object_str, arg_str);
            }

            // Handle clear()
            if (strcmp(member_name_str, "clear") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_array_clear(%s)", object_str);
            }

            // Handle pop()
            if (strcmp(member_name_str, "pop") == 0 && call->arg_count == 0) {
                const char *pop_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        pop_func = "rt_array_pop_long";
                        break;
                    case TYPE_DOUBLE:
                        pop_func = "rt_array_pop_double";
                        break;
                    case TYPE_CHAR:
                        pop_func = "rt_array_pop_char";
                        break;
                    case TYPE_STRING:
                        pop_func = "rt_array_pop_string";
                        break;
                    case TYPE_BOOL:
                        pop_func = "rt_array_pop_bool";
                        break;
                    case TYPE_BYTE:
                        pop_func = "rt_array_pop_byte";
                        break;
                    case TYPE_FUNCTION:
                    case TYPE_ARRAY:
                        pop_func = "rt_array_pop_ptr";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for pop\n");
                        exit(1);
                }
                // For pointer types (function/array), we need to cast the result
                if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
                    const char *elem_type_str = get_c_type(gen->arena, element_type);
                    return arena_sprintf(gen->arena, "(%s)%s((void **)%s)", elem_type_str, pop_func, object_str);
                }
                return arena_sprintf(gen->arena, "%s(%s)", pop_func, object_str);
            }

            // Handle concat(other_array)
            if (strcmp(member_name_str, "concat") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                const char *concat_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        concat_func = "rt_array_concat_long";
                        break;
                    case TYPE_DOUBLE:
                        concat_func = "rt_array_concat_double";
                        break;
                    case TYPE_CHAR:
                        concat_func = "rt_array_concat_char";
                        break;
                    case TYPE_STRING:
                        concat_func = "rt_array_concat_string";
                        break;
                    case TYPE_BOOL:
                        concat_func = "rt_array_concat_bool";
                        break;
                    case TYPE_BYTE:
                        concat_func = "rt_array_concat_byte";
                        break;
                    case TYPE_FUNCTION:
                    case TYPE_ARRAY:
                        concat_func = "rt_array_concat_ptr";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for concat\n");
                        exit(1);
                }
                // concat returns a new array, doesn't modify the original
                // For pointer types (function/array), we need to cast
                if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
                    const char *elem_type_str = get_c_type(gen->arena, element_type);
                    return arena_sprintf(gen->arena, "(%s *)%s(%s, (void **)%s, (void **)%s)", elem_type_str, concat_func, ARENA_VAR(gen), object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", concat_func, ARENA_VAR(gen), object_str, arg_str);
            }

            // Handle indexOf(element)
            if (strcmp(member_name_str, "indexOf") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                const char *indexof_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        indexof_func = "rt_array_indexOf_long";
                        break;
                    case TYPE_DOUBLE:
                        indexof_func = "rt_array_indexOf_double";
                        break;
                    case TYPE_CHAR:
                        indexof_func = "rt_array_indexOf_char";
                        break;
                    case TYPE_STRING:
                        indexof_func = "rt_array_indexOf_string";
                        break;
                    case TYPE_BOOL:
                        indexof_func = "rt_array_indexOf_bool";
                        break;
                    case TYPE_BYTE:
                        indexof_func = "rt_array_indexOf_byte";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for indexOf\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", indexof_func, object_str, arg_str);
            }

            // Handle contains(element)
            if (strcmp(member_name_str, "contains") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                const char *contains_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        contains_func = "rt_array_contains_long";
                        break;
                    case TYPE_DOUBLE:
                        contains_func = "rt_array_contains_double";
                        break;
                    case TYPE_CHAR:
                        contains_func = "rt_array_contains_char";
                        break;
                    case TYPE_STRING:
                        contains_func = "rt_array_contains_string";
                        break;
                    case TYPE_BOOL:
                        contains_func = "rt_array_contains_bool";
                        break;
                    case TYPE_BYTE:
                        contains_func = "rt_array_contains_byte";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for contains\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", contains_func, object_str, arg_str);
            }

            // Handle clone()
            if (strcmp(member_name_str, "clone") == 0 && call->arg_count == 0) {
                const char *clone_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        clone_func = "rt_array_clone_long";
                        break;
                    case TYPE_DOUBLE:
                        clone_func = "rt_array_clone_double";
                        break;
                    case TYPE_CHAR:
                        clone_func = "rt_array_clone_char";
                        break;
                    case TYPE_STRING:
                        clone_func = "rt_array_clone_string";
                        break;
                    case TYPE_BOOL:
                        clone_func = "rt_array_clone_bool";
                        break;
                    case TYPE_BYTE:
                        clone_func = "rt_array_clone_byte";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for clone\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", clone_func, ARENA_VAR(gen), object_str);
            }

            // Handle join(separator)
            if (strcmp(member_name_str, "join") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                const char *join_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        join_func = "rt_array_join_long";
                        break;
                    case TYPE_DOUBLE:
                        join_func = "rt_array_join_double";
                        break;
                    case TYPE_CHAR:
                        join_func = "rt_array_join_char";
                        break;
                    case TYPE_STRING:
                        join_func = "rt_array_join_string";
                        break;
                    case TYPE_BOOL:
                        join_func = "rt_array_join_bool";
                        break;
                    case TYPE_BYTE:
                        join_func = "rt_array_join_byte";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for join\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", join_func, ARENA_VAR(gen), object_str, arg_str);
            }

            // Handle reverse() - in-place reverse
            if (strcmp(member_name_str, "reverse") == 0 && call->arg_count == 0) {
                const char *rev_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        rev_func = "rt_array_rev_long";
                        break;
                    case TYPE_DOUBLE:
                        rev_func = "rt_array_rev_double";
                        break;
                    case TYPE_CHAR:
                        rev_func = "rt_array_rev_char";
                        break;
                    case TYPE_STRING:
                        rev_func = "rt_array_rev_string";
                        break;
                    case TYPE_BOOL:
                        rev_func = "rt_array_rev_bool";
                        break;
                    case TYPE_BYTE:
                        rev_func = "rt_array_rev_byte";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for reverse\n");
                        exit(1);
                }
                // reverse in-place: assign result back to variable
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s))", object_str, rev_func, ARENA_VAR(gen), object_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", rev_func, ARENA_VAR(gen), object_str);
            }

            // Handle insert(elem, index)
            if (strcmp(member_name_str, "insert") == 0 && call->arg_count == 2) {
                char *elem_str = code_gen_expression(gen, call->arguments[0]);
                char *idx_str = code_gen_expression(gen, call->arguments[1]);
                const char *ins_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        ins_func = "rt_array_ins_long";
                        break;
                    case TYPE_DOUBLE:
                        ins_func = "rt_array_ins_double";
                        break;
                    case TYPE_CHAR:
                        ins_func = "rt_array_ins_char";
                        break;
                    case TYPE_STRING:
                        ins_func = "rt_array_ins_string";
                        break;
                    case TYPE_BOOL:
                        ins_func = "rt_array_ins_bool";
                        break;
                    case TYPE_BYTE:
                        ins_func = "rt_array_ins_byte";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for insert\n");
                        exit(1);
                }
                // insert in-place: assign result back to variable
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s, %s))", object_str, ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s)", ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
            }

            // Handle remove(index)
            if (strcmp(member_name_str, "remove") == 0 && call->arg_count == 1) {
                char *idx_str = code_gen_expression(gen, call->arguments[0]);
                const char *rem_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        rem_func = "rt_array_rem_long";
                        break;
                    case TYPE_DOUBLE:
                        rem_func = "rt_array_rem_double";
                        break;
                    case TYPE_CHAR:
                        rem_func = "rt_array_rem_char";
                        break;
                    case TYPE_STRING:
                        rem_func = "rt_array_rem_string";
                        break;
                    case TYPE_BOOL:
                        rem_func = "rt_array_rem_bool";
                        break;
                    case TYPE_BYTE:
                        rem_func = "rt_array_rem_byte";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for remove\n");
                        exit(1);
                }
                // remove in-place: assign result back to variable
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))", object_str, rem_func, ARENA_VAR(gen), object_str, idx_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", rem_func, ARENA_VAR(gen), object_str, idx_str);
            }

            /* Byte array extension methods - only for byte[] */
            if (element_type->kind == TYPE_BYTE) {
                // Handle toString() - UTF-8 decoding
                if (strcmp(member_name_str, "toString") == 0 && call->arg_count == 0) {
                    return arena_sprintf(gen->arena, "rt_byte_array_to_string(%s, %s)",
                        ARENA_VAR(gen), object_str);
                }

                // Handle toStringLatin1() - Latin-1/ISO-8859-1 decoding
                if (strcmp(member_name_str, "toStringLatin1") == 0 && call->arg_count == 0) {
                    return arena_sprintf(gen->arena, "rt_byte_array_to_string_latin1(%s, %s)",
                        ARENA_VAR(gen), object_str);
                }

                // Handle toHex() - hexadecimal encoding
                if (strcmp(member_name_str, "toHex") == 0 && call->arg_count == 0) {
                    return arena_sprintf(gen->arena, "rt_byte_array_to_hex(%s, %s)",
                        ARENA_VAR(gen), object_str);
                }

                // Handle toBase64() - Base64 encoding
                if (strcmp(member_name_str, "toBase64") == 0 && call->arg_count == 0) {
                    return arena_sprintf(gen->arena, "rt_byte_array_to_base64(%s, %s)",
                        ARENA_VAR(gen), object_str);
                }
            }
        }

        /* Handle string methods
         * NOTE: These methods are also implemented in code_gen_expr_call_string.c
         * for modular code generation. The implementations here remain for
         * backward compatibility during the transition.
         */
        if (object_type->kind == TYPE_STRING) {
            char *object_str = code_gen_expression(gen, member->object);
            bool object_is_temp = expression_produces_temp(member->object);

            // Helper macro-like pattern for string methods that return strings
            // If object is temp, we need to capture it, call method, free it, return result
            // Skip freeing in arena context - arena handles cleanup
            #define STRING_METHOD_RETURNING_STRING(method_call) \
                do { \
                    if (object_is_temp) { \
                        if (gen->current_arena_var != NULL) { \
                            return arena_sprintf(gen->arena, \
                                "({ char *_obj_tmp = %s; char *_res = %s; _res; })", \
                                object_str, method_call); \
                        } else { \
                            return arena_sprintf(gen->arena, \
                                "({ char *_obj_tmp = %s; char *_res = %s; rt_free_string(_obj_tmp); _res; })", \
                                object_str, method_call); \
                        } \
                    } else { \
                        return arena_sprintf(gen->arena, "%s", method_call); \
                    } \
                } while(0)

            // Handle substring(start, end) - returns string
            if (strcmp(member_name_str, "substring") == 0 && call->arg_count == 2) {
                char *start_str = code_gen_expression(gen, call->arguments[0]);
                char *end_str = code_gen_expression(gen, call->arguments[1]);
                char *method_call = arena_sprintf(gen->arena, "rt_str_substring(%s, %s, %s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, start_str, end_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle regionEquals(start, end, pattern) - non-allocating comparison, returns bool
            if (strcmp(member_name_str, "regionEquals") == 0 && call->arg_count == 3) {
                char *start_str = code_gen_expression(gen, call->arguments[0]);
                char *end_str = code_gen_expression(gen, call->arguments[1]);
                char *pattern_str = code_gen_expression(gen, call->arguments[2]);
                if (object_is_temp) {
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_region_equals(_obj_tmp, %s, %s, %s); _res; })",
                        object_str, start_str, end_str, pattern_str);
                }
                return arena_sprintf(gen->arena, "rt_str_region_equals(%s, %s, %s, %s)",
                    object_str, start_str, end_str, pattern_str);
            }

            // Handle indexOf(search) - returns int, no string cleanup needed for result
            if (strcmp(member_name_str, "indexOf") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; long _res = rt_str_indexOf(_obj_tmp, %s); _res; })",
                            object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; long _res = rt_str_indexOf(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_indexOf(%s, %s)", object_str, arg_str);
            }

            // Handle split(delimiter) - returns array, object cleanup needed
            if (strcmp(member_name_str, "split") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; char **_res = rt_str_split(%s, _obj_tmp, %s); _res; })",
                            object_str, ARENA_VAR(gen), arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; char **_res = rt_str_split(%s, _obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, ARENA_VAR(gen), arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_split(%s, %s, %s)", ARENA_VAR(gen), object_str, arg_str);
            }

            // Handle trim() - returns string
            if (strcmp(member_name_str, "trim") == 0 && call->arg_count == 0) {
                char *method_call = arena_sprintf(gen->arena, "rt_str_trim(%s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle toUpper() - returns string
            if (strcmp(member_name_str, "toUpper") == 0 && call->arg_count == 0) {
                char *method_call = arena_sprintf(gen->arena, "rt_str_toUpper(%s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle toLower() - returns string
            if (strcmp(member_name_str, "toLower") == 0 && call->arg_count == 0) {
                char *method_call = arena_sprintf(gen->arena, "rt_str_toLower(%s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle startsWith(prefix) - returns bool
            if (strcmp(member_name_str, "startsWith") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; int _res = rt_str_startsWith(_obj_tmp, %s); _res; })",
                            object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_startsWith(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_startsWith(%s, %s)", object_str, arg_str);
            }

            // Handle endsWith(suffix) - returns bool
            if (strcmp(member_name_str, "endsWith") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; int _res = rt_str_endsWith(_obj_tmp, %s); _res; })",
                            object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_endsWith(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_endsWith(%s, %s)", object_str, arg_str);
            }

            // Handle contains(search) - returns bool
            if (strcmp(member_name_str, "contains") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; int _res = rt_str_contains(_obj_tmp, %s); _res; })",
                            object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_contains(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_contains(%s, %s)", object_str, arg_str);
            }

            // Handle replace(old, new) - returns string
            if (strcmp(member_name_str, "replace") == 0 && call->arg_count == 2) {
                char *old_str = code_gen_expression(gen, call->arguments[0]);
                char *new_str = code_gen_expression(gen, call->arguments[1]);
                char *method_call = arena_sprintf(gen->arena, "rt_str_replace(%s, %s, %s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, old_str, new_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle charAt(index) - returns char
            if (strcmp(member_name_str, "charAt") == 0 && call->arg_count == 1) {
                char *index_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; char _res = (char)rt_str_charAt(_obj_tmp, %s); _res; })",
                            object_str, index_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; char _res = (char)rt_str_charAt(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, index_str);
                }
                return arena_sprintf(gen->arena, "(char)rt_str_charAt(%s, %s)", object_str, index_str);
            }

            // Handle toBytes() - returns byte array (UTF-8 encoding)
            if (strcmp(member_name_str, "toBytes") == 0 && call->arg_count == 0) {
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; unsigned char *_res = rt_string_to_bytes(%s, _obj_tmp); _res; })",
                            object_str, ARENA_VAR(gen));
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; unsigned char *_res = rt_string_to_bytes(%s, _obj_tmp); rt_free_string(_obj_tmp); _res; })",
                        object_str, ARENA_VAR(gen));
                }
                return arena_sprintf(gen->arena, "rt_string_to_bytes(%s, %s)", ARENA_VAR(gen), object_str);
            }

            // Handle splitWhitespace() - returns string array
            if (strcmp(member_name_str, "splitWhitespace") == 0 && call->arg_count == 0) {
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; char **_res = rt_str_split_whitespace(%s, _obj_tmp); _res; })",
                            object_str, ARENA_VAR(gen));
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; char **_res = rt_str_split_whitespace(%s, _obj_tmp); rt_free_string(_obj_tmp); _res; })",
                        object_str, ARENA_VAR(gen));
                }
                return arena_sprintf(gen->arena, "rt_str_split_whitespace(%s, %s)", ARENA_VAR(gen), object_str);
            }

            // Handle splitLines() - returns string array
            if (strcmp(member_name_str, "splitLines") == 0 && call->arg_count == 0) {
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; char **_res = rt_str_split_lines(%s, _obj_tmp); _res; })",
                            object_str, ARENA_VAR(gen));
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; char **_res = rt_str_split_lines(%s, _obj_tmp); rt_free_string(_obj_tmp); _res; })",
                        object_str, ARENA_VAR(gen));
                }
                return arena_sprintf(gen->arena, "rt_str_split_lines(%s, %s)", ARENA_VAR(gen), object_str);
            }

            // Handle isBlank() - returns bool
            if (strcmp(member_name_str, "isBlank") == 0 && call->arg_count == 0) {
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; int _res = rt_str_is_blank(_obj_tmp); _res; })",
                            object_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_is_blank(_obj_tmp); rt_free_string(_obj_tmp); _res; })",
                        object_str);
                }
                return arena_sprintf(gen->arena, "rt_str_is_blank(%s)", object_str);
            }

            // Handle append(str) - appends to mutable string, returns new string pointer
            if (strcmp(member_name_str, "append") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                Type *arg_type = call->arguments[0]->expr_type;

                if (arg_type->kind != TYPE_STRING) {
                    fprintf(stderr, "Error: append() argument must be a string\n");
                    exit(1);
                }

                // First ensure the string is mutable, then append.
                // rt_string_ensure_mutable_inline uses fast inlined check when already mutable.
                // rt_string_append returns potentially new pointer, assign back if variable.
                // IMPORTANT: Use the function's main arena (__arena_1__), not the loop arena,
                // because strings need to outlive the loop iteration.
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena,
                        "(%s = rt_string_append(rt_string_ensure_mutable_inline(__arena_1__, %s), %s))",
                        object_str, object_str, arg_str);
                }
                return arena_sprintf(gen->arena,
                    "rt_string_append(rt_string_ensure_mutable_inline(__arena_1__, %s), %s)",
                    object_str, arg_str);
            }

            #undef STRING_METHOD_RETURNING_STRING
        }

        /* TextFile instance methods
         * NOTE: These methods are also implemented in code_gen_expr_call_file.c
         * for modular code generation. The implementations here remain for
         * backward compatibility during the transition.
         */
        if (object_type->kind == TYPE_TEXT_FILE) {
            char *object_str = code_gen_expression(gen, member->object);

            /* readChar() -> rt_text_file_read_char(file) returns long (-1 on EOF) */
            if (strcmp(member_name_str, "readChar") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_read_char(%s)", object_str);
            }

            /* readWord() -> rt_text_file_read_word(arena, file) */
            if (strcmp(member_name_str, "readWord") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_read_word(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* readLine() -> rt_text_file_read_line(arena, file) */
            if (strcmp(member_name_str, "readLine") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_read_line(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* readAll() (instance) -> rt_text_file_instance_read_all(arena, file) */
            if (strcmp(member_name_str, "readAll") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_instance_read_all(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* readLines() -> rt_text_file_read_lines(arena, file) */
            if (strcmp(member_name_str, "readLines") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_read_lines(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* readInto(buffer) -> rt_text_file_read_into(file, buffer) */
            if (strcmp(member_name_str, "readInto") == 0 && call->arg_count == 1) {
                char *buffer_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_text_file_read_into(%s, %s)",
                    object_str, buffer_str);
            }

            /* close() -> rt_text_file_close(file) */
            if (strcmp(member_name_str, "close") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_close(%s)", object_str);
            }

            /* writeChar(ch) -> rt_text_file_write_char(file, ch) */
            if (strcmp(member_name_str, "writeChar") == 0 && call->arg_count == 1) {
                char *ch_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_text_file_write_char(%s, %s)",
                    object_str, ch_str);
            }

            /* write(text) -> rt_text_file_write(file, text) */
            if (strcmp(member_name_str, "write") == 0 && call->arg_count == 1) {
                char *text_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_text_file_write(%s, %s)",
                    object_str, text_str);
            }

            /* writeLine(text) -> rt_text_file_write_line(file, text) */
            if (strcmp(member_name_str, "writeLine") == 0 && call->arg_count == 1) {
                char *text_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_text_file_write_line(%s, %s)",
                    object_str, text_str);
            }

            /* print(text) -> rt_text_file_print(file, text) */
            if (strcmp(member_name_str, "print") == 0 && call->arg_count == 1) {
                char *text_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_text_file_print(%s, %s)",
                    object_str, text_str);
            }

            /* println(text) -> rt_text_file_println(file, text) */
            if (strcmp(member_name_str, "println") == 0 && call->arg_count == 1) {
                char *text_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_text_file_println(%s, %s)",
                    object_str, text_str);
            }

            /* hasChars() -> rt_text_file_has_chars(file) */
            if (strcmp(member_name_str, "hasChars") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_has_chars(%s)", object_str);
            }

            /* hasWords() -> rt_text_file_has_words(file) */
            if (strcmp(member_name_str, "hasWords") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_has_words(%s)", object_str);
            }

            /* hasLines() -> rt_text_file_has_lines(file) */
            if (strcmp(member_name_str, "hasLines") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_has_lines(%s)", object_str);
            }

            /* isEof() -> rt_text_file_is_eof(file) */
            if (strcmp(member_name_str, "isEof") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_is_eof(%s)", object_str);
            }

            /* position() -> rt_text_file_position(file) */
            if (strcmp(member_name_str, "position") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_position(%s)", object_str);
            }

            /* seek(pos) -> rt_text_file_seek(file, pos) */
            if (strcmp(member_name_str, "seek") == 0 && call->arg_count == 1) {
                char *pos_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_text_file_seek(%s, %s)",
                    object_str, pos_str);
            }

            /* rewind() -> rt_text_file_rewind(file) */
            if (strcmp(member_name_str, "rewind") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_rewind(%s)", object_str);
            }

            /* flush() -> rt_text_file_flush(file) */
            if (strcmp(member_name_str, "flush") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_text_file_flush(%s)", object_str);
            }
        }

        /* BinaryFile instance methods
         * NOTE: These methods are also implemented in code_gen_expr_call_file.c
         * for modular code generation. The implementations here remain for
         * backward compatibility during the transition.
         */
        if (object_type->kind == TYPE_BINARY_FILE) {
            char *object_str = code_gen_expression(gen, member->object);

            /* readByte() -> rt_binary_file_read_byte(file) returns long (-1 on EOF) */
            if (strcmp(member_name_str, "readByte") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_binary_file_read_byte(%s)", object_str);
            }

            /* readBytes(count) -> rt_binary_file_read_bytes(arena, file, count) */
            if (strcmp(member_name_str, "readBytes") == 0 && call->arg_count == 1) {
                char *count_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_binary_file_read_bytes(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, count_str);
            }

            /* readAll() (instance) -> rt_binary_file_instance_read_all(arena, file) */
            if (strcmp(member_name_str, "readAll") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_binary_file_instance_read_all(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* readInto(buffer) -> rt_binary_file_read_into(file, buffer) */
            if (strcmp(member_name_str, "readInto") == 0 && call->arg_count == 1) {
                char *buffer_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_binary_file_read_into(%s, %s)",
                    object_str, buffer_str);
            }

            /* close() -> rt_binary_file_close(file) */
            if (strcmp(member_name_str, "close") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_binary_file_close(%s)", object_str);
            }

            /* writeByte(b) -> rt_binary_file_write_byte(file, b) */
            if (strcmp(member_name_str, "writeByte") == 0 && call->arg_count == 1) {
                char *byte_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_binary_file_write_byte(%s, %s)",
                    object_str, byte_str);
            }

            /* writeBytes(data) -> rt_binary_file_write_bytes(file, data) */
            if (strcmp(member_name_str, "writeBytes") == 0 && call->arg_count == 1) {
                char *data_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_binary_file_write_bytes(%s, %s)",
                    object_str, data_str);
            }

            /* hasBytes() -> rt_binary_file_has_bytes(file) */
            if (strcmp(member_name_str, "hasBytes") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_binary_file_has_bytes(%s)", object_str);
            }

            /* isEof() -> rt_binary_file_is_eof(file) */
            if (strcmp(member_name_str, "isEof") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_binary_file_is_eof(%s)", object_str);
            }

            /* position() -> rt_binary_file_position(file) */
            if (strcmp(member_name_str, "position") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_binary_file_position(%s)", object_str);
            }

            /* seek(pos) -> rt_binary_file_seek(file, pos) */
            if (strcmp(member_name_str, "seek") == 0 && call->arg_count == 1) {
                char *pos_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_binary_file_seek(%s, %s)",
                    object_str, pos_str);
            }

            /* rewind() -> rt_binary_file_rewind(file) */
            if (strcmp(member_name_str, "rewind") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_binary_file_rewind(%s)", object_str);
            }

            /* flush() -> rt_binary_file_flush(file) */
            if (strcmp(member_name_str, "flush") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_binary_file_flush(%s)", object_str);
            }
        }

        /* Time instance methods
         * NOTE: These methods are also implemented in code_gen_expr_call_time.c
         * for modular code generation. The implementations here remain for
         * backward compatibility during the transition.
         */
        if (object_type->kind == TYPE_TIME) {
            char *object_str = code_gen_expression(gen, member->object);

            /* Getter methods - return int/long */
            if (strcmp(member_name_str, "millis") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_millis(%s)", object_str);
            }
            if (strcmp(member_name_str, "seconds") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_seconds(%s)", object_str);
            }
            if (strcmp(member_name_str, "year") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_year(%s)", object_str);
            }
            if (strcmp(member_name_str, "month") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_month(%s)", object_str);
            }
            if (strcmp(member_name_str, "day") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_day(%s)", object_str);
            }
            if (strcmp(member_name_str, "hour") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_hour(%s)", object_str);
            }
            if (strcmp(member_name_str, "minute") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_minute(%s)", object_str);
            }
            if (strcmp(member_name_str, "second") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_second(%s)", object_str);
            }
            if (strcmp(member_name_str, "weekday") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_weekday(%s)", object_str);
            }

            /* Formatting methods - return string */
            if (strcmp(member_name_str, "format") == 0 && call->arg_count == 1) {
                char *pattern_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_format(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, pattern_str);
            }
            if (strcmp(member_name_str, "toIso") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_to_iso(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }
            if (strcmp(member_name_str, "toDate") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_get_date(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }
            if (strcmp(member_name_str, "toTime") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_time_to_time(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* Arithmetic methods - return Time */
            if (strcmp(member_name_str, "add") == 0 && call->arg_count == 1) {
                char *ms_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_add(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, ms_str);
            }
            if (strcmp(member_name_str, "addSeconds") == 0 && call->arg_count == 1) {
                char *s_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_add_seconds(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, s_str);
            }
            if (strcmp(member_name_str, "addMinutes") == 0 && call->arg_count == 1) {
                char *m_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_add_minutes(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, m_str);
            }
            if (strcmp(member_name_str, "addHours") == 0 && call->arg_count == 1) {
                char *h_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_add_hours(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, h_str);
            }
            if (strcmp(member_name_str, "addDays") == 0 && call->arg_count == 1) {
                char *d_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_add_days(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, d_str);
            }
            if (strcmp(member_name_str, "diff") == 0 && call->arg_count == 1) {
                char *other_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_diff(%s, %s)",
                    object_str, other_str);
            }

            /* Comparison methods - return bool */
            if (strcmp(member_name_str, "isBefore") == 0 && call->arg_count == 1) {
                char *other_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_is_before(%s, %s)",
                    object_str, other_str);
            }
            if (strcmp(member_name_str, "isAfter") == 0 && call->arg_count == 1) {
                char *other_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_is_after(%s, %s)",
                    object_str, other_str);
            }
            if (strcmp(member_name_str, "equals") == 0 && call->arg_count == 1) {
                char *other_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_time_equals(%s, %s)",
                    object_str, other_str);
            }
        }

        /* Date instance methods */
        if (object_type->kind == TYPE_DATE) {
            char *object_str = code_gen_expression(gen, member->object);

            /* Getter methods - return int/long */
            if (strcmp(member_name_str, "year") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_get_year(%s)", object_str);
            }
            if (strcmp(member_name_str, "month") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_get_month(%s)", object_str);
            }
            if (strcmp(member_name_str, "day") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_get_day(%s)", object_str);
            }
            if (strcmp(member_name_str, "weekday") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_get_weekday(%s)", object_str);
            }
            if (strcmp(member_name_str, "dayOfYear") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_get_day_of_year(%s)", object_str);
            }
            if (strcmp(member_name_str, "epochDays") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_get_epoch_days(%s)", object_str);
            }
            if (strcmp(member_name_str, "daysInMonth") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_get_days_in_month(%s)", object_str);
            }

            /* Bool getter methods */
            if (strcmp(member_name_str, "isLeapYear") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_is_leap(%s)", object_str);
            }
            if (strcmp(member_name_str, "isWeekend") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_is_weekend(%s)", object_str);
            }
            if (strcmp(member_name_str, "isWeekday") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_is_weekday(%s)", object_str);
            }

            /* Formatting methods - return string */
            if (strcmp(member_name_str, "format") == 0 && call->arg_count == 1) {
                char *pattern_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_format(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, pattern_str);
            }
            if (strcmp(member_name_str, "toIso") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_to_iso(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }
            if (strcmp(member_name_str, "toString") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_to_string(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* Arithmetic methods - return Date */
            if (strcmp(member_name_str, "addDays") == 0 && call->arg_count == 1) {
                char *days_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_add_days(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, days_str);
            }
            if (strcmp(member_name_str, "addWeeks") == 0 && call->arg_count == 1) {
                char *weeks_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_add_weeks(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, weeks_str);
            }
            if (strcmp(member_name_str, "addMonths") == 0 && call->arg_count == 1) {
                char *months_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_add_months(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, months_str);
            }
            if (strcmp(member_name_str, "addYears") == 0 && call->arg_count == 1) {
                char *years_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_add_years(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, years_str);
            }
            if (strcmp(member_name_str, "diffDays") == 0 && call->arg_count == 1) {
                char *other_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_diff_days(%s, %s)",
                    object_str, other_str);
            }

            /* Boundary methods - return Date */
            if (strcmp(member_name_str, "startOfMonth") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_start_of_month(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }
            if (strcmp(member_name_str, "endOfMonth") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_end_of_month(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }
            if (strcmp(member_name_str, "startOfYear") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_start_of_year(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }
            if (strcmp(member_name_str, "endOfYear") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_end_of_year(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* Comparison methods - return bool */
            if (strcmp(member_name_str, "isBefore") == 0 && call->arg_count == 1) {
                char *other_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_is_before(%s, %s)",
                    object_str, other_str);
            }
            if (strcmp(member_name_str, "isAfter") == 0 && call->arg_count == 1) {
                char *other_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_is_after(%s, %s)",
                    object_str, other_str);
            }
            if (strcmp(member_name_str, "equals") == 0 && call->arg_count == 1) {
                char *other_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_date_equals(%s, %s)",
                    object_str, other_str);
            }

            /* Conversion methods */
            if (strcmp(member_name_str, "toTime") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_date_to_time(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }
        }

        /* TcpListener instance methods */
        if (object_type->kind == TYPE_TCP_LISTENER) {
            char *object_str = code_gen_expression(gen, member->object);

            /* accept() -> rt_tcp_listener_accept(arena, listener) */
            if (strcmp(member_name_str, "accept") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_tcp_listener_accept(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* close() -> rt_tcp_listener_close(listener) */
            if (strcmp(member_name_str, "close") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_tcp_listener_close(%s)", object_str);
            }
        }

        /* TcpStream instance methods */
        if (object_type->kind == TYPE_TCP_STREAM) {
            char *object_str = code_gen_expression(gen, member->object);

            /* read(maxBytes) -> rt_tcp_stream_read(arena, stream, maxBytes) */
            if (strcmp(member_name_str, "read") == 0 && call->arg_count == 1) {
                char *max_bytes_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_tcp_stream_read(%s, %s, %s)",
                    ARENA_VAR(gen), object_str, max_bytes_str);
            }

            /* readAll() -> rt_tcp_stream_read_all(arena, stream) */
            if (strcmp(member_name_str, "readAll") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_tcp_stream_read_all(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* readLine() -> rt_tcp_stream_read_line(arena, stream) */
            if (strcmp(member_name_str, "readLine") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_tcp_stream_read_line(%s, %s)",
                    ARENA_VAR(gen), object_str);
            }

            /* write(data) -> rt_tcp_stream_write(stream, data) */
            if (strcmp(member_name_str, "write") == 0 && call->arg_count == 1) {
                char *data_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_tcp_stream_write(%s, %s)",
                    object_str, data_str);
            }

            /* writeLine(line) -> rt_tcp_stream_write_line(stream, line) */
            if (strcmp(member_name_str, "writeLine") == 0 && call->arg_count == 1) {
                char *line_str = code_gen_expression(gen, call->arguments[0]);
                return arena_sprintf(gen->arena, "rt_tcp_stream_write_line(%s, %s)",
                    object_str, line_str);
            }

            /* close() -> rt_tcp_stream_close(stream) */
            if (strcmp(member_name_str, "close") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_tcp_stream_close(%s)", object_str);
            }
        }

        /* UdpSocket instance methods */
        if (object_type->kind == TYPE_UDP_SOCKET) {
            char *object_str = code_gen_expression(gen, member->object);

            /* sendTo(data, address) -> rt_udp_socket_send_to(socket, data, address) */
            if (strcmp(member_name_str, "sendTo") == 0 && call->arg_count == 2) {
                char *data_str = code_gen_expression(gen, call->arguments[0]);
                char *address_str = code_gen_expression(gen, call->arguments[1]);
                return arena_sprintf(gen->arena, "rt_udp_socket_send_to(%s, %s, %s)",
                    object_str, data_str, address_str);
            }

            /* receiveFrom(maxBytes) -> rt_udp_socket_receive_from(arena, socket, maxBytes, &sender) */
            if (strcmp(member_name_str, "receiveFrom") == 0 && call->arg_count == 1) {
                char *max_bytes_str = code_gen_expression(gen, call->arguments[0]);
                /* Note: sender address handling is complex - for now return byte[] only */
                return arena_sprintf(gen->arena, "rt_udp_socket_receive_from(%s, %s, %s, NULL)",
                    ARENA_VAR(gen), object_str, max_bytes_str);
            }

            /* close() -> rt_udp_socket_close(socket) */
            if (strcmp(member_name_str, "close") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_udp_socket_close(%s)", object_str);
            }
        }
    }

    /* Check if the callee is a closure (function type variable) */
    /* Skip builtins like print and len, and skip named functions */
    bool is_closure_call = false;
    Type *callee_type = call->callee->expr_type;

    if (callee_type && callee_type->kind == TYPE_FUNCTION && call->callee->type == EXPR_VARIABLE)
    {
        /* Native callbacks are called directly as function pointers, not closures */
        if (!callee_type->as.function.is_native)
        {
            char *name = get_var_name(gen->arena, call->callee->as.variable.name);
            /* Skip builtins */
            if (strcmp(name, "print") != 0 && strcmp(name, "len") != 0 &&
                strcmp(name, "readLine") != 0 && strcmp(name, "println") != 0 &&
                strcmp(name, "printErr") != 0 && strcmp(name, "printErrLn") != 0)
            {
                /* Check if this is a named function or a closure variable */
                Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
                if (sym != NULL && !sym->is_function)
                {
                    /* This is a closure variable (not a named function) */
                    is_closure_call = true;
                }
            }
        }
    }
    /* Also handle array access where element is a function type (e.g., callbacks[0]()) */
    else if (callee_type && callee_type->kind == TYPE_FUNCTION && call->callee->type == EXPR_ARRAY_ACCESS)
    {
        /* Native callback arrays are not closures */
        if (!callee_type->as.function.is_native)
        {
            is_closure_call = true;
        }
    }

    if (is_closure_call)
    {
        /* Generate closure call: ((ret (*)(void*, params...))closure->fn)(closure, args...) */
        char *closure_str = code_gen_expression(gen, call->callee);

        /* Build function pointer cast */
        const char *ret_c_type = get_c_type(gen->arena, callee_type->as.function.return_type);
        char *param_types_str = arena_strdup(gen->arena, "void *");  /* First param is closure */
        for (int i = 0; i < callee_type->as.function.param_count; i++)
        {
            const char *param_c_type = get_c_type(gen->arena, callee_type->as.function.param_types[i]);
            param_types_str = arena_sprintf(gen->arena, "%s, %s", param_types_str, param_c_type);
        }

        /* Generate arguments */
        char *args_str = closure_str;  /* First arg is the closure itself */
        for (int i = 0; i < call->arg_count; i++)
        {
            char *arg_str = code_gen_expression(gen, call->arguments[i]);
            args_str = arena_sprintf(gen->arena, "%s, %s", args_str, arg_str);
        }

        /* Generate the call: ((<ret> (*)(<params>))closure->fn)(args) */
        return arena_sprintf(gen->arena, "((%s (*)(%s))%s->fn)(%s)",
                             ret_c_type, param_types_str, closure_str, args_str);
    }

    char *callee_str = code_gen_expression(gen, call->callee);

    char **arg_strs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
    bool *arg_is_temp = arena_alloc(gen->arena, call->arg_count * sizeof(bool));
    bool has_temps = false;
    for (int i = 0; i < call->arg_count; i++)
    {
        arg_strs[i] = code_gen_expression(gen, call->arguments[i]);
        arg_is_temp[i] = (call->arguments[i]->expr_type && call->arguments[i]->expr_type->kind == TYPE_STRING &&
                          expression_produces_temp(call->arguments[i]));
        if (arg_is_temp[i])
            has_temps = true;
    }

    // Special case for builtin 'print': error if wrong arg count, else map to appropriate rt_print_* based on arg type.
    if (call->callee->type == EXPR_VARIABLE)
    {
        char *callee_name = get_var_name(gen->arena, call->callee->as.variable.name);
        if (strcmp(callee_name, "print") == 0)
        {
            if (call->arg_count != 1)
            {
                fprintf(stderr, "Error: print expects exactly one argument\n");
                exit(1);
            }
            Type *arg_type = call->arguments[0]->expr_type;
            const char *print_func = NULL;
            switch (arg_type->kind)
            {
            case TYPE_INT:
            case TYPE_LONG:
                print_func = "rt_print_long";
                break;
            case TYPE_DOUBLE:
                print_func = "rt_print_double";
                break;
            case TYPE_CHAR:
                print_func = "rt_print_char";
                break;
            case TYPE_BOOL:
                print_func = "rt_print_bool";
                break;
            case TYPE_BYTE:
                print_func = "rt_print_byte";
                break;
            case TYPE_STRING:
                print_func = "rt_print_string";
                break;
            case TYPE_ARRAY:
            {
                Type *elem_type = arg_type->as.array.element_type;
                switch (elem_type->kind)
                {
                case TYPE_INT:
                case TYPE_LONG:
                    print_func = "rt_print_array_long";
                    break;
                case TYPE_DOUBLE:
                    print_func = "rt_print_array_double";
                    break;
                case TYPE_CHAR:
                    print_func = "rt_print_array_char";
                    break;
                case TYPE_BOOL:
                    print_func = "rt_print_array_bool";
                    break;
                case TYPE_BYTE:
                    print_func = "rt_print_array_byte";
                    break;
                case TYPE_STRING:
                    print_func = "rt_print_array_string";
                    break;
                default:
                    fprintf(stderr, "Error: unsupported array element type for print\n");
                    exit(1);
                }
                break;
            }
            default:
                fprintf(stderr, "Error: unsupported type for print\n");
                exit(1);
            }
            callee_str = arena_strdup(gen->arena, print_func);
        }
        // Handle len(arr) -> rt_array_length for arrays, strlen for strings
        else if (strcmp(callee_name, "len") == 0 && call->arg_count == 1)
        {
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type && arg_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "(long)strlen(%s)", arg_strs[0]);
            }
            return arena_sprintf(gen->arena, "rt_array_length(%s)", arg_strs[0]);
        }
        // readLine() -> rt_read_line(arena)
        else if (strcmp(callee_name, "readLine") == 0 && call->arg_count == 0)
        {
            return arena_sprintf(gen->arena, "rt_read_line(%s)", ARENA_VAR(gen));
        }
        // println(arg) -> rt_println(to_string(arg))
        else if (strcmp(callee_name, "println") == 0 && call->arg_count == 1)
        {
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "rt_println(%s)", arg_strs[0]);
            }
            else
            {
                const char *to_str_func = get_rt_to_string_func(arg_type->kind);
                return arena_sprintf(gen->arena, "rt_println(%s(%s, %s))",
                                     to_str_func, ARENA_VAR(gen), arg_strs[0]);
            }
        }
        // printErr(arg) -> rt_print_err(to_string(arg))
        else if (strcmp(callee_name, "printErr") == 0 && call->arg_count == 1)
        {
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "rt_print_err(%s)", arg_strs[0]);
            }
            else
            {
                const char *to_str_func = get_rt_to_string_func(arg_type->kind);
                return arena_sprintf(gen->arena, "rt_print_err(%s(%s, %s))",
                                     to_str_func, ARENA_VAR(gen), arg_strs[0]);
            }
        }
        // printErrLn(arg) -> rt_print_err_ln(to_string(arg))
        else if (strcmp(callee_name, "printErrLn") == 0 && call->arg_count == 1)
        {
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "rt_print_err_ln(%s)", arg_strs[0]);
            }
            else
            {
                const char *to_str_func = get_rt_to_string_func(arg_type->kind);
                return arena_sprintf(gen->arena, "rt_print_err_ln(%s(%s, %s))",
                                     to_str_func, ARENA_VAR(gen), arg_strs[0]);
            }
        }
        // Note: Other array operations are method-style only:
        //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)
    }

    // Check if the callee is a shared function - if so, we need to pass the arena.
    // Functions returning heap-allocated types (TYPE_STRING, TYPE_ARRAY, TYPE_FUNCTION)
    // are implicitly shared (set in type_checker_stmt.c:163-170) to match the code
    // generator's logic in code_gen_stmt.c:301-307.
    bool callee_is_shared = false;
    if (call->callee->type == EXPR_VARIABLE)
    {
        Symbol *callee_sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
        if (callee_sym && callee_sym->func_mod == FUNC_SHARED)
        {
            callee_is_shared = true;
        }
    }

    // Collect arg names for the call: use temp var if temp, else original str.
    char **arg_names = arena_alloc(gen->arena, sizeof(char *) * call->arg_count);

    // Build args list (comma-separated).
    // If calling a shared function, prepend the current arena as first argument
    char *args_list;
    if (callee_is_shared && gen->current_arena_var != NULL)
    {
        args_list = arena_strdup(gen->arena, gen->current_arena_var);
    }
    else if (callee_is_shared)
    {
        args_list = arena_strdup(gen->arena, "NULL");
    }
    else
    {
        args_list = arena_strdup(gen->arena, "");
    }

    /* Get parameter memory qualifiers from callee's function type */
    MemoryQualifier *param_quals = NULL;
    int param_count = 0;
    if (call->callee->expr_type && call->callee->expr_type->kind == TYPE_FUNCTION)
    {
        param_quals = call->callee->expr_type->as.function.param_mem_quals;
        param_count = call->callee->expr_type->as.function.param_count;
    }

    for (int i = 0; i < call->arg_count; i++)
    {
        if (arg_is_temp[i])
        {
            arg_names[i] = arena_sprintf(gen->arena, "_str_arg%d", i);
        }
        else
        {
            arg_names[i] = arg_strs[i];
        }

        /* For 'as ref' primitive parameters, pass address of the argument */
        bool is_ref_param = false;
        if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
            call->arguments[i]->expr_type != NULL)
        {
            TypeKind kind = call->arguments[i]->expr_type->kind;
            bool is_prim = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                           kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                           kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                           kind == TYPE_BYTE);
            is_ref_param = is_prim;
        }
        if (is_ref_param)
        {
            arg_names[i] = arena_sprintf(gen->arena, "&%s", arg_names[i]);
        }

        bool need_comma = (i > 0) || callee_is_shared;
        char *new_args = arena_sprintf(gen->arena, "%s%s%s", args_list, need_comma ? ", " : "", arg_names[i]);
        args_list = new_args;
    }

    // Determine if the call returns void (affects statement expression).
    bool returns_void = (expr->expr_type && expr->expr_type->kind == TYPE_VOID);

    // If no temps, simple call (no statement expression needed).
    // Note: Expression returns without semicolon - statement handler adds it
    if (!has_temps)
    {
        return arena_sprintf(gen->arena, "%s(%s)", callee_str, args_list);
    }

    // Temps present: generate multi-line statement expression for readability
    char *result = arena_strdup(gen->arena, "({\n");

    // Declare and initialize temp string arguments
    for (int i = 0; i < call->arg_count; i++)
    {
        if (arg_is_temp[i])
        {
            result = arena_sprintf(gen->arena, "%s        char *%s = %s;\n", result, arg_names[i], arg_strs[i]);
        }
    }

    // Make the actual call
    const char *ret_c = get_c_type(gen->arena, expr->expr_type);
    if (returns_void)
    {
        result = arena_sprintf(gen->arena, "%s        %s(%s);\n", result, callee_str, args_list);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s        %s _call_result = %s(%s);\n", result, ret_c, callee_str, args_list);
    }

    // Free temps (only strings) - skip if in arena context
    if (gen->current_arena_var == NULL)
    {
        for (int i = 0; i < call->arg_count; i++)
        {
            if (arg_is_temp[i])
            {
                result = arena_sprintf(gen->arena, "%s        rt_free_string(%s);\n", result, arg_names[i]);
            }
        }
    }

    // End statement expression.
    if (returns_void)
    {
        result = arena_sprintf(gen->arena, "%s    })", result);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s        _call_result;\n    })", result);
    }

    return result;
}
