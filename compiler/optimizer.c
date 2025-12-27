#include "optimizer.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

void optimizer_init(Optimizer *opt, Arena *arena)
{
    opt->arena = arena;
    opt->statements_removed = 0;
    opt->variables_removed = 0;
    opt->noops_removed = 0;
    opt->tail_calls_optimized = 0;
    opt->string_literals_merged = 0;
}

void optimizer_get_stats(Optimizer *opt, int *stmts_removed, int *vars_removed, int *noops_removed)
{
    if (stmts_removed) *stmts_removed = opt->statements_removed;
    if (vars_removed) *vars_removed = opt->variables_removed;
    if (noops_removed) *noops_removed = opt->noops_removed;
}

/* ============================================================================
 * Terminator Detection
 * ============================================================================
 * Detect statements that always terminate control flow: return, break, continue
 */

bool stmt_is_terminator(Stmt *stmt)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_RETURN:
    case STMT_BREAK:
    case STMT_CONTINUE:
        return true;

    case STMT_BLOCK:
        /* A block is a terminator if any of its statements is a terminator
           that is not inside a conditional */
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (stmt_is_terminator(stmt->as.block.statements[i]))
            {
                return true;
            }
        }
        return false;

    case STMT_IF:
        /* An if is a terminator only if BOTH branches exist and BOTH terminate */
        if (stmt->as.if_stmt.else_branch == NULL)
        {
            return false;
        }
        return stmt_is_terminator(stmt->as.if_stmt.then_branch) &&
               stmt_is_terminator(stmt->as.if_stmt.else_branch);

    default:
        return false;
    }
}

/* ============================================================================
 * No-op Detection
 * ============================================================================
 * Detect expressions that can be simplified:
 * - x + 0, 0 + x => x
 * - x - 0 => x
 * - x * 1, 1 * x => x
 * - x * 0, 0 * x => 0
 * - x / 1 => x
 * - !(!x) => x (double negation)
 */

/* Check if an expression is the literal integer 0 */
static bool is_literal_zero(Expr *expr)
{
    if (expr == NULL) return false;
    if (expr->type != EXPR_LITERAL) return false;
    if (expr->as.literal.type == NULL) return false;

    if (expr->as.literal.type->kind == TYPE_INT ||
        expr->as.literal.type->kind == TYPE_LONG)
    {
        return expr->as.literal.value.int_value == 0;
    }
    if (expr->as.literal.type->kind == TYPE_DOUBLE)
    {
        return expr->as.literal.value.double_value == 0.0;
    }
    return false;
}

/* Check if an expression is the literal integer 1 */
static bool is_literal_one(Expr *expr)
{
    if (expr == NULL) return false;
    if (expr->type != EXPR_LITERAL) return false;
    if (expr->as.literal.type == NULL) return false;

    if (expr->as.literal.type->kind == TYPE_INT ||
        expr->as.literal.type->kind == TYPE_LONG)
    {
        return expr->as.literal.value.int_value == 1;
    }
    if (expr->as.literal.type->kind == TYPE_DOUBLE)
    {
        return expr->as.literal.value.double_value == 1.0;
    }
    return false;
}

bool expr_is_noop(Expr *expr, Expr **simplified)
{
    if (expr == NULL)
    {
        *simplified = NULL;
        return false;
    }

    switch (expr->type)
    {
    case EXPR_BINARY:
    {
        TokenType op = expr->as.binary.operator;
        Expr *left = expr->as.binary.left;
        Expr *right = expr->as.binary.right;

        /* x + 0 or 0 + x => x */
        if (op == TOKEN_PLUS)
        {
            if (is_literal_zero(right))
            {
                *simplified = left;
                return true;
            }
            if (is_literal_zero(left))
            {
                *simplified = right;
                return true;
            }
        }

        /* x - 0 => x */
        if (op == TOKEN_MINUS)
        {
            if (is_literal_zero(right))
            {
                *simplified = left;
                return true;
            }
        }

        /* x * 1 or 1 * x => x */
        if (op == TOKEN_STAR)
        {
            if (is_literal_one(right))
            {
                *simplified = left;
                return true;
            }
            if (is_literal_one(left))
            {
                *simplified = right;
                return true;
            }
            /* Note: x * 0 optimization is tricky because of side effects
               in x, so we don't do it here */
        }

        /* x / 1 => x */
        if (op == TOKEN_SLASH)
        {
            if (is_literal_one(right))
            {
                *simplified = left;
                return true;
            }
        }

        /* && with false, || with true could be simplified but
           we need to be careful about side effects */
        break;
    }

    case EXPR_UNARY:
    {
        /* !(!x) => x (double negation for boolean) */
        if (expr->as.unary.operator == TOKEN_BANG)
        {
            Expr *operand = expr->as.unary.operand;
            if (operand != NULL &&
                operand->type == EXPR_UNARY &&
                operand->as.unary.operator == TOKEN_BANG)
            {
                *simplified = operand->as.unary.operand;
                return true;
            }
        }

        /* -(-x) => x (double negation for numbers) */
        if (expr->as.unary.operator == TOKEN_MINUS)
        {
            Expr *operand = expr->as.unary.operand;
            if (operand != NULL &&
                operand->type == EXPR_UNARY &&
                operand->as.unary.operator == TOKEN_MINUS)
            {
                *simplified = operand->as.unary.operand;
                return true;
            }
        }
        break;
    }

    default:
        break;
    }

    *simplified = expr;
    return false;
}

/* ============================================================================
 * Variable Usage Tracking
 * ============================================================================
 * Track which variables are actually read to identify unused declarations
 */

static void add_used_variable(Token **used_vars, int *used_count, int *used_capacity,
                              Arena *arena, Token name)
{
    /* Check if already in list */
    for (int i = 0; i < *used_count; i++)
    {
        if ((*used_vars)[i].length == name.length &&
            strncmp((*used_vars)[i].start, name.start, name.length) == 0)
        {
            return;  /* Already tracked */
        }
    }

    /* Grow array if needed */
    if (*used_count >= *used_capacity)
    {
        int new_cap = (*used_capacity == 0) ? 16 : (*used_capacity * 2);
        Token *new_vars = arena_alloc(arena, new_cap * sizeof(Token));
        for (int i = 0; i < *used_count; i++)
        {
            new_vars[i] = (*used_vars)[i];
        }
        *used_vars = new_vars;
        *used_capacity = new_cap;
    }

    (*used_vars)[*used_count] = name;
    (*used_count)++;
}

void collect_used_variables(Expr *expr, Token **used_vars, int *used_count,
                            int *used_capacity, Arena *arena)
{
    if (expr == NULL) return;

    switch (expr->type)
    {
    case EXPR_VARIABLE:
        add_used_variable(used_vars, used_count, used_capacity, arena, expr->as.variable.name);
        break;

    case EXPR_BINARY:
        collect_used_variables(expr->as.binary.left, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.binary.right, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_UNARY:
        collect_used_variables(expr->as.unary.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_ASSIGN:
        /* The variable being assigned TO is not a "use" (it's a def),
           but the value being assigned IS a use */
        collect_used_variables(expr->as.assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INDEX_ASSIGN:
        collect_used_variables(expr->as.index_assign.array, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.index_assign.index, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.index_assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_CALL:
        collect_used_variables(expr->as.call.callee, used_vars, used_count, used_capacity, arena);
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            collect_used_variables(expr->as.call.arguments[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            collect_used_variables(expr->as.array.elements[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        collect_used_variables(expr->as.array_access.array, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.array_access.index, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_ARRAY_SLICE:
        collect_used_variables(expr->as.array_slice.array, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.start)
            collect_used_variables(expr->as.array_slice.start, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.end)
            collect_used_variables(expr->as.array_slice.end, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.step)
            collect_used_variables(expr->as.array_slice.step, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_RANGE:
        collect_used_variables(expr->as.range.start, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.range.end, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_SPREAD:
        collect_used_variables(expr->as.spread.array, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        collect_used_variables(expr->as.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            collect_used_variables(expr->as.interpol.parts[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_MEMBER:
        collect_used_variables(expr->as.member.object, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_LAMBDA:
        /* Lambda bodies should track their own variables, but captured
           variables from outer scope count as uses */
        if (expr->as.lambda.body)
        {
            collect_used_variables(expr->as.lambda.body, used_vars, used_count, used_capacity, arena);
        }
        for (int i = 0; i < expr->as.lambda.body_stmt_count; i++)
        {
            collect_used_variables_stmt(expr->as.lambda.body_stmts[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            collect_used_variables(expr->as.static_call.arguments[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_LITERAL:
    default:
        break;
    }
}

void collect_used_variables_stmt(Stmt *stmt, Token **used_vars, int *used_count,
                                  int *used_capacity, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        collect_used_variables(stmt->as.expression.expression, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_VAR_DECL:
        /* The variable being declared is not a use, but the initializer is */
        if (stmt->as.var_decl.initializer)
        {
            collect_used_variables(stmt->as.var_decl.initializer, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            collect_used_variables(stmt->as.return_stmt.value, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            collect_used_variables_stmt(stmt->as.block.statements[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_IF:
        collect_used_variables(stmt->as.if_stmt.condition, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.if_stmt.then_branch, used_vars, used_count, used_capacity, arena);
        if (stmt->as.if_stmt.else_branch)
        {
            collect_used_variables_stmt(stmt->as.if_stmt.else_branch, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_WHILE:
        collect_used_variables(stmt->as.while_stmt.condition, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.while_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
        {
            collect_used_variables_stmt(stmt->as.for_stmt.initializer, used_vars, used_count, used_capacity, arena);
        }
        if (stmt->as.for_stmt.condition)
        {
            collect_used_variables(stmt->as.for_stmt.condition, used_vars, used_count, used_capacity, arena);
        }
        if (stmt->as.for_stmt.increment)
        {
            collect_used_variables(stmt->as.for_stmt.increment, used_vars, used_count, used_capacity, arena);
        }
        collect_used_variables_stmt(stmt->as.for_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FOR_EACH:
        collect_used_variables(stmt->as.for_each_stmt.iterable, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.for_each_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FUNCTION:
        /* Don't descend into nested function definitions for variable tracking */
        break;

    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
    default:
        break;
    }
}

bool is_variable_used(Token *used_vars, int used_count, Token name)
{
    for (int i = 0; i < used_count; i++)
    {
        if (used_vars[i].length == name.length &&
            strncmp(used_vars[i].start, name.start, name.length) == 0)
        {
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * Dead Code Removal
 * ============================================================================
 */

/* Remove unreachable statements after a terminator in a block.
   Returns the number of statements removed. */
int remove_unreachable_statements(Optimizer *opt, Stmt ***stmts, int *count)
{
    if (stmts == NULL || *stmts == NULL || *count <= 0) return 0;

    int removed = 0;
    int new_count = 0;

    for (int i = 0; i < *count; i++)
    {
        Stmt *stmt = (*stmts)[i];

        /* If we found a terminator, all remaining statements are unreachable */
        if (new_count > 0 && stmt_is_terminator((*stmts)[new_count - 1]))
        {
            /* This statement is unreachable */
            removed++;
            continue;
        }

        /* Keep this statement */
        (*stmts)[new_count] = stmt;
        new_count++;

        /* Recursively process nested blocks */
        switch (stmt->type)
        {
        case STMT_BLOCK:
            removed += remove_unreachable_statements(opt, &stmt->as.block.statements, &stmt->as.block.count);
            break;

        case STMT_IF:
            if (stmt->as.if_stmt.then_branch && stmt->as.if_stmt.then_branch->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.if_stmt.then_branch->as.block.statements,
                    &stmt->as.if_stmt.then_branch->as.block.count);
            }
            if (stmt->as.if_stmt.else_branch && stmt->as.if_stmt.else_branch->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.if_stmt.else_branch->as.block.statements,
                    &stmt->as.if_stmt.else_branch->as.block.count);
            }
            break;

        case STMT_WHILE:
            if (stmt->as.while_stmt.body && stmt->as.while_stmt.body->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.while_stmt.body->as.block.statements,
                    &stmt->as.while_stmt.body->as.block.count);
            }
            break;

        case STMT_FOR:
            if (stmt->as.for_stmt.body && stmt->as.for_stmt.body->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.for_stmt.body->as.block.statements,
                    &stmt->as.for_stmt.body->as.block.count);
            }
            break;

        case STMT_FOR_EACH:
            if (stmt->as.for_each_stmt.body && stmt->as.for_each_stmt.body->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.for_each_stmt.body->as.block.statements,
                    &stmt->as.for_each_stmt.body->as.block.count);
            }
            break;

        default:
            break;
        }
    }

    *count = new_count;
    opt->statements_removed += removed;
    return removed;
}

/* Remove unused variable declarations from a list of statements.
   This is a conservative pass - only removes variables that are definitely unused. */
static int remove_unused_variables(Optimizer *opt, Stmt **stmts, int *count)
{
    if (stmts == NULL || *count <= 0) return 0;

    /* First, collect all variable uses in the entire block */
    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    for (int i = 0; i < *count; i++)
    {
        collect_used_variables_stmt(stmts[i], &used_vars, &used_count, &used_capacity, opt->arena);
    }

    /* Now filter out unused variable declarations */
    int removed = 0;
    int new_count = 0;

    for (int i = 0; i < *count; i++)
    {
        Stmt *stmt = stmts[i];

        if (stmt->type == STMT_VAR_DECL)
        {
            /* Check if this variable is used */
            if (!is_variable_used(used_vars, used_count, stmt->as.var_decl.name))
            {
                /* Variable is not used - but we can only remove it if:
                   1. It has no initializer, OR
                   2. The initializer has no side effects */
                Expr *init = stmt->as.var_decl.initializer;
                bool has_side_effects = false;

                if (init != NULL)
                {
                    /* Conservative: assume function calls have side effects */
                    switch (init->type)
                    {
                    case EXPR_CALL:
                    case EXPR_INCREMENT:
                    case EXPR_DECREMENT:
                    case EXPR_ASSIGN:
                    case EXPR_INDEX_ASSIGN:
                        has_side_effects = true;
                        break;
                    default:
                        has_side_effects = false;
                    }
                }

                if (!has_side_effects)
                {
                    removed++;
                    continue;  /* Skip this declaration */
                }
            }
        }

        /* Keep this statement */
        stmts[new_count] = stmt;
        new_count++;
    }

    *count = new_count;
    opt->variables_removed += removed;
    return removed;
}

/* Simplify no-op expressions recursively in an expression */
static Expr *simplify_noop_expr(Optimizer *opt, Expr *expr)
{
    if (expr == NULL) return NULL;

    /* First, recursively simplify sub-expressions */
    switch (expr->type)
    {
    case EXPR_BINARY:
        expr->as.binary.left = simplify_noop_expr(opt, expr->as.binary.left);
        expr->as.binary.right = simplify_noop_expr(opt, expr->as.binary.right);
        break;

    case EXPR_UNARY:
        expr->as.unary.operand = simplify_noop_expr(opt, expr->as.unary.operand);
        break;

    case EXPR_ASSIGN:
        expr->as.assign.value = simplify_noop_expr(opt, expr->as.assign.value);
        break;

    case EXPR_INDEX_ASSIGN:
        expr->as.index_assign.array = simplify_noop_expr(opt, expr->as.index_assign.array);
        expr->as.index_assign.index = simplify_noop_expr(opt, expr->as.index_assign.index);
        expr->as.index_assign.value = simplify_noop_expr(opt, expr->as.index_assign.value);
        break;

    case EXPR_CALL:
        expr->as.call.callee = simplify_noop_expr(opt, expr->as.call.callee);
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            expr->as.call.arguments[i] = simplify_noop_expr(opt, expr->as.call.arguments[i]);
        }
        break;

    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            expr->as.array.elements[i] = simplify_noop_expr(opt, expr->as.array.elements[i]);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        expr->as.array_access.array = simplify_noop_expr(opt, expr->as.array_access.array);
        expr->as.array_access.index = simplify_noop_expr(opt, expr->as.array_access.index);
        break;

    case EXPR_ARRAY_SLICE:
        expr->as.array_slice.array = simplify_noop_expr(opt, expr->as.array_slice.array);
        if (expr->as.array_slice.start)
            expr->as.array_slice.start = simplify_noop_expr(opt, expr->as.array_slice.start);
        if (expr->as.array_slice.end)
            expr->as.array_slice.end = simplify_noop_expr(opt, expr->as.array_slice.end);
        if (expr->as.array_slice.step)
            expr->as.array_slice.step = simplify_noop_expr(opt, expr->as.array_slice.step);
        break;

    case EXPR_RANGE:
        expr->as.range.start = simplify_noop_expr(opt, expr->as.range.start);
        expr->as.range.end = simplify_noop_expr(opt, expr->as.range.end);
        break;

    case EXPR_SPREAD:
        expr->as.spread.array = simplify_noop_expr(opt, expr->as.spread.array);
        break;

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        expr->as.operand = simplify_noop_expr(opt, expr->as.operand);
        break;

    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            expr->as.interpol.parts[i] = simplify_noop_expr(opt, expr->as.interpol.parts[i]);
        }
        break;

    case EXPR_MEMBER:
        expr->as.member.object = simplify_noop_expr(opt, expr->as.member.object);
        break;

    default:
        break;
    }

    /* Now check if this expression itself is a no-op */
    Expr *simplified;
    if (expr_is_noop(expr, &simplified))
    {
        opt->noops_removed++;
        return simplified;
    }

    return expr;
}

/* Simplify no-op expressions in a statement */
static void simplify_noop_stmt(Optimizer *opt, Stmt *stmt)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        stmt->as.expression.expression = simplify_noop_expr(opt, stmt->as.expression.expression);
        break;

    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
        {
            stmt->as.var_decl.initializer = simplify_noop_expr(opt, stmt->as.var_decl.initializer);
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            stmt->as.return_stmt.value = simplify_noop_expr(opt, stmt->as.return_stmt.value);
        }
        break;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            simplify_noop_stmt(opt, stmt->as.block.statements[i]);
        }
        break;

    case STMT_IF:
        stmt->as.if_stmt.condition = simplify_noop_expr(opt, stmt->as.if_stmt.condition);
        simplify_noop_stmt(opt, stmt->as.if_stmt.then_branch);
        if (stmt->as.if_stmt.else_branch)
        {
            simplify_noop_stmt(opt, stmt->as.if_stmt.else_branch);
        }
        break;

    case STMT_WHILE:
        stmt->as.while_stmt.condition = simplify_noop_expr(opt, stmt->as.while_stmt.condition);
        simplify_noop_stmt(opt, stmt->as.while_stmt.body);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
        {
            simplify_noop_stmt(opt, stmt->as.for_stmt.initializer);
        }
        if (stmt->as.for_stmt.condition)
        {
            stmt->as.for_stmt.condition = simplify_noop_expr(opt, stmt->as.for_stmt.condition);
        }
        if (stmt->as.for_stmt.increment)
        {
            stmt->as.for_stmt.increment = simplify_noop_expr(opt, stmt->as.for_stmt.increment);
        }
        simplify_noop_stmt(opt, stmt->as.for_stmt.body);
        break;

    case STMT_FOR_EACH:
        stmt->as.for_each_stmt.iterable = simplify_noop_expr(opt, stmt->as.for_each_stmt.iterable);
        simplify_noop_stmt(opt, stmt->as.for_each_stmt.body);
        break;

    default:
        break;
    }
}

/* Run dead code elimination on a function */
void optimizer_eliminate_dead_code_function(Optimizer *opt, FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL || fn->body_count == 0) return;

    /* 1. Remove unreachable statements after return/break/continue */
    remove_unreachable_statements(opt, &fn->body, &fn->body_count);

    /* 2. Simplify no-op expressions */
    for (int i = 0; i < fn->body_count; i++)
    {
        simplify_noop_stmt(opt, fn->body[i]);
    }

    /* 3. Remove unused variable declarations (do this last since
       simplification might affect variable usage) */
    remove_unused_variables(opt, fn->body, &fn->body_count);
}

/* Run dead code elimination on an entire module */
void optimizer_dead_code_elimination(Optimizer *opt, Module *module)
{
    if (module == NULL || module->statements == NULL) return;

    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            optimizer_eliminate_dead_code_function(opt, &stmt->as.function);
        }
    }
}

/* ============================================================================
 * Tail Call Optimization
 * ============================================================================
 * Detect and mark tail-recursive calls for optimization.
 *
 * A tail call is when a function's last action before returning is to call
 * another function and return its result directly. For self-recursive calls,
 * this can be converted to a loop, eliminating stack frame overhead.
 *
 * Example of tail recursion:
 *   fn loop(n: int): int =>
 *       if n <= 0 => return 0
 *       return loop(n - 1)   // <-- tail call, last action is the call itself
 *
 * Example of NON-tail recursion:
 *   fn factorial(n: int): int =>
 *       if n <= 1 => return 1
 *       return n * factorial(n - 1)  // NOT a tail call, multiplication after call
 */

/* Compare two tokens for name equality */
static bool tokens_equal(Token a, Token b)
{
    if (a.length != b.length) return false;
    return strncmp(a.start, b.start, a.length) == 0;
}

/* Check if an expression is a direct tail call to the given function name.
   Returns the call expression if it's a tail call, NULL otherwise. */
static Expr *get_tail_call_expr(Expr *expr, Token func_name)
{
    if (expr == NULL) return NULL;

    /* A direct call is a tail call if it calls the function directly */
    if (expr->type == EXPR_CALL)
    {
        CallExpr *call = &expr->as.call;
        if (call->callee->type == EXPR_VARIABLE)
        {
            if (tokens_equal(call->callee->as.variable.name, func_name))
            {
                return expr;
            }
        }
    }

    return NULL;
}

/* Check if a return statement contains a tail-recursive call to the given function */
bool is_tail_recursive_return(Stmt *stmt, Token func_name)
{
    if (stmt == NULL) return false;
    if (stmt->type != STMT_RETURN) return false;

    ReturnStmt *ret = &stmt->as.return_stmt;
    if (ret->value == NULL) return false;

    return get_tail_call_expr(ret->value, func_name) != NULL;
}

/* Check if a function has any tail-recursive patterns.
   Searches all return statements for tail calls. */
static bool check_stmt_for_tail_recursion(Stmt *stmt, Token func_name)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_RETURN:
        return is_tail_recursive_return(stmt, func_name);

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (check_stmt_for_tail_recursion(stmt->as.block.statements[i], func_name))
            {
                return true;
            }
        }
        return false;

    case STMT_IF:
        if (check_stmt_for_tail_recursion(stmt->as.if_stmt.then_branch, func_name))
        {
            return true;
        }
        if (stmt->as.if_stmt.else_branch &&
            check_stmt_for_tail_recursion(stmt->as.if_stmt.else_branch, func_name))
        {
            return true;
        }
        return false;

    default:
        return false;
    }
}

bool function_has_tail_recursion(FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL || fn->body_count == 0) return false;

    for (int i = 0; i < fn->body_count; i++)
    {
        if (check_stmt_for_tail_recursion(fn->body[i], fn->name))
        {
            return true;
        }
    }
    return false;
}

/* Mark tail calls in a statement, returns count of calls marked */
static int mark_tail_calls_in_stmt(Stmt *stmt, Token func_name)
{
    if (stmt == NULL) return 0;

    int marked = 0;

    switch (stmt->type)
    {
    case STMT_RETURN:
    {
        Expr *call_expr = get_tail_call_expr(stmt->as.return_stmt.value, func_name);
        if (call_expr != NULL)
        {
            call_expr->as.call.is_tail_call = true;
            marked++;
        }
        break;
    }

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            marked += mark_tail_calls_in_stmt(stmt->as.block.statements[i], func_name);
        }
        break;

    case STMT_IF:
        marked += mark_tail_calls_in_stmt(stmt->as.if_stmt.then_branch, func_name);
        if (stmt->as.if_stmt.else_branch)
        {
            marked += mark_tail_calls_in_stmt(stmt->as.if_stmt.else_branch, func_name);
        }
        break;

    default:
        break;
    }

    return marked;
}

int optimizer_mark_tail_calls(Optimizer *opt, FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL || fn->body_count == 0) return 0;

    int marked = 0;
    for (int i = 0; i < fn->body_count; i++)
    {
        marked += mark_tail_calls_in_stmt(fn->body[i], fn->name);
    }

    opt->tail_calls_optimized += marked;
    return marked;
}

void optimizer_tail_call_optimization(Optimizer *opt, Module *module)
{
    if (module == NULL || module->statements == NULL) return;

    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            optimizer_mark_tail_calls(opt, &stmt->as.function);
        }
    }
}

/* ============================================================================
 * String Interpolation Optimization
 * ============================================================================
 * Merge adjacent string literals in interpolated expressions to reduce
 * runtime concatenations and temporary variables.
 */

/* Check if an expression is a string literal */
static bool is_string_literal(Expr *expr)
{
    if (expr == NULL) return false;
    if (expr->type != EXPR_LITERAL) return false;
    return expr->as.literal.type && expr->as.literal.type->kind == TYPE_STRING;
}

/* Get the string value from a string literal expression */
static const char *get_string_literal_value(Expr *expr)
{
    if (!is_string_literal(expr)) return NULL;
    return expr->as.literal.value.string_value;
}

/* Create a new string literal expression with the given value */
static Expr *create_string_literal(Arena *arena, const char *value)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.type = ast_create_primitive_type(arena, TYPE_STRING);
    expr->as.literal.value.string_value = arena_strdup(arena, value);
    expr->expr_type = expr->as.literal.type;
    return expr;
}

/* Merge adjacent string literals in an interpolated expression.
   Returns true if any merging was done. */
static bool merge_interpolated_parts(Optimizer *opt, InterpolExpr *interpol)
{
    if (interpol == NULL || interpol->part_count < 2) return false;

    int new_count = 0;
    Expr **new_parts = arena_alloc(opt->arena, interpol->part_count * sizeof(Expr *));
    bool any_merged = false;

    int i = 0;
    while (i < interpol->part_count)
    {
        if (is_string_literal(interpol->parts[i]))
        {
            /* Start merging string literals */
            const char *merged = get_string_literal_value(interpol->parts[i]);
            int merge_start = i;
            i++;

            /* Keep merging adjacent string literals */
            while (i < interpol->part_count && is_string_literal(interpol->parts[i]))
            {
                const char *next = get_string_literal_value(interpol->parts[i]);
                size_t merged_len = strlen(merged);
                size_t next_len = strlen(next);
                char *new_merged = arena_alloc(opt->arena, merged_len + next_len + 1);
                memcpy(new_merged, merged, merged_len);
                memcpy(new_merged + merged_len, next, next_len);
                new_merged[merged_len + next_len] = '\0';
                merged = new_merged;
                i++;
            }

            if (i - merge_start > 1)
            {
                /* We merged multiple literals */
                opt->string_literals_merged += (i - merge_start - 1);
                any_merged = true;
            }

            /* Create a new literal with the merged string */
            new_parts[new_count++] = create_string_literal(opt->arena, merged);
        }
        else
        {
            /* Non-string-literal, just copy it */
            new_parts[new_count++] = interpol->parts[i];
            i++;
        }
    }

    if (any_merged)
    {
        interpol->parts = new_parts;
        interpol->part_count = new_count;
    }

    return any_merged;
}

/* Recursively optimize string expressions */
Expr *optimize_string_expr(Optimizer *opt, Expr *expr)
{
    if (expr == NULL) return NULL;

    switch (expr->type)
    {
    case EXPR_INTERPOLATED:
        /* Merge adjacent string literals */
        merge_interpolated_parts(opt, &expr->as.interpol);

        /* Recursively optimize parts (they may contain nested interpolations) */
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            expr->as.interpol.parts[i] = optimize_string_expr(opt, expr->as.interpol.parts[i]);
        }
        break;

    case EXPR_BINARY:
        expr->as.binary.left = optimize_string_expr(opt, expr->as.binary.left);
        expr->as.binary.right = optimize_string_expr(opt, expr->as.binary.right);

        /* Merge string literal concatenations: "a" + "b" -> "ab" */
        if (expr->as.binary.operator == TOKEN_PLUS &&
            is_string_literal(expr->as.binary.left) &&
            is_string_literal(expr->as.binary.right))
        {
            const char *left = get_string_literal_value(expr->as.binary.left);
            const char *right = get_string_literal_value(expr->as.binary.right);
            size_t left_len = strlen(left);
            size_t right_len = strlen(right);
            char *merged = arena_alloc(opt->arena, left_len + right_len + 1);
            memcpy(merged, left, left_len);
            memcpy(merged + left_len, right, right_len);
            merged[left_len + right_len] = '\0';

            opt->string_literals_merged++;
            return create_string_literal(opt->arena, merged);
        }
        break;

    case EXPR_CALL:
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            expr->as.call.arguments[i] = optimize_string_expr(opt, expr->as.call.arguments[i]);
        }
        break;

    case EXPR_UNARY:
        expr->as.unary.operand = optimize_string_expr(opt, expr->as.unary.operand);
        break;

    case EXPR_ASSIGN:
        expr->as.assign.value = optimize_string_expr(opt, expr->as.assign.value);
        break;

    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            expr->as.array.elements[i] = optimize_string_expr(opt, expr->as.array.elements[i]);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        expr->as.array_access.array = optimize_string_expr(opt, expr->as.array_access.array);
        expr->as.array_access.index = optimize_string_expr(opt, expr->as.array_access.index);
        break;

    default:
        break;
    }

    return expr;
}

/* Optimize string expressions in a statement */
static void optimize_string_stmt(Optimizer *opt, Stmt *stmt)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        stmt->as.expression.expression = optimize_string_expr(opt, stmt->as.expression.expression);
        break;

    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
        {
            stmt->as.var_decl.initializer = optimize_string_expr(opt, stmt->as.var_decl.initializer);
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            stmt->as.return_stmt.value = optimize_string_expr(opt, stmt->as.return_stmt.value);
        }
        break;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            optimize_string_stmt(opt, stmt->as.block.statements[i]);
        }
        break;

    case STMT_IF:
        stmt->as.if_stmt.condition = optimize_string_expr(opt, stmt->as.if_stmt.condition);
        optimize_string_stmt(opt, stmt->as.if_stmt.then_branch);
        if (stmt->as.if_stmt.else_branch)
        {
            optimize_string_stmt(opt, stmt->as.if_stmt.else_branch);
        }
        break;

    case STMT_WHILE:
        stmt->as.while_stmt.condition = optimize_string_expr(opt, stmt->as.while_stmt.condition);
        optimize_string_stmt(opt, stmt->as.while_stmt.body);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
        {
            optimize_string_stmt(opt, stmt->as.for_stmt.initializer);
        }
        if (stmt->as.for_stmt.condition)
        {
            stmt->as.for_stmt.condition = optimize_string_expr(opt, stmt->as.for_stmt.condition);
        }
        if (stmt->as.for_stmt.increment)
        {
            stmt->as.for_stmt.increment = optimize_string_expr(opt, stmt->as.for_stmt.increment);
        }
        optimize_string_stmt(opt, stmt->as.for_stmt.body);
        break;

    case STMT_FOR_EACH:
        stmt->as.for_each_stmt.iterable = optimize_string_expr(opt, stmt->as.for_each_stmt.iterable);
        optimize_string_stmt(opt, stmt->as.for_each_stmt.body);
        break;

    default:
        break;
    }
}

/* Optimize string expressions in a function */
static void optimize_string_function(Optimizer *opt, FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL) return;

    for (int i = 0; i < fn->body_count; i++)
    {
        optimize_string_stmt(opt, fn->body[i]);
    }
}

int optimizer_merge_string_literals(Optimizer *opt, Module *module)
{
    if (module == NULL || module->statements == NULL) return 0;

    int initial = opt->string_literals_merged;

    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            optimize_string_function(opt, &stmt->as.function);
        }
    }

    return opt->string_literals_merged - initial;
}
