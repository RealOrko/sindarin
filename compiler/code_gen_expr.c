#include "code_gen_expr.h"
#include "code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

bool expression_produces_temp(Expr *expr)
{
    DEBUG_VERBOSE("Entering expression_produces_temp");
    if (expr->expr_type->kind != TYPE_STRING)
        return false;
    switch (expr->type)
    {
    case EXPR_VARIABLE:
    case EXPR_ASSIGN:
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

char *code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_binary_expression");
    char *left_str = code_gen_expression(gen, expr->left);
    char *right_str = code_gen_expression(gen, expr->right);
    Type *type = expr->left->expr_type;
    TokenType op = expr->operator;
    if (op == TOKEN_AND)
    {
        return arena_sprintf(gen->arena, "((%s != 0 && %s != 0) ? 1L : 0L)", left_str, right_str);
    }
    else if (op == TOKEN_OR)
    {
        return arena_sprintf(gen->arena, "((%s != 0 || %s != 0) ? 1L : 0L)", left_str, right_str);
    }

    // Handle array comparison (== and !=)
    if (type->kind == TYPE_ARRAY && (op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL))
    {
        Type *elem_type = type->as.array.element_type;
        const char *arr_suffix;
        switch (elem_type->kind)
        {
            case TYPE_INT:
            case TYPE_LONG:
                arr_suffix = "long";
                break;
            case TYPE_DOUBLE:
                arr_suffix = "double";
                break;
            case TYPE_CHAR:
                arr_suffix = "char";
                break;
            case TYPE_BOOL:
                arr_suffix = "bool";
                break;
            case TYPE_STRING:
                arr_suffix = "string";
                break;
            default:
                fprintf(stderr, "Error: Unsupported array element type for comparison\n");
                exit(1);
        }
        if (op == TOKEN_EQUAL_EQUAL)
        {
            return arena_sprintf(gen->arena, "rt_array_eq_%s(%s, %s)", arr_suffix, left_str, right_str);
        }
        else
        {
            return arena_sprintf(gen->arena, "(!rt_array_eq_%s(%s, %s))", arr_suffix, left_str, right_str);
        }
    }

    char *op_str = code_gen_binary_op_str(op);
    char *suffix = code_gen_type_suffix(type);
    if (op == TOKEN_PLUS && type->kind == TYPE_STRING)
    {
        bool free_left = expression_produces_temp(expr->left);
        bool free_right = expression_produces_temp(expr->right);
        // Optimization: Direct call if no temps (common for literals/variables).
        if (!free_left && !free_right)
        {
            return arena_sprintf(gen->arena, "rt_str_concat(%s, %s)", left_str, right_str);
        }
        // Otherwise, use temps/block for safe freeing.
        char *free_l_str = free_left ? "rt_free_string(_left); " : "";
        char *free_r_str = free_right ? "rt_free_string(_right); " : "";
        return arena_sprintf(gen->arena, "({ char *_left = %s; char *_right = %s; char *_res = rt_str_concat(_left, _right); %s%s _res; })",
                             left_str, right_str, free_l_str, free_r_str);
    }
    else
    {
        return arena_sprintf(gen->arena, "rt_%s_%s(%s, %s)", op_str, suffix, left_str, right_str);
    }
}

char *code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_unary_expression");
    char *operand_str = code_gen_expression(gen, expr->operand);
    Type *type = expr->operand->expr_type;
    switch (expr->operator)
    {
    case TOKEN_MINUS:
        if (type->kind == TYPE_DOUBLE)
        {
            return arena_sprintf(gen->arena, "rt_neg_double(%s)", operand_str);
        }
        else
        {
            return arena_sprintf(gen->arena, "rt_neg_long(%s)", operand_str);
        }
    case TOKEN_BANG:
        return arena_sprintf(gen->arena, "rt_not_bool(%s)", operand_str);
    default:
        exit(1);
    }
    return NULL;
}

char *code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_literal_expression");
    Type *type = expr->type;
    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return arena_sprintf(gen->arena, "%ldL", expr->value.int_value);
    case TYPE_DOUBLE:
    {
        char *str = arena_sprintf(gen->arena, "%.17g", expr->value.double_value);
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            str = arena_sprintf(gen->arena, "%s.0", str);
        }
        return str;
    }
    case TYPE_CHAR:
        return escape_char_literal(gen->arena, expr->value.char_value);
    case TYPE_STRING:
    {
        // This might break string interpolation
        /*char *escaped = escape_c_string(gen->arena, expr->value.string_value);
        return arena_sprintf(gen->arena, "rt_to_string_string(%s)", escaped);*/
        return escape_c_string(gen->arena, expr->value.string_value);
    }
    case TYPE_BOOL:
        return arena_sprintf(gen->arena, "%ldL", expr->value.bool_value ? 1L : 0L);
    case TYPE_NIL:
        return arena_strdup(gen->arena, "0L");
    default:
        exit(1);
    }
    return NULL;
}

char *code_gen_variable_expression(CodeGen *gen, VariableExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_variable_expression");
    return get_var_name(gen->arena, expr->name);
}

char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_assign_expression");
    char *var_name = get_var_name(gen->arena, expr->name);
    char *value_str = code_gen_expression(gen, expr->value);
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol == NULL)
    {
        exit(1);
    }
    Type *type = symbol->type;
    if (type->kind == TYPE_STRING)
    {
        return arena_sprintf(gen->arena, "({ char *_val = %s; if (%s) rt_free_string(%s); %s = _val; _val; })",
                             value_str, var_name, var_name, var_name);
    }
    else
    {
        return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
    }
}

char *code_gen_interpolated_expression(CodeGen *gen, InterpolExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_interpolated_expression");
    int count = expr->part_count;
    if (count == 0)
    {
        return arena_strdup(gen->arena, "rt_to_string_string(\"\")");
    }
    char **part_strs = arena_alloc(gen->arena, count * sizeof(char *));
    Type **part_types = arena_alloc(gen->arena, count * sizeof(Type *));
    bool *free_parts = arena_alloc(gen->arena, count * sizeof(bool));
    int non_string_count = 0;
    for (int i = 0; i < count; i++)
    {
        part_strs[i] = code_gen_expression(gen, expr->parts[i]);
        part_types[i] = expr->parts[i]->expr_type;
        free_parts[i] = expression_produces_temp(expr->parts[i]);
        if (part_types[i]->kind != TYPE_STRING)
        {
            non_string_count++;
        }
    }
    if (non_string_count == 0)
    {
        // All strings case
        if (count == 1)
        {
            // Single part, just return it (with duplication if needed)
            if (free_parts[0])
            {
                return part_strs[0];
            }
            else
            {
                return arena_sprintf(gen->arena, "rt_to_string_string(%s)", part_strs[0]);
            }
        }
        if (count == 2)
        {
            // Two parts - simple concat, no intermediate to free
            return arena_sprintf(gen->arena, "rt_str_concat(%s, %s)", part_strs[0], part_strs[1]);
        }
        // Multiple parts - need intermediate variables to avoid memory leaks
        char *result = arena_strdup(gen->arena, "({\n");

        // Declare intermediate concat variables
        for (int i = 0; i < count - 2; i++)
        {
            result = arena_sprintf(gen->arena, "%s        char *_concat_tmp%d;\n", result, i);
        }

        // First concat
        result = arena_sprintf(gen->arena, "%s        _concat_tmp0 = rt_str_concat(%s, %s);\n",
                               result, part_strs[0], part_strs[1]);

        // Middle concats
        for (int i = 2; i < count - 1; i++)
        {
            result = arena_sprintf(gen->arena, "%s        _concat_tmp%d = rt_str_concat(_concat_tmp%d, %s);\n",
                                   result, i - 1, i - 2, part_strs[i]);
        }

        // Final concat
        result = arena_sprintf(gen->arena, "%s        char *_interpol_result = rt_str_concat(_concat_tmp%d, %s);\n",
                               result, count - 3, part_strs[count - 1]);

        // Free intermediate concat results
        for (int i = 0; i < count - 2; i++)
        {
            result = arena_sprintf(gen->arena, "%s        rt_free_string(_concat_tmp%d);\n", result, i);
        }

        // Return result
        result = arena_sprintf(gen->arena, "%s        _interpol_result;\n    })", result);
        return result;
    }
    else
    {
        // Mix, convert non-strings to strings - generate multi-line readable code
        char *result = arena_strdup(gen->arena, "({\n");

        // Declare string part variables
        result = arena_sprintf(gen->arena, "%s        char *", result);
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
            {
                result = arena_sprintf(gen->arena, "%s, *", result);
            }
            result = arena_sprintf(gen->arena, "%s_str_part%d", result, i);
        }
        result = arena_sprintf(gen->arena, "%s;\n", result);

        // Initialize each string part on its own line
        for (int i = 0; i < count; i++)
        {
            if (part_types[i]->kind == TYPE_STRING)
            {
                if (free_parts[i])
                {
                    result = arena_sprintf(gen->arena, "%s        _str_part%d = %s;\n", result, i, part_strs[i]);
                }
                else
                {
                    result = arena_sprintf(gen->arena, "%s        _str_part%d = rt_to_string_string(%s);\n", result, i, part_strs[i]);
                }
            }
            else
            {
                const char *to_str = get_rt_to_string_func(part_types[i]->kind);
                result = arena_sprintf(gen->arena, "%s        _str_part%d = %s(%s);\n", result, i, to_str, part_strs[i]);
            }
        }

        // Build concatenation result with intermediate variables to avoid memory leaks
        if (count == 2)
        {
            // Simple case: just two parts, no intermediate needed
            result = arena_sprintf(gen->arena, "%s        char *_interpol_result = rt_str_concat(_str_part0, _str_part1);\n", result);
        }
        else
        {
            // Multiple parts: need intermediate variables
            // Declare intermediate concat variables
            for (int i = 0; i < count - 2; i++)
            {
                result = arena_sprintf(gen->arena, "%s        char *_concat_tmp%d;\n", result, i);
            }
            // First concat
            result = arena_sprintf(gen->arena, "%s        _concat_tmp0 = rt_str_concat(_str_part0, _str_part1);\n", result);
            // Middle concats
            for (int i = 2; i < count - 1; i++)
            {
                result = arena_sprintf(gen->arena, "%s        _concat_tmp%d = rt_str_concat(_concat_tmp%d, _str_part%d);\n", result, i - 1, i - 2, i);
            }
            // Final concat
            result = arena_sprintf(gen->arena, "%s        char *_interpol_result = rt_str_concat(_concat_tmp%d, _str_part%d);\n", result, count - 3, count - 1);
        }

        // Free temporary strings (the original parts)
        for (int i = 0; i < count; i++)
        {
            result = arena_sprintf(gen->arena, "%s        rt_free_string(_str_part%d);\n", result, i);
        }

        // Free intermediate concat results
        for (int i = 0; i < count - 2; i++)
        {
            result = arena_sprintf(gen->arena, "%s        rt_free_string(_concat_tmp%d);\n", result, i);
        }

        // Return result
        result = arena_sprintf(gen->arena, "%s        _interpol_result;\n    })", result);
        return result;
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

        if (object_type->kind == TYPE_ARRAY) {
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
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for push\n");
                        exit(1);
                }
                // push returns new array pointer, assign back to variable if object is a variable
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s))", object_str, push_func, object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", push_func, object_str, arg_str);
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
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for pop\n");
                        exit(1);
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
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for concat\n");
                        exit(1);
                }
                // concat returns a new array, doesn't modify the original
                return arena_sprintf(gen->arena, "%s(%s, %s)", concat_func, object_str, arg_str);
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
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for clone\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s)", clone_func, object_str);
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
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for join\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", join_func, object_str, arg_str);
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
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for reverse\n");
                        exit(1);
                }
                // reverse in-place: assign result back to variable, free old array
                if (member->object->type == EXPR_VARIABLE) {
                    int label = code_gen_new_label(gen);
                    return arena_sprintf(gen->arena,
                        "({\n"
                        "        void *__old_arr_%d__ = %s;\n"
                        "        %s = %s(%s);\n"
                        "        rt_array_free(__old_arr_%d__);\n"
                        "    })",
                        label, object_str, object_str, rev_func, object_str, label);
                }
                return arena_sprintf(gen->arena, "%s(%s)", rev_func, object_str);
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
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for insert\n");
                        exit(1);
                }
                // insert in-place: assign result back to variable, free old array
                if (member->object->type == EXPR_VARIABLE) {
                    int label = code_gen_new_label(gen);
                    return arena_sprintf(gen->arena,
                        "({\n"
                        "        void *__old_arr_%d__ = %s;\n"
                        "        %s = %s(%s, %s, %s);\n"
                        "        rt_array_free(__old_arr_%d__);\n"
                        "    })",
                        label, object_str, object_str, ins_func, object_str, elem_str, idx_str, label);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", ins_func, object_str, elem_str, idx_str);
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
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for remove\n");
                        exit(1);
                }
                // remove in-place: assign result back to variable, free old array
                if (member->object->type == EXPR_VARIABLE) {
                    int label = code_gen_new_label(gen);
                    return arena_sprintf(gen->arena,
                        "({\n"
                        "        void *__old_arr_%d__ = %s;\n"
                        "        %s = %s(%s, %s);\n"
                        "        rt_array_free(__old_arr_%d__);\n"
                        "    })",
                        label, object_str, object_str, rem_func, object_str, idx_str, label);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", rem_func, object_str, idx_str);
            }
        }
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
        // Handle len(arr) -> rt_array_length
        else if (strcmp(callee_name, "len") == 0 && call->arg_count == 1)
        {
            return arena_sprintf(gen->arena, "rt_array_length(%s)", arg_strs[0]);
        }
        // Handle pop(arr) -> rt_array_pop_*
        else if (strcmp(callee_name, "pop") == 0 && call->arg_count == 1)
        {
            Type *arr_type = call->arguments[0]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *pop_func = NULL;
            switch (elem_type->kind)
            {
            case TYPE_INT:
            case TYPE_LONG:
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
            default:
                fprintf(stderr, "Error: unsupported array element type for pop\n");
                exit(1);
            }
            return arena_sprintf(gen->arena, "%s(%s)", pop_func, arg_strs[0]);
        }
        // Handle rev(arr) -> rt_array_rev_*
        else if (strcmp(callee_name, "rev") == 0 && call->arg_count == 1)
        {
            Type *arr_type = call->arguments[0]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *rev_func = NULL;
            switch (elem_type->kind)
            {
            case TYPE_INT:
            case TYPE_LONG:
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
            default:
                fprintf(stderr, "Error: unsupported array element type for rev\n");
                exit(1);
            }
            return arena_sprintf(gen->arena, "%s(%s)", rev_func, arg_strs[0]);
        }
        // Handle push(elem, arr) -> rt_array_push_copy_* (returns new array)
        else if (strcmp(callee_name, "push") == 0 && call->arg_count == 2)
        {
            Type *arr_type = call->arguments[1]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *push_func = NULL;
            switch (elem_type->kind)
            {
            case TYPE_INT:
            case TYPE_LONG:
                push_func = "rt_array_push_copy_long";
                break;
            case TYPE_DOUBLE:
                push_func = "rt_array_push_copy_double";
                break;
            case TYPE_CHAR:
                push_func = "rt_array_push_copy_char";
                break;
            case TYPE_STRING:
                push_func = "rt_array_push_copy_string";
                break;
            case TYPE_BOOL:
                push_func = "rt_array_push_copy_bool";
                break;
            default:
                fprintf(stderr, "Error: unsupported array element type for push\n");
                exit(1);
            }
            return arena_sprintf(gen->arena, "%s(%s, %s)", push_func, arg_strs[1], arg_strs[0]);
        }
        // Handle rem(index, arr) -> rt_array_rem_*
        else if (strcmp(callee_name, "rem") == 0 && call->arg_count == 2)
        {
            Type *arr_type = call->arguments[1]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *rem_func = NULL;
            switch (elem_type->kind)
            {
            case TYPE_INT:
            case TYPE_LONG:
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
            default:
                fprintf(stderr, "Error: unsupported array element type for rem\n");
                exit(1);
            }
            return arena_sprintf(gen->arena, "%s(%s, %s)", rem_func, arg_strs[1], arg_strs[0]);
        }
        // Handle ins(elem, index, arr) -> rt_array_ins_*
        else if (strcmp(callee_name, "ins") == 0 && call->arg_count == 3)
        {
            Type *arr_type = call->arguments[2]->expr_type;
            Type *elem_type = arr_type->as.array.element_type;
            const char *ins_func = NULL;
            switch (elem_type->kind)
            {
            case TYPE_INT:
            case TYPE_LONG:
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
            default:
                fprintf(stderr, "Error: unsupported array element type for ins\n");
                exit(1);
            }
            return arena_sprintf(gen->arena, "%s(%s, %s, %s)", ins_func, arg_strs[2], arg_strs[0], arg_strs[1]);
        }
    }

    // Collect arg names for the call: use temp var if temp, else original str.
    char **arg_names = arena_alloc(gen->arena, sizeof(char *) * call->arg_count);

    // Build args list (comma-separated).
    char *args_list = arena_strdup(gen->arena, "");
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
        char *new_args = arena_sprintf(gen->arena, "%s%s%s", args_list, i > 0 ? ", " : "", arg_names[i]);
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

    // Free temps (only strings).
    for (int i = 0; i < call->arg_count; i++)
    {
        if (arg_is_temp[i])
        {
            result = arena_sprintf(gen->arena, "%s        rt_free_string(%s);\n", result, arg_names[i]);
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

char *code_gen_array_expression(CodeGen *gen, Expr *e)
{
    ArrayExpr *arr = &e->as.array;
    DEBUG_VERBOSE("Entering code_gen_array_expression");
    Type *arr_type = e->expr_type;
    if (arr_type->kind != TYPE_ARRAY) {
        fprintf(stderr, "Error: Expected array type\n");
        exit(1);
    }
    Type *elem_type = arr_type->as.array.element_type;
    const char *elem_c = get_c_type(gen->arena, elem_type);

    // Build the element list
    char *inits = arena_strdup(gen->arena, "");
    for (int i = 0; i < arr->element_count; i++) {
        char *el = code_gen_expression(gen, arr->elements[i]);
        char *sep = i > 0 ? ", " : "";
        inits = arena_sprintf(gen->arena, "%s%s%s", inits, sep, el);
    }

    // Determine the runtime function suffix based on element type
    const char *suffix = NULL;
    switch (elem_type->kind) {
        case TYPE_INT: suffix = "long"; break;
        case TYPE_DOUBLE: suffix = "double"; break;
        case TYPE_CHAR: suffix = "char"; break;
        case TYPE_BOOL: suffix = "bool"; break;
        case TYPE_STRING: suffix = "string"; break;
        default:
            // For unsupported element types (like nested arrays), fall back to
            // compound literal without runtime wrapper
            return arena_sprintf(gen->arena, "(%s[]){%s}", elem_c, inits);
    }

    // Generate: rt_array_create_<suffix>(count, (elem_type[]){...})
    return arena_sprintf(gen->arena, "rt_array_create_%s(%d, (%s[]){%s})",
                         suffix, arr->element_count, elem_c, inits);
}

char *code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_access_expression");
    char *array_str = code_gen_expression(gen, expr->array);
    char *index_str = code_gen_expression(gen, expr->index);

    // Check if index is a literal - if it's a non-negative literal, skip the runtime check
    if (expr->index->type == EXPR_LITERAL && expr->index->as.literal.type->kind == TYPE_INT)
    {
        long idx_val = expr->index->as.literal.value.int_value;
        if (idx_val >= 0)
        {
            // Positive literal index - no adjustment needed
            return arena_sprintf(gen->arena, "%s[%s]", array_str, index_str);
        }
        // Negative literal - can simplify to: arr[rt_array_length(arr) + idx]
        return arena_sprintf(gen->arena, "%s[rt_array_length(%s) + %s]",
                             array_str, array_str, index_str);
    }

    // For variable indices, generate runtime check for negative index
    // arr[idx < 0 ? rt_array_length(arr) + idx : idx]
    return arena_sprintf(gen->arena, "%s[(%s) < 0 ? rt_array_length(%s) + (%s) : (%s)]",
                         array_str, index_str, array_str, index_str, index_str);
}

char *code_gen_increment_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_increment_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = get_var_name(gen->arena, expr->as.operand->as.variable.name);
    return arena_sprintf(gen->arena, "rt_post_inc_long(&%s)", var_name);
}

char *code_gen_decrement_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_decrement_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = get_var_name(gen->arena, expr->as.operand->as.variable.name);
    return arena_sprintf(gen->arena, "rt_post_dec_long(&%s)", var_name);
}

char *code_gen_member_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_member_expression");
    MemberExpr *member = &expr->as.member;
    char *member_name_str = get_var_name(gen->arena, member->member_name);
    Type *object_type = member->object->expr_type;
    char *object_str = code_gen_expression(gen, member->object);

    // Handle array.length
    if (object_type->kind == TYPE_ARRAY && strcmp(member_name_str, "length") == 0) {
        return arena_sprintf(gen->arena, "rt_array_length(%s)", object_str);
    }

    // Generic struct member access (not currently supported)
    fprintf(stderr, "Error: Unsupported member access on type\n");
    exit(1);
}

char *code_gen_array_slice_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_slice_expression");
    ArraySliceExpr *slice = &expr->as.array_slice;

    char *array_str = code_gen_expression(gen, slice->array);

    // Get start, end, and step values - use LONG_MIN to signal defaults
    char *start_str = slice->start ? code_gen_expression(gen, slice->start) : "LONG_MIN";
    char *end_str = slice->end ? code_gen_expression(gen, slice->end) : "LONG_MIN";
    char *step_str = slice->step ? code_gen_expression(gen, slice->step) : "LONG_MIN";

    // Determine element type for the correct slice function
    Type *array_type = slice->array->expr_type;
    Type *elem_type = array_type->as.array.element_type;

    const char *slice_func = NULL;
    switch (elem_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            slice_func = "rt_array_slice_long";
            break;
        case TYPE_DOUBLE:
            slice_func = "rt_array_slice_double";
            break;
        case TYPE_CHAR:
            slice_func = "rt_array_slice_char";
            break;
        case TYPE_STRING:
            slice_func = "rt_array_slice_string";
            break;
        case TYPE_BOOL:
            slice_func = "rt_array_slice_bool";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for slice\n");
            exit(1);
    }

    return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s)", slice_func, array_str, start_str, end_str, step_str);
}

char *code_gen_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_expression");
    if (expr == NULL)
    {
        return arena_strdup(gen->arena, "0L");
    }
    switch (expr->type)
    {
    case EXPR_BINARY:
        return code_gen_binary_expression(gen, &expr->as.binary);
    case EXPR_UNARY:
        return code_gen_unary_expression(gen, &expr->as.unary);
    case EXPR_LITERAL:
        return code_gen_literal_expression(gen, &expr->as.literal);
    case EXPR_VARIABLE:
        return code_gen_variable_expression(gen, &expr->as.variable);
    case EXPR_ASSIGN:
        return code_gen_assign_expression(gen, &expr->as.assign);
    case EXPR_CALL:
        return code_gen_call_expression(gen, expr);
    case EXPR_ARRAY:
        return code_gen_array_expression(gen, expr);
    case EXPR_ARRAY_ACCESS:
        return code_gen_array_access_expression(gen, &expr->as.array_access);
    case EXPR_INCREMENT:
        return code_gen_increment_expression(gen, expr);
    case EXPR_DECREMENT:
        return code_gen_decrement_expression(gen, expr);
    case EXPR_INTERPOLATED:
        return code_gen_interpolated_expression(gen, &expr->as.interpol);
    case EXPR_MEMBER:
        return code_gen_member_expression(gen, expr);
    case EXPR_ARRAY_SLICE:
        return code_gen_array_slice_expression(gen, expr);
    default:
        exit(1);
    }
    return NULL;
}
