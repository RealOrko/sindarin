#ifndef COMPILER_H
#define COMPILER_H

#include "arena.h"
#include "file.h"
#include "lexer.h"
#include "parser.h"
#include "code_gen.h"
#include <stdio.h>
#include <stdbool.h>

/* Optimization levels:
 * -O0: No optimization (for debugging, generates simpler code)
 * -O1: Basic optimizations (dead code elimination, string literal merging)
 * -O2: Full optimizations (+ tail call optimization, constant folding)
 */
#define OPT_LEVEL_NONE  0  /* -O0: No optimization */
#define OPT_LEVEL_BASIC 1  /* -O1: Basic optimizations */
#define OPT_LEVEL_FULL  2  /* -O2: Full optimizations (default) */

typedef struct
{
    Arena arena;
    SymbolTable symbol_table;
    char *source_file;
    char *output_file;
    char *source;
    int verbose;
    int log_level;
    ArithmeticMode arithmetic_mode;  /* Checked or unchecked arithmetic */
    int optimization_level;          /* Optimization level (0, 1, or 2) */
} CompilerOptions;

void compiler_init(CompilerOptions *options, int argc, char **argv);
void compiler_cleanup(CompilerOptions *options);
int compiler_parse_args(int argc, char **argv, CompilerOptions *options);
Module* compiler_compile(CompilerOptions *options);

#endif