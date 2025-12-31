#include "code_gen_expr.h"
#include "code_gen_util.h"
#include "code_gen_stmt.h"
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

static void captured_vars_init(CapturedVars *cv)
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

/* Check if a type is a primitive that needs pointer indirection for capture-by-reference.
 * Primitives (int, long, double, bool, byte, char) need to be captured by pointer
 * so that mutations inside the lambda persist to the original variable.
 * Reference types (arrays, strings, files) are already pointers and don't need indirection. */
static bool is_primitive_type(Type *type)
{
    if (type == NULL) return false;
    switch (type->kind) {
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_BOOL:
        case TYPE_BYTE:
        case TYPE_CHAR:
            return true;
        default:
            return false;
    }
}

/* Check if an expression is provably non-negative (for array index optimization).
 * Returns true for:
 *   - Integer literals >= 0
 *   - Long literals >= 0
 *   - Variables that are tracked as loop counters (provably non-negative)
 * Returns false for negative literals, untracked variables, and all other expressions.
 */
bool is_provably_non_negative(CodeGen *gen, Expr *expr)
{
    if (expr == NULL) return false;

    /* Check for non-negative integer/long literals */
    if (expr->type == EXPR_LITERAL)
    {
        if (expr->as.literal.type == NULL) return false;

        if (expr->as.literal.type->kind == TYPE_INT ||
            expr->as.literal.type->kind == TYPE_LONG)
        {
            return expr->as.literal.value.int_value >= 0;
        }
        /* Other literal types (double, bool, etc.) are not valid array indices */
        return false;
    }

    /* Check for loop counter variables (provably non-negative) */
    if (expr->type == EXPR_VARIABLE)
    {
        char *var_name = get_var_name(gen->arena, expr->as.variable.name);
        return is_tracked_loop_counter(gen, var_name);
    }

    /* All other expression types are not provably non-negative */
    return false;
}

/* Helper structure for local variable tracking in lambda bodies */
typedef struct {
    char **names;
    int count;
    int capacity;
} LocalVars;

/* Helper structure for tracking enclosing lambda parameters */
typedef struct EnclosingLambdaContext {
    LambdaExpr **lambdas;
    int count;
    int capacity;
} EnclosingLambdaContext;

/* Check if a name is a parameter of any enclosing lambda, and get its type */
static Type *find_enclosing_lambda_param(EnclosingLambdaContext *ctx, const char *name)
{
    if (ctx == NULL) return NULL;
    for (int i = 0; i < ctx->count; i++)
    {
        LambdaExpr *lambda = ctx->lambdas[i];
        for (int j = 0; j < lambda->param_count; j++)
        {
            char param_name[256];
            int len = lambda->params[j].name.length < 255 ? lambda->params[j].name.length : 255;
            strncpy(param_name, lambda->params[j].name.start, len);
            param_name[len] = '\0';
            if (strcmp(param_name, name) == 0)
            {
                return lambda->params[j].type;
            }
        }
    }
    return NULL;
}

static void local_vars_init(LocalVars *lv)
{
    lv->names = NULL;
    lv->count = 0;
    lv->capacity = 0;
}

static void local_vars_add(LocalVars *lv, Arena *arena, const char *name)
{
    /* Check if already tracked */
    for (int i = 0; i < lv->count; i++)
    {
        if (strcmp(lv->names[i], name) == 0)
            return;
    }
    /* Grow array if needed */
    if (lv->count >= lv->capacity)
    {
        int new_cap = lv->capacity == 0 ? 8 : lv->capacity * 2;
        char **new_names = arena_alloc(arena, new_cap * sizeof(char *));
        for (int i = 0; i < lv->count; i++)
        {
            new_names[i] = lv->names[i];
        }
        lv->names = new_names;
        lv->capacity = new_cap;
    }
    lv->names[lv->count] = arena_strdup(arena, name);
    lv->count++;
}

static bool is_local_var(LocalVars *lv, const char *name)
{
    for (int i = 0; i < lv->count; i++)
    {
        if (strcmp(lv->names[i], name) == 0)
            return true;
    }
    return false;
}

/* Forward declaration for collecting locals from statements */
static void collect_local_vars_from_stmt(Stmt *stmt, LocalVars *lv, Arena *arena);

/* Collect local variable declarations from a statement */
static void collect_local_vars_from_stmt(Stmt *stmt, LocalVars *lv, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_VAR_DECL:
    {
        /* Add this variable to locals */
        char name[256];
        int len = stmt->as.var_decl.name.length < 255 ? stmt->as.var_decl.name.length : 255;
        strncpy(name, stmt->as.var_decl.name.start, len);
        name[len] = '\0';
        local_vars_add(lv, arena, name);
        break;
    }
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            collect_local_vars_from_stmt(stmt->as.block.statements[i], lv, arena);
        break;
    case STMT_IF:
        collect_local_vars_from_stmt(stmt->as.if_stmt.then_branch, lv, arena);
        if (stmt->as.if_stmt.else_branch)
            collect_local_vars_from_stmt(stmt->as.if_stmt.else_branch, lv, arena);
        break;
    case STMT_WHILE:
        collect_local_vars_from_stmt(stmt->as.while_stmt.body, lv, arena);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            collect_local_vars_from_stmt(stmt->as.for_stmt.initializer, lv, arena);
        collect_local_vars_from_stmt(stmt->as.for_stmt.body, lv, arena);
        break;
    case STMT_FOR_EACH:
    {
        /* The loop variable is a local */
        char name[256];
        int len = stmt->as.for_each_stmt.var_name.length < 255 ? stmt->as.for_each_stmt.var_name.length : 255;
        strncpy(name, stmt->as.for_each_stmt.var_name.start, len);
        name[len] = '\0';
        local_vars_add(lv, arena, name);
        collect_local_vars_from_stmt(stmt->as.for_each_stmt.body, lv, arena);
        break;
    }
    default:
        break;
    }
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

/* Forward declaration for statement traversal */
static void collect_captured_vars_from_stmt(Stmt *stmt, LambdaExpr *lambda, SymbolTable *table,
                                            CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena);

/* Recursively collect captured variables from an expression */
static void collect_captured_vars(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                                  CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena)
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

        /* Skip if it's a local variable declared in the lambda body */
        if (lv != NULL && is_local_var(lv, name))
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
        else
        {
            /* Check if it's a parameter from an enclosing lambda */
            Type *enclosing_type = find_enclosing_lambda_param(enclosing, name);
            if (enclosing_type != NULL)
            {
                captured_vars_add(cv, arena, name, enclosing_type);
            }
        }
        break;
    }
    case EXPR_BINARY:
        collect_captured_vars(expr->as.binary.left, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars(expr->as.binary.right, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_UNARY:
        collect_captured_vars(expr->as.unary.operand, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ASSIGN:
        collect_captured_vars(expr->as.assign.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INDEX_ASSIGN:
        collect_captured_vars(expr->as.index_assign.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars(expr->as.index_assign.index, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars(expr->as.index_assign.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_CALL:
        collect_captured_vars(expr->as.call.callee, lambda, table, cv, lv, enclosing, arena);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            collect_captured_vars(expr->as.call.arguments[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
            collect_captured_vars(expr->as.array.elements[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY_ACCESS:
        collect_captured_vars(expr->as.array_access.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars(expr->as.array_access.index, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        collect_captured_vars(expr->as.operand, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            collect_captured_vars(expr->as.interpol.parts[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_MEMBER:
        collect_captured_vars(expr->as.member.object, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY_SLICE:
        collect_captured_vars(expr->as.array_slice.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars(expr->as.array_slice.start, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars(expr->as.array_slice.end, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars(expr->as.array_slice.step, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_RANGE:
        collect_captured_vars(expr->as.range.start, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars(expr->as.range.end, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_SPREAD:
        collect_captured_vars(expr->as.spread.array, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_LAMBDA:
        /* Recurse into nested lambdas to collect transitive captures */
        /* Variables captured by nested lambdas that are from outer scopes
           need to be captured by this lambda too */
        {
            LambdaExpr *nested_lambda = &expr->as.lambda;
            if (nested_lambda->has_stmt_body)
            {
                for (int i = 0; i < nested_lambda->body_stmt_count; i++)
                {
                    collect_captured_vars_from_stmt(nested_lambda->body_stmts[i], lambda, table, cv, lv, enclosing, arena);
                }
            }
            else if (nested_lambda->body)
            {
                collect_captured_vars(nested_lambda->body, lambda, table, cv, lv, enclosing, arena);
            }
        }
        break;
    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            collect_captured_vars(expr->as.static_call.arguments[i], lambda, table, cv, lv, enclosing, arena);
        }
        break;
    case EXPR_LITERAL:
    default:
        break;
    }
}

/* Recursively collect captured variables from a statement */
static void collect_captured_vars_from_stmt(Stmt *stmt, LambdaExpr *lambda, SymbolTable *table,
                                            CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        collect_captured_vars(stmt->as.expression.expression, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
            collect_captured_vars(stmt->as.var_decl.initializer, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            collect_captured_vars(stmt->as.return_stmt.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            collect_captured_vars_from_stmt(stmt->as.block.statements[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_IF:
        collect_captured_vars(stmt->as.if_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.if_stmt.then_branch, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.if_stmt.else_branch)
            collect_captured_vars_from_stmt(stmt->as.if_stmt.else_branch, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_WHILE:
        collect_captured_vars(stmt->as.while_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.while_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            collect_captured_vars_from_stmt(stmt->as.for_stmt.initializer, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.for_stmt.condition)
            collect_captured_vars(stmt->as.for_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.for_stmt.increment)
            collect_captured_vars(stmt->as.for_stmt.increment, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.for_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FOR_EACH:
        collect_captured_vars(stmt->as.for_each_stmt.iterable, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.for_each_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FUNCTION:
        /* Don't recurse into nested functions - they have their own scope */
        break;
    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
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

/* Check if an expression is a string literal - can be used directly without copying */
static bool is_string_literal_expr(Expr *expr)
{
    if (expr == NULL) return false;
    if (expr->type != EXPR_LITERAL) return false;
    if (expr->expr_type == NULL) return false;
    return expr->expr_type->kind == TYPE_STRING;
}

/* Helper to determine if a type is numeric */
static bool is_numeric(Type *type)
{
    return type && (type->kind == TYPE_INT || type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE);
}

/* Helper to get the promoted type for binary operations with mixed numeric types */
static Type *get_binary_promoted_type(Type *left, Type *right)
{
    if (left == NULL || right == NULL) return left;

    /* If both are numeric, promote to the wider type */
    if (is_numeric(left) && is_numeric(right))
    {
        /* double is the widest */
        if (left->kind == TYPE_DOUBLE || right->kind == TYPE_DOUBLE)
        {
            /* Return whichever is double */
            return left->kind == TYPE_DOUBLE ? left : right;
        }
        /* long is wider than int */
        if (left->kind == TYPE_LONG || right->kind == TYPE_LONG)
        {
            return left->kind == TYPE_LONG ? left : right;
        }
    }
    /* Otherwise use left type */
    return left;
}

char *code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_binary_expression");

    /* Try constant folding first - if both operands are constants,
       evaluate at compile time and emit a direct literal */
    char *folded = try_constant_fold_binary(gen, expr);
    if (folded != NULL)
    {
        return folded;
    }

    char *left_str = code_gen_expression(gen, expr->left);
    char *right_str = code_gen_expression(gen, expr->right);
    Type *left_type = expr->left->expr_type;
    Type *right_type = expr->right->expr_type;
    /* Use promoted type for mixed numeric operations */
    Type *type = get_binary_promoted_type(left_type, right_type);
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
            case TYPE_BYTE:
                arr_suffix = "byte";
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
        /* Try to use native C operators in unchecked mode */
        char *native = gen_native_arithmetic(gen, left_str, right_str, op, type);
        if (native != NULL)
        {
            return native;
        }
        /* Fall back to runtime functions (checked mode or div/mod) */
        return arena_sprintf(gen->arena, "rt_%s_%s(%s, %s)", op_str, suffix, left_str, right_str);
    }
}

char *code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_unary_expression");

    /* Try constant folding first - if operand is a constant,
       evaluate at compile time and emit a direct literal */
    char *folded = try_constant_fold_unary(gen, expr);
    if (folded != NULL)
    {
        return folded;
    }

    char *operand_str = code_gen_expression(gen, expr->operand);
    Type *type = expr->operand->expr_type;

    /* Try to use native C operators in unchecked mode */
    char *native = gen_native_unary(gen, operand_str, expr->operator, type);
    if (native != NULL)
    {
        return native;
    }

    /* Fall back to runtime functions (checked mode) */
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

    // Check if we're inside a lambda and this is a lambda parameter.
    // Lambda parameters shadow outer variables, so don't look up in symbol table.
    if (gen->enclosing_lambda_count > 0)
    {
        LambdaExpr *innermost = gen->enclosing_lambdas[gen->enclosing_lambda_count - 1];
        if (is_lambda_param(innermost, var_name))
        {
            // It's a parameter of the innermost lambda - use directly, no dereference
            return var_name;
        }
    }

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

char *code_gen_index_assign_expression(CodeGen *gen, IndexAssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_index_assign_expression");
    char *array_str = code_gen_expression(gen, expr->array);
    char *index_str = code_gen_expression(gen, expr->index);
    char *value_str = code_gen_expression(gen, expr->value);

    // Check if index is provably non-negative (literal >= 0 or tracked loop counter)
    if (is_provably_non_negative(gen, expr->index))
    {
        // Non-negative index - direct array access
        return arena_sprintf(gen->arena, "(%s[%s] = %s)",
                             array_str, index_str, value_str);
    }

    // Check if index is a negative literal - can simplify to: arr[len + idx]
    if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        // Negative literal - adjust by array length
        return arena_sprintf(gen->arena, "(%s[rt_array_length(%s) + %s] = %s)",
                             array_str, array_str, index_str, value_str);
    }

    // For potentially negative variable indices, generate runtime check
    return arena_sprintf(gen->arena, "(%s[(%s) < 0 ? rt_array_length(%s) + (%s) : (%s)] = %s)",
                         array_str, index_str, array_str, index_str, index_str, value_str);
}

/* Helper to get the format function for a type kind */
static const char *get_rt_format_func(TypeKind kind)
{
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return "rt_format_long";
    case TYPE_DOUBLE:
        return "rt_format_double";
    case TYPE_STRING:
        return "rt_format_string";
    default:
        return NULL;  /* No format function for this type */
    }
}

/* Check if any part has a format specifier */
static bool has_any_format_spec(InterpolExpr *expr)
{
    if (expr->format_specs == NULL) return false;
    for (int i = 0; i < expr->part_count; i++)
    {
        if (expr->format_specs[i] != NULL) return true;
    }
    return false;
}

char *code_gen_interpolated_expression(CodeGen *gen, InterpolExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_interpolated_expression");
    int count = expr->part_count;
    if (count == 0)
    {
        /* Empty interpolation - just return empty string literal directly */
        return "\"\"";
    }

    /* Gather information about each part */
    char **part_strs = arena_alloc(gen->arena, count * sizeof(char *));
    Type **part_types = arena_alloc(gen->arena, count * sizeof(Type *));
    bool *is_literal = arena_alloc(gen->arena, count * sizeof(bool));
    bool *is_temp = arena_alloc(gen->arena, count * sizeof(bool));

    int non_literal_count = 0;
    int needs_conversion_count = 0;
    bool uses_format_specs = has_any_format_spec(expr);

    for (int i = 0; i < count; i++)
    {
        part_strs[i] = code_gen_expression(gen, expr->parts[i]);
        part_types[i] = expr->parts[i]->expr_type;
        is_literal[i] = is_string_literal_expr(expr->parts[i]);
        is_temp[i] = expression_produces_temp(expr->parts[i]);

        if (!is_literal[i]) non_literal_count++;
        if (part_types[i]->kind != TYPE_STRING) needs_conversion_count++;
    }

    /* Optimization: Single string literal without format - use directly */
    if (count == 1 && is_literal[0] && !uses_format_specs)
    {
        return part_strs[0];
    }

    /* Optimization: Single string variable/temp without format - return as is or copy */
    if (count == 1 && part_types[0]->kind == TYPE_STRING && !uses_format_specs)
    {
        if (is_temp[0])
        {
            return part_strs[0];
        }
        else if (is_literal[0])
        {
            return part_strs[0];
        }
        else
        {
            /* Variable needs to be copied to arena */
            return arena_sprintf(gen->arena, "rt_to_string_string(%s, %s)", ARENA_VAR(gen), part_strs[0]);
        }
    }

    /* Optimization: Two string literals or all literals without format - simple concat */
    if (count == 2 && needs_conversion_count == 0 && !is_temp[0] && !is_temp[1] && !uses_format_specs)
    {
        return arena_sprintf(gen->arena, "rt_str_concat(%s, %s, %s)", ARENA_VAR(gen), part_strs[0], part_strs[1]);
    }

    /* General case: Need to build a block expression */
    char *result = arena_strdup(gen->arena, "({\n");

    /* Track which parts need temp variables and which can be used directly */
    char **use_strs = arena_alloc(gen->arena, count * sizeof(char *));
    int temp_var_count = 0;

    /* First pass: convert non-strings and capture temps, handle format specifiers */
    for (int i = 0; i < count; i++)
    {
        char *format_spec = (expr->format_specs != NULL) ? expr->format_specs[i] : NULL;

        if (format_spec != NULL)
        {
            /* Has format specifier - use rt_format_* functions */
            const char *format_func = get_rt_format_func(part_types[i]->kind);
            if (format_func != NULL)
            {
                result = arena_sprintf(gen->arena, "%s        char *_p%d = %s(%s, %s, \"%s\");\n",
                                       result, temp_var_count, format_func, ARENA_VAR(gen), part_strs[i], format_spec);
            }
            else
            {
                /* Fallback: convert to string first, then format */
                const char *to_str = get_rt_to_string_func(part_types[i]->kind);
                result = arena_sprintf(gen->arena, "%s        char *_tmp%d = %s(%s, %s);\n",
                                       result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i]);
                result = arena_sprintf(gen->arena, "%s        char *_p%d = rt_format_string(%s, _tmp%d, \"%s\");\n",
                                       result, temp_var_count, ARENA_VAR(gen), temp_var_count, format_spec);
            }
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (part_types[i]->kind != TYPE_STRING)
        {
            /* Non-string needs conversion (no format specifier) */
            const char *to_str = get_rt_to_string_func(part_types[i]->kind);
            result = arena_sprintf(gen->arena, "%s        char *_p%d = %s(%s, %s);\n",
                                   result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i]);
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (is_temp[i])
        {
            /* Temp string - capture it */
            result = arena_sprintf(gen->arena, "%s        char *_p%d = %s;\n",
                                   result, temp_var_count, part_strs[i]);
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (is_literal[i])
        {
            /* String literal - use directly (no copy needed) */
            use_strs[i] = part_strs[i];
        }
        else
        {
            /* String variable - can use directly in concat (rt_str_concat handles it) */
            use_strs[i] = part_strs[i];
        }
    }

    /* Build concatenation chain */
    if (count == 1)
    {
        result = arena_sprintf(gen->arena, "%s        %s;\n    })", result, use_strs[0]);
        return result;
    }
    else if (count == 2)
    {
        result = arena_sprintf(gen->arena, "%s        rt_str_concat(%s, %s, %s);\n    })",
                               result, ARENA_VAR(gen), use_strs[0], use_strs[1]);
        return result;
    }
    else
    {
        /* Chain of concats - minimize intermediate vars */
        result = arena_sprintf(gen->arena, "%s        char *_r = rt_str_concat(%s, %s, %s);\n",
                               result, ARENA_VAR(gen), use_strs[0], use_strs[1]);

        for (int i = 2; i < count; i++)
        {
            result = arena_sprintf(gen->arena, "%s        _r = rt_str_concat(%s, _r, %s);\n",
                                   result, ARENA_VAR(gen), use_strs[i]);
        }

        result = arena_sprintf(gen->arena, "%s        _r;\n    })", result);
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

        // Handle string methods
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
                // rt_string_ensure_mutable converts immutable strings to mutable.
                // rt_string_append returns potentially new pointer, assign back if variable.
                // IMPORTANT: Use the function's main arena (__arena_1__), not the loop arena,
                // because strings need to outlive the loop iteration.
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena,
                        "(%s = rt_string_append(rt_string_ensure_mutable(__arena_1__, %s), %s))",
                        object_str, object_str, arg_str);
                }
                return arena_sprintf(gen->arena,
                    "rt_string_append(rt_string_ensure_mutable(__arena_1__, %s), %s)",
                    object_str, arg_str);
            }

            #undef STRING_METHOD_RETURNING_STRING
        }

        /* TextFile instance methods */
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

        /* BinaryFile instance methods */
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

        /* Time instance methods */
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
                return arena_sprintf(gen->arena, "rt_time_to_date(%s, %s)",
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
    }

    /* Check if the callee is a closure (function type variable) */
    /* Skip builtins like print and len, and skip named functions */
    bool is_closure_call = false;
    Type *callee_type = call->callee->expr_type;

    if (callee_type && callee_type->kind == TYPE_FUNCTION && call->callee->type == EXPR_VARIABLE)
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
    /* Also handle array access where element is a function type (e.g., callbacks[0]()) */
    else if (callee_type && callee_type->kind == TYPE_FUNCTION && call->callee->type == EXPR_ARRAY_ACCESS)
    {
        is_closure_call = true;
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
        case TYPE_BYTE: suffix = "byte"; break;
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
        // For empty arrays with unknown element type (TYPE_NIL), return NULL.
        // The runtime handles NULL as an empty array.
        if (arr->element_count == 0 && elem_type->kind == TYPE_NIL) {
            return arena_strdup(gen->arena, "NULL");
        }
        // For empty arrays of function or nested array types, return NULL.
        // The runtime push functions handle NULL as an empty array.
        if (arr->element_count == 0 && (elem_type->kind == TYPE_FUNCTION || elem_type->kind == TYPE_ARRAY)) {
            return arena_strdup(gen->arena, "NULL");
        }
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

    // Check if index is provably non-negative (literal >= 0 or tracked loop counter)
    if (is_provably_non_negative(gen, expr->index))
    {
        // Non-negative index - direct array access, no adjustment needed
        return arena_sprintf(gen->arena, "%s[%s]", array_str, index_str);
    }

    // Check if index is a negative literal - can simplify to: arr[len + idx]
    if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        // Negative literal - adjust by array length
        return arena_sprintf(gen->arena, "%s[rt_array_length(%s) + %s]",
                             array_str, array_str, index_str);
    }

    // For potentially negative variable indices, generate runtime check
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

    // Handle TextFile.path
    if (object_type->kind == TYPE_TEXT_FILE && strcmp(member_name_str, "path") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_path(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    // Handle TextFile.name
    if (object_type->kind == TYPE_TEXT_FILE && strcmp(member_name_str, "name") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_name(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    // Handle TextFile.size
    if (object_type->kind == TYPE_TEXT_FILE && strcmp(member_name_str, "size") == 0) {
        return arena_sprintf(gen->arena, "rt_text_file_get_size(%s)", object_str);
    }

    // Handle BinaryFile.path
    if (object_type->kind == TYPE_BINARY_FILE && strcmp(member_name_str, "path") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_path(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    // Handle BinaryFile.name
    if (object_type->kind == TYPE_BINARY_FILE && strcmp(member_name_str, "name") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_name(%s, %s)",
            ARENA_VAR(gen), object_str);
    }

    // Handle BinaryFile.size
    if (object_type->kind == TYPE_BINARY_FILE && strcmp(member_name_str, "size") == 0) {
        return arena_sprintf(gen->arena, "rt_binary_file_get_size(%s)", object_str);
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
        case TYPE_BYTE:
            slice_func = "rt_array_slice_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for slice\n");
            exit(1);
    }

    return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s, %s)", slice_func, ARENA_VAR(gen), array_str, start_str, end_str, step_str);
}

/* Helper to generate statement body code for lambda
 * lambda_func_name: the generated function name like "__lambda_5__"
 * This sets up context so return statements work correctly */
static char *code_gen_lambda_stmt_body(CodeGen *gen, LambdaExpr *lambda, int indent,
                                        const char *lambda_func_name, Type *return_type)
{
    /* Save current context */
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;

    /* Set up lambda context - use the lambda function name for return labels */
    gen->current_function = (char *)lambda_func_name;
    gen->current_return_type = return_type;

    /* Generate code for each statement in the lambda body */
    /* We need to capture the output since code_gen_statement writes to gen->output */
    FILE *old_output = gen->output;
    char *body_buffer = NULL;
    size_t body_size = 0;
    gen->output = open_memstream(&body_buffer, &body_size);

    for (int i = 0; i < lambda->body_stmt_count; i++)
    {
        code_gen_statement(gen, lambda->body_stmts[i], indent);
    }

    fclose(gen->output);
    gen->output = old_output;

    /* Restore context */
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;

    /* Copy to arena and free temp buffer */
    char *result = arena_strdup(gen->arena, body_buffer ? body_buffer : "");
    free(body_buffer);

    return result;
}

static char *code_gen_lambda_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_lambda_expression");
    LambdaExpr *lambda = &expr->as.lambda;
    int lambda_id = gen->lambda_count++;
    FunctionModifier modifier = lambda->modifier;

    /* Store the lambda_id in the expression for later reference */
    expr->as.lambda.lambda_id = lambda_id;

    /* Collect captured variables - from expression body or statement body */
    CapturedVars cv;
    captured_vars_init(&cv);

    /* First collect local variables declared in the lambda body */
    LocalVars lv;
    local_vars_init(&lv);
    if (lambda->has_stmt_body)
    {
        for (int i = 0; i < lambda->body_stmt_count; i++)
        {
            collect_local_vars_from_stmt(lambda->body_stmts[i], &lv, gen->arena);
        }
    }

    /* Build enclosing lambda context from CodeGen state */
    EnclosingLambdaContext enclosing;
    enclosing.lambdas = gen->enclosing_lambdas;
    enclosing.count = gen->enclosing_lambda_count;
    enclosing.capacity = gen->enclosing_lambda_capacity;

    /* Now collect captured variables, skipping locals */
    if (lambda->has_stmt_body)
    {
        for (int i = 0; i < lambda->body_stmt_count; i++)
        {
            collect_captured_vars_from_stmt(lambda->body_stmts[i], lambda, gen->symbol_table, &cv, &lv, &enclosing, gen->arena);
        }
    }
    else
    {
        collect_captured_vars(lambda->body, lambda, gen->symbol_table, &cv, NULL, &enclosing, gen->arena);
    }

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

    if (modifier == FUNC_PRIVATE)
    {
        /* Private lambda: create isolated arena, destroy before return */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArena *__lambda_arena__ = rt_arena_create(NULL);\n"
            "    (void)__closure__;\n");
        arena_cleanup = arena_sprintf(gen->arena,
            "    rt_arena_destroy(__lambda_arena__);\n");
    }
    else
    {
        /* Default/Shared lambda: use arena from closure */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;\n");
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
            /* For primitives, store pointers to enable mutation persistence.
             * When a lambda modifies a captured primitive, the change must
             * persist to the original variable and across multiple calls.
             * Reference types (arrays, strings) are already pointers. */
            if (is_primitive_type(cv.types[i]))
            {
                struct_def = arena_sprintf(gen->arena, "%s    %s *%s;\n",
                                           struct_def, c_type, cv.names[i]);
            }
            else
            {
                struct_def = arena_sprintf(gen->arena, "%s    %s %s;\n",
                                           struct_def, c_type, cv.names[i]);
            }
        }
        struct_def = arena_sprintf(gen->arena, "%s} __closure_%d__;\n",
                                   struct_def, lambda_id);

        /* Add struct def to forward declarations (before lambda functions) */
        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, struct_def);

        /* Generate local variable declarations for captured vars in lambda body.
         * For primitives, we create a pointer alias that points to the closure's
         * stored pointer. This way, reads/writes use the pointer and mutations
         * persist both to the original variable and across lambda calls.
         * For reference types, we just copy the pointer value. */
        char *capture_decls = arena_strdup(gen->arena, "");
        for (int i = 0; i < cv.count; i++)
        {
            const char *c_type = get_c_type(gen->arena, cv.types[i]);
            if (is_primitive_type(cv.types[i]))
            {
                /* For primitives, the closure stores a pointer (long*). We declare a local
                 * pointer variable that references the closure's stored pointer, so access
                 * like (*count) works naturally. We use a local variable instead of #define
                 * to avoid macro replacement issues when this lambda creates nested closures
                 * (where __cl__->name would have 'name' replaced by the macro). */
                capture_decls = arena_sprintf(gen->arena,
                    "%s    %s *%s = ((__closure_%d__ *)__closure__)->%s;\n",
                    capture_decls, c_type, cv.names[i], lambda_id, cv.names[i]);
            }
            else
            {
                /* Reference types: copy the pointer value */
                capture_decls = arena_sprintf(gen->arena,
                    "%s    %s %s = ((__closure_%d__ *)__closure__)->%s;\n",
                    capture_decls, c_type, cv.names[i], lambda_id, cv.names[i]);
            }
        }

        /* Generate the static lambda function body - use lambda's arena */
        char *saved_arena_var = gen->current_arena_var;
        gen->current_arena_var = "__lambda_arena__";

        /* Push this lambda to enclosing context for nested lambdas */
        if (gen->enclosing_lambda_count >= gen->enclosing_lambda_capacity)
        {
            int new_cap = gen->enclosing_lambda_capacity == 0 ? 4 : gen->enclosing_lambda_capacity * 2;
            LambdaExpr **new_lambdas = arena_alloc(gen->arena, new_cap * sizeof(LambdaExpr *));
            for (int i = 0; i < gen->enclosing_lambda_count; i++)
            {
                new_lambdas[i] = gen->enclosing_lambdas[i];
            }
            gen->enclosing_lambdas = new_lambdas;
            gen->enclosing_lambda_capacity = new_cap;
        }
        gen->enclosing_lambdas[gen->enclosing_lambda_count++] = lambda;

        /* Generate forward declaration */
        char *forward_decl = arena_sprintf(gen->arena,
            "static %s __lambda_%d__(%s);\n",
            ret_c_type, lambda_id, params_decl);

        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, forward_decl);

        /* Generate the actual lambda function definition with capture access */
        char *lambda_func;
        char *lambda_func_name = arena_sprintf(gen->arena, "__lambda_%d__", lambda_id);
        if (lambda->has_stmt_body)
        {
            /* Multi-line lambda with statement body - needs return value and label */
            char *body_code = code_gen_lambda_stmt_body(gen, lambda, 1, lambda_func_name, lambda->return_type);

            /* Check if void return type - special handling needed */
            int is_void_return = (lambda->return_type && lambda->return_type->kind == TYPE_VOID);

            if (is_void_return)
            {
                /* Void return - no return value declaration needed */
                if (modifier == FUNC_PRIVATE)
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static void %s(%s) {\n"
                        "%s"
                        "%s"
                        "%s"
                        "%s_return:\n"
                        "%s"
                        "    return;\n"
                        "}\n\n",
                        lambda_func_name, params_decl, arena_setup, capture_decls,
                        body_code, lambda_func_name, arena_cleanup);
                }
                else
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static void %s(%s) {\n"
                        "%s"
                        "%s"
                        "%s"
                        "%s_return:\n"
                        "    return;\n"
                        "}\n\n",
                        lambda_func_name, params_decl, arena_setup, capture_decls,
                        body_code, lambda_func_name);
                }
            }
            else
            {
                const char *default_val = get_default_value(lambda->return_type);
                if (modifier == FUNC_PRIVATE)
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "%s"
                        "%s"
                        "    %s _return_value = %s;\n"
                        "%s"
                        "%s_return:\n"
                        "%s"
                        "    return _return_value;\n"
                        "}\n\n",
                        ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls,
                        ret_c_type, default_val, body_code, lambda_func_name, arena_cleanup);
                }
                else
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "%s"
                        "%s"
                        "    %s _return_value = %s;\n"
                        "%s"
                        "%s_return:\n"
                        "    return _return_value;\n"
                        "}\n\n",
                        ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls,
                        ret_c_type, default_val, body_code, lambda_func_name);
                }
            }
        }
        else
        {
            /* Single-line lambda with expression body */
            char *body_code = code_gen_expression(gen, lambda->body);
            if (modifier == FUNC_PRIVATE)
            {
                /* Private: create arena, compute result, destroy arena, return */
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "%s"
                    "    %s __result__ = %s;\n"
                    "%s"
                    "    return __result__;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls,
                    ret_c_type, body_code, arena_cleanup);
            }
            else
            {
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "%s"
                    "    return %s;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls, body_code);
            }
        }
        gen->current_arena_var = saved_arena_var;

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
            /* For primitives: the outer variable is now declared as a pointer
             * (via the pre-pass that marks captured primitives for heap allocation).
             * We simply store that pointer in the closure - both outer scope and
             * closure now share the same arena-allocated storage.
             * For reference types (arrays, strings), just copy the pointer value. */
            if (is_primitive_type(cv.types[i]))
            {
                /* The variable is already a pointer - just copy it to the closure */
                closure_init = arena_sprintf(gen->arena, "%s    __cl__->%s = %s;\n",
                                             closure_init, cv.names[i], cv.names[i]);
            }
            else
            {
                closure_init = arena_sprintf(gen->arena, "%s    __cl__->%s = %s;\n",
                                             closure_init, cv.names[i], cv.names[i]);
            }
        }
        closure_init = arena_sprintf(gen->arena,
            "%s    (__Closure__ *)__cl__;\n"
            "})",
            closure_init);

        /* Pop this lambda from enclosing context */
        gen->enclosing_lambda_count--;

        return closure_init;
    }
    else
    {
        /* No captures - use simple generic closure */
        /* Generate the static lambda function body - use lambda's arena */
        char *saved_arena_var = gen->current_arena_var;
        gen->current_arena_var = "__lambda_arena__";

        /* Push this lambda to enclosing context for nested lambdas */
        if (gen->enclosing_lambda_count >= gen->enclosing_lambda_capacity)
        {
            int new_cap = gen->enclosing_lambda_capacity == 0 ? 4 : gen->enclosing_lambda_capacity * 2;
            LambdaExpr **new_lambdas = arena_alloc(gen->arena, new_cap * sizeof(LambdaExpr *));
            for (int i = 0; i < gen->enclosing_lambda_count; i++)
            {
                new_lambdas[i] = gen->enclosing_lambdas[i];
            }
            gen->enclosing_lambdas = new_lambdas;
            gen->enclosing_lambda_capacity = new_cap;
        }
        gen->enclosing_lambdas[gen->enclosing_lambda_count++] = lambda;

        char *lambda_func_name = arena_sprintf(gen->arena, "__lambda_%d__", lambda_id);

        /* Generate forward declaration */
        char *forward_decl = arena_sprintf(gen->arena,
            "static %s %s(%s);\n",
            ret_c_type, lambda_func_name, params_decl);

        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, forward_decl);

        /* Generate the actual lambda function definition */
        char *lambda_func;
        if (lambda->has_stmt_body)
        {
            /* Multi-line lambda with statement body - needs return value and label */
            char *body_code = code_gen_lambda_stmt_body(gen, lambda, 1, lambda_func_name, lambda->return_type);

            /* Check if void return type - special handling needed */
            int is_void_return = (lambda->return_type && lambda->return_type->kind == TYPE_VOID);

            if (is_void_return)
            {
                /* Void return - no return value declaration needed */
                if (modifier == FUNC_PRIVATE)
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static void %s(%s) {\n"
                        "%s"
                        "%s"
                        "%s_return:\n"
                        "%s"
                        "    return;\n"
                        "}\n\n",
                        lambda_func_name, params_decl, arena_setup,
                        body_code, lambda_func_name, arena_cleanup);
                }
                else
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static void %s(%s) {\n"
                        "%s"
                        "%s"
                        "%s_return:\n"
                        "    return;\n"
                        "}\n\n",
                        lambda_func_name, params_decl, arena_setup,
                        body_code, lambda_func_name);
                }
            }
            else
            {
                const char *default_val = get_default_value(lambda->return_type);
                if (modifier == FUNC_PRIVATE)
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "%s"
                        "    %s _return_value = %s;\n"
                        "%s"
                        "%s_return:\n"
                        "%s"
                        "    return _return_value;\n"
                        "}\n\n",
                        ret_c_type, lambda_func_name, params_decl, arena_setup,
                        ret_c_type, default_val, body_code, lambda_func_name, arena_cleanup);
                }
                else
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "%s"
                        "    %s _return_value = %s;\n"
                        "%s"
                        "%s_return:\n"
                        "    return _return_value;\n"
                        "}\n\n",
                        ret_c_type, lambda_func_name, params_decl, arena_setup,
                        ret_c_type, default_val, body_code, lambda_func_name);
                }
            }
        }
        else
        {
            /* Single-line lambda with expression body */
            char *body_code = code_gen_expression(gen, lambda->body);
            if (modifier == FUNC_PRIVATE)
            {
                /* Private: create arena, compute result, destroy arena, return */
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "    %s __result__ = %s;\n"
                    "%s"
                    "    return __result__;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup,
                    ret_c_type, body_code, arena_cleanup);
            }
            else
            {
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "    return %s;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, body_code);
            }
        }
        gen->current_arena_var = saved_arena_var;

        /* Append to definitions buffer */
        gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                gen->lambda_definitions, lambda_func);

        /* Pop this lambda from enclosing context */
        gen->enclosing_lambda_count--;

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

static bool codegen_token_equals(Token tok, const char *str)
{
    size_t len = strlen(str);
    return tok.length == (int)len && strncmp(tok.start, str, len) == 0;
}

static char *code_gen_static_call_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_static_call_expression");
    StaticCallExpr *call = &expr->as.static_call;
    Token type_name = call->type_name;
    Token method_name = call->method_name;

    /* Generate argument expressions */
    char *arg0 = call->arg_count > 0 ? code_gen_expression(gen, call->arguments[0]) : NULL;
    char *arg1 = call->arg_count > 1 ? code_gen_expression(gen, call->arguments[1]) : NULL;

    /* TextFile static methods */
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

    /* BinaryFile static methods */
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

static char *code_gen_sized_array_alloc_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_sized_array_alloc_expression");

    /* Extract fields from sized_array_alloc */
    SizedArrayAllocExpr *alloc = &expr->as.sized_array_alloc;
    Type *element_type = alloc->element_type;
    Expr *size_expr = alloc->size_expr;
    Expr *default_value = alloc->default_value;

    /* Determine the runtime function suffix based on element type */
    const char *suffix = NULL;
    switch (element_type->kind) {
        case TYPE_INT: suffix = "long"; break;
        case TYPE_DOUBLE: suffix = "double"; break;
        case TYPE_CHAR: suffix = "char"; break;
        case TYPE_BOOL: suffix = "bool"; break;
        case TYPE_BYTE: suffix = "byte"; break;
        case TYPE_STRING: suffix = "string"; break;
        default:
            fprintf(stderr, "Error: Unsupported element type for sized array allocation\n");
            exit(1);
    }

    /* Generate code for the size expression */
    char *size_str = code_gen_expression(gen, size_expr);

    /* Generate code for the default value */
    char *default_str;
    if (default_value != NULL) {
        default_str = code_gen_expression(gen, default_value);
    } else {
        /* Use type-appropriate zero value when no default provided */
        switch (element_type->kind) {
            case TYPE_INT: default_str = "0"; break;
            case TYPE_DOUBLE: default_str = "0.0"; break;
            case TYPE_CHAR: default_str = "'\\0'"; break;
            case TYPE_BOOL: default_str = "0"; break;
            case TYPE_BYTE: default_str = "0"; break;
            case TYPE_STRING: default_str = "NULL"; break;
            default: default_str = "0"; break;
        }
    }

    /* Construct the runtime function call: rt_array_alloc_{suffix}(arena, size, default) */
    return arena_sprintf(gen->arena, "rt_array_alloc_%s(%s, %s, %s)",
                         suffix, ARENA_VAR(gen), size_str, default_str);
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
    case EXPR_INDEX_ASSIGN:
        return code_gen_index_assign_expression(gen, &expr->as.index_assign);
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
    case EXPR_STATIC_CALL:
        return code_gen_static_call_expression(gen, expr);
    case EXPR_SIZED_ARRAY_ALLOC:
        return code_gen_sized_array_alloc_expression(gen, expr);
    default:
        exit(1);
    }
    return NULL;
}
