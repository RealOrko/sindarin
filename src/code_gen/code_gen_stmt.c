#include "code_gen/code_gen_stmt.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Push a loop arena onto the stack when entering a loop with per-iteration arena */
static void push_loop_arena(CodeGen *gen, char *arena_var, char *cleanup_label)
{
    if (gen->loop_arena_depth >= gen->loop_arena_capacity)
    {
        int new_capacity = gen->loop_arena_capacity == 0 ? 8 : gen->loop_arena_capacity * 2;
        char **new_arena_stack = arena_alloc(gen->arena, new_capacity * sizeof(char *));
        char **new_cleanup_stack = arena_alloc(gen->arena, new_capacity * sizeof(char *));
        for (int i = 0; i < gen->loop_arena_depth; i++)
        {
            new_arena_stack[i] = gen->loop_arena_stack[i];
            new_cleanup_stack[i] = gen->loop_cleanup_stack[i];
        }
        gen->loop_arena_stack = new_arena_stack;
        gen->loop_cleanup_stack = new_cleanup_stack;
        gen->loop_arena_capacity = new_capacity;
    }
    gen->loop_arena_stack[gen->loop_arena_depth] = arena_var;
    gen->loop_cleanup_stack[gen->loop_arena_depth] = cleanup_label;
    gen->loop_arena_depth++;

    /* Update current loop arena vars */
    gen->loop_arena_var = arena_var;
    gen->loop_cleanup_label = cleanup_label;
}

/* Pop a loop arena from the stack when exiting a loop */
static void pop_loop_arena(CodeGen *gen)
{
    if (gen->loop_arena_depth > 0)
    {
        gen->loop_arena_depth--;
        if (gen->loop_arena_depth > 0)
        {
            /* Restore to the enclosing loop's arena */
            gen->loop_arena_var = gen->loop_arena_stack[gen->loop_arena_depth - 1];
            gen->loop_cleanup_label = gen->loop_cleanup_stack[gen->loop_arena_depth - 1];
        }
        else
        {
            /* No more enclosing loops */
            gen->loop_arena_var = NULL;
            gen->loop_cleanup_label = NULL;
        }
    }
}

void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_expression_statement");
    char *expr_str = code_gen_expression(gen, stmt->expression);
    if (stmt->expression->expr_type->kind == TYPE_STRING && expression_produces_temp(stmt->expression))
    {
        // Skip freeing in arena context - arena handles cleanup
        if (gen->current_arena_var == NULL)
        {
            indented_fprintf(gen, indent, "{\n");
            indented_fprintf(gen, indent + 1, "char *_tmp = %s;\n", expr_str);
            indented_fprintf(gen, indent + 1, "(void)_tmp;\n");
            indented_fprintf(gen, indent + 1, "rt_free_string(_tmp);\n");
            indented_fprintf(gen, indent, "}\n");
        }
        else
        {
            indented_fprintf(gen, indent, "%s;\n", expr_str);
        }
    }
    else if (stmt->expression->type == EXPR_CALL && stmt->expression->expr_type->kind == TYPE_VOID)
    {
        // Statement expressions need a semicolon after them
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
}

void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_var_declaration");
    const char *type_c = get_c_type(gen->arena, stmt->type);
    char *var_name = get_var_name(gen->arena, stmt->name);

    // Check if this primitive is captured by a closure - if so, treat it like 'as ref'
    // This ensures mutations inside closures are visible to the outer scope
    MemoryQualifier effective_qual = stmt->mem_qualifier;
    if (effective_qual == MEM_DEFAULT && code_gen_is_captured_primitive(gen, var_name))
    {
        effective_qual = MEM_AS_REF;
    }

    // Add to symbol table with effective qualifier so accesses are dereferenced correctly
    symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL, effective_qual);

    char *init_str;
    if (stmt->initializer)
    {
        init_str = code_gen_expression(gen, stmt->initializer);
        // Wrap string literals in rt_to_string_string to create heap-allocated copies
        // This is needed because string variables may be freed/reassigned later
        if (stmt->type->kind == TYPE_STRING && stmt->initializer->type == EXPR_LITERAL)
        {
            init_str = arena_sprintf(gen->arena, "rt_to_string_string(%s, %s)", ARENA_VAR(gen), init_str);
        }

        // Handle 'as val' - create a copy for arrays and strings
        if (stmt->mem_qualifier == MEM_AS_VAL)
        {
            if (stmt->type->kind == TYPE_ARRAY)
            {
                // Get element type suffix for the clone function
                Type *elem_type = stmt->type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                init_str = arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), init_str);
            }
            else if (stmt->type->kind == TYPE_STRING)
            {
                init_str = arena_sprintf(gen->arena, "rt_to_string_string(%s, %s)", ARENA_VAR(gen), init_str);
            }
        }
    }
    else
    {
        init_str = arena_strdup(gen->arena, get_default_value(stmt->type));
    }

    // Handle 'as ref' or captured primitives - heap-allocate via arena
    if (effective_qual == MEM_AS_REF)
    {
        // Allocate on arena and store pointer
        // e.g., long *x = (long *)rt_arena_alloc(__arena_1__, sizeof(long)); *x = 42L;
        indented_fprintf(gen, indent, "%s *%s = (%s *)rt_arena_alloc(%s, sizeof(%s));\n",
                         type_c, var_name, type_c, ARENA_VAR(gen), type_c);
        indented_fprintf(gen, indent, "*%s = %s;\n", var_name, init_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s %s = %s;\n", type_c, var_name, init_str);
    }
}

void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_free_locals");

    // Skip manual freeing when in arena context - arena handles all deallocation
    if (gen->current_arena_var != NULL)
    {
        return;
    }

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
        else if (sym->type && sym->type->kind == TYPE_ARRAY && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = get_var_name(gen->arena, sym->name);
            Type *elem_type = sym->type->as.array.element_type;
            indented_fprintf(gen, indent, "if (%s) {\n", var_name);
            if (is_function && gen->current_return_type && gen->current_return_type->kind == TYPE_ARRAY)
            {
                indented_fprintf(gen, indent + 1, "if (%s != _return_value) {\n", var_name);
                if (elem_type && elem_type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent + 2, "rt_array_free_string(%s);\n", var_name);
                }
                else
                {
                    indented_fprintf(gen, indent + 2, "rt_array_free(%s);\n", var_name);
                }
                indented_fprintf(gen, indent + 1, "}\n");
            }
            else
            {
                if (elem_type && elem_type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent + 1, "rt_array_free_string(%s);\n", var_name);
                }
                else
                {
                    indented_fprintf(gen, indent + 1, "rt_array_free(%s);\n", var_name);
                }
            }
            indented_fprintf(gen, indent, "}\n");
        }
        sym = sym->next;
    }
}

void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_block");

    bool old_in_shared_context = gen->in_shared_context;
    bool old_in_private_context = gen->in_private_context;
    char *old_arena_var = gen->current_arena_var;
    int old_arena_depth = gen->arena_depth;

    bool is_shared = stmt->modifier == BLOCK_SHARED;
    bool is_private = stmt->modifier == BLOCK_PRIVATE;

    symbol_table_push_scope(gen->symbol_table);

    // Handle private block - create new arena
    if (is_private)
    {
        gen->in_private_context = true;
        gen->in_shared_context = false;
        gen->arena_depth++;
        gen->current_arena_var = arena_sprintf(gen->arena, "__arena_%d__", gen->arena_depth);
        /* Push arena name to stack for tracking nested private blocks */
        push_arena_to_stack(gen, gen->current_arena_var);
        symbol_table_enter_arena(gen->symbol_table);
    }
    // Handle shared block - uses parent's arena
    else if (is_shared)
    {
        gen->in_shared_context = true;
    }

    indented_fprintf(gen, indent, "{\n");

    // For private blocks, create a local arena
    if (is_private)
    {
        indented_fprintf(gen, indent + 1, "RtArena *%s = rt_arena_create(NULL);\n", gen->current_arena_var);
    }

    for (int i = 0; i < stmt->count; i++)
    {
        code_gen_statement(gen, stmt->statements[i], indent + 1);
    }
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);

    // For private blocks, destroy the arena
    if (is_private)
    {
        indented_fprintf(gen, indent + 1, "rt_arena_destroy(%s);\n", gen->current_arena_var);
        symbol_table_exit_arena(gen->symbol_table);
        /* Pop arena name from stack */
        pop_arena_from_stack(gen);
    }

    indented_fprintf(gen, indent, "}\n");
    symbol_table_pop_scope(gen->symbol_table);

    // Restore context
    gen->in_shared_context = old_in_shared_context;
    gen->in_private_context = old_in_private_context;
    gen->current_arena_var = old_arena_var;
    gen->arena_depth = old_arena_depth;
}

void code_gen_function(CodeGen *gen, FunctionStmt *stmt)
{
    DEBUG_VERBOSE("Entering code_gen_function");
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;
    FunctionModifier old_func_modifier = gen->current_func_modifier;
    bool old_in_private_context = gen->in_private_context;
    bool old_in_shared_context = gen->in_shared_context;
    char *old_arena_var = gen->current_arena_var;
    int old_arena_depth = gen->arena_depth;

    gen->current_function = get_var_name(gen->arena, stmt->name);
    gen->current_return_type = stmt->return_type;
    gen->current_func_modifier = stmt->modifier;

    bool is_main = strcmp(gen->current_function, "main") == 0;
    bool is_private = stmt->modifier == FUNC_PRIVATE;
    bool is_shared = stmt->modifier == FUNC_SHARED;
    // Functions returning heap-allocated types (closures, strings, arrays) must be
    // implicitly shared to avoid arena lifetime issues - the returned value must
    // live in caller's arena, not the function's arena which is destroyed on return
    bool returns_heap_type = stmt->return_type && (
        stmt->return_type->kind == TYPE_FUNCTION ||
        stmt->return_type->kind == TYPE_STRING ||
        stmt->return_type->kind == TYPE_ARRAY);
    if (returns_heap_type && !is_main) {
        is_shared = true;
    }
    // Check if function actually uses heap-allocated types
    bool uses_heap_types = function_needs_arena(stmt);
    // Functions need arena only if: (1) not shared AND (2) actually use heap types
    // Main always needs arena for safety, but regular functions can skip it
    bool needs_arena = is_main || (!is_shared && uses_heap_types);

    // Non-shared functions and main have their own arena context
    if (needs_arena)
    {
        if (is_private) gen->in_private_context = true;
        gen->in_shared_context = false;  // Reset shared context in non-shared functions
        gen->arena_depth++;
        gen->current_arena_var = arena_sprintf(gen->arena, "__arena_%d__", gen->arena_depth);
    }
    else if (is_shared)
    {
        // Shared functions use caller's arena passed as hidden parameter
        // and propagate shared context to nested loops
        gen->current_arena_var = "__caller_arena__";
        gen->in_shared_context = true;
    }

    // Special case for main: always use "int" return type in C for standard entry point.
    const char *ret_c = is_main ? "int" : get_c_type(gen->arena, gen->current_return_type);
    // Determine if we need a _return_value variable: only for non-void or main.
    bool has_return_value = (gen->current_return_type && gen->current_return_type->kind != TYPE_VOID) || is_main;
    symbol_table_push_scope(gen->symbol_table);

    // For private functions and main, enter arena scope in symbol table
    if (needs_arena)
    {
        symbol_table_enter_arena(gen->symbol_table);
    }

    for (int i = 0; i < stmt->param_count; i++)
    {
        symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->params[i].name, stmt->params[i].type, SYMBOL_PARAM);
    }

    // Pre-pass: scan function body for primitives captured by closures
    // These need to be declared as pointers for mutation persistence
    code_gen_scan_captured_primitives(gen, stmt->body, stmt->body_count);

    indented_fprintf(gen, 0, "%s %s(", ret_c, gen->current_function);

    // Shared functions receive caller's arena as first parameter
    if (is_shared)
    {
        fprintf(gen->output, "RtArena *__caller_arena__");
        if (stmt->param_count > 0)
        {
            fprintf(gen->output, ", ");
        }
    }

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

    // For private functions and main, create a local arena
    if (needs_arena)
    {
        indented_fprintf(gen, 1, "RtArena *%s = rt_arena_create(NULL);\n", gen->current_arena_var);
    }

    // Add _return_value only if needed (non-void or main).
    if (has_return_value)
    {
        const char *default_val = is_main ? "0" : get_default_value(gen->current_return_type);
        indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_c, default_val);
    }

    // Clone 'as val' array parameters to ensure copy semantics
    for (int i = 0; i < stmt->param_count; i++)
    {
        if (stmt->params[i].mem_qualifier == MEM_AS_VAL)
        {
            Type *param_type = stmt->params[i].type;
            if (param_type->kind == TYPE_ARRAY)
            {
                char *param_name = get_var_name(gen->arena, stmt->params[i].name);
                Type *elem_type = param_type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                indented_fprintf(gen, 1, "%s = rt_array_clone_%s(%s, %s);\n",
                                 param_name, suffix, ARENA_VAR(gen), param_name);
            }
            else if (param_type->kind == TYPE_STRING)
            {
                char *param_name = get_var_name(gen->arena, stmt->params[i].name);
                indented_fprintf(gen, 1, "%s = rt_to_string_string(%s, %s);\n",
                                 param_name, ARENA_VAR(gen), param_name);
            }
        }
    }

    // Check if function has marked tail calls for optimization
    bool has_tail_calls = function_has_marked_tail_calls(stmt);

    // Set up tail call optimization state
    bool old_in_tail_call_function = gen->in_tail_call_function;
    FunctionStmt *old_tail_call_fn = gen->tail_call_fn;

    if (has_tail_calls)
    {
        gen->in_tail_call_function = true;
        gen->tail_call_fn = stmt;
        // Wrap function body in a loop for tail call optimization
        indented_fprintf(gen, 1, "while (1) { /* tail call loop */\n");
    }

    bool has_return = false;
    if (stmt->body_count > 0 && stmt->body[stmt->body_count - 1]->type == STMT_RETURN)
    {
        has_return = true;
    }

    int body_indent = has_tail_calls ? 2 : 1;
    for (int i = 0; i < stmt->body_count; i++)
    {
        code_gen_statement(gen, stmt->body[i], body_indent);
    }
    if (!has_return)
    {
        indented_fprintf(gen, body_indent, "goto %s_return;\n", gen->current_function);
    }

    if (has_tail_calls)
    {
        indented_fprintf(gen, 1, "} /* end tail call loop */\n");
    }

    // Restore tail call state
    gen->in_tail_call_function = old_in_tail_call_function;
    gen->tail_call_fn = old_tail_call_fn;

    indented_fprintf(gen, 0, "%s_return:\n", gen->current_function);
    code_gen_free_locals(gen, gen->symbol_table->current, true, 1);

    // For private functions and main, destroy the arena before returning
    if (needs_arena)
    {
        indented_fprintf(gen, 1, "rt_arena_destroy(%s);\n", gen->current_arena_var);
    }

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

    // Exit arena scope in symbol table for private functions and main
    if (needs_arena)
    {
        symbol_table_exit_arena(gen->symbol_table);
    }

    symbol_table_pop_scope(gen->symbol_table);

    // Clear captured primitives list
    code_gen_clear_captured_primitives(gen);

    gen->current_function = old_function;
    gen->current_return_type = old_return_type;
    gen->current_func_modifier = old_func_modifier;
    gen->in_private_context = old_in_private_context;
    gen->in_shared_context = old_in_shared_context;
    gen->current_arena_var = old_arena_var;
    gen->arena_depth = old_arena_depth;
}

void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_return_statement");
    /* Check if returning from a void function/lambda */
    int is_void_return = (gen->current_return_type && gen->current_return_type->kind == TYPE_VOID);

    /* Check if this return contains a tail call that should be optimized */
    if (gen->in_tail_call_function && stmt->value &&
        stmt->value->type == EXPR_CALL && stmt->value->as.call.is_tail_call)
    {
        CallExpr *call = &stmt->value->as.call;
        FunctionStmt *fn = gen->tail_call_fn;

        /* Generate parameter assignments */
        /* For multiple parameters, we need temp variables to handle cases like
           return f(b, a) when the current params are (a, b) */
        if (fn->param_count > 1)
        {
            /* First, generate temp variables for all new argument values */
            for (int i = 0; i < call->arg_count; i++)
            {
                const char *param_type_c = get_c_type(gen->arena, fn->params[i].type);
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                indented_fprintf(gen, indent, "%s __tail_arg_%d__ = %s;\n",
                                 param_type_c, i, arg_str);
            }
            /* Then, assign temps to actual parameters */
            for (int i = 0; i < call->arg_count; i++)
            {
                char *param_name = get_var_name(gen->arena, fn->params[i].name);
                indented_fprintf(gen, indent, "%s = __tail_arg_%d__;\n",
                                 param_name, i);
            }
        }
        else if (fn->param_count == 1)
        {
            /* Single parameter - direct assignment is safe */
            char *param_name = get_var_name(gen->arena, fn->params[0].name);
            char *arg_str = code_gen_expression(gen, call->arguments[0]);
            indented_fprintf(gen, indent, "%s = %s;\n", param_name, arg_str);
        }
        /* Continue the tail call loop */
        indented_fprintf(gen, indent, "continue;\n");
        return;
    }

    /* Normal return */
    if (stmt->value && !is_void_return)
    {
        char *value_str = code_gen_expression(gen, stmt->value);
        indented_fprintf(gen, indent, "_return_value = %s;\n", value_str);
    }

    /* Clean up all active loop arenas before returning (innermost first) */
    for (int i = gen->loop_arena_depth - 1; i >= 0; i--)
    {
        if (gen->loop_arena_stack[i] != NULL)
        {
            indented_fprintf(gen, indent, "rt_arena_destroy(%s);\n", gen->loop_arena_stack[i]);
        }
    }

    /* Clean up all active private block arenas before returning (innermost first).
     * The function-level arena is NOT on this stack - it's destroyed at the return label.
     * This stack only contains private block arenas that need explicit cleanup. */
    for (int i = gen->arena_stack_depth - 1; i >= 0; i--)
    {
        if (gen->arena_stack[i] != NULL)
        {
            indented_fprintf(gen, indent, "rt_arena_destroy(%s);\n", gen->arena_stack[i]);
        }
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

    bool old_in_shared_context = gen->in_shared_context;
    char *old_current_arena_var = gen->current_arena_var;

    bool is_shared = stmt->is_shared;
    // Don't create loop arena if: loop is shared, OR we're inside a shared context
    bool needs_loop_arena = !is_shared && !gen->in_shared_context && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context (and not inside shared block), create per-iteration arena
    char *loop_arena = NULL;
    char *loop_cleanup = NULL;
    if (needs_loop_arena)
    {
        int label_num = code_gen_new_label(gen);
        loop_arena = arena_sprintf(gen->arena, "__loop_arena_%d__", label_num);
        loop_cleanup = arena_sprintf(gen->arena, "__loop_cleanup_%d__", label_num);
        push_loop_arena(gen, loop_arena, loop_cleanup);
    }

    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "while (%s) {\n", cond_str);

    // Create per-iteration arena at start of loop body
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "RtArena *%s = rt_arena_create(%s);\n",
                         loop_arena, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = loop_arena;
    }

    code_gen_statement(gen, stmt->body, indent + 1);

    // Restore parent arena before cleanup (for cleanup label context)
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent, "%s:\n", loop_cleanup);
        indented_fprintf(gen, indent + 1, "rt_arena_destroy(%s);\n", loop_arena);
        pop_loop_arena(gen);
    }

    indented_fprintf(gen, indent, "}\n");

    gen->in_shared_context = old_in_shared_context;
}

void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_statement");

    bool old_in_shared_context = gen->in_shared_context;
    char *old_current_arena_var = gen->current_arena_var;

    bool is_shared = stmt->is_shared;
    // Don't create loop arena if: loop is shared, OR we're inside a shared context
    bool needs_loop_arena = !is_shared && !gen->in_shared_context && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context (and not inside shared block), create per-iteration arena
    char *loop_arena = NULL;
    char *loop_cleanup = NULL;
    if (needs_loop_arena)
    {
        int arena_label_num = code_gen_new_label(gen);
        loop_arena = arena_sprintf(gen->arena, "__loop_arena_%d__", arena_label_num);
        loop_cleanup = arena_sprintf(gen->arena, "__loop_cleanup_%d__", arena_label_num);
        push_loop_arena(gen, loop_arena, loop_cleanup);
    }

    symbol_table_push_scope(gen->symbol_table);
    indented_fprintf(gen, indent, "{\n");

    /* Track loop counter variable for optimization if initializer is a var declaration */
    bool tracking_loop_counter = false;
    if (stmt->initializer && stmt->initializer->type == STMT_VAR_DECL)
    {
        const char *var_name = stmt->initializer->as.var_decl.name.start;
        push_loop_counter(gen, var_name);
        tracking_loop_counter = true;
    }

    if (stmt->initializer)
    {
        code_gen_statement(gen, stmt->initializer, indent + 1);
    }
    char *cond_str = NULL;
    if (stmt->condition)
    {
        cond_str = code_gen_expression(gen, stmt->condition);
    }

    // Save old continue label and create new one for this for loop
    char *old_continue_label = gen->for_continue_label;
    int label_num = code_gen_new_label(gen);
    char *continue_label = arena_sprintf(gen->arena, "__for_continue_%d__", label_num);
    gen->for_continue_label = continue_label;

    indented_fprintf(gen, indent + 1, "while (%s) {\n", cond_str ? cond_str : "1");

    // Create per-iteration arena at start of loop body
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 2, "RtArena *%s = rt_arena_create(%s);\n",
                         loop_arena, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = loop_arena;
    }

    code_gen_statement(gen, stmt->body, indent + 2);

    // Restore parent arena before cleanup
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction (before increment)
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "%s:\n", loop_cleanup);
        indented_fprintf(gen, indent + 2, "rt_arena_destroy(%s);\n", loop_arena);
        pop_loop_arena(gen);
    }

    // Generate continue label before increment
    indented_fprintf(gen, indent + 1, "%s:;\n", continue_label);

    if (stmt->increment)
    {
        char *inc_str = code_gen_expression(gen, stmt->increment);
        indented_fprintf(gen, indent + 2, "%s;\n", inc_str);
    }
    indented_fprintf(gen, indent + 1, "}\n");

    // Restore old continue label
    gen->for_continue_label = old_continue_label;

    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");

    /* Pop loop counter if we were tracking one */
    if (tracking_loop_counter)
    {
        pop_loop_counter(gen);
    }

    symbol_table_pop_scope(gen->symbol_table);

    gen->in_shared_context = old_in_shared_context;
}

void code_gen_for_each_statement(CodeGen *gen, ForEachStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_each_statement");

    bool old_in_shared_context = gen->in_shared_context;

    bool is_shared = stmt->is_shared;
    // Don't create loop arena if: loop is shared, OR we're inside a shared context
    bool needs_loop_arena = !is_shared && !gen->in_shared_context && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context (and not inside shared block), create per-iteration arena
    char *loop_arena = NULL;
    char *loop_cleanup = NULL;
    if (needs_loop_arena)
    {
        int arena_label_num = code_gen_new_label(gen);
        loop_arena = arena_sprintf(gen->arena, "__loop_arena_%d__", arena_label_num);
        loop_cleanup = arena_sprintf(gen->arena, "__loop_cleanup_%d__", arena_label_num);
        push_loop_arena(gen, loop_arena, loop_cleanup);
    }

    // Generate a unique index variable name
    int temp_idx = gen->temp_count++;
    char *idx_var = arena_sprintf(gen->arena, "__idx_%d__", temp_idx);
    char *len_var = arena_sprintf(gen->arena, "__len_%d__", temp_idx);
    char *arr_var = arena_sprintf(gen->arena, "__arr_%d__", temp_idx);

    // Get the iterable expression
    char *iterable_str = code_gen_expression(gen, stmt->iterable);

    // Get the element type from the iterable's type
    Type *iterable_type = stmt->iterable->expr_type;
    Type *elem_type = iterable_type->as.array.element_type;
    const char *elem_c_type = get_c_type(gen->arena, elem_type);
    const char *arr_c_type = get_c_type(gen->arena, iterable_type);

    // Get the loop variable name
    char *var_name = get_var_name(gen->arena, stmt->var_name);

    // Create a new scope
    symbol_table_push_scope(gen->symbol_table);

    // Add the loop variable to the symbol table
    // Use SYMBOL_PARAM so it's not freed - loop var is a reference to array element, not owned
    symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->var_name, elem_type, SYMBOL_PARAM);

    // Generate the for-each loop desugared to:
    // {
    //     arr_type __arr__ = iterable;
    //     long __len__ = rt_array_length(__arr__);
    //     for (long __idx__ = 0; __idx__ < __len__; __idx__++) {
    //         [RtArena *__loop_arena__ = rt_arena_create(parent);]  // if needs_loop_arena
    //         elem_type var = __arr__[__idx__];
    //         body
    //         [__loop_cleanup__: rt_arena_destroy(__loop_arena__);] // if needs_loop_arena
    //     }
    // }
    indented_fprintf(gen, indent, "{\n");
    indented_fprintf(gen, indent + 1, "%s %s = %s;\n", arr_c_type, arr_var, iterable_str);
    indented_fprintf(gen, indent + 1, "long %s = rt_array_length(%s);\n", len_var, arr_var);
    indented_fprintf(gen, indent + 1, "for (long %s = 0; %s < %s; %s++) {\n", idx_var, idx_var, len_var, idx_var);

    // Create per-iteration arena at start of loop body
    char *old_current_arena_var = gen->current_arena_var;
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 2, "RtArena *%s = rt_arena_create(%s);\n",
                         loop_arena, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = loop_arena;
    }

    indented_fprintf(gen, indent + 2, "%s %s = %s[%s];\n", elem_c_type, var_name, arr_var, idx_var);

    // Generate the body
    code_gen_statement(gen, stmt->body, indent + 2);

    // Restore parent arena before cleanup
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "%s:\n", loop_cleanup);
        indented_fprintf(gen, indent + 2, "rt_arena_destroy(%s);\n", loop_arena);
        pop_loop_arena(gen);
    }

    indented_fprintf(gen, indent + 1, "}\n");
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");

    gen->in_shared_context = old_in_shared_context;

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
    case STMT_FOR_EACH:
        code_gen_for_each_statement(gen, &stmt->as.for_each_stmt, indent);
        break;
    case STMT_BREAK:
        // If in a loop with per-iteration arena, destroy it before breaking
        if (gen->loop_arena_var)
        {
            indented_fprintf(gen, indent, "{ rt_arena_destroy(%s); break; }\n", gen->loop_arena_var);
        }
        else
        {
            indented_fprintf(gen, indent, "break;\n");
        }
        break;
    case STMT_CONTINUE:
        // If there's a loop cleanup label (for per-iteration arena), jump to it
        // The cleanup label destroys the arena and falls through to continue/increment
        if (gen->loop_cleanup_label)
        {
            indented_fprintf(gen, indent, "goto %s;\n", gen->loop_cleanup_label);
        }
        // In for loops without arena, continue needs to jump to the continue label (before increment)
        else if (gen->for_continue_label)
        {
            indented_fprintf(gen, indent, "goto %s;\n", gen->for_continue_label);
        }
        // In while/for-each loops without arena, regular continue works fine
        else
        {
            indented_fprintf(gen, indent, "continue;\n");
        }
        break;
    case STMT_IMPORT:
        break;
    }
}

/* Helper to check if type is a primitive (for capture analysis) */
static bool is_primitive_for_capture(Type *type)
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

/* Forward declaration for expression and statement scanning.
 * lambda_depth tracks how many lambda scopes we're nested in - we only
 * capture variables when lambda_depth > 0 (i.e., inside a lambda). */
static void scan_expr_for_captures(CodeGen *gen, Expr *expr, SymbolTable *table, int lambda_depth);
static void scan_stmt_for_captures(CodeGen *gen, Stmt *stmt, SymbolTable *table, int lambda_depth);

/* Add a variable name to the captured primitives list */
static void add_captured_primitive(CodeGen *gen, const char *name)
{
    /* Check if already in list */
    for (int i = 0; i < gen->captured_prim_count; i++)
    {
        if (strcmp(gen->captured_primitives[i], name) == 0)
            return;
    }
    /* Grow array if needed */
    if (gen->captured_prim_count >= gen->captured_prim_capacity)
    {
        int new_cap = gen->captured_prim_capacity == 0 ? 8 : gen->captured_prim_capacity * 2;
        char **new_names = arena_alloc(gen->arena, new_cap * sizeof(char *));
        for (int i = 0; i < gen->captured_prim_count; i++)
        {
            new_names[i] = gen->captured_primitives[i];
        }
        gen->captured_primitives = new_names;
        gen->captured_prim_capacity = new_cap;
    }
    gen->captured_primitives[gen->captured_prim_count] = arena_strdup(gen->arena, name);
    gen->captured_prim_count++;
}

/* Note: scan_lambda_for_captures removed - lambda handling is now done
 * directly in scan_expr_for_captures with lambda_depth tracking */

/* Scan an expression to find primitive identifiers that are captured by lambdas.
 * lambda_depth tracks how many lambda scopes we're nested in. We only mark
 * variables as captured when lambda_depth > 0 (i.e., we're inside a lambda). */
static void scan_expr_for_captures(CodeGen *gen, Expr *expr, SymbolTable *table, int lambda_depth)
{
    if (expr == NULL) return;

    switch (expr->type)
    {
    case EXPR_LAMBDA:
    {
        /* Found a lambda - look for outer scope primitives it references */
        LambdaExpr *lambda = &expr->as.lambda;

        /* Create temp scope for lambda params */
        symbol_table_push_scope(table);
        for (int i = 0; i < lambda->param_count; i++)
        {
            symbol_table_add_symbol(table, lambda->params[i].name, lambda->params[i].type);
        }

        /* Now scan the lambda body for identifiers that reference outer scope primitives.
         * Increment lambda_depth so we know we're inside a lambda. */
        /* Scan single expression body */
        if (lambda->body)
        {
            scan_expr_for_captures(gen, lambda->body, table, lambda_depth + 1);
        }
        /* Scan statement body - use full statement scanner to handle all statement types */
        if (lambda->has_stmt_body)
        {
            for (int i = 0; i < lambda->body_stmt_count; i++)
            {
                scan_stmt_for_captures(gen, lambda->body_stmts[i], table, lambda_depth + 1);
            }
        }
        symbol_table_pop_scope(table);
        break;
    }
    case EXPR_VARIABLE:
    {
        /* Only capture variables when we're inside a lambda (lambda_depth > 0) */
        if (lambda_depth > 0)
        {
            /* Check if this identifier is a primitive in outer scope that would be captured */
            char name[256];
            int len = expr->as.variable.name.length < 255 ? expr->as.variable.name.length : 255;
            strncpy(name, expr->as.variable.name.start, len);
            name[len] = '\0';

            Token tok = expr->as.variable.name;
            Symbol *sym = symbol_table_lookup_symbol(table, tok);
            if (sym && sym->kind == SYMBOL_LOCAL && is_primitive_for_capture(sym->type))
            {
                /* This is a local primitive referenced from inside a lambda - it's captured */
                add_captured_primitive(gen, name);
            }
        }
        break;
    }
    case EXPR_BINARY:
        scan_expr_for_captures(gen, expr->as.binary.left, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.binary.right, table, lambda_depth);
        break;
    case EXPR_UNARY:
        scan_expr_for_captures(gen, expr->as.unary.operand, table, lambda_depth);
        break;
    case EXPR_ASSIGN:
        scan_expr_for_captures(gen, expr->as.assign.value, table, lambda_depth);
        break;
    case EXPR_CALL:
        scan_expr_for_captures(gen, expr->as.call.callee, table, lambda_depth);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            scan_expr_for_captures(gen, expr->as.call.arguments[i], table, lambda_depth);
        break;
    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
            scan_expr_for_captures(gen, expr->as.array.elements[i], table, lambda_depth);
        break;
    case EXPR_ARRAY_ACCESS:
        scan_expr_for_captures(gen, expr->as.array_access.array, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.array_access.index, table, lambda_depth);
        break;
    case EXPR_INDEX_ASSIGN:
        scan_expr_for_captures(gen, expr->as.index_assign.array, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.index_assign.index, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.index_assign.value, table, lambda_depth);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        scan_expr_for_captures(gen, expr->as.operand, table, lambda_depth);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            scan_expr_for_captures(gen, expr->as.interpol.parts[i], table, lambda_depth);
        break;
    case EXPR_MEMBER:
        scan_expr_for_captures(gen, expr->as.member.object, table, lambda_depth);
        break;
    case EXPR_ARRAY_SLICE:
        scan_expr_for_captures(gen, expr->as.array_slice.array, table, lambda_depth);
        if (expr->as.array_slice.start) scan_expr_for_captures(gen, expr->as.array_slice.start, table, lambda_depth);
        if (expr->as.array_slice.end) scan_expr_for_captures(gen, expr->as.array_slice.end, table, lambda_depth);
        if (expr->as.array_slice.step) scan_expr_for_captures(gen, expr->as.array_slice.step, table, lambda_depth);
        break;
    case EXPR_RANGE:
        scan_expr_for_captures(gen, expr->as.range.start, table, lambda_depth);
        scan_expr_for_captures(gen, expr->as.range.end, table, lambda_depth);
        break;
    case EXPR_SPREAD:
        scan_expr_for_captures(gen, expr->as.spread.array, table, lambda_depth);
        break;
    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
            scan_expr_for_captures(gen, expr->as.static_call.arguments[i], table, lambda_depth);
        break;
    default:
        break;
    }
}

/* Scan a statement for lambda expressions and their captures.
 * lambda_depth tracks how many lambda scopes we're nested in. */
static void scan_stmt_for_captures(CodeGen *gen, Stmt *stmt, SymbolTable *table, int lambda_depth)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_VAR_DECL:
        /* First add this variable to scope so nested lambdas can see it */
        symbol_table_add_symbol_full(table, stmt->as.var_decl.name, stmt->as.var_decl.type,
                                     SYMBOL_LOCAL, stmt->as.var_decl.mem_qualifier);
        /* Then scan the initializer for lambda captures */
        if (stmt->as.var_decl.initializer)
            scan_expr_for_captures(gen, stmt->as.var_decl.initializer, table, lambda_depth);
        break;
    case STMT_EXPR:
        scan_expr_for_captures(gen, stmt->as.expression.expression, table, lambda_depth);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            scan_expr_for_captures(gen, stmt->as.return_stmt.value, table, lambda_depth);
        break;
    case STMT_BLOCK:
        symbol_table_push_scope(table);
        for (int i = 0; i < stmt->as.block.count; i++)
            scan_stmt_for_captures(gen, stmt->as.block.statements[i], table, lambda_depth);
        symbol_table_pop_scope(table);
        break;
    case STMT_IF:
        scan_expr_for_captures(gen, stmt->as.if_stmt.condition, table, lambda_depth);
        scan_stmt_for_captures(gen, stmt->as.if_stmt.then_branch, table, lambda_depth);
        if (stmt->as.if_stmt.else_branch)
            scan_stmt_for_captures(gen, stmt->as.if_stmt.else_branch, table, lambda_depth);
        break;
    case STMT_WHILE:
        scan_expr_for_captures(gen, stmt->as.while_stmt.condition, table, lambda_depth);
        scan_stmt_for_captures(gen, stmt->as.while_stmt.body, table, lambda_depth);
        break;
    case STMT_FOR:
        symbol_table_push_scope(table);
        if (stmt->as.for_stmt.initializer)
            scan_stmt_for_captures(gen, stmt->as.for_stmt.initializer, table, lambda_depth);
        if (stmt->as.for_stmt.condition)
            scan_expr_for_captures(gen, stmt->as.for_stmt.condition, table, lambda_depth);
        if (stmt->as.for_stmt.increment)
            scan_expr_for_captures(gen, stmt->as.for_stmt.increment, table, lambda_depth);
        scan_stmt_for_captures(gen, stmt->as.for_stmt.body, table, lambda_depth);
        symbol_table_pop_scope(table);
        break;
    case STMT_FOR_EACH:
        symbol_table_push_scope(table);
        /* Add loop variable - get element type from iterable's expr_type */
        scan_expr_for_captures(gen, stmt->as.for_each_stmt.iterable, table, lambda_depth);
        if (stmt->as.for_each_stmt.iterable->expr_type &&
            stmt->as.for_each_stmt.iterable->expr_type->kind == TYPE_ARRAY)
        {
            Type *elem_type = stmt->as.for_each_stmt.iterable->expr_type->as.array.element_type;
            symbol_table_add_symbol(table, stmt->as.for_each_stmt.var_name, elem_type);
        }
        scan_stmt_for_captures(gen, stmt->as.for_each_stmt.body, table, lambda_depth);
        symbol_table_pop_scope(table);
        break;
    default:
        break;
    }
}

/* Pre-pass to scan a function body for primitives captured by closures */
void code_gen_scan_captured_primitives(CodeGen *gen, Stmt **stmts, int stmt_count)
{
    /* Clear any existing captured primitives */
    code_gen_clear_captured_primitives(gen);

    /* Create a temporary symbol table scope for scanning */
    symbol_table_push_scope(gen->symbol_table);

    /* Start with lambda_depth = 0 (not inside any lambda) */
    for (int i = 0; i < stmt_count; i++)
    {
        scan_stmt_for_captures(gen, stmts[i], gen->symbol_table, 0);
    }

    symbol_table_pop_scope(gen->symbol_table);
}

/* Check if a variable name is a captured primitive */
bool code_gen_is_captured_primitive(CodeGen *gen, const char *name)
{
    for (int i = 0; i < gen->captured_prim_count; i++)
    {
        if (strcmp(gen->captured_primitives[i], name) == 0)
            return true;
    }
    return false;
}

/* Clear the captured primitives list */
void code_gen_clear_captured_primitives(CodeGen *gen)
{
    gen->captured_prim_count = 0;
}

/* Push arena name onto the private block arena stack */
void push_arena_to_stack(CodeGen *gen, const char *arena_name)
{
    /* Grow stack if needed */
    if (gen->arena_stack_depth >= gen->arena_stack_capacity)
    {
        int new_cap = gen->arena_stack_capacity == 0 ? 4 : gen->arena_stack_capacity * 2;
        char **new_stack = arena_alloc(gen->arena, new_cap * sizeof(char *));
        for (int i = 0; i < gen->arena_stack_depth; i++)
        {
            new_stack[i] = gen->arena_stack[i];
        }
        gen->arena_stack = new_stack;
        gen->arena_stack_capacity = new_cap;
    }
    gen->arena_stack[gen->arena_stack_depth] = arena_strdup(gen->arena, arena_name);
    gen->arena_stack_depth++;
}

/* Pop arena name from the private block arena stack */
const char *pop_arena_from_stack(CodeGen *gen)
{
    if (gen->arena_stack_depth <= 0)
    {
        return NULL;
    }
    gen->arena_stack_depth--;
    return gen->arena_stack[gen->arena_stack_depth];
}

/* Push a loop counter variable name onto the tracking stack.
 * Loop counters (like for-each __idx__ vars or C-style for loop vars starting at 0)
 * are provably non-negative and can skip negative index checks. */
void push_loop_counter(CodeGen *gen, const char *var_name)
{
    /* Grow stack if needed */
    if (gen->loop_counter_count >= gen->loop_counter_capacity)
    {
        int new_cap = gen->loop_counter_capacity == 0 ? 8 : gen->loop_counter_capacity * 2;
        char **new_stack = arena_alloc(gen->arena, new_cap * sizeof(char *));
        for (int i = 0; i < gen->loop_counter_count; i++)
        {
            new_stack[i] = gen->loop_counter_names[i];
        }
        gen->loop_counter_names = new_stack;
        gen->loop_counter_capacity = new_cap;
    }
    gen->loop_counter_names[gen->loop_counter_count] = arena_strdup(gen->arena, var_name);
    gen->loop_counter_count++;
}

/* Pop a loop counter variable name from the tracking stack. */
void pop_loop_counter(CodeGen *gen)
{
    if (gen->loop_counter_count > 0)
    {
        gen->loop_counter_count--;
    }
}

/* Check if a variable name is a tracked loop counter (provably non-negative). */
bool is_tracked_loop_counter(CodeGen *gen, const char *var_name)
{
    if (var_name == NULL)
    {
        return false;
    }
    for (int i = 0; i < gen->loop_counter_count; i++)
    {
        if (gen->loop_counter_names[i] != NULL &&
            strcmp(gen->loop_counter_names[i], var_name) == 0)
        {
            return true;
        }
    }
    return false;
}
