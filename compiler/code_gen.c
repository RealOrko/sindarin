#include "code_gen.h"
#include "code_gen_util.h"
#include "code_gen_expr.h"
#include "code_gen_stmt.h"
#include "debug.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "arena.h"

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
    gen->for_continue_label = NULL;

    /* Initialize arena context fields */
    gen->arena_depth = 0;
    gen->in_shared_context = false;
    gen->in_private_context = false;
    gen->current_arena_var = NULL;
    gen->current_func_modifier = FUNC_DEFAULT;

    /* Initialize loop arena fields */
    gen->loop_arena_var = NULL;
    gen->loop_cleanup_label = NULL;

    /* Initialize lambda fields */
    gen->lambda_count = 0;
    gen->lambda_forward_decls = arena_strdup(arena, "");
    gen->lambda_definitions = arena_strdup(arena, "");

    /* Initialize buffering fields */
    gen->function_definitions = arena_strdup(arena, "");
    gen->buffering_functions = false;

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

static void code_gen_headers(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_headers");
    indented_fprintf(gen, 0, "#include <stdlib.h>\n");
    indented_fprintf(gen, 0, "#include <string.h>\n");
    indented_fprintf(gen, 0, "#include <stdio.h>\n");
    indented_fprintf(gen, 0, "#include <stdbool.h>\n");
    indented_fprintf(gen, 0, "#include <limits.h>\n\n");
}

static void code_gen_externs(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_externs");

    /* Runtime arena operations - declare first since other functions use RtArena */
    indented_fprintf(gen, 0, "/* Runtime arena operations */\n");
    indented_fprintf(gen, 0, "typedef struct RtArena RtArena;\n");
    indented_fprintf(gen, 0, "extern RtArena *rt_arena_create(RtArena *parent);\n");
    indented_fprintf(gen, 0, "extern void rt_arena_destroy(RtArena *arena);\n");
    indented_fprintf(gen, 0, "extern void *rt_arena_alloc(RtArena *arena, size_t size);\n\n");

    /* Generic closure type for lambdas */
    indented_fprintf(gen, 0, "/* Closure type for lambdas */\n");
    indented_fprintf(gen, 0, "typedef struct __Closure__ { void *fn; RtArena *arena; } __Closure__;\n\n");

    indented_fprintf(gen, 0, "/* Runtime string operations */\n");
    indented_fprintf(gen, 0, "extern char *rt_str_concat(RtArena *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_str_length(const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_substring(RtArena *, const char *, long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_str_indexOf(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_str_split(RtArena *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_trim(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_toUpper(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_toLower(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_startsWith(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_endsWith(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_contains(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_replace(RtArena *, const char *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_str_charAt(const char *, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime print functions */\n");
    indented_fprintf(gen, 0, "extern void rt_print_long(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_double(double);\n");
    indented_fprintf(gen, 0, "extern void rt_print_char(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_string(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_bool(long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime type conversions */\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_long(RtArena *, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_double(RtArena *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_char(RtArena *, char);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_bool(RtArena *, int);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_string(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_void(RtArena *);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_pointer(RtArena *, void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime long arithmetic */\n");
    indented_fprintf(gen, 0, "extern long rt_add_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_sub_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_mul_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_div_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_mod_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_neg_long(long);\n");
    indented_fprintf(gen, 0, "extern long rt_eq_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_ne_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_lt_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_le_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_gt_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_ge_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_post_inc_long(long *);\n");
    indented_fprintf(gen, 0, "extern long rt_post_dec_long(long *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime double arithmetic */\n");
    indented_fprintf(gen, 0, "extern double rt_add_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_sub_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_mul_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_div_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_neg_double(double);\n");
    indented_fprintf(gen, 0, "extern long rt_eq_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_ne_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_lt_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_le_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_gt_double(double, double);\n");
    indented_fprintf(gen, 0, "extern long rt_ge_double(double, double);\n\n");

    indented_fprintf(gen, 0, "/* Runtime boolean and string comparisons */\n");
    indented_fprintf(gen, 0, "extern long rt_not_bool(long);\n");
    indented_fprintf(gen, 0, "extern long rt_eq_string(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_ne_string(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_lt_string(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_le_string(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_gt_string(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_ge_string(const char *, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array operations */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_push_long(RtArena *, long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_push_double(RtArena *, double *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_push_char(RtArena *, char *, char);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_push_string(RtArena *, char **, const char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_push_bool(RtArena *, int *, int);\n");
    indented_fprintf(gen, 0, "extern long rt_array_length(void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array print functions */\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_long(long *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_double(double *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_char(char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_bool(int *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_string(char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array clear */\n");
    indented_fprintf(gen, 0, "extern void rt_array_clear(void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array pop functions */\n");
    indented_fprintf(gen, 0, "extern long rt_array_pop_long(long *);\n");
    indented_fprintf(gen, 0, "extern double rt_array_pop_double(double *);\n");
    indented_fprintf(gen, 0, "extern char rt_array_pop_char(char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_pop_bool(int *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_pop_string(char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array concat functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_concat_long(RtArena *, long *, long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_concat_double(RtArena *, double *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_concat_char(RtArena *, char *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_concat_bool(RtArena *, int *, int *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_concat_string(RtArena *, char **, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array slice functions (start, end, step) */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_slice_long(RtArena *, long *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_slice_double(RtArena *, double *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_slice_char(RtArena *, char *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_slice_string(RtArena *, char **, long, long, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array reverse functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_rev_long(RtArena *, long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_rev_double(RtArena *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_rev_char(RtArena *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_rev_bool(RtArena *, int *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_rev_string(RtArena *, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array remove functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_rem_long(RtArena *, long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_rem_double(RtArena *, double *, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_rem_char(RtArena *, char *, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_rem_bool(RtArena *, int *, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_rem_string(RtArena *, char **, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array insert functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_ins_long(RtArena *, long *, long, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_ins_double(RtArena *, double *, double, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_ins_char(RtArena *, char *, char, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_ins_bool(RtArena *, int *, int, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_ins_string(RtArena *, char **, const char *, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array push (copy) functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_push_copy_long(RtArena *, long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_push_copy_double(RtArena *, double *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_push_copy_char(RtArena *, char *, char);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_push_copy_bool(RtArena *, int *, int);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_push_copy_string(RtArena *, char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array indexOf functions */\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_long(long *, long);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_double(double *, double);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_char(char *, char);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_bool(int *, int);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_string(char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array contains functions */\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_long(long *, long);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_double(double *, double);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_char(char *, char);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_bool(int *, int);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_string(char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array clone functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_clone_long(RtArena *, long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_clone_double(RtArena *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_clone_char(RtArena *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_clone_bool(RtArena *, int *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_clone_string(RtArena *, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array join functions */\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_long(RtArena *, long *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_double(RtArena *, double *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_char(RtArena *, char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_bool(RtArena *, int *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_string(RtArena *, char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array create from static data */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_create_long(RtArena *, size_t, const long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_create_double(RtArena *, size_t, const double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_create_char(RtArena *, size_t, const char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_create_bool(RtArena *, size_t, const int *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_create_string(RtArena *, size_t, const char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array equality functions */\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_long(long *, long *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_double(double *, double *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_char(char *, char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_bool(int *, int *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_string(char **, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime range creation */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_range(RtArena *, long, long);\n\n");
}

static void code_gen_forward_declaration(CodeGen *gen, FunctionStmt *fn)
{
    char *fn_name = get_var_name(gen->arena, fn->name);

    // Skip main - it doesn't need a forward declaration
    if (strcmp(fn_name, "main") == 0)
    {
        return;
    }

    bool is_shared = fn->modifier == FUNC_SHARED;
    const char *ret_c = get_c_type(gen->arena, fn->return_type);
    indented_fprintf(gen, 0, "%s %s(", ret_c, fn_name);

    // Shared functions receive caller's arena as first parameter
    if (is_shared)
    {
        fprintf(gen->output, "RtArena *");
        if (fn->param_count > 0)
        {
            fprintf(gen->output, ", ");
        }
    }

    for (int i = 0; i < fn->param_count; i++)
    {
        const char *param_type = get_c_type(gen->arena, fn->params[i].type);
        if (i > 0)
        {
            fprintf(gen->output, ", ");
        }
        fprintf(gen->output, "%s", param_type);
    }

    if (fn->param_count == 0 && !is_shared)
    {
        fprintf(gen->output, "void");
    }

    fprintf(gen->output, ");\n");
}

void code_gen_module(CodeGen *gen, Module *module)
{
    DEBUG_VERBOSE("Entering code_gen_module");
    code_gen_headers(gen);
    code_gen_externs(gen);

    // First pass: emit forward declarations for all user-defined functions
    indented_fprintf(gen, 0, "/* Forward declarations */\n");
    int forward_decl_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            char *fn_name = get_var_name(gen->arena, stmt->as.function.name);
            if (strcmp(fn_name, "main") != 0)
            {
                code_gen_forward_declaration(gen, &stmt->as.function);
                forward_decl_count++;
            }
        }
    }
    if (forward_decl_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Second pass: emit full function definitions to a temp file
    // This allows us to collect lambda forward declarations first
    FILE *original_output = gen->output;
    FILE *func_temp = tmpfile();
    if (func_temp == NULL)
    {
        fprintf(stderr, "Error: Failed to create temp file for function buffering\n");
        exit(1);
    }
    gen->output = func_temp;

    bool has_main = false;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION &&
            stmt->as.function.name.length == 4 &&
            strncmp(stmt->as.function.name.start, "main", 4) == 0)
        {
            has_main = true;
        }
        code_gen_statement(gen, stmt, 0);
    }

    if (!has_main)
    {
        // Generate main with arena lifecycle
        indented_fprintf(gen, 0, "int main() {\n");
        indented_fprintf(gen, 1, "RtArena *__arena_1__ = rt_arena_create(NULL);\n");
        indented_fprintf(gen, 1, "int _return_value = 0;\n");
        indented_fprintf(gen, 1, "goto main_return;\n");
        indented_fprintf(gen, 0, "main_return:\n");
        indented_fprintf(gen, 1, "rt_arena_destroy(__arena_1__);\n");
        indented_fprintf(gen, 1, "return _return_value;\n");
        indented_fprintf(gen, 0, "}\n");
    }

    // Restore original output
    gen->output = original_output;

    /* Output accumulated lambda forward declarations BEFORE function definitions */
    if (gen->lambda_forward_decls && strlen(gen->lambda_forward_decls) > 0)
    {
        indented_fprintf(gen, 0, "/* Lambda forward declarations */\n");
        fprintf(gen->output, "%s", gen->lambda_forward_decls);
        indented_fprintf(gen, 0, "\n");
    }

    /* Now copy the buffered function definitions */
    fseek(func_temp, 0, SEEK_END);
    long func_size = ftell(func_temp);
    if (func_size > 0)
    {
        fseek(func_temp, 0, SEEK_SET);
        char *func_buf = arena_alloc(gen->arena, func_size + 1);
        size_t read_size = fread(func_buf, 1, func_size, func_temp);
        func_buf[read_size] = '\0';
        fprintf(gen->output, "%s", func_buf);
    }
    fclose(func_temp);

    /* Output accumulated lambda function definitions at the end */
    if (gen->lambda_definitions && strlen(gen->lambda_definitions) > 0)
    {
        indented_fprintf(gen, 0, "\n/* Lambda function definitions */\n");
        fprintf(gen->output, "%s", gen->lambda_definitions);
    }
}
