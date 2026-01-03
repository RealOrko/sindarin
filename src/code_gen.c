#include "code_gen.h"
#include "code_gen/code_gen_util.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_stmt.h"
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

    /* Initialize loop arena stack for nested loops */
    gen->loop_arena_stack = NULL;
    gen->loop_cleanup_stack = NULL;
    gen->loop_arena_depth = 0;
    gen->loop_arena_capacity = 0;

    /* Initialize loop counter tracking for optimization */
    gen->loop_counter_names = NULL;
    gen->loop_counter_count = 0;
    gen->loop_counter_capacity = 0;

    /* Initialize private block arena stack */
    gen->arena_stack = NULL;
    gen->arena_stack_depth = 0;
    gen->arena_stack_capacity = 0;

    /* Initialize lambda fields */
    gen->lambda_count = 0;
    gen->lambda_forward_decls = arena_strdup(arena, "");
    gen->lambda_definitions = arena_strdup(arena, "");
    gen->enclosing_lambdas = NULL;
    gen->enclosing_lambda_count = 0;
    gen->enclosing_lambda_capacity = 0;

    /* Initialize thread wrapper support */
    gen->thread_wrapper_count = 0;

    /* Initialize buffering fields */
    gen->function_definitions = arena_strdup(arena, "");
    gen->buffering_functions = false;

    /* Initialize optimization settings */
    gen->arithmetic_mode = ARITH_CHECKED;  /* Default to checked arithmetic */

    /* Initialize tail call optimization state */
    gen->in_tail_call_function = false;
    gen->tail_call_fn = NULL;

    /* Initialize captured primitive tracking */
    gen->captured_primitives = NULL;
    gen->captured_prim_ptrs = NULL;
    gen->captured_prim_count = 0;
    gen->captured_prim_capacity = 0;

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
    indented_fprintf(gen, 0, "#include <limits.h>\n");
    indented_fprintf(gen, 0, "#include <setjmp.h>\n");  /* For thread panic handling */
    /* Include runtime.h for inline function definitions (comparisons, array_length, etc.) */
    indented_fprintf(gen, 0, "#include \"runtime.h\"\n\n");
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
    indented_fprintf(gen, 0, "extern void rt_print_bool(long);\n");
    indented_fprintf(gen, 0, "extern void rt_print_byte(unsigned char);\n\n");

    indented_fprintf(gen, 0, "/* Runtime type conversions */\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_long(RtArena *, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_double(RtArena *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_char(RtArena *, char);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_bool(RtArena *, int);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_byte(RtArena *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_string(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_void(RtArena *);\n");
    indented_fprintf(gen, 0, "extern char *rt_to_string_pointer(RtArena *, void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime format specifier functions */\n");
    indented_fprintf(gen, 0, "extern char *rt_format_long(RtArena *, long, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_format_double(RtArena *, double, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_format_string(RtArena *, const char *, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime long arithmetic (comparisons are static inline in runtime.h) */\n");
    indented_fprintf(gen, 0, "extern long rt_add_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_sub_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_mul_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_div_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_mod_long(long, long);\n");
    indented_fprintf(gen, 0, "extern long rt_neg_long(long);\n");
    /* rt_eq_long, rt_ne_long, etc. are static inline in runtime.h */
    indented_fprintf(gen, 0, "extern long rt_post_inc_long(long *);\n");
    indented_fprintf(gen, 0, "extern long rt_post_dec_long(long *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime double arithmetic (comparisons are static inline in runtime.h) */\n");
    indented_fprintf(gen, 0, "extern double rt_add_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_sub_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_mul_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_div_double(double, double);\n");
    indented_fprintf(gen, 0, "extern double rt_neg_double(double);\n\n");
    /* rt_eq_double, rt_ne_double, etc. are static inline in runtime.h */

    /* rt_not_bool, rt_eq_string, etc. are declared in runtime.h */

    indented_fprintf(gen, 0, "/* Runtime array operations */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_push_long(RtArena *, long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_push_double(RtArena *, double *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_push_char(RtArena *, char *, char);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_push_string(RtArena *, char **, const char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_push_bool(RtArena *, int *, int);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_push_byte(RtArena *, unsigned char *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern void **rt_array_push_ptr(RtArena *, void **, void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array print functions */\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_long(long *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_double(double *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_char(char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_bool(int *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_byte(unsigned char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_array_string(char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array clear */\n");
    indented_fprintf(gen, 0, "extern void rt_array_clear(void *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array pop functions */\n");
    indented_fprintf(gen, 0, "extern long rt_array_pop_long(long *);\n");
    indented_fprintf(gen, 0, "extern double rt_array_pop_double(double *);\n");
    indented_fprintf(gen, 0, "extern char rt_array_pop_char(char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_pop_bool(int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char rt_array_pop_byte(unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_pop_string(char **);\n");
    indented_fprintf(gen, 0, "extern void *rt_array_pop_ptr(void **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array concat functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_concat_long(RtArena *, long *, long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_concat_double(RtArena *, double *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_concat_char(RtArena *, char *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_concat_bool(RtArena *, int *, int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_concat_byte(RtArena *, unsigned char *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_concat_string(RtArena *, char **, char **);\n");
    indented_fprintf(gen, 0, "extern void **rt_array_concat_ptr(RtArena *, void **, void **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array slice functions (start, end, step) */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_slice_long(RtArena *, long *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_slice_double(RtArena *, double *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_slice_char(RtArena *, char *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_slice_bool(RtArena *, int *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_slice_byte(RtArena *, unsigned char *, long, long, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_slice_string(RtArena *, char **, long, long, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array reverse functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_rev_long(RtArena *, long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_rev_double(RtArena *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_rev_char(RtArena *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_rev_bool(RtArena *, int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_rev_byte(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_rev_string(RtArena *, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array remove functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_rem_long(RtArena *, long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_rem_double(RtArena *, double *, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_rem_char(RtArena *, char *, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_rem_bool(RtArena *, int *, long);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_rem_byte(RtArena *, unsigned char *, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_rem_string(RtArena *, char **, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array insert functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_ins_long(RtArena *, long *, long, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_ins_double(RtArena *, double *, double, long);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_ins_char(RtArena *, char *, char, long);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_ins_bool(RtArena *, int *, int, long);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_ins_byte(RtArena *, unsigned char *, unsigned char, long);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_ins_string(RtArena *, char **, const char *, long);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array push (copy) functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_push_copy_long(RtArena *, long *, long);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_push_copy_double(RtArena *, double *, double);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_push_copy_char(RtArena *, char *, char);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_push_copy_bool(RtArena *, int *, int);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_push_copy_byte(RtArena *, unsigned char *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_push_copy_string(RtArena *, char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array indexOf functions */\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_long(long *, long);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_double(double *, double);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_char(char *, char);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_bool(int *, int);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_byte(unsigned char *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern long rt_array_indexOf_string(char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array contains functions */\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_long(long *, long);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_double(double *, double);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_char(char *, char);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_bool(int *, int);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_byte(unsigned char *, unsigned char);\n");
    indented_fprintf(gen, 0, "extern int rt_array_contains_string(char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array clone functions */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_clone_long(RtArena *, long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_clone_double(RtArena *, double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_clone_char(RtArena *, char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_clone_bool(RtArena *, int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_clone_byte(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_clone_string(RtArena *, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array join functions */\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_long(RtArena *, long *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_double(RtArena *, double *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_char(RtArena *, char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_bool(RtArena *, int *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_byte(RtArena *, unsigned char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_join_string(RtArena *, char **, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array create from static data */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_create_long(RtArena *, size_t, const long *);\n");
    indented_fprintf(gen, 0, "extern double *rt_array_create_double(RtArena *, size_t, const double *);\n");
    indented_fprintf(gen, 0, "extern char *rt_array_create_char(RtArena *, size_t, const char *);\n");
    indented_fprintf(gen, 0, "extern int *rt_array_create_bool(RtArena *, size_t, const int *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_array_create_byte(RtArena *, size_t, const unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_array_create_string(RtArena *, size_t, const char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime array equality functions */\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_long(long *, long *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_double(double *, double *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_char(char *, char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_bool(int *, int *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_byte(unsigned char *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern int rt_array_eq_string(char **, char **);\n\n");

    indented_fprintf(gen, 0, "/* Runtime range creation */\n");
    indented_fprintf(gen, 0, "extern long *rt_array_range(RtArena *, long, long);\n\n");

    indented_fprintf(gen, 0, "/* TextFile static methods */\n");
    indented_fprintf(gen, 0, "typedef struct RtTextFile RtTextFile;\n");
    indented_fprintf(gen, 0, "extern RtTextFile *rt_text_file_open(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_text_file_exists(const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_text_file_read_all(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_write_all(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_delete(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_copy(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_move(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_close(RtTextFile *);\n\n");

    indented_fprintf(gen, 0, "/* TextFile instance reading methods */\n");
    indented_fprintf(gen, 0, "extern long rt_text_file_read_char(RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern char *rt_text_file_read_word(RtArena *, RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern char *rt_text_file_read_line(RtArena *, RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern char *rt_text_file_instance_read_all(RtArena *, RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern char **rt_text_file_read_lines(RtArena *, RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern long rt_text_file_read_into(RtTextFile *, char *);\n\n");

    indented_fprintf(gen, 0, "/* TextFile instance writing methods */\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_write_char(RtTextFile *, long);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_write(RtTextFile *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_write_line(RtTextFile *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_print(RtTextFile *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_println(RtTextFile *, const char *);\n\n");

    indented_fprintf(gen, 0, "/* TextFile state methods */\n");
    indented_fprintf(gen, 0, "extern int rt_text_file_has_chars(RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern int rt_text_file_has_words(RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern int rt_text_file_has_lines(RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern int rt_text_file_is_eof(RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern long rt_text_file_position(RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_seek(RtTextFile *, long);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_rewind(RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern void rt_text_file_flush(RtTextFile *);\n\n");

    indented_fprintf(gen, 0, "/* TextFile properties */\n");
    indented_fprintf(gen, 0, "extern char *rt_text_file_get_path(RtArena *, RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern char *rt_text_file_get_name(RtArena *, RtTextFile *);\n");
    indented_fprintf(gen, 0, "extern long rt_text_file_get_size(RtTextFile *);\n\n");

    indented_fprintf(gen, 0, "/* BinaryFile static methods */\n");
    indented_fprintf(gen, 0, "typedef struct RtBinaryFile RtBinaryFile;\n");
    indented_fprintf(gen, 0, "extern RtBinaryFile *rt_binary_file_open(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_binary_file_exists(const char *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_binary_file_read_all(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_write_all(const char *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_delete(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_copy(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_move(const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_close(RtBinaryFile *);\n\n");

    indented_fprintf(gen, 0, "/* BinaryFile instance reading methods */\n");
    indented_fprintf(gen, 0, "extern long rt_binary_file_read_byte(RtBinaryFile *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_binary_file_read_bytes(RtArena *, RtBinaryFile *, long);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_binary_file_instance_read_all(RtArena *, RtBinaryFile *);\n");
    indented_fprintf(gen, 0, "extern long rt_binary_file_read_into(RtBinaryFile *, unsigned char *);\n\n");

    indented_fprintf(gen, 0, "/* BinaryFile instance writing methods */\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_write_byte(RtBinaryFile *, long);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_write_bytes(RtBinaryFile *, unsigned char *);\n\n");

    indented_fprintf(gen, 0, "/* BinaryFile state methods */\n");
    indented_fprintf(gen, 0, "extern int rt_binary_file_has_bytes(RtBinaryFile *);\n");
    indented_fprintf(gen, 0, "extern int rt_binary_file_is_eof(RtBinaryFile *);\n");
    indented_fprintf(gen, 0, "extern long rt_binary_file_position(RtBinaryFile *);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_seek(RtBinaryFile *, long);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_rewind(RtBinaryFile *);\n");
    indented_fprintf(gen, 0, "extern void rt_binary_file_flush(RtBinaryFile *);\n\n");

    indented_fprintf(gen, 0, "/* BinaryFile properties */\n");
    indented_fprintf(gen, 0, "extern char *rt_binary_file_get_path(RtArena *, RtBinaryFile *);\n");
    indented_fprintf(gen, 0, "extern char *rt_binary_file_get_name(RtArena *, RtBinaryFile *);\n");
    indented_fprintf(gen, 0, "extern long rt_binary_file_get_size(RtBinaryFile *);\n\n");

    indented_fprintf(gen, 0, "/* Standard streams (Stdin, Stdout, Stderr) */\n");
    indented_fprintf(gen, 0, "extern char *rt_stdin_read_line(RtArena *);\n");
    indented_fprintf(gen, 0, "extern long rt_stdin_read_char(void);\n");
    indented_fprintf(gen, 0, "extern char *rt_stdin_read_word(RtArena *);\n");
    indented_fprintf(gen, 0, "extern int rt_stdin_has_chars(void);\n");
    indented_fprintf(gen, 0, "extern int rt_stdin_has_lines(void);\n");
    indented_fprintf(gen, 0, "extern int rt_stdin_is_eof(void);\n");
    indented_fprintf(gen, 0, "extern void rt_stdout_write(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_stdout_write_line(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_stdout_flush(void);\n");
    indented_fprintf(gen, 0, "extern void rt_stderr_write(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_stderr_write_line(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_stderr_flush(void);\n\n");

    indented_fprintf(gen, 0, "/* Global convenience functions */\n");
    indented_fprintf(gen, 0, "extern char *rt_read_line(RtArena *);\n");
    indented_fprintf(gen, 0, "extern void rt_println(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_err(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_print_err_ln(const char *);\n\n");

    indented_fprintf(gen, 0, "/* Byte array extension methods */\n");
    indented_fprintf(gen, 0, "extern char *rt_byte_array_to_string(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_byte_array_to_string_latin1(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_byte_array_to_hex(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_byte_array_to_base64(RtArena *, unsigned char *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_string_to_bytes(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_bytes_from_hex(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern unsigned char *rt_bytes_from_base64(RtArena *, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Path utilities */\n");
    indented_fprintf(gen, 0, "extern char *rt_path_directory(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_path_filename(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_path_extension(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_path_join2(RtArena *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_path_join3(RtArena *, const char *, const char *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_path_absolute(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_path_exists(const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_path_is_file(const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_path_is_directory(const char *);\n\n");

    indented_fprintf(gen, 0, "/* Directory operations */\n");
    indented_fprintf(gen, 0, "extern char **rt_directory_list(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_directory_list_recursive(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_directory_create(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_directory_delete(const char *);\n");
    indented_fprintf(gen, 0, "extern void rt_directory_delete_recursive(const char *);\n\n");

    indented_fprintf(gen, 0, "/* String splitting methods */\n");
    indented_fprintf(gen, 0, "extern char **rt_str_split_whitespace(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char **rt_str_split_lines(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern int rt_str_is_blank(const char *);\n\n");

    indented_fprintf(gen, 0, "/* Mutable string operations */\n");
    indented_fprintf(gen, 0, "extern char *rt_string_with_capacity(RtArena *, size_t);\n");
    indented_fprintf(gen, 0, "extern char *rt_string_from(RtArena *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_string_ensure_mutable(RtArena *, char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_string_append(char *, const char *);\n\n");

    indented_fprintf(gen, 0, "/* Time type and operations */\n");
    indented_fprintf(gen, 0, "typedef struct RtTime RtTime;\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_now(RtArena *);\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_utc(RtArena *);\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_from_millis(RtArena *, long long);\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_from_seconds(RtArena *, long long);\n");
    indented_fprintf(gen, 0, "extern void rt_time_sleep(long);\n");
    indented_fprintf(gen, 0, "extern long long rt_time_get_millis(RtTime *);\n");
    indented_fprintf(gen, 0, "extern long long rt_time_get_seconds(RtTime *);\n");
    indented_fprintf(gen, 0, "extern long rt_time_get_year(RtTime *);\n");
    indented_fprintf(gen, 0, "extern long rt_time_get_month(RtTime *);\n");
    indented_fprintf(gen, 0, "extern long rt_time_get_day(RtTime *);\n");
    indented_fprintf(gen, 0, "extern long rt_time_get_hour(RtTime *);\n");
    indented_fprintf(gen, 0, "extern long rt_time_get_minute(RtTime *);\n");
    indented_fprintf(gen, 0, "extern long rt_time_get_second(RtTime *);\n");
    indented_fprintf(gen, 0, "extern long rt_time_get_weekday(RtTime *);\n");
    indented_fprintf(gen, 0, "extern char *rt_time_format(RtArena *, RtTime *, const char *);\n");
    indented_fprintf(gen, 0, "extern char *rt_time_to_iso(RtArena *, RtTime *);\n");
    indented_fprintf(gen, 0, "extern char *rt_time_to_date(RtArena *, RtTime *);\n");
    indented_fprintf(gen, 0, "extern char *rt_time_to_time(RtArena *, RtTime *);\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_add(RtArena *, RtTime *, long long);\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_add_seconds(RtArena *, RtTime *, long);\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_add_minutes(RtArena *, RtTime *, long);\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_add_hours(RtArena *, RtTime *, long);\n");
    indented_fprintf(gen, 0, "extern RtTime *rt_time_add_days(RtArena *, RtTime *, long);\n");
    indented_fprintf(gen, 0, "extern long long rt_time_diff(RtTime *, RtTime *);\n");
    indented_fprintf(gen, 0, "extern int rt_time_is_before(RtTime *, RtTime *);\n");
    indented_fprintf(gen, 0, "extern int rt_time_is_after(RtTime *, RtTime *);\n");
    indented_fprintf(gen, 0, "extern int rt_time_equals(RtTime *, RtTime *);\n\n");
}

static void code_gen_forward_declaration(CodeGen *gen, FunctionStmt *fn)
{
    char *fn_name = get_var_name(gen->arena, fn->name);

    // Skip main - it doesn't need a forward declaration
    if (strcmp(fn_name, "main") == 0)
    {
        return;
    }

    char *fn_name_str = fn_name;  // Already computed above
    bool is_main = strcmp(fn_name_str, "main") == 0;
    bool is_shared = fn->modifier == FUNC_SHARED;
    /* Functions returning heap-allocated types (closures, strings, arrays) must be
     * implicitly shared to avoid arena lifetime issues - the returned value must
     * live in caller's arena, not the function's arena which is destroyed on return */
    bool returns_heap_type = fn->return_type && (
        fn->return_type->kind == TYPE_FUNCTION ||
        fn->return_type->kind == TYPE_STRING ||
        fn->return_type->kind == TYPE_ARRAY);
    if (returns_heap_type && !is_main)
    {
        is_shared = true;
    }
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
        /* 'as ref' primitive parameters become pointer types */
        bool is_ref_primitive = false;
        if (fn->params[i].mem_qualifier == MEM_AS_REF && fn->params[i].type != NULL)
        {
            TypeKind kind = fn->params[i].type->kind;
            is_ref_primitive = (kind == TYPE_INT || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                               kind == TYPE_CHAR || kind == TYPE_BOOL || kind == TYPE_BYTE);
        }
        if (is_ref_primitive)
        {
            fprintf(gen->output, "%s *", param_type);
        }
        else
        {
            fprintf(gen->output, "%s", param_type);
        }
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
