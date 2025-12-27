#include "code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *arena_vsprintf(Arena *arena, const char *fmt, va_list args)
{
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

char *arena_sprintf(Arena *arena, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *result = arena_vsprintf(arena, fmt, args);
    va_end(args);
    return result;
}

char *escape_char_literal(Arena *arena, char ch)
{
    DEBUG_VERBOSE("Entering escape_char_literal");
    char buf[16];
    if (ch == '\'') {
        sprintf(buf, "'\\''");
    } else if (ch == '\\') {
        sprintf(buf, "'\\\\'");
    } else if (ch == '\n') {
        sprintf(buf, "'\\n'");
    } else if (ch == '\t') {
        sprintf(buf, "'\\t'");
    } else if (ch == '\r') {
        sprintf(buf, "'\\r'");
    } else if (ch == '\0') {
        sprintf(buf, "'\\0'");
    } else if ((unsigned char)ch >= 0x80 || ch < 0) {
        sprintf(buf, "'\\x%02x'", (unsigned char)ch);
    } else if (ch < ' ' || ch > '~') {
        sprintf(buf, "'\\x%02x'", (unsigned char)ch);
    } else {
        sprintf(buf, "'%c'", ch);
    }
    return arena_strdup(arena, buf);
}

char *escape_c_string(Arena *arena, const char *str)
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

const char *get_c_type(Arena *arena, Type *type)
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
    case TYPE_BYTE:
        return arena_strdup(arena, "unsigned char");
    case TYPE_VOID:
        return arena_strdup(arena, "void");
    case TYPE_NIL:
        return arena_strdup(arena, "void *");
    case TYPE_ANY:
        return arena_strdup(arena, "void *");
    case TYPE_TEXT_FILE:
        return arena_strdup(arena, "RtTextFile *");
    case TYPE_BINARY_FILE:
        return arena_strdup(arena, "RtBinaryFile *");
    case TYPE_ARRAY:
    {
        // For bool arrays, use int* since runtime stores bools as int internally
        const char *element_c_type;
        if (type->as.array.element_type->kind == TYPE_BOOL)
        {
            element_c_type = "int";
        }
        else
        {
            element_c_type = get_c_type(arena, type->as.array.element_type);
        }
        if (type->as.array.element_type->kind == TYPE_ARRAY)
        {
            size_t len = strlen(element_c_type) + 10;
            char *result = arena_alloc(arena, len);
            snprintf(result, len, "%s (*)[]", element_c_type);
            return result;
        }
        else
        {
            size_t len = strlen(element_c_type) + 3;
            char *result = arena_alloc(arena, len);
            snprintf(result, len, "%s *", element_c_type);
            return result;
        }
    }
    case TYPE_FUNCTION:
    {
        /* Function values are represented as closures */
        return arena_strdup(arena, "__Closure__ *");
    }
    default:
        fprintf(stderr, "Error: Unknown type kind %d\n", type->kind);
        exit(1);
    }

    return NULL;
}

const char *get_rt_to_string_func(TypeKind kind)
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
    case TYPE_STRING:
        return "rt_to_string_string";
    case TYPE_BOOL:
        return "rt_to_string_bool";
    case TYPE_BYTE:
        return "rt_to_string_byte";
    case TYPE_VOID:
        return "rt_to_string_void";
    case TYPE_NIL:
    case TYPE_ANY:
    case TYPE_ARRAY:
    case TYPE_FUNCTION:
    case TYPE_TEXT_FILE:
    case TYPE_BINARY_FILE:
        return "rt_to_string_pointer";
    default:
        exit(1);
    }
    return NULL;
}

const char *get_default_value(Type *type)
{
    DEBUG_VERBOSE("Entering get_default_value");
    if (type->kind == TYPE_STRING || type->kind == TYPE_ARRAY ||
        type->kind == TYPE_TEXT_FILE || type->kind == TYPE_BINARY_FILE)
    {
        return "NULL";
    }
    else
    {
        return "0";
    }
}

char *get_var_name(Arena *arena, Token name)
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

void indented_fprintf(CodeGen *gen, int indent, const char *fmt, ...)
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

char *code_gen_binary_op_str(TokenType op)
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
        return NULL;
    }
}

char *code_gen_type_suffix(Type *type)
{
    DEBUG_VERBOSE("Entering code_gen_type_suffix");
    if (type == NULL)
    {
        return "void";
    }
    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_CHAR:
    case TYPE_BYTE:
        return "long";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "bool";
    default:
        return "void";
    }
}

/* ============================================================================
 * Constant Folding Optimization
 * ============================================================================
 * These functions detect compile-time constant expressions and evaluate them
 * at compile time to generate direct literals instead of runtime function calls.
 * For example: rt_add_long(5L, 3L) becomes 8L
 */

bool is_constant_expr(Expr *expr)
{
    if (expr == NULL)
    {
        return false;
    }

    switch (expr->type)
    {
    case EXPR_LITERAL:
    {
        /* Literals are constant if they're numeric or boolean */
        Type *type = expr->as.literal.type;
        if (type == NULL) return false;
        return type->kind == TYPE_INT ||
               type->kind == TYPE_LONG ||
               type->kind == TYPE_DOUBLE ||
               type->kind == TYPE_BOOL;
    }
    case EXPR_BINARY:
    {
        /* Binary expressions are constant if both operands are constant
           and the operator is a foldable arithmetic/comparison op */
        TokenType op = expr->as.binary.operator;
        /* Only fold arithmetic and comparison operations */
        if (op == TOKEN_PLUS || op == TOKEN_MINUS ||
            op == TOKEN_STAR || op == TOKEN_SLASH ||
            op == TOKEN_MODULO ||
            op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL ||
            op == TOKEN_LESS || op == TOKEN_LESS_EQUAL ||
            op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL ||
            op == TOKEN_AND || op == TOKEN_OR)
        {
            return is_constant_expr(expr->as.binary.left) &&
                   is_constant_expr(expr->as.binary.right);
        }
        return false;
    }
    case EXPR_UNARY:
    {
        /* Unary expressions are constant if the operand is constant */
        TokenType op = expr->as.unary.operator;
        if (op == TOKEN_MINUS || op == TOKEN_BANG)
        {
            return is_constant_expr(expr->as.unary.operand);
        }
        return false;
    }
    default:
        return false;
    }
}

bool try_fold_constant(Expr *expr, int64_t *out_int_value, double *out_double_value, bool *out_is_double)
{
    if (expr == NULL)
    {
        return false;
    }

    switch (expr->type)
    {
    case EXPR_LITERAL:
    {
        Type *type = expr->as.literal.type;
        if (type == NULL) return false;

        switch (type->kind)
        {
        case TYPE_INT:
        case TYPE_LONG:
            *out_int_value = expr->as.literal.value.int_value;
            *out_is_double = false;
            return true;
        case TYPE_DOUBLE:
            *out_double_value = expr->as.literal.value.double_value;
            *out_is_double = true;
            return true;
        case TYPE_BOOL:
            *out_int_value = expr->as.literal.value.bool_value ? 1 : 0;
            *out_is_double = false;
            return true;
        default:
            return false;
        }
    }

    case EXPR_UNARY:
    {
        int64_t operand_int;
        double operand_double;
        bool operand_is_double;

        if (!try_fold_constant(expr->as.unary.operand, &operand_int, &operand_double, &operand_is_double))
        {
            return false;
        }

        TokenType op = expr->as.unary.operator;
        switch (op)
        {
        case TOKEN_MINUS:
            if (operand_is_double)
            {
                *out_double_value = -operand_double;
                *out_is_double = true;
            }
            else
            {
                *out_int_value = -operand_int;
                *out_is_double = false;
            }
            return true;
        case TOKEN_BANG:
            /* Logical not - result is always an integer (boolean) */
            if (operand_is_double)
            {
                *out_int_value = (operand_double == 0.0) ? 1 : 0;
            }
            else
            {
                *out_int_value = (operand_int == 0) ? 1 : 0;
            }
            *out_is_double = false;
            return true;
        default:
            return false;
        }
    }

    case EXPR_BINARY:
    {
        int64_t left_int, right_int;
        double left_double, right_double;
        bool left_is_double, right_is_double;

        if (!try_fold_constant(expr->as.binary.left, &left_int, &left_double, &left_is_double))
        {
            return false;
        }
        if (!try_fold_constant(expr->as.binary.right, &right_int, &right_double, &right_is_double))
        {
            return false;
        }

        /* Promote to double if either operand is double */
        bool result_is_double = left_is_double || right_is_double;

        /* Convert to common type */
        double left_val = left_is_double ? left_double : (double)left_int;
        double right_val = right_is_double ? right_double : (double)right_int;

        TokenType op = expr->as.binary.operator;

        /* For comparison and logical operators, result is always integer (bool) */
        if (op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL ||
            op == TOKEN_LESS || op == TOKEN_LESS_EQUAL ||
            op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL ||
            op == TOKEN_AND || op == TOKEN_OR)
        {
            *out_is_double = false;
            switch (op)
            {
            case TOKEN_EQUAL_EQUAL:
                *out_int_value = (left_val == right_val) ? 1 : 0;
                return true;
            case TOKEN_BANG_EQUAL:
                *out_int_value = (left_val != right_val) ? 1 : 0;
                return true;
            case TOKEN_LESS:
                *out_int_value = (left_val < right_val) ? 1 : 0;
                return true;
            case TOKEN_LESS_EQUAL:
                *out_int_value = (left_val <= right_val) ? 1 : 0;
                return true;
            case TOKEN_GREATER:
                *out_int_value = (left_val > right_val) ? 1 : 0;
                return true;
            case TOKEN_GREATER_EQUAL:
                *out_int_value = (left_val >= right_val) ? 1 : 0;
                return true;
            case TOKEN_AND:
                *out_int_value = ((left_val != 0) && (right_val != 0)) ? 1 : 0;
                return true;
            case TOKEN_OR:
                *out_int_value = ((left_val != 0) || (right_val != 0)) ? 1 : 0;
                return true;
            default:
                return false;
            }
        }

        /* Arithmetic operations */
        *out_is_double = result_is_double;

        if (result_is_double)
        {
            switch (op)
            {
            case TOKEN_PLUS:
                *out_double_value = left_val + right_val;
                return true;
            case TOKEN_MINUS:
                *out_double_value = left_val - right_val;
                return true;
            case TOKEN_STAR:
                *out_double_value = left_val * right_val;
                return true;
            case TOKEN_SLASH:
                if (right_val == 0.0)
                {
                    /* Division by zero - don't fold, let runtime handle */
                    return false;
                }
                *out_double_value = left_val / right_val;
                return true;
            case TOKEN_MODULO:
                /* Modulo on doubles is not standard - don't fold */
                return false;
            default:
                return false;
            }
        }
        else
        {
            /* Integer arithmetic */
            switch (op)
            {
            case TOKEN_PLUS:
                *out_int_value = left_int + right_int;
                return true;
            case TOKEN_MINUS:
                *out_int_value = left_int - right_int;
                return true;
            case TOKEN_STAR:
                *out_int_value = left_int * right_int;
                return true;
            case TOKEN_SLASH:
                if (right_int == 0)
                {
                    /* Division by zero - don't fold, let runtime handle */
                    return false;
                }
                *out_int_value = left_int / right_int;
                return true;
            case TOKEN_MODULO:
                if (right_int == 0)
                {
                    /* Division by zero - don't fold, let runtime handle */
                    return false;
                }
                *out_int_value = left_int % right_int;
                return true;
            default:
                return false;
            }
        }
    }

    default:
        return false;
    }
}

char *try_constant_fold_binary(CodeGen *gen, BinaryExpr *expr)
{
    /* Create a temporary Expr wrapper to use try_fold_constant */
    Expr temp_expr;
    temp_expr.type = EXPR_BINARY;
    temp_expr.as.binary = *expr;

    int64_t int_result;
    double double_result;
    bool is_double;

    if (!try_fold_constant(&temp_expr, &int_result, &double_result, &is_double))
    {
        return NULL;
    }

    if (is_double)
    {
        char *str = arena_sprintf(gen->arena, "%.17g", double_result);
        /* Ensure the literal has a decimal point for doubles */
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            str = arena_sprintf(gen->arena, "%s.0", str);
        }
        return str;
    }
    else
    {
        return arena_sprintf(gen->arena, "%ldL", int_result);
    }
}

char *try_constant_fold_unary(CodeGen *gen, UnaryExpr *expr)
{
    /* Create a temporary Expr wrapper to use try_fold_constant */
    Expr temp_expr;
    temp_expr.type = EXPR_UNARY;
    temp_expr.as.unary = *expr;

    int64_t int_result;
    double double_result;
    bool is_double;

    if (!try_fold_constant(&temp_expr, &int_result, &double_result, &is_double))
    {
        return NULL;
    }

    if (is_double)
    {
        char *str = arena_sprintf(gen->arena, "%.17g", double_result);
        /* Ensure the literal has a decimal point for doubles */
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            str = arena_sprintf(gen->arena, "%s.0", str);
        }
        return str;
    }
    else
    {
        return arena_sprintf(gen->arena, "%ldL", int_result);
    }
}

/* ============================================================================
 * Native C Operator Generation for Unchecked Arithmetic Mode
 * ============================================================================
 * These functions generate native C operators instead of runtime function calls
 * when ARITH_UNCHECKED mode is enabled. This eliminates function call overhead
 * but removes overflow checking.
 */

const char *get_native_c_operator(TokenType op)
{
    switch (op)
    {
    case TOKEN_PLUS:
        return "+";
    case TOKEN_MINUS:
        return "-";
    case TOKEN_STAR:
        return "*";
    case TOKEN_SLASH:
        return "/";
    case TOKEN_MODULO:
        return "%";
    case TOKEN_EQUAL_EQUAL:
        return "==";
    case TOKEN_BANG_EQUAL:
        return "!=";
    case TOKEN_LESS:
        return "<";
    case TOKEN_LESS_EQUAL:
        return "<=";
    case TOKEN_GREATER:
        return ">";
    case TOKEN_GREATER_EQUAL:
        return ">=";
    default:
        return NULL;
    }
}

bool can_use_native_operator(TokenType op)
{
    /* Division and modulo still need runtime functions for zero-division check.
       All other arithmetic and comparison operators can use native C operators. */
    switch (op)
    {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_BANG_EQUAL:
    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
        return true;
    case TOKEN_SLASH:
    case TOKEN_MODULO:
        /* Keep runtime functions for division/modulo to check for zero */
        return false;
    default:
        return false;
    }
}

char *gen_native_arithmetic(CodeGen *gen, const char *left_str, const char *right_str,
                            TokenType op, Type *type)
{
    if (gen->arithmetic_mode != ARITH_UNCHECKED)
    {
        return NULL;  /* Not in unchecked mode, use runtime functions */
    }

    if (!can_use_native_operator(op))
    {
        return NULL;  /* This operator needs runtime function (div/mod) */
    }

    const char *c_op = get_native_c_operator(op);
    if (c_op == NULL)
    {
        return NULL;  /* Unknown operator */
    }

    /* Generate native C expression */
    if (type->kind == TYPE_DOUBLE)
    {
        /* For doubles, no suffix needed */
        return arena_sprintf(gen->arena, "((%s) %s (%s))", left_str, c_op, right_str);
    }
    else if (type->kind == TYPE_INT || type->kind == TYPE_LONG)
    {
        /* For integers, result is long */
        return arena_sprintf(gen->arena, "((long)((%s) %s (%s)))", left_str, c_op, right_str);
    }
    else if (type->kind == TYPE_BOOL)
    {
        /* For booleans, comparison operations return 0 or 1 */
        return arena_sprintf(gen->arena, "((%s) %s (%s))", left_str, c_op, right_str);
    }

    return NULL;  /* Unsupported type */
}

char *gen_native_unary(CodeGen *gen, const char *operand_str, TokenType op, Type *type)
{
    if (gen->arithmetic_mode != ARITH_UNCHECKED)
    {
        return NULL;  /* Not in unchecked mode, use runtime functions */
    }

    switch (op)
    {
    case TOKEN_MINUS:
        if (type->kind == TYPE_DOUBLE)
        {
            return arena_sprintf(gen->arena, "(-(%s))", operand_str);
        }
        else if (type->kind == TYPE_INT || type->kind == TYPE_LONG)
        {
            return arena_sprintf(gen->arena, "((long)(-(%s)))", operand_str);
        }
        break;
    case TOKEN_BANG:
        return arena_sprintf(gen->arena, "(!(%s))", operand_str);
    default:
        break;
    }

    return NULL;  /* Unsupported operator or type */
}

/* ============================================================================
 * Arena Requirement Analysis
 * ============================================================================
 * These functions analyze AST nodes to determine if they require arena
 * allocation. Functions that only use primitives don't need to create/destroy
 * arenas, which reduces overhead.
 */

/* Check if a type requires arena allocation */
static bool type_needs_arena(Type *type)
{
    if (type == NULL) return false;

    switch (type->kind)
    {
    case TYPE_STRING:
    case TYPE_ARRAY:
    case TYPE_FUNCTION:  /* Closures need arena */
        return true;
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_DOUBLE:
    case TYPE_CHAR:
    case TYPE_BOOL:
    case TYPE_BYTE:
    case TYPE_VOID:
    case TYPE_NIL:
    case TYPE_ANY:
    default:
        return false;
    }
}

bool expr_needs_arena(Expr *expr)
{
    if (expr == NULL) return false;

    switch (expr->type)
    {
    case EXPR_LITERAL:
        /* String literals don't need arena when just reading them,
           but they do when assigned to variables (handled in var_decl) */
        return false;

    case EXPR_VARIABLE:
        /* Variable references don't need arena, even function references.
           The function's closure was already allocated elsewhere.
           The type check is skipped here - a function reference doesn't
           mean we're creating a closure. */
        return false;

    case EXPR_BINARY:
        /* String concatenation needs arena */
        if (expr->as.binary.left->expr_type &&
            expr->as.binary.left->expr_type->kind == TYPE_STRING)
        {
            return true;
        }
        return expr_needs_arena(expr->as.binary.left) ||
               expr_needs_arena(expr->as.binary.right);

    case EXPR_UNARY:
        return expr_needs_arena(expr->as.unary.operand);

    case EXPR_ASSIGN:
        return expr_needs_arena(expr->as.assign.value);

    case EXPR_INDEX_ASSIGN:
        return expr_needs_arena(expr->as.index_assign.array) ||
               expr_needs_arena(expr->as.index_assign.index) ||
               expr_needs_arena(expr->as.index_assign.value);

    case EXPR_CALL:
        /* Function calls may return strings/arrays */
        if (type_needs_arena(expr->expr_type))
        {
            return true;
        }
        /* Check arguments */
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            if (expr_needs_arena(expr->as.call.arguments[i]))
            {
                return true;
            }
        }
        /* Check callee - but skip if it's a simple function reference.
           Only complex callees (like method calls or computed functions)
           might need arena allocation. */
        if (expr->as.call.callee->type != EXPR_VARIABLE)
        {
            return expr_needs_arena(expr->as.call.callee);
        }
        return false;

    case EXPR_ARRAY:
        /* Array literals need arena */
        return true;

    case EXPR_ARRAY_ACCESS:
        return expr_needs_arena(expr->as.array_access.array) ||
               expr_needs_arena(expr->as.array_access.index);

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        return expr_needs_arena(expr->as.operand);

    case EXPR_INTERPOLATED:
        /* String interpolation always needs arena */
        return true;

    case EXPR_MEMBER:
        return expr_needs_arena(expr->as.member.object);

    case EXPR_ARRAY_SLICE:
        /* Slices create new arrays */
        return true;

    case EXPR_RANGE:
        /* Ranges create arrays */
        return true;

    case EXPR_SPREAD:
        return true;

    case EXPR_LAMBDA:
        /* Lambdas create closures */
        return true;

    default:
        return false;
    }
}

bool stmt_needs_arena(Stmt *stmt)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_EXPR:
        return expr_needs_arena(stmt->as.expression.expression);

    case STMT_VAR_DECL:
        /* Variable declarations with string/array types need arena */
        if (type_needs_arena(stmt->as.var_decl.type))
        {
            return true;
        }
        /* Also check initializer */
        if (stmt->as.var_decl.initializer)
        {
            return expr_needs_arena(stmt->as.var_decl.initializer);
        }
        /* 'as ref' needs arena for heap allocation */
        if (stmt->as.var_decl.mem_qualifier == MEM_AS_REF)
        {
            return true;
        }
        return false;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            return expr_needs_arena(stmt->as.return_stmt.value);
        }
        return false;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (stmt_needs_arena(stmt->as.block.statements[i]))
            {
                return true;
            }
        }
        return false;

    case STMT_IF:
        if (expr_needs_arena(stmt->as.if_stmt.condition))
        {
            return true;
        }
        if (stmt_needs_arena(stmt->as.if_stmt.then_branch))
        {
            return true;
        }
        if (stmt->as.if_stmt.else_branch &&
            stmt_needs_arena(stmt->as.if_stmt.else_branch))
        {
            return true;
        }
        return false;

    case STMT_WHILE:
        if (expr_needs_arena(stmt->as.while_stmt.condition))
        {
            return true;
        }
        return stmt_needs_arena(stmt->as.while_stmt.body);

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer &&
            stmt_needs_arena(stmt->as.for_stmt.initializer))
        {
            return true;
        }
        if (stmt->as.for_stmt.condition &&
            expr_needs_arena(stmt->as.for_stmt.condition))
        {
            return true;
        }
        if (stmt->as.for_stmt.increment &&
            expr_needs_arena(stmt->as.for_stmt.increment))
        {
            return true;
        }
        return stmt_needs_arena(stmt->as.for_stmt.body);

    case STMT_FOR_EACH:
        /* For-each iterates over arrays/strings */
        return true;

    case STMT_FUNCTION:
        /* Nested functions don't affect parent's arena needs */
        return false;

    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
    default:
        return false;
    }
}

bool function_needs_arena(FunctionStmt *fn)
{
    if (fn == NULL) return false;

    /* Check return type */
    if (type_needs_arena(fn->return_type))
    {
        return true;
    }

    /* Check parameters for 'as val' with string/array types */
    for (int i = 0; i < fn->param_count; i++)
    {
        if (fn->params[i].mem_qualifier == MEM_AS_VAL)
        {
            if (type_needs_arena(fn->params[i].type))
            {
                return true;
            }
        }
    }

    /* Check function body */
    for (int i = 0; i < fn->body_count; i++)
    {
        if (stmt_needs_arena(fn->body[i]))
        {
            return true;
        }
    }

    return false;
}

/* ============================================================================
 * Tail Call Optimization Helpers
 * ============================================================================ */

/* Check if an expression contains a marked tail call */
static bool expr_has_marked_tail_call(Expr *expr)
{
    if (expr == NULL) return false;

    switch (expr->type)
    {
    case EXPR_CALL:
        return expr->as.call.is_tail_call;

    case EXPR_BINARY:
        return expr_has_marked_tail_call(expr->as.binary.left) ||
               expr_has_marked_tail_call(expr->as.binary.right);

    case EXPR_UNARY:
        return expr_has_marked_tail_call(expr->as.unary.operand);

    case EXPR_ASSIGN:
        return expr_has_marked_tail_call(expr->as.assign.value);

    case EXPR_INDEX_ASSIGN:
        return expr_has_marked_tail_call(expr->as.index_assign.array) ||
               expr_has_marked_tail_call(expr->as.index_assign.index) ||
               expr_has_marked_tail_call(expr->as.index_assign.value);

    case EXPR_ARRAY_ACCESS:
        return expr_has_marked_tail_call(expr->as.array_access.array) ||
               expr_has_marked_tail_call(expr->as.array_access.index);

    default:
        return false;
    }
}

bool stmt_has_marked_tail_calls(Stmt *stmt)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_RETURN:
        return stmt->as.return_stmt.value &&
               expr_has_marked_tail_call(stmt->as.return_stmt.value);

    case STMT_EXPR:
        return expr_has_marked_tail_call(stmt->as.expression.expression);

    case STMT_VAR_DECL:
        return stmt->as.var_decl.initializer &&
               expr_has_marked_tail_call(stmt->as.var_decl.initializer);

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (stmt_has_marked_tail_calls(stmt->as.block.statements[i]))
            {
                return true;
            }
        }
        return false;

    case STMT_IF:
        if (stmt_has_marked_tail_calls(stmt->as.if_stmt.then_branch))
        {
            return true;
        }
        if (stmt->as.if_stmt.else_branch &&
            stmt_has_marked_tail_calls(stmt->as.if_stmt.else_branch))
        {
            return true;
        }
        return false;

    case STMT_WHILE:
        return stmt_has_marked_tail_calls(stmt->as.while_stmt.body);

    case STMT_FOR:
        return stmt_has_marked_tail_calls(stmt->as.for_stmt.body);

    case STMT_FOR_EACH:
        return stmt_has_marked_tail_calls(stmt->as.for_each_stmt.body);

    default:
        return false;
    }
}

bool function_has_marked_tail_calls(FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL) return false;

    for (int i = 0; i < fn->body_count; i++)
    {
        if (stmt_has_marked_tail_calls(fn->body[i]))
        {
            return true;
        }
    }
    return false;
}
