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
    indented_fprintf(gen, 0, "/* Runtime string operations */\n");
    indented_fprintf(gen, 0, "extern char *rt_str_concat(char *, char *);\n");
    indented_fprintf(gen, 0, "extern void rt_free_string(char *);\n");
    indented_fprintf(gen, 0, "extern long rt_str_length(const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_substring(const char *, long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_str_indexOf(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_str_split(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_trim(const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_toUpper(const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_toLower(const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_startsWith(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_endsWith(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_contains(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_str_replace(const char *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern long rt_str_charAt(const char *, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime print functions */\n");
    indented_fprintf(gen, 0, "extern void rt_print_long(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_double(double);\n");
    indented_fprintf(gen, 0, "extern void rt_print_char(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_string(char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_bool(long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime type conversions */\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_long(long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_double(double);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_char(long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_bool(long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_string(char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_void(void);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_pointer(void *);\n\n");

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
    indented_fprintf(gen, 0, "extern long rt_eq_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_ne_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_lt_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_le_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_gt_string(char *, char *);\n");
    indented_fprintf(gen, 0, "extern long rt_ge_string(char *, char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array operations */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_push_long(long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_push_double(double *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_push_char(char *, char);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_push_string(char **, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_push_bool(int *, int);\n");
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
    indented_fprintf(gen, 0, "extern long *rt_array_concat_long(long *, long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_concat_double(double *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_concat_char(char *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_concat_bool(int *, int *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_concat_string(char **, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array free functions */\n");
    indented_fprintf(gen, 0, "extern void rt_array_free(void *);\n");
    indented_fprintf(gen, 0, "extern void rt_array_free_string(char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array slice functions (start, end, step) */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_slice_long(long *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_slice_double(double *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_slice_char(char *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_slice_bool(int *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_slice_string(char **, long, long, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array reverse functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_rev_long(long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_rev_double(double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_rev_char(char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_rev_bool(int *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_rev_string(char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array remove functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_rem_long(long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_rem_double(double *, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_rem_char(char *, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_rem_bool(int *, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_rem_string(char **, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array insert functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_ins_long(long *, long, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_ins_double(double *, double, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_ins_char(char *, char, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_ins_bool(int *, int, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_ins_string(char **, const char *, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array push (copy) functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_push_copy_long(long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_push_copy_double(double *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_push_copy_char(char *, char);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_push_copy_bool(int *, int);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_push_copy_string(char **, const char *);\n\n");

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
    indented_fprintf(gen, 0, "extern long *rt_array_clone_long(long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_clone_double(double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_clone_char(char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_clone_bool(int *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_clone_string(char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array join functions */\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_long(long *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_double(double *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_char(char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_bool(int *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_string(char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array create from static data */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_create_long(size_t, const long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_create_double(size_t, const double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_create_char(size_t, const char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_create_bool(size_t, const int *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_create_string(size_t, const char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array equality functions */\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_long(long *, long *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_double(double *, double *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_char(char *, char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_bool(int *, int *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_string(char **, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime range creation */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_range(long, long);\n\n");
}

static void code_gen_forward_declaration(CodeGen *gen, FunctionStmt *fn)
{
    char *fn_name = get_var_name(gen->arena, fn->name);

    // Skip main - it doesn't need a forward declaration
    if (strcmp(fn_name, "main") == 0)
    {
        return;
    }

    const char *ret_c = get_c_type(gen->arena, fn->return_type);
    indented_fprintf(gen, 0, "%s %s(", ret_c, fn_name);

    for (int i = 0; i < fn->param_count; i++)
    {
        const char *param_type = get_c_type(gen->arena, fn->params[i].type);
        if (i > 0)
        {
            fprintf(gen->output, ", ");
        }
        fprintf(gen->output, "%s", param_type);
    }

    if (fn->param_count == 0)
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

    // Second pass: emit full function definitions
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
        indented_fprintf(gen, 0, "int main() {\n");
        indented_fprintf(gen, 1, "return 0;\n");
        indented_fprintf(gen, 0, "}\n");
    }
}
