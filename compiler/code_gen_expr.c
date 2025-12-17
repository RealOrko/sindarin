#include "code_gen_expr.h"
#include "code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Forward declarations */
char *code_gen_range_expression(CodeGen *gen, Expr *expr);
static char *code_gen_lambda_expression(CodeGen *gen, Expr *expr);

/* Helper structure for captured variable tracking */
typedef struct {
    char **names;
    Type **types;
    int count;
    int capacity;
} CapturedVars;

static void captured_vars_init(CapturedVars *cv, Arena *arena)
{
    cv->names = NULL;
    cv->types = NULL;
    cv->count = 0;
    cv->capacity = 0;
}

static void captured_vars_add(CapturedVars *cv, Arena *arena, const char *name, Type *type)
{
    /* Check if already captured */
    for (int i = 0; i < cv->count; i++)
    {
        if (strcmp(cv->names[i], name) == 0)
            return;
    }
    /* Grow arrays if needed */
    if (cv->count >= cv->capacity)
    {
        int new_cap = cv->capacity == 0 ? 4 : cv->capacity * 2;
        char **new_names = arena_alloc(arena, new_cap * sizeof(char *));
        Type **new_types = arena_alloc(arena, new_cap * sizeof(Type *));
        for (int i = 0; i < cv->count; i++)
        {
            new_names[i] = cv->names[i];
            new_types[i] = cv->types[i];
        }
        cv->names = new_names;
        cv->types = new_types;
        cv->capacity = new_cap;
    }
    cv->names[cv->count] = arena_strdup(arena, name);
    cv->types[cv->count] = type;
    cv->count++;
}

/* Helper to check if a name is a lambda parameter */
static bool is_lambda_param(LambdaExpr *lambda, const char *name)
{
    for (int i = 0; i < lambda->param_count; i++)
    {
        char param_name[256];
        int len = lambda->params[i].name.length < 255 ? lambda->params[i].name.length : 255;
        strncpy(param_name, lambda->params[i].name.start, len);
        param_name[len] = '\0';
        if (strcmp(param_name, name) == 0)
            return true;
    }
    return false;
}

/* Recursively collect captured variables from an expression */
static void collect_captured_vars(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                                  CapturedVars *cv, Arena *arena)
{
    if (expr == NULL) return;

    switch (expr->type)
    {
    case EXPR_VARIABLE:
    {
        char name[256];
        int len = expr->as.variable.name.length < 255 ? expr->as.variable.name.length : 255;
        strncpy(name, expr->as.variable.name.start, len);
        name[len] = '\0';

        /* Skip if it's a lambda parameter */
        if (is_lambda_param(lambda, name))
            return;

        /* Skip builtins */
        if (strcmp(name, "print") == 0 || strcmp(name, "len") == 0)
            return;

        /* Look up in symbol table to see if it's an outer variable */
        Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
        if (sym != NULL)
        {
            /* It's a captured variable from outer scope */
            captured_vars_add(cv, arena, name, sym->type);
        }
        break;
    }
    case EXPR_BINARY:
        collect_captured_vars(expr->as.binary.left, lambda, table, cv, arena);
        collect_captured_vars(expr->as.binary.right, lambda, table, cv, arena);
        break;
    case EXPR_UNARY:
        collect_captured_vars(expr->as.unary.operand, lambda, table, cv, arena);
        break;
    case EXPR_ASSIGN:
        collect_captured_vars(expr->as.assign.value, lambda, table, cv, arena);
        break;
    case EXPR_CALL:
        collect_captured_vars(expr->as.call.callee, lambda, table, cv, arena);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            collect_captured_vars(expr->as.call.arguments[i], lambda, table, cv, arena);
        break;
    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
            collect_captured_vars(expr->as.array.elements[i], lambda, table, cv, arena);
        break;
    case EXPR_ARRAY_ACCESS:
        collect_captured_vars(expr->as.array_access.array, lambda, table, cv, arena);
        collect_captured_vars(expr->as.array_access.index, lambda, table, cv, arena);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        collect_captured_vars(expr->as.operand, lambda, table, cv, arena);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            collect_captured_vars(expr->as.interpol.parts[i], lambda, table, cv, arena);
        break;
    case EXPR_MEMBER:
        collect_captured_vars(expr->as.member.object, lambda, table, cv, arena);
        break;
    case EXPR_ARRAY_SLICE:
        collect_captured_vars(expr->as.array_slice.array, lambda, table, cv, arena);
        collect_captured_vars(expr->as.array_slice.start, lambda, table, cv, arena);
        collect_captured_vars(expr->as.array_slice.end, lambda, table, cv, arena);
        collect_captured_vars(expr->as.array_slice.step, lambda, table, cv, arena);
        break;
    case EXPR_RANGE:
        collect_captured_vars(expr->as.range.start, lambda, table, cv, arena);
        collect_captured_vars(expr->as.range.end, lambda, table, cv, arena);
        break;
    case EXPR_SPREAD:
        collect_captured_vars(expr->as.spread.array, lambda, table, cv, arena);
        break;
    case EXPR_LAMBDA:
        /* Don't recurse into nested lambdas - they have their own captures */
        break;
    case EXPR_LITERAL:
    default:
        break;
    }
}

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
            return arena_sprintf(gen->arena, "rt_str_concat(%s, %s, %s)", ARENA_VAR(gen), left_str, right_str);
        }
        // Otherwise, use temps/block for safe freeing (skip freeing in arena context).
        char *free_l_str = (free_left && gen->current_arena_var == NULL) ? "rt_free_string(_left); " : "";
        char *free_r_str = (free_right && gen->current_arena_var == NULL) ? "rt_free_string(_right); " : "";
        return arena_sprintf(gen->arena, "({ char *_left = %s; char *_right = %s; char *_res = rt_str_concat(%s, _left, _right); %s%s _res; })",
                             left_str, right_str, ARENA_VAR(gen), free_l_str, free_r_str);
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
    char *var_name = get_var_name(gen->arena, expr->name);

    // Check if variable is 'as ref' - if so, dereference it
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "(*%s)", var_name);
    }
    return var_name;
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

    // Handle 'as ref' - dereference pointer for assignment
    if (symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "(*%s = %s)", var_name, value_str);
    }

    if (type->kind == TYPE_STRING)
    {
        // Skip freeing old value in arena context - arena handles cleanup
        if (gen->current_arena_var != NULL)
        {
            return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
        }
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
        return arena_sprintf(gen->arena, "rt_to_string_string(%s, \"\")", ARENA_VAR(gen));
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
                return arena_sprintf(gen->arena, "rt_to_string_string(%s, %s)", ARENA_VAR(gen), part_strs[0]);
            }
        }

        // Check if any parts need freeing (are temp strings from calls, etc.)
        bool any_free_parts = false;
        for (int i = 0; i < count; i++)
        {
            if (free_parts[i])
            {
                any_free_parts = true;
                break;
            }
        }

        if (count == 2 && !any_free_parts)
        {
            // Two parts, neither needs freeing - simple concat
            return arena_sprintf(gen->arena, "rt_str_concat(%s, %s, %s)", ARENA_VAR(gen), part_strs[0], part_strs[1]);
        }

        // Multiple parts or some need freeing - need intermediate variables
        char *result = arena_strdup(gen->arena, "({\n");

        // Build arrays of what to use in concat (either original or captured temp)
        char **concat_parts = arena_alloc(gen->arena, count * sizeof(char *));

        // Capture parts that need freeing into temp variables
        for (int i = 0; i < count; i++)
        {
            if (free_parts[i])
            {
                result = arena_sprintf(gen->arena, "%s        char *_str_tmp%d = %s;\n", result, i, part_strs[i]);
                concat_parts[i] = arena_sprintf(gen->arena, "_str_tmp%d", i);
            }
            else
            {
                concat_parts[i] = part_strs[i];
            }
        }

        // Declare intermediate concat variables
        for (int i = 0; i < count - 2; i++)
        {
            result = arena_sprintf(gen->arena, "%s        char *_concat_tmp%d;\n", result, i);
        }

        if (count == 2)
        {
            // Two parts - simple concat
            result = arena_sprintf(gen->arena, "%s        char *_interpol_result = rt_str_concat(%s, %s, %s);\n",
                                   result, ARENA_VAR(gen), concat_parts[0], concat_parts[1]);
        }
        else
        {
            // First concat
            result = arena_sprintf(gen->arena, "%s        _concat_tmp0 = rt_str_concat(%s, %s, %s);\n",
                                   result, ARENA_VAR(gen), concat_parts[0], concat_parts[1]);

            // Middle concats
            for (int i = 2; i < count - 1; i++)
            {
                result = arena_sprintf(gen->arena, "%s        _concat_tmp%d = rt_str_concat(%s, _concat_tmp%d, %s);\n",
                                       result, i - 1, ARENA_VAR(gen), i - 2, concat_parts[i]);
            }

            // Final concat
            result = arena_sprintf(gen->arena, "%s        char *_interpol_result = rt_str_concat(%s, _concat_tmp%d, %s);\n",
                                   result, ARENA_VAR(gen), count - 3, concat_parts[count - 1]);
        }

        // Free captured temp strings - skip in arena context
        if (gen->current_arena_var == NULL)
        {
            for (int i = 0; i < count; i++)
            {
                if (free_parts[i])
                {
                    result = arena_sprintf(gen->arena, "%s        rt_free_string(_str_tmp%d);\n", result, i);
                }
            }

            // Free intermediate concat results
            for (int i = 0; i < count - 2; i++)
            {
                result = arena_sprintf(gen->arena, "%s        rt_free_string(_concat_tmp%d);\n", result, i);
            }
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
                    result = arena_sprintf(gen->arena, "%s        _str_part%d = rt_to_string_string(%s, %s);\n", result, i, ARENA_VAR(gen), part_strs[i]);
                }
            }
            else
            {
                const char *to_str = get_rt_to_string_func(part_types[i]->kind);
                result = arena_sprintf(gen->arena, "%s        _str_part%d = %s(%s, %s);\n", result, i, to_str, ARENA_VAR(gen), part_strs[i]);
            }
        }

        // Build concatenation result with intermediate variables to avoid memory leaks
        if (count == 2)
        {
            // Simple case: just two parts, no intermediate needed
            result = arena_sprintf(gen->arena, "%s        char *_interpol_result = rt_str_concat(%s, _str_part0, _str_part1);\n", result, ARENA_VAR(gen));
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
            result = arena_sprintf(gen->arena, "%s        _concat_tmp0 = rt_str_concat(%s, _str_part0, _str_part1);\n", result, ARENA_VAR(gen));
            // Middle concats
            for (int i = 2; i < count - 1; i++)
            {
                result = arena_sprintf(gen->arena, "%s        _concat_tmp%d = rt_str_concat(%s, _concat_tmp%d, _str_part%d);\n", result, i - 1, ARENA_VAR(gen), i - 2, i);
            }
            // Final concat
            result = arena_sprintf(gen->arena, "%s        char *_interpol_result = rt_str_concat(%s, _concat_tmp%d, _str_part%d);\n", result, ARENA_VAR(gen), count - 3, count - 1);
        }

        // Free temporary strings (the original parts) - skip in arena context
        if (gen->current_arena_var == NULL)
        {
            for (int i = 0; i < count; i++)
            {
                result = arena_sprintf(gen->arena, "%s        rt_free_string(_str_part%d);\n", result, i);
            }

            // Free intermediate concat results
            for (int i = 0; i < count - 2; i++)
            {
                result = arena_sprintf(gen->arena, "%s        rt_free_string(_concat_tmp%d);\n", result, i);
            }
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
        }

        // String methods
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

            #undef STRING_METHOD_RETURNING_STRING
        }
    }

    /* Check if the callee is a closure (function type variable) */
    /* Skip builtins like print and len, and skip named functions */
    bool is_closure_call = false;
    Type *callee_type = call->callee->expr_type;

    if (callee_type && callee_type->kind == TYPE_FUNCTION && call->callee->type == EXPR_VARIABLE)
    {
        char *name = get_var_name(gen->arena, call->callee->as.variable.name);
        /* Skip builtins */
        if (strcmp(name, "print") != 0 && strcmp(name, "len") != 0)
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
        // Note: Other array operations are method-style only:
        //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)
    }

    // Check if the callee is a shared function - if so, we need to pass the arena
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

    // Check if we have any spread or range elements
    bool has_complex = false;
    for (int i = 0; i < arr->element_count; i++) {
        if (arr->elements[i]->type == EXPR_SPREAD || arr->elements[i]->type == EXPR_RANGE) {
            has_complex = true;
            break;
        }
    }

    // Determine the runtime function suffix based on element type
    const char *suffix = NULL;
    switch (elem_type->kind) {
        case TYPE_INT: suffix = "long"; break;
        case TYPE_DOUBLE: suffix = "double"; break;
        case TYPE_CHAR: suffix = "char"; break;
        case TYPE_BOOL: suffix = "bool"; break;
        case TYPE_STRING: suffix = "string"; break;
        default: suffix = NULL; break;
    }

    // If we have spread or range elements, generate concatenation code
    if (has_complex && suffix != NULL) {
        // Start with empty array or first element
        char *result = NULL;

        for (int i = 0; i < arr->element_count; i++) {
            Expr *elem = arr->elements[i];
            char *elem_str;

            if (elem->type == EXPR_SPREAD) {
                // Spread: clone the array to avoid aliasing issues
                char *arr_str = code_gen_expression(gen, elem->as.spread.array);
                elem_str = arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), arr_str);
            } else if (elem->type == EXPR_RANGE) {
                // Range: concat the range result
                elem_str = code_gen_range_expression(gen, elem);
            } else {
                // Regular element: create single-element array
                char *val = code_gen_expression(gen, elem);
                const char *literal_type = (elem_type->kind == TYPE_BOOL) ? "int" : elem_c;
                elem_str = arena_sprintf(gen->arena, "rt_array_create_%s(%s, 1, (%s[]){%s})",
                                        suffix, ARENA_VAR(gen), literal_type, val);
            }

            if (result == NULL) {
                result = elem_str;
            } else {
                // Concat with previous result
                result = arena_sprintf(gen->arena, "rt_array_concat_%s(%s, %s, %s)",
                                      suffix, ARENA_VAR(gen), result, elem_str);
            }
        }

        return result ? result : arena_sprintf(gen->arena, "rt_array_create_%s(%s, 0, NULL)", suffix, ARENA_VAR(gen));
    }

    // Simple case: no spread or range elements
    // Build the element list
    char *inits = arena_strdup(gen->arena, "");
    for (int i = 0; i < arr->element_count; i++) {
        char *el = code_gen_expression(gen, arr->elements[i]);
        char *sep = i > 0 ? ", " : "";
        inits = arena_sprintf(gen->arena, "%s%s%s", inits, sep, el);
    }

    if (suffix == NULL) {
        // For unsupported element types (like nested arrays), fall back to
        // compound literal without runtime wrapper
        return arena_sprintf(gen->arena, "(%s[]){%s}", elem_c, inits);
    }

    // Generate: rt_array_create_<suffix>(arena, count, (elem_type[]){...})
    // For bool arrays, use "int" for compound literal since runtime uses int internally
    const char *literal_type = (elem_type->kind == TYPE_BOOL) ? "int" : elem_c;
    return arena_sprintf(gen->arena, "rt_array_create_%s(%s, %d, (%s[]){%s})",
                         suffix, ARENA_VAR(gen), arr->element_count, literal_type, inits);
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
    // For 'as ref' variables, they're already pointers, so pass directly
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->as.operand->as.variable.name);
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "rt_post_inc_long(%s)", var_name);
    }
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
    // For 'as ref' variables, they're already pointers, so pass directly
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->as.operand->as.variable.name);
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "rt_post_dec_long(%s)", var_name);
    }
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

    // Handle string.length
    if (object_type->kind == TYPE_STRING && strcmp(member_name_str, "length") == 0) {
        return arena_sprintf(gen->arena, "rt_str_length(%s)", object_str);
    }

    // Generic struct member access (not currently supported)
    fprintf(stderr, "Error: Unsupported member access on type\n");
    exit(1);
}

char *code_gen_range_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_range_expression");
    RangeExpr *range = &expr->as.range;

    char *start_str = code_gen_expression(gen, range->start);
    char *end_str = code_gen_expression(gen, range->end);

    return arena_sprintf(gen->arena, "rt_array_range(%s, %s, %s)", ARENA_VAR(gen), start_str, end_str);
}

char *code_gen_spread_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_spread_expression");
    // Spread expressions are typically handled in array literal context
    // but if used standalone, just return the array
    return code_gen_expression(gen, expr->as.spread.array);
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

    return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s, %s)", slice_func, ARENA_VAR(gen), array_str, start_str, end_str, step_str);
}

static char *code_gen_lambda_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_lambda_expression");
    LambdaExpr *lambda = &expr->as.lambda;
    int lambda_id = gen->lambda_count++;
    FunctionModifier modifier = lambda->modifier;

    /* Store the lambda_id in the expression for later reference */
    expr->as.lambda.lambda_id = lambda_id;

    /* Collect captured variables */
    CapturedVars cv;
    captured_vars_init(&cv, gen->arena);
    collect_captured_vars(lambda->body, lambda, gen->symbol_table, &cv, gen->arena);

    /* Get C types for return type and parameters */
    const char *ret_c_type = get_c_type(gen->arena, lambda->return_type);

    /* Build parameter list string for the static function */
    /* First param is always the closure pointer (void *) */
    char *params_decl = arena_strdup(gen->arena, "void *__closure__");

    for (int i = 0; i < lambda->param_count; i++)
    {
        const char *param_c_type = get_c_type(gen->arena, lambda->params[i].type);
        char *param_name = arena_strndup(gen->arena, lambda->params[i].name.start,
                                         lambda->params[i].name.length);
        params_decl = arena_sprintf(gen->arena, "%s, %s %s", params_decl, param_c_type, param_name);
    }

    /* Generate arena handling code based on modifier */
    char *arena_setup = arena_strdup(gen->arena, "");
    char *arena_cleanup = arena_strdup(gen->arena, "");
    const char *lambda_arena_var = "((__Closure__ *)__closure__)->arena";

    if (modifier == FUNC_PRIVATE)
    {
        /* Private lambda: create isolated arena, destroy before return */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArena *__lambda_arena__ = rt_arena_create(NULL);\n"
            "    (void)__closure__;\n");
        arena_cleanup = arena_sprintf(gen->arena,
            "    rt_arena_destroy(__lambda_arena__);\n");
        lambda_arena_var = "__lambda_arena__";
    }
    else
    {
        /* Default/Shared lambda: use arena from closure */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;\n");
        lambda_arena_var = "__lambda_arena__";
    }

    if (cv.count > 0)
    {
        /* Generate custom closure struct for this lambda (with arena field) */
        char *struct_def = arena_sprintf(gen->arena,
            "typedef struct __closure_%d__ {\n"
            "    void *fn;\n"
            "    RtArena *arena;\n",
            lambda_id);
        for (int i = 0; i < cv.count; i++)
        {
            const char *c_type = get_c_type(gen->arena, cv.types[i]);
            struct_def = arena_sprintf(gen->arena, "%s    %s *%s;\n",
                                       struct_def, c_type, cv.names[i]);
        }
        struct_def = arena_sprintf(gen->arena, "%s} __closure_%d__;\n",
                                   struct_def, lambda_id);

        /* Add struct def to forward declarations (before lambda functions) */
        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, struct_def);

        /* Generate local variable declarations for captured vars in lambda body */
        char *capture_decls = arena_strdup(gen->arena, "");
        for (int i = 0; i < cv.count; i++)
        {
            const char *c_type = get_c_type(gen->arena, cv.types[i]);
            capture_decls = arena_sprintf(gen->arena,
                "%s    %s %s = *((__closure_%d__ *)__closure__)->%s;\n",
                capture_decls, c_type, cv.names[i], lambda_id, cv.names[i]);
        }

        /* Generate the static lambda function body - use lambda's arena */
        char *saved_arena_var = gen->current_arena_var;
        gen->current_arena_var = "__lambda_arena__";
        char *body_code = code_gen_expression(gen, lambda->body);
        gen->current_arena_var = saved_arena_var;

        /* Generate forward declaration */
        char *forward_decl = arena_sprintf(gen->arena,
            "static %s __lambda_%d__(%s);\n",
            ret_c_type, lambda_id, params_decl);

        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, forward_decl);

        /* Generate the actual lambda function definition with capture access */
        char *lambda_func;
        if (modifier == FUNC_PRIVATE)
        {
            /* Private: create arena, compute result, destroy arena, return */
            lambda_func = arena_sprintf(gen->arena,
                "static %s __lambda_%d__(%s) {\n"
                "%s"
                "%s"
                "    %s __result__ = %s;\n"
                "%s"
                "    return __result__;\n"
                "}\n\n",
                ret_c_type, lambda_id, params_decl, arena_setup, capture_decls,
                ret_c_type, body_code, arena_cleanup);
        }
        else
        {
            lambda_func = arena_sprintf(gen->arena,
                "static %s __lambda_%d__(%s) {\n"
                "%s"
                "%s"
                "    return %s;\n"
                "}\n\n",
                ret_c_type, lambda_id, params_decl, arena_setup, capture_decls, body_code);
        }

        /* Append to definitions buffer */
        gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                gen->lambda_definitions, lambda_func);

        /* Return code that creates and populates the closure */
        char *closure_init = arena_sprintf(gen->arena,
            "({\n"
            "    __closure_%d__ *__cl__ = rt_arena_alloc(%s, sizeof(__closure_%d__));\n"
            "    __cl__->fn = (void *)__lambda_%d__;\n"
            "    __cl__->arena = %s;\n",
            lambda_id, ARENA_VAR(gen), lambda_id, lambda_id, ARENA_VAR(gen));

        for (int i = 0; i < cv.count; i++)
        {
            closure_init = arena_sprintf(gen->arena, "%s    __cl__->%s = &%s;\n",
                                         closure_init, cv.names[i], cv.names[i]);
        }
        closure_init = arena_sprintf(gen->arena,
            "%s    (__Closure__ *)__cl__;\n"
            "})",
            closure_init);

        return closure_init;
    }
    else
    {
        /* No captures - use simple generic closure */
        /* Generate the static lambda function body - use lambda's arena */
        char *saved_arena_var = gen->current_arena_var;
        gen->current_arena_var = "__lambda_arena__";
        char *body_code = code_gen_expression(gen, lambda->body);
        gen->current_arena_var = saved_arena_var;

        /* Generate forward declaration */
        char *forward_decl = arena_sprintf(gen->arena,
            "static %s __lambda_%d__(%s);\n",
            ret_c_type, lambda_id, params_decl);

        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, forward_decl);

        /* Generate the actual lambda function definition */
        char *lambda_func;
        if (modifier == FUNC_PRIVATE)
        {
            /* Private: create arena, compute result, destroy arena, return */
            lambda_func = arena_sprintf(gen->arena,
                "static %s __lambda_%d__(%s) {\n"
                "%s"
                "    %s __result__ = %s;\n"
                "%s"
                "    return __result__;\n"
                "}\n\n",
                ret_c_type, lambda_id, params_decl, arena_setup,
                ret_c_type, body_code, arena_cleanup);
        }
        else
        {
            lambda_func = arena_sprintf(gen->arena,
                "static %s __lambda_%d__(%s) {\n"
                "%s"
                "    return %s;\n"
                "}\n\n",
                ret_c_type, lambda_id, params_decl, arena_setup, body_code);
        }

        /* Append to definitions buffer */
        gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                gen->lambda_definitions, lambda_func);

        /* Return code that creates the closure using generic __Closure__ type */
        return arena_sprintf(gen->arena,
            "({\n"
            "    __Closure__ *__cl__ = rt_arena_alloc(%s, sizeof(__Closure__));\n"
            "    __cl__->fn = (void *)__lambda_%d__;\n"
            "    __cl__->arena = %s;\n"
            "    __cl__;\n"
            "})",
            ARENA_VAR(gen), lambda_id, ARENA_VAR(gen));
    }
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
    case EXPR_RANGE:
        return code_gen_range_expression(gen, expr);
    case EXPR_SPREAD:
        return code_gen_spread_expression(gen, expr);
    case EXPR_LAMBDA:
        return code_gen_lambda_expression(gen, expr);
    default:
        exit(1);
    }
    return NULL;
}
