#include "type_checker/type_checker_stmt.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_expr.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* Reserved keyword table for namespace validation */
static const char *reserved_keywords[] = {
    "fn", "var", "return", "if", "else", "for", "while", "break", "continue",
    "in", "import", "nil", "int", "long", "double", "char", "str", "bool",
    "byte", "void", "shared", "private", "as", "val", "ref", "true", "false",
    NULL
};

/* Check if a token matches a reserved keyword.
 * Returns the keyword string if it matches, NULL otherwise. */
static const char *is_reserved_keyword(Token token)
{
    for (int i = 0; reserved_keywords[i] != NULL; i++)
    {
        const char *keyword = reserved_keywords[i];
        int keyword_len = strlen(keyword);
        if (token.length == keyword_len &&
            memcmp(token.start, keyword, keyword_len) == 0)
        {
            return keyword;
        }
    }
    return NULL;
}

/* Infer missing lambda types from a function type annotation */
static void infer_lambda_types(Expr *lambda_expr, Type *func_type)
{
    if (lambda_expr == NULL || lambda_expr->type != EXPR_LAMBDA)
        return;
    if (func_type == NULL || func_type->kind != TYPE_FUNCTION)
        return;

    LambdaExpr *lambda = &lambda_expr->as.lambda;

    /* Check parameter count matches */
    if (lambda->param_count != func_type->as.function.param_count)
    {
        DEBUG_VERBOSE("Lambda param count %d doesn't match function type param count %d",
                      lambda->param_count, func_type->as.function.param_count);
        return;
    }

    /* Infer missing parameter types */
    for (int i = 0; i < lambda->param_count; i++)
    {
        if (lambda->params[i].type == NULL)
        {
            lambda->params[i].type = func_type->as.function.param_types[i];
            DEBUG_VERBOSE("Inferred parameter %d type from function type", i);
        }
    }

    /* Infer missing return type */
    if (lambda->return_type == NULL)
    {
        lambda->return_type = func_type->as.function.return_type;
        DEBUG_VERBOSE("Inferred return type from function type");
    }
}

static void type_check_var_decl(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    (void)return_type;
    DEBUG_VERBOSE("Type checking variable declaration: %.*s", stmt->as.var_decl.name.length, stmt->as.var_decl.name.start);
    Type *decl_type = stmt->as.var_decl.type;
    Type *init_type = NULL;
    if (stmt->as.var_decl.initializer)
    {
        /* If initializer is a lambda with missing types, infer from declared type */
        if (stmt->as.var_decl.initializer->type == EXPR_LAMBDA &&
            decl_type != NULL && decl_type->kind == TYPE_FUNCTION)
        {
            infer_lambda_types(stmt->as.var_decl.initializer, decl_type);
        }

        init_type = type_check_expr(stmt->as.var_decl.initializer, table);
        if (init_type == NULL)
        {
            // If we have a declared type, use it; otherwise use NIL as placeholder
            Type *fallback = decl_type ? decl_type : ast_create_primitive_type(table->arena, TYPE_NIL);
            symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, fallback, SYMBOL_LOCAL);
            return;
        }
        // Void thread spawns cannot be assigned to variables (fire-and-forget only)
        if (stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN &&
            init_type->kind == TYPE_VOID)
        {
            type_error(&stmt->as.var_decl.name, "Cannot assign void thread spawn to variable");
            return;
        }
        // For empty array literals, adopt the declared type for code generation
        if (decl_type && init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_NIL &&
            decl_type->kind == TYPE_ARRAY)
        {
            // Update the expression's type to match the declared type
            stmt->as.var_decl.initializer->expr_type = decl_type;
            init_type = decl_type;
        }
        // For int[] assigned to byte[], update the expression type to byte[]
        // This allows int literals to be used in byte array literals
        if (decl_type && decl_type->kind == TYPE_ARRAY &&
            decl_type->as.array.element_type->kind == TYPE_BYTE &&
            init_type->kind == TYPE_ARRAY &&
            init_type->as.array.element_type->kind == TYPE_INT)
        {
            stmt->as.var_decl.initializer->expr_type = decl_type;
            init_type = decl_type;
        }
    }

    // Type inference: if no declared type, infer from initializer
    if (decl_type == NULL)
    {
        if (init_type == NULL)
        {
            type_error(&stmt->as.var_decl.name, "Cannot infer type without initializer");
            decl_type = ast_create_primitive_type(table->arena, TYPE_NIL);
        }
        else
        {
            decl_type = init_type;
            // Update the statement's type for code generation
            stmt->as.var_decl.type = decl_type;
        }
    }

    // Validate memory qualifier usage
    MemoryQualifier mem_qual = stmt->as.var_decl.mem_qualifier;
    if (mem_qual == MEM_AS_REF)
    {
        // 'as ref' can only be used with primitive types
        if (!is_primitive_type(decl_type))
        {
            type_error(&stmt->as.var_decl.name, "'as ref' can only be used with primitive types");
        }
    }
    else if (mem_qual == MEM_AS_VAL)
    {
        // 'as val' is meaningful only for reference types (arrays, strings)
        // For primitives, it's a no-op but we allow it
        if (is_primitive_type(decl_type))
        {
            DEBUG_VERBOSE("Warning: 'as val' on primitive type has no effect");
        }
    }

    symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name, decl_type, SYMBOL_LOCAL);
    if (init_type && !ast_type_equals(init_type, decl_type))
    {
        if (stmt->as.var_decl.initializer &&
            stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN)
        {
            type_error(&stmt->as.var_decl.name,
                       "Thread spawn return type does not match variable type");
        }
        else
        {
            type_error(&stmt->as.var_decl.name, "Initializer type does not match variable type");
        }
    }

    // Mark variable as pending if initialized with a thread spawn (non-void)
    if (stmt->as.var_decl.initializer &&
        stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN &&
        init_type && init_type->kind != TYPE_VOID)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, stmt->as.var_decl.name);
        if (sym != NULL)
        {
            symbol_table_mark_pending(sym);

            /* Collect frozen arguments from the spawn call and store on pending symbol.
             * This allows unfreezing when the variable is synced. */
            Expr *spawn = stmt->as.var_decl.initializer;
            Expr *call = spawn->as.thread_spawn.call;

            /* Static calls (like Process.run) don't have frozen args to track */
            if (call != NULL && call->type == EXPR_STATIC_CALL)
            {
                /* No frozen args handling needed for static method spawns */
            }
            else if (call != NULL && call->type == EXPR_CALL)
            {
                int arg_count = call->as.call.arg_count;
                Expr **arguments = call->as.call.arguments;

                /* Get function type to access param_mem_quals for 'as ref' detection */
                Expr *callee = call->as.call.callee;
                Type *func_type = NULL;
                if (callee != NULL && callee->type == EXPR_VARIABLE)
                {
                    Symbol *func_sym = symbol_table_lookup_symbol(table, callee->as.variable.name);
                    if (func_sym != NULL && func_sym->type != NULL &&
                        func_sym->type->kind == TYPE_FUNCTION)
                    {
                        func_type = func_sym->type;
                    }
                }
                MemoryQualifier *param_quals = (func_type != NULL) ?
                    func_type->as.function.param_mem_quals : NULL;
                int param_count = (func_type != NULL) ?
                    func_type->as.function.param_count : 0;

                /* Count frozen args first (arrays, strings, and 'as ref' primitives) */
                int frozen_count = 0;
                for (int i = 0; i < arg_count; i++)
                {
                    Expr *arg = arguments[i];
                    if (arg != NULL && arg->type == EXPR_VARIABLE)
                    {
                        Symbol *arg_sym = symbol_table_lookup_symbol(table, arg->as.variable.name);
                        if (arg_sym != NULL && arg_sym->type != NULL)
                        {
                            /* Arrays and strings are always frozen */
                            if (arg_sym->type->kind == TYPE_ARRAY || arg_sym->type->kind == TYPE_STRING)
                            {
                                frozen_count++;
                            }
                            /* Primitives with 'as ref' are also frozen */
                            else if (param_quals != NULL && i < param_count &&
                                     param_quals[i] == MEM_AS_REF)
                            {
                                frozen_count++;
                            }
                        }
                    }
                }

                /* Allocate and fill frozen_args array */
                if (frozen_count > 0)
                {
                    Symbol **frozen_args = (Symbol **)arena_alloc(table->arena,
                                                                   sizeof(Symbol *) * frozen_count);
                    int idx = 0;
                    for (int i = 0; i < arg_count; i++)
                    {
                        Expr *arg = arguments[i];
                        if (arg != NULL && arg->type == EXPR_VARIABLE)
                        {
                            Symbol *arg_sym = symbol_table_lookup_symbol(table, arg->as.variable.name);
                            if (arg_sym != NULL && arg_sym->type != NULL)
                            {
                                /* Arrays and strings are always frozen */
                                if (arg_sym->type->kind == TYPE_ARRAY || arg_sym->type->kind == TYPE_STRING)
                                {
                                    frozen_args[idx++] = arg_sym;
                                }
                                /* Primitives with 'as ref' are also frozen */
                                else if (param_quals != NULL && i < param_count &&
                                         param_quals[i] == MEM_AS_REF)
                                {
                                    frozen_args[idx++] = arg_sym;
                                }
                            }
                        }
                    }
                    symbol_table_set_frozen_args(sym, frozen_args, frozen_count);
                }
            }
        }
    }
}

/* Type-check only the function body, without adding to global scope.
 * Used for namespaced imports where the function is registered under a namespace. */
static void type_check_function_body_only(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function body only: %.*s", stmt->as.function.name.length, stmt->as.function.name.start);
    Arena *arena = table->arena;

    symbol_table_push_scope(table);

    for (int i = 0; i < stmt->as.function.param_count; i++)
    {
        Parameter param = stmt->as.function.params[i];
        if (param.type == NULL)
        {
            param.type = ast_create_primitive_type(arena, TYPE_NIL);
        }
        symbol_table_add_symbol_full(table, param.name, param.type, SYMBOL_PARAM, param.mem_qualifier);
    }

    table->current->next_local_offset = table->current->next_param_offset;

    for (int i = 0; i < stmt->as.function.body_count; i++)
    {
        type_check_stmt(stmt->as.function.body[i], table, stmt->as.function.return_type);
    }

    symbol_table_pop_scope(table);
}

static void type_check_function(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function with %d parameters", stmt->as.function.param_count);

    /* Create function type from declaration */
    Arena *arena = table->arena;
    Type **param_types = (Type **)arena_alloc(arena, sizeof(Type *) * stmt->as.function.param_count);
    for (int i = 0; i < stmt->as.function.param_count; i++) {
        Type *param_type = stmt->as.function.params[i].type;
        /* Handle null parameter type - use NIL as placeholder */
        if (param_type == NULL) {
            param_type = ast_create_primitive_type(arena, TYPE_NIL);
        }
        param_types[i] = param_type;
    }
    Type *func_type = ast_create_function_type(arena, stmt->as.function.return_type, param_types, stmt->as.function.param_count);

    /* Store parameter memory qualifiers in the function type for thread safety analysis.
     * This allows detecting 'as ref' primitives when checking thread spawn arguments. */
    if (stmt->as.function.param_count > 0)
    {
        bool has_non_default_qual = false;
        for (int i = 0; i < stmt->as.function.param_count; i++)
        {
            if (stmt->as.function.params[i].mem_qualifier != MEM_DEFAULT)
            {
                has_non_default_qual = true;
                break;
            }
        }

        if (has_non_default_qual)
        {
            func_type->as.function.param_mem_quals = (MemoryQualifier *)arena_alloc(arena,
                sizeof(MemoryQualifier) * stmt->as.function.param_count);
            for (int i = 0; i < stmt->as.function.param_count; i++)
            {
                func_type->as.function.param_mem_quals[i] = stmt->as.function.params[i].mem_qualifier;
            }
        }
    }

    /* Validate private function return type */
    FunctionModifier modifier = stmt->as.function.modifier;
    if (modifier == FUNC_PRIVATE)
    {
        Type *return_type = stmt->as.function.return_type;
        if (!can_escape_private(return_type))
        {
            type_error(&stmt->as.function.name,
                       "Private function can only return primitive types (int, double, bool, char)");
        }
    }

    /* Functions returning heap-allocated types (closures, strings, arrays) must be
     * implicitly shared to avoid arena lifetime issues - the returned value must
     * live in caller's arena, not the function's arena which is destroyed on return.
     * This matches the implicit sharing logic in code_gen_stmt.c:301-307. */
    FunctionModifier effective_modifier = modifier;
    if (stmt->as.function.return_type &&
        (stmt->as.function.return_type->kind == TYPE_FUNCTION ||
         stmt->as.function.return_type->kind == TYPE_STRING ||
         stmt->as.function.return_type->kind == TYPE_ARRAY) &&
        modifier != FUNC_PRIVATE)
    {
        effective_modifier = FUNC_SHARED;
    }

    /* Check for duplicate function definition (collision from imports).
     * If a function with this name already exists, report a collision error. */
    Symbol *existing = symbol_table_lookup_symbol(table, stmt->as.function.name);
    if (existing != NULL && existing->is_function)
    {
        char name_str[128];
        int name_len = stmt->as.function.name.length < 127 ? stmt->as.function.name.length : 127;
        memcpy(name_str, stmt->as.function.name.start, name_len);
        name_str[name_len] = '\0';

        char msg[256];
        snprintf(msg, sizeof(msg), "Function '%s' is already defined (possible import collision)", name_str);
        type_error(&stmt->as.function.name, msg);
        return;
    }

    /* Add function symbol to current scope (e.g., global) with its modifier.
     * We pass both the effective modifier (for code gen arena passing) and
     * the declared modifier (for thread spawn mode selection). */
    symbol_table_add_function(table, stmt->as.function.name, func_type, effective_modifier, modifier);

    symbol_table_push_scope(table);

    for (int i = 0; i < stmt->as.function.param_count; i++)
    {
        Parameter param = stmt->as.function.params[i];
        DEBUG_VERBOSE("Adding parameter %d: %.*s", i, param.name.length, param.name.start);

        /* Check for null parameter type - report error and use placeholder */
        if (param.type == NULL)
        {
            type_error(&param.name, "Parameter type is missing");
            param.type = ast_create_primitive_type(arena, TYPE_NIL);
        }

        /* Validate parameter memory qualifier */
        if (param.mem_qualifier == MEM_AS_VAL)
        {
            /* 'as val' on parameters is meaningful only for reference types */
            if (is_primitive_type(param.type))
            {
                DEBUG_VERBOSE("Warning: 'as val' on primitive parameter has no effect");
            }
        }
        else if (param.mem_qualifier == MEM_AS_REF)
        {
            /* 'as ref' on primitive parameters allows caller to pass a reference
             * that the function can modify. This enables shared mutable state. */
            if (!is_primitive_type(param.type))
            {
                /* 'as ref' only makes sense for primitives - arrays are already references */
                type_error(&param.name, "'as ref' only applies to primitive parameters");
            }
        }

        /* Add symbol with the memory qualifier so code gen can handle dereferencing */
        symbol_table_add_symbol_full(table, param.name, param.type, SYMBOL_PARAM, param.mem_qualifier);
    }

    table->current->next_local_offset = table->current->next_param_offset;

    for (int i = 0; i < stmt->as.function.body_count; i++)
    {
        type_check_stmt(stmt->as.function.body[i], table, stmt->as.function.return_type);
    }
    symbol_table_pop_scope(table);
}

static void type_check_return(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking return statement");
    Type *value_type;
    if (stmt->as.return_stmt.value)
    {
        value_type = type_check_expr(stmt->as.return_stmt.value, table);
        if (value_type == NULL)
            return;
    }
    else
    {
        value_type = ast_create_primitive_type(table->arena, TYPE_VOID);
    }
    if (!ast_type_equals(value_type, return_type))
    {
        type_error(stmt->token, "Return type does not match function return type");
    }
}

static void type_check_block(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking block with %d statements", stmt->as.block.count);

    BlockModifier modifier = stmt->as.block.modifier;
    bool is_private = modifier == BLOCK_PRIVATE;

    if (is_private)
    {
        DEBUG_VERBOSE("Entering private block - escape analysis will be enforced");
        symbol_table_enter_arena(table);
    }
    else if (modifier == BLOCK_SHARED)
    {
        DEBUG_VERBOSE("Entering shared block - using parent's arena");
        /* Shared block: allocations use parent's arena, no special restrictions */
    }

    symbol_table_push_scope(table);
    for (int i = 0; i < stmt->as.block.count; i++)
    {
        type_check_stmt(stmt->as.block.statements[i], table, return_type);
    }
    symbol_table_pop_scope(table);

    if (is_private)
    {
        symbol_table_exit_arena(table);
    }
}

static void type_check_if(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking if statement");
    Type *cond_type = type_check_expr(stmt->as.if_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.if_stmt.condition->token, "If condition must be boolean");
    }
    type_check_stmt(stmt->as.if_stmt.then_branch, table, return_type);
    if (stmt->as.if_stmt.else_branch)
    {
        DEBUG_VERBOSE("Type checking else branch");
        type_check_stmt(stmt->as.if_stmt.else_branch, table, return_type);
    }
}

static void type_check_while(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking while statement");
    Type *cond_type = type_check_expr(stmt->as.while_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.while_stmt.condition->token, "While condition must be boolean");
    }

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.while_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    type_check_stmt(stmt->as.while_stmt.body, table, return_type);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }
}

static void type_check_for(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for statement");
    symbol_table_push_scope(table);
    if (stmt->as.for_stmt.initializer)
    {
        type_check_stmt(stmt->as.for_stmt.initializer, table, return_type);
    }
    if (stmt->as.for_stmt.condition)
    {
        Type *cond_type = type_check_expr(stmt->as.for_stmt.condition, table);
        if (cond_type && cond_type->kind != TYPE_BOOL)
        {
            type_error(stmt->as.for_stmt.condition->token, "For condition must be boolean");
        }
    }
    if (stmt->as.for_stmt.increment)
    {
        type_check_expr(stmt->as.for_stmt.increment, table);
    }

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.for_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    type_check_stmt(stmt->as.for_stmt.body, table, return_type);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }

    symbol_table_pop_scope(table);
}

static void type_check_for_each(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for-each statement");

    // Type check the iterable expression
    Type *iterable_type = type_check_expr(stmt->as.for_each_stmt.iterable, table);
    if (iterable_type == NULL)
    {
        return;
    }

    // Verify the iterable is an array type
    if (iterable_type->kind != TYPE_ARRAY)
    {
        type_error(stmt->as.for_each_stmt.iterable->token, "For-each iterable must be an array");
        return;
    }

    // Get the element type from the array
    Type *element_type = iterable_type->as.array.element_type;

    // Create a new scope and add the loop variable
    // Use SYMBOL_PARAM so it's not freed - loop var is a reference to array element, not owned
    symbol_table_push_scope(table);
    symbol_table_add_symbol_with_kind(table, stmt->as.for_each_stmt.var_name, element_type, SYMBOL_PARAM);

    // Non-shared loops have per-iteration arenas - enter arena context for escape analysis
    bool is_shared = stmt->as.for_each_stmt.is_shared;
    if (!is_shared)
    {
        symbol_table_enter_arena(table);
    }

    // Type check the body
    type_check_stmt(stmt->as.for_each_stmt.body, table, return_type);

    if (!is_shared)
    {
        symbol_table_exit_arena(table);
    }

    symbol_table_pop_scope(table);
}

/* Type check an import statement.
 *
 * For non-namespaced imports (namespace == NULL):
 *   - Module symbols are added to global scope when their function definitions
 *     are type-checked (handled by type_check_function)
 *   - Collision detection happens in type_check_function
 *   - This function just logs for debugging purposes
 *
 * For namespaced imports (namespace != NULL):
 *   - Creates a namespace entry in the symbol table
 *   - Registers all function symbols from imported module under that namespace
 *   - Namespaced symbols are NOT added to global scope directly
 *   - They are only accessible via namespace.symbol syntax
 */
static void type_check_import_stmt(Stmt *stmt, SymbolTable *table)
{
    ImportStmt *import = &stmt->as.import;

    char mod_str[128];
    int mod_len = import->module_name.length < 127 ? import->module_name.length : 127;
    memcpy(mod_str, import->module_name.start, mod_len);
    mod_str[mod_len] = '\0';

    if (import->namespace == NULL)
    {
        /* Non-namespaced import: symbols are added to global scope when
         * the imported function definitions are type-checked. The parser
         * merges imported statements into the main module, and collision
         * detection is handled by type_check_function when those merged
         * function statements are processed. */
        DEBUG_VERBOSE("Type checking non-namespaced import of '%s'", mod_str);
    }
    else
    {
        /* Namespaced import: create namespace and register symbols */
        Token ns_token = *import->namespace;
        char ns_str[128];
        int ns_len = ns_token.length < 127 ? ns_token.length : 127;
        memcpy(ns_str, ns_token.start, ns_len);
        ns_str[ns_len] = '\0';
        DEBUG_VERBOSE("Type checking namespaced import of '%s' as '%s'", mod_str, ns_str);

        /* Check if namespace identifier is a reserved keyword */
        const char *reserved = is_reserved_keyword(ns_token);
        if (reserved != NULL)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot use reserved keyword '%s' as namespace name", reserved);
            type_error(&ns_token, msg);
            return;
        }

        /* Check if namespace already exists */
        if (symbol_table_is_namespace(table, ns_token))
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Namespace '%s' is already defined", ns_str);
            type_error(&ns_token, msg);
            return;
        }

        /* Also check if a non-namespace symbol with this name exists */
        Symbol *existing = symbol_table_lookup_symbol(table, ns_token);
        if (existing != NULL)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot use '%s' as namespace: name already in use", ns_str);
            type_error(&ns_token, msg);
            return;
        }

        /* Create the namespace entry in the symbol table */
        symbol_table_add_namespace(table, ns_token);

        /* Use get_module_symbols to extract symbols and types from imported module.
         * Create a temporary Module structure to use with the helper function. */
        Module temp_module;
        temp_module.statements = import->imported_stmts;
        temp_module.count = import->imported_count;
        temp_module.capacity = import->imported_count;
        temp_module.filename = NULL;

        Token **symbols = NULL;
        Type **types = NULL;
        int symbol_count = 0;

        get_module_symbols(&temp_module, table, &symbols, &types, &symbol_count);

        /* Handle empty modules gracefully */
        if (symbol_count == 0)
        {
            DEBUG_VERBOSE("No symbols to import from module '%s'", mod_str);
            return;
        }

        /* Register all extracted symbols under the namespace.
         * We need to iterate through original statements to get function modifiers
         * since get_module_symbols only extracts names and types. */
        int sym_idx = 0;
        for (int i = 0; i < import->imported_count && sym_idx < symbol_count; i++)
        {
            Stmt *imported_stmt = import->imported_stmts[i];
            if (imported_stmt == NULL)
                continue;

            if (imported_stmt->type == STMT_FUNCTION)
            {
                FunctionStmt *func = &imported_stmt->as.function;

                /* Use the type extracted by get_module_symbols */
                Type *func_type = types[sym_idx];
                Token *func_name = symbols[sym_idx];
                sym_idx++;

                /* Determine effective modifier - same logic as type_check_function.
                 * Functions returning heap-allocated types are implicitly shared. */
                FunctionModifier modifier = func->modifier;
                FunctionModifier effective_modifier = modifier;
                if (func->return_type &&
                    (func->return_type->kind == TYPE_FUNCTION ||
                     func->return_type->kind == TYPE_STRING ||
                     func->return_type->kind == TYPE_ARRAY) &&
                    modifier != FUNC_PRIVATE)
                {
                    effective_modifier = FUNC_SHARED;
                }

                /* Add function symbol to namespace with proper function modifier */
                symbol_table_add_function_to_namespace(table, ns_token, *func_name, func_type, effective_modifier, modifier);

                char func_str[128];
                int func_len = func_name->length < 127 ? func_name->length : 127;
                memcpy(func_str, func_name->start, func_len);
                func_str[func_len] = '\0';
                DEBUG_VERBOSE("Added function '%s' to namespace '%s' (mod=%d)", func_str, ns_str, effective_modifier);

                /* Type-check the function body so expr_type is set for code generation.
                 * Use the body-only version to avoid adding to global scope. */
                type_check_function_body_only(imported_stmt, table);
            }
        }
    }
}

void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    if (stmt == NULL)
    {
        DEBUG_VERBOSE("Statement is NULL");
        return;
    }
    DEBUG_VERBOSE("Type checking statement type: %d", stmt->type);
    switch (stmt->type)
    {
    case STMT_EXPR:
        type_check_expr(stmt->as.expression.expression, table);
        break;
    case STMT_VAR_DECL:
        type_check_var_decl(stmt, table, return_type);
        break;
    case STMT_FUNCTION:
        type_check_function(stmt, table);
        break;
    case STMT_RETURN:
        type_check_return(stmt, table, return_type);
        break;
    case STMT_BLOCK:
        type_check_block(stmt, table, return_type);
        break;
    case STMT_IF:
        type_check_if(stmt, table, return_type);
        break;
    case STMT_WHILE:
        type_check_while(stmt, table, return_type);
        break;
    case STMT_FOR:
        type_check_for(stmt, table, return_type);
        break;
    case STMT_FOR_EACH:
        type_check_for_each(stmt, table, return_type);
        break;
    case STMT_BREAK:
        DEBUG_VERBOSE("Type checking break statement");
        // TODO: Verify break is inside a loop
        break;
    case STMT_CONTINUE:
        DEBUG_VERBOSE("Type checking continue statement");
        // TODO: Verify continue is inside a loop
        break;
    case STMT_IMPORT:
        type_check_import_stmt(stmt, table);
        break;
    }
}
