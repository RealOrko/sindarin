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
    indented_fprintf(gen, 0, "#include <stdbool.h>\n\n");
}

static void code_gen_externs(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_externs");
    indented_fprintf(gen, 0, "/* Runtime string operations */\n");
    indented_fprintf(gen, 0, "extern char *rt_str_concat(char *, char *);\n");
    indented_fprintf(gen, 0, "extern void rt_free_string(char *);\n\n");

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
}

void code_gen_module(CodeGen *gen, Module *module)
{
    DEBUG_VERBOSE("Entering code_gen_module");
    code_gen_headers(gen);
    code_gen_externs(gen);

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
