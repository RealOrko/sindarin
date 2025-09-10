#include "code_gen.h"
#include "debug.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "arena.h"

static char *code_gen_expression(CodeGen *gen, Expr *expr);
static char *code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr);
static char *code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr);
static char *code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr);
static char *code_gen_variable_expression(CodeGen *gen, VariableExpr *expr);
static char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr);
static char *code_gen_interpolated_expression(CodeGen *gen, InterpolExpr *expr);
static char *code_gen_call_expression(CodeGen *gen, Expr *expr);
static char *code_gen_array_expression(CodeGen *gen, Expr *expr);
static char *code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr);
static char *code_gen_increment_expression(CodeGen *gen, Expr *expr);
static char *code_gen_decrement_expression(CodeGen *gen, Expr *expr);
static bool expression_produces_temp(Expr *expr);

static void indented_fprintf(CodeGen *gen, int indent, const char *fmt, ...);

static char *arena_vsprintf(Arena *arena, const char *fmt, va_list args)
{
    // DEBUG_VERBOSE("Entering arena_vsprintf");
    va_list args_copy;
    va_copy(args_copy, args);
    int size = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    if (size < 0)
    {
        exit(1);
    }
    char *buf = arena_alloc(arena, size + 1);
    if (buf == NULL)
    {
        exit(1);
    }
    vsnprintf(buf, size + 1, fmt, args);
    return buf;
}

static char *arena_sprintf(Arena *arena, const char *fmt, ...)
{
    // DEBUG_VERBOSE("Entering arena_sprintf");
    va_list args;
    va_start(args, fmt);
    char *result = arena_vsprintf(arena, fmt, args);
    va_end(args);
    return result;
}

static char *escape_c_string(Arena *arena, const char *str)
{
    DEBUG_VERBOSE("Entering escape_c_string");
    if (str == NULL)
    {
        return arena_strdup(arena, "NULL");
    }
    size_t len = strlen(str);
    char *buf = arena_alloc(arena, len * 2 + 3);
    if (buf == NULL)
    {
        exit(1);
    }
    char *p = buf;
    *p++ = '"';
    for (const char *s = str; *s; s++)
    {
        switch (*s)
        {
        case '"':
            *p++ = '\\';
            *p++ = '"';
            break;
        case '\\':
            *p++ = '\\';
            *p++ = '\\';
            break;
        case '\n':
            *p++ = '\\';
            *p++ = 'n';
            break;
        case '\t':
            *p++ = '\\';
            *p++ = 't';
            break;
        case '\r':
            *p++ = '\\';
            *p++ = 'r';
            break;
        default:
            *p++ = *s;
            break;
        }
    }
    *p++ = '"';
    *p = '\0';
    return buf;
}

static const char *get_c_type(Arena *arena, Type *type)
{
    DEBUG_VERBOSE("Entering get_c_type");

    if (type == NULL)
    {
        return arena_strdup(arena, "void");
    }

    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return arena_strdup(arena, "long");
    case TYPE_DOUBLE:
        return arena_strdup(arena, "double");
    case TYPE_CHAR:
        return arena_strdup(arena, "char");
    case TYPE_STRING:
        return arena_strdup(arena, "char *");
    case TYPE_BOOL:
        return arena_strdup(arena, "bool");
    case TYPE_VOID:
        return arena_strdup(arena, "void");
    case TYPE_NIL:
        return arena_strdup(arena, "void *");
    case TYPE_ANY:
        return arena_strdup(arena, "void *");
    case TYPE_ARRAY:
    {
        const char *element_c_type = get_c_type(arena, type->as.array.element_type);
        if (type->as.array.element_type->kind == TYPE_ARRAY)
        {
            size_t len = strlen(element_c_type) + 10; // For " (*)[]" and null terminator
            char *result = arena_alloc(arena, len);
            snprintf(result, len, "%s (*)[]", element_c_type);
            return result;
        }
        else
        {
            size_t len = strlen(element_c_type) + 3; // For ' *' and null terminator
            char *result = arena_alloc(arena, len);
            snprintf(result, len, "%s *", element_c_type);
            return result;
        }
    }
    case TYPE_FUNCTION:
    {
        const char *return_c_type = get_c_type(arena, type->as.function.return_type);
        size_t len = strlen(return_c_type) + 10; // Base for "(*)(...)"
        for (int i = 0; i < type->as.function.param_count; i++)
        {
            len += strlen(get_c_type(arena, type->as.function.param_types[i]));
            if (i < type->as.function.param_count - 1)
            {
                len += 2; // For ", "
            }
        }
        char *result = arena_alloc(arena, len);
        char *current = result;
        current += sprintf(current, "%s (*)(", return_c_type);
        for (int i = 0; i < type->as.function.param_count; i++)
        {
            current += sprintf(current, "%s", get_c_type(arena, type->as.function.param_types[i]));
            if (i < type->as.function.param_count - 1)
            {
                current += sprintf(current, ", ");
            }
        }
        sprintf(current, ")");
        return result;
    }
    default:
        fprintf(stderr, "Error: Unknown type kind %d\n", type->kind);
        exit(1);
    }

    return NULL; // Unreachable
}

static const char *get_rt_to_string_func(TypeKind kind)
{
    DEBUG_VERBOSE("Entering get_rt_to_string_func");
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return "rt_to_string_long";
    case TYPE_DOUBLE:
        return "rt_to_string_double";
    case TYPE_CHAR:
        return "rt_to_string_char";
    case TYPE_BOOL:
        return "rt_to_string_bool";
    default:
        exit(1);
    }
    return NULL;
}

static const char *get_default_value(Type *type)
{
    DEBUG_VERBOSE("Entering get_default_value");
    if (type->kind == TYPE_STRING || type->kind == TYPE_ARRAY)
    {
        return "NULL";
    }
    else
    {
        return "0";
    }
}

static char *get_var_name(Arena *arena, Token name)
{
    DEBUG_VERBOSE("Entering get_var_name");
    char *var_name = arena_alloc(arena, name.length + 1);
    if (var_name == NULL)
    {
        exit(1);
    }
    strncpy(var_name, name.start, name.length);
    var_name[name.length] = '\0';
    return var_name;
}

void code_gen_init(Arena *arena, CodeGen *gen, SymbolTable *symbol_table, const char *output_file)
{
    DEBUG_VERBOSE("Entering code_gen_init");
    gen->arena = arena;
    gen->label_count = 0;
    gen->symbol_table = symbol_table;
    gen->output = fopen(output_file, "w");
    gen->current_function = NULL;
    gen->current_return_type = NULL;
    gen->temp_count = 0;
    if (gen->output == NULL)
    {
        exit(1);
    }
}

void code_gen_cleanup(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_cleanup");
    if (gen->output != NULL)
    {
        fclose(gen->output);
    }
    gen->current_function = NULL;
}

int code_gen_new_label(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_new_label");
    return gen->label_count++;
}

static void indented_fprintf(CodeGen *gen, int indent, const char *fmt, ...)
{
    for (int i = 0; i < indent; i++)
    {
        fprintf(gen->output, "    ");
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(gen->output, fmt, args);
    va_end(args);
}

static void code_gen_headers(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_headers");
    indented_fprintf(gen, 0, "#include <stdlib.h>\n");
    indented_fprintf(gen, 0, "#include <string.h>\n");
    indented_fprintf(gen, 0, "#include <stdio.h>\n\n");
}

static void code_gen_externs(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_externs");
    indented_fprintf(gen, 0, "extern char *rt_str_concat(char *, char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_long(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_double(double);\n");
    indented_fprintf(gen, 0, "extern void rt_print_char(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_string(char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_bool(long);\n");
    indented_fprintf(gen, 0, "extern long rt_add_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_sub_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_mul_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_div_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_mod_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_eq_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_ne_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_lt_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_le_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_gt_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_ge_long(long, long);\n");
    indented_fprintf(gen, 0, "extern double rt_add_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_sub_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_mul_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_div_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_eq_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_ne_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_lt_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_le_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_gt_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_ge_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_neg_long(long);\n");
    indented_fprintf(gen, 0, "extern double rt_neg_double(double);\n");
    indented_fprintf(gen, 0, "extern long rt_not_bool(long);\n");
    indented_fprintf(gen, 0, "extern long rt_post_inc_long(long *);\n");
    indented_fprintf(gen, 0, "extern long rt_post_dec_long(long *);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_long(long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_double(double);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_char(long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_bool(long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_string(char *);\n");
    indented_fprintf(gen, 0, "extern long rt_eq_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_ne_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_lt_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_le_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_gt_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_ge_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern void rt_free_string(char *);\n\n");
}

static char *code_gen_binary_op_str(TokenType op)
{
    DEBUG_VERBOSE("Entering code_gen_binary_op_str");
    switch (op)
    {
    case TOKEN_PLUS:
        return "add";
    case TOKEN_MINUS:
        return "sub";
    case TOKEN_STAR:
        return "mul";
    case TOKEN_SLASH:
        return "div";
    case TOKEN_MODULO:
        return "mod";
    case TOKEN_EQUAL_EQUAL:
        return "eq";
    case TOKEN_BANG_EQUAL:
        return "ne";
    case TOKEN_LESS:
        return "lt";
    case TOKEN_LESS_EQUAL:
        return "le";
    case TOKEN_GREATER:
        return "gt";
    case TOKEN_GREATER_EQUAL:
        return "ge";
    default:
        exit(1);
    }
    return NULL;
}

static char *code_gen_type_suffix(Type *type)
{
    DEBUG_VERBOSE("Entering code_gen_type_suffix");
    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_CHAR:
    case TYPE_BOOL:
        return "long";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_STRING:
        return "string";
    default:
        exit(1);
    }
    return NULL;
}

static bool expression_produces_temp(Expr *expr)
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

static char *code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr)
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

static char *code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr)
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

static char *code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_literal_expression");
    Type *type = expr->type;
    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return arena_sprintf(gen->arena, "%ldL", expr->value.int_value);
    case TYPE_DOUBLE:
        return arena_sprintf(gen->arena, "%.17g", expr->value.double_value);
    case TYPE_CHAR:
        return arena_sprintf(gen->arena, "%ldL", (long)(unsigned char)expr->value.char_value);
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

static char *code_gen_variable_expression(CodeGen *gen, VariableExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_variable_expression");
    return get_var_name(gen->arena, expr->name);
}

static char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr)
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

static char *code_gen_interpolated_expression(CodeGen *gen, InterpolExpr *expr)
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
        // All strings, concat directly.
        char *concat = arena_strdup(gen->arena, "");
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
            {
                concat = arena_sprintf(gen->arena, "%s, ", concat);
            }
            if (free_parts[i])
            {
                concat = arena_sprintf(gen->arena, "%s_tmp%d", concat, i);
            }
            else
            {
                concat = arena_sprintf(gen->arena, "%spart_strs[i]", concat);
            }
        }
        return arena_sprintf(gen->arena, "rt_str_concat_multiple(%s)", concat);
    }
    else
    {
        // Mix, convert non-strings to strings.
        char *result = arena_strdup(gen->arena, "({ char *");
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
            {
                result = arena_sprintf(gen->arena, "%s, *", result);
            }
            result = arena_sprintf(gen->arena, "%s_tmp%d", result, i);
        }
        result = arena_sprintf(gen->arena, "%s = {", result);
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
            {
                result = arena_sprintf(gen->arena, "%s, ", result);
            }
            if (part_types[i]->kind == TYPE_STRING)
            {
                if (free_parts[i])
                {
                    result = arena_sprintf(gen->arena, "%s_tmp%d = %s", result, i, part_strs[i]);
                }
                else
                {
                    result = arena_sprintf(gen->arena, "%s_tmp%d = rt_to_string_string(%s)", result, i, part_strs[i]);
                }
            }
            else
            {
                const char *to_str = get_rt_to_string_func(part_types[i]->kind);
                result = arena_sprintf(gen->arena, "%s_tmp%d = %s(%s)", result, i, to_str, part_strs[i]);
            }
        }
        result = arena_sprintf(gen->arena, "%s}; char *_res = ", result);
        char *concat = arena_strdup(gen->arena, "_tmp0");
        for (int i = 1; i < count; i++)
        {
            concat = arena_sprintf(gen->arena, "rt_str_concat(%s, _tmp%d)", concat, i);
        }
        result = arena_sprintf(gen->arena, "%s%s; ", result, concat);
        for (int i = 0; i < count; i++)
        {
            result = arena_sprintf(gen->arena, "%srt_free_string(_tmp%d); ", result, i);
        }
        result = arena_sprintf(gen->arena, "%s_res; })", result);
        return result;
    }
}

static char *code_gen_call_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_call_expression");
    CallExpr *call = &expr->as.call;
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
            default:
                fprintf(stderr, "Error: unsupported type for print\n");
                exit(1);
            }
            callee_str = arena_strdup(gen->arena, print_func);
        }
    }

    // Collect arg names for the call: use temp var if temp, else original str.
    char **arg_names = arena_alloc(gen->arena, sizeof(char *) * call->arg_count);
    char *result = arena_strdup(gen->arena, "({ ");
    for (int i = 0; i < call->arg_count; i++)
    {
        if (arg_is_temp[i])
        {
            char *tmp_var = arena_sprintf(gen->arena, "_tmp_arg%d", i);
            result = arena_sprintf(gen->arena, "%schar *%s = %s; ", result, tmp_var, arg_strs[i]);
            arg_names[i] = tmp_var;
        }
        else
        {
            arg_names[i] = arg_strs[i];
        }
    }

    // Build args list (comma-separated).
    char *args_list = arena_strdup(gen->arena, "");
    for (int i = 0; i < call->arg_count; i++)
    {
        char *new_args = arena_sprintf(gen->arena, "%s%s%s", args_list, i > 0 ? ", " : "", arg_names[i]);
        args_list = new_args;
    }

    // Determine if the call returns void (affects statement expression).
    bool returns_void = (expr->expr_type && expr->expr_type->kind == TYPE_VOID);

    // If no temps, simple call (no statement expression needed).
    if (!has_temps)
    {
        if (returns_void)
        {
            return arena_sprintf(gen->arena, "%s(%s);", callee_str, args_list);
        }
        else
        {
            return arena_sprintf(gen->arena, "%s(%s)", callee_str, args_list);
        }
    }

    // Temps present: continue building statement expression.
    const char *ret_c = get_c_type(gen->arena, expr->expr_type);
    if (returns_void)
    {
        result = arena_sprintf(gen->arena, "%s%s(%s); ", result, callee_str, args_list);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s%s _res = %s(%s); ", result, ret_c, callee_str, args_list);
    }

    // Free temps (only strings).
    for (int i = 0; i < call->arg_count; i++)
    {
        if (arg_is_temp[i])
        {
            result = arena_sprintf(gen->arena, "%srt_free_string(%s); ", result, arg_names[i]);
        }
    }

    // End statement expression.
    if (returns_void)
    {
        result = arena_sprintf(gen->arena, "%s})", result);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s_res; })", result);
    }

    return result;
}

static char *code_gen_array_expression(CodeGen *gen, Expr *e)
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
    char *literal_type;
    if (elem_type->kind == TYPE_ARRAY) {
        literal_type = arena_sprintf(gen->arena, "%s (*)[]", elem_c);
    } else {
        literal_type = arena_sprintf(gen->arena, "%s []", elem_c);
    }
    char *inits = arena_strdup(gen->arena, "");
    for (int i = 0; i < arr->element_count; i++) {
        char *el = code_gen_expression(gen, arr->elements[i]);
        char *sep = i > 0 ? ", " : "";
        inits = arena_sprintf(gen->arena, "%s%s%s", inits, sep, el);
    }
    return arena_sprintf(gen->arena, "(%s){%s}", literal_type, inits);
}

static char *code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_access_expression");
    char *array_str = code_gen_expression(gen, expr->array);
    char *index_str = code_gen_expression(gen, expr->index);
    return arena_sprintf(gen->arena, "%s[%s]", array_str, index_str);
}

static char *code_gen_increment_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_increment_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = get_var_name(gen->arena, expr->as.operand->as.variable.name);
    return arena_sprintf(gen->arena, "rt_post_inc_long(&%s)", var_name);
}

static char *code_gen_decrement_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_decrement_expression");
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    char *var_name = get_var_name(gen->arena, expr->as.operand->as.variable.name);
    return arena_sprintf(gen->arena, "rt_post_dec_long(&%s)", var_name);
}

static char *code_gen_expression(CodeGen *gen, Expr *expr)
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
    default:
        exit(1);
    }
    return NULL;
}

static void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_expression_statement");
    char *expr_str = code_gen_expression(gen, stmt->expression);
    if (stmt->expression->expr_type->kind == TYPE_STRING && expression_produces_temp(stmt->expression))
    {
        indented_fprintf(gen, indent, "{\n");
        indented_fprintf(gen, indent + 1, "char *_tmp = %s;\n", expr_str);
        indented_fprintf(gen, indent + 1, "(void)_tmp;\n");
        indented_fprintf(gen, indent + 1, "rt_free_string(_tmp);\n");
        indented_fprintf(gen, indent, "}\n");
    }
    else if (stmt->expression->type == EXPR_CALL && stmt->expression->expr_type->kind == TYPE_VOID)
    {
        indented_fprintf(gen, indent, "%s\n", expr_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
}

static void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_var_declaration");
    symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL);
    const char *type_c = get_c_type(gen->arena, stmt->type);
    char *var_name = get_var_name(gen->arena, stmt->name);
    char *init_str;
    if (stmt->initializer)
    {
        init_str = code_gen_expression(gen, stmt->initializer);
    }
    else
    {
        init_str = arena_strdup(gen->arena, get_default_value(stmt->type));
    }
    indented_fprintf(gen, indent, "%s %s = %s;\n", type_c, var_name, init_str);
}

static void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_free_locals");
    Symbol *sym = scope->symbols;
    while (sym)
    {
        if (sym->type && sym->type->kind == TYPE_STRING && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = get_var_name(gen->arena, sym->name);
            indented_fprintf(gen, indent, "if (%s) {\n", var_name);
            if (is_function && gen->current_return_type && gen->current_return_type->kind == TYPE_STRING)
            {
                indented_fprintf(gen, indent + 1, "if (%s != _return_value) {\n", var_name);
                indented_fprintf(gen, indent + 2, "rt_free_string(%s);\n", var_name);
                indented_fprintf(gen, indent + 1, "}\n");
            }
            else
            {
                indented_fprintf(gen, indent + 1, "rt_free_string(%s);\n", var_name);
            }
            indented_fprintf(gen, indent, "}\n");
        }
        sym = sym->next;
    }
}

void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_block");
    symbol_table_push_scope(gen->symbol_table);
    indented_fprintf(gen, indent, "{\n");
    for (int i = 0; i < stmt->count; i++)
    {
        code_gen_statement(gen, stmt->statements[i], indent + 1);
    }
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");
    symbol_table_pop_scope(gen->symbol_table);
}

void code_gen_function(CodeGen *gen, FunctionStmt *stmt)
{
    DEBUG_VERBOSE("Entering code_gen_function");
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;
    gen->current_function = get_var_name(gen->arena, stmt->name);
    gen->current_return_type = stmt->return_type;
    bool is_main = strcmp(gen->current_function, "main") == 0;
    // Special case for main: always use "int" return type in C for standard entry point.
    const char *ret_c = is_main ? "int" : get_c_type(gen->arena, gen->current_return_type);
    // Determine if we need a _return_value variable: only for non-void or main.
    bool has_return_value = (gen->current_return_type && gen->current_return_type->kind != TYPE_VOID) || is_main;
    symbol_table_push_scope(gen->symbol_table);
    for (int i = 0; i < stmt->param_count; i++)
    {
        symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->params[i].name, stmt->params[i].type, SYMBOL_PARAM);
    }
    indented_fprintf(gen, 0, "%s %s(", ret_c, gen->current_function);
    for (int i = 0; i < stmt->param_count; i++)
    {
        const char *param_type_c = get_c_type(gen->arena, stmt->params[i].type);
        char *param_name = get_var_name(gen->arena, stmt->params[i].name);
        fprintf(gen->output, "%s %s", param_type_c, param_name);
        if (i < stmt->param_count - 1)
        {
            fprintf(gen->output, ", ");
        }
    }
    indented_fprintf(gen, 0, ") {\n");
    // Add _return_value only if needed (non-void or main).
    if (has_return_value)
    {
        const char *default_val = is_main ? "0" : get_default_value(gen->current_return_type);
        indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_c, default_val);
    }
    bool has_return = false;
    if (stmt->body_count > 0 && stmt->body[stmt->body_count - 1]->type == STMT_RETURN)
    {
        has_return = true;
    }
    for (int i = 0; i < stmt->body_count; i++)
    {
        code_gen_statement(gen, stmt->body[i], 1);
    }
    if (!has_return)
    {
        indented_fprintf(gen, 1, "goto %s_return;\n", gen->current_function);
    }
    indented_fprintf(gen, 0, "%s_return:\n", gen->current_function);
    code_gen_free_locals(gen, gen->symbol_table->current, true, 1);
    // Return _return_value only if needed; otherwise, plain return.
    if (has_return_value)
    {
        indented_fprintf(gen, 1, "return _return_value;\n");
    }
    else
    {
        indented_fprintf(gen, 1, "return;\n");
    }
    indented_fprintf(gen, 0, "}\n\n");
    symbol_table_pop_scope(gen->symbol_table);
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;
}

void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_return_statement");
    if (stmt->value)
    {
        char *value_str = code_gen_expression(gen, stmt->value);
        indented_fprintf(gen, indent, "_return_value = %s;\n", value_str);
    }
    indented_fprintf(gen, indent, "goto %s_return;\n", gen->current_function);
}

void code_gen_if_statement(CodeGen *gen, IfStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_if_statement");
    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "if (%s) {\n", cond_str);
    code_gen_statement(gen, stmt->then_branch, indent + 1);
    indented_fprintf(gen, indent, "}\n");
    if (stmt->else_branch)
    {
        indented_fprintf(gen, indent, "else {\n");
        code_gen_statement(gen, stmt->else_branch, indent + 1);
        indented_fprintf(gen, indent, "}\n");
    }
}

void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_while_statement");
    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "while (%s) {\n", cond_str);
    code_gen_statement(gen, stmt->body, indent + 1);
    indented_fprintf(gen, indent, "}\n");
}

void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_statement");
    symbol_table_push_scope(gen->symbol_table);
    indented_fprintf(gen, indent, "{\n");
    if (stmt->initializer)
    {
        code_gen_statement(gen, stmt->initializer, indent + 1);
    }
    char *cond_str = NULL;
    if (stmt->condition)
    {
        cond_str = code_gen_expression(gen, stmt->condition);
    }
    indented_fprintf(gen, indent + 1, "while (%s) {\n", cond_str ? cond_str : "1");
    code_gen_statement(gen, stmt->body, indent + 2);
    if (stmt->increment)
    {
        char *inc_str = code_gen_expression(gen, stmt->increment);
        indented_fprintf(gen, indent + 2, "%s;\n", inc_str);
    }
    indented_fprintf(gen, indent + 1, "}\n");
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");
    symbol_table_pop_scope(gen->symbol_table);
}

void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_statement");
    switch (stmt->type)
    {
    case STMT_EXPR:
        code_gen_expression_statement(gen, &stmt->as.expression, indent);
        break;
    case STMT_VAR_DECL:
        code_gen_var_declaration(gen, &stmt->as.var_decl, indent);
        break;
    case STMT_FUNCTION:
        code_gen_function(gen, &stmt->as.function);
        break;
    case STMT_RETURN:
        code_gen_return_statement(gen, &stmt->as.return_stmt, indent);
        break;
    case STMT_BLOCK:
        code_gen_block(gen, &stmt->as.block, indent);
        break;
    case STMT_IF:
        code_gen_if_statement(gen, &stmt->as.if_stmt, indent);
        break;
    case STMT_WHILE:
        code_gen_while_statement(gen, &stmt->as.while_stmt, indent);
        break;
    case STMT_FOR:
        code_gen_for_statement(gen, &stmt->as.for_stmt, indent);
        break;
    case STMT_IMPORT:
        break;
    }
}

void code_gen_module(CodeGen *gen, Module *module)
{
    DEBUG_VERBOSE("Entering code_gen_module");
    code_gen_headers(gen);
    code_gen_externs(gen);
    bool has_main = false;
    for (int i = 0; i < module->count; i++)
    {
        if (module->statements[i]->type == STMT_FUNCTION)
        {
            char *name = get_var_name(gen->arena, module->statements[i]->as.function.name);
            if (strcmp(name, "main") == 0)
            {
                has_main = true;
            }
        }
        code_gen_statement(gen, module->statements[i], 0);
    }
    if (!has_main)
    {
        // If no main is defined, add a dummy int main() for valid C program entry point.
        indented_fprintf(gen, 0, "int main() {\n");
        indented_fprintf(gen, 1, "return 0;\n");
        indented_fprintf(gen, 0, "}\n");
    }
}
