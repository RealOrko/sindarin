#include "compiler.h"
#include "debug.h"
#include "type_checker.h"
#include "optimizer.h"
#include <stdlib.h>
#include <string.h>

void compiler_init(CompilerOptions *options, int argc, char **argv)
{
    if (!options)
    {
        DEBUG_ERROR("CompilerOptions is NULL");
        exit(1);
    }

    arena_init(&options->arena, 4096);
    options->source_file = NULL;
    options->output_file = NULL;
    options->source = NULL;
    options->verbose = 0;
    options->log_level = DEBUG_LEVEL_ERROR;
    options->arithmetic_mode = ARITH_CHECKED;  /* Default to checked arithmetic */
    options->optimization_level = OPT_LEVEL_FULL;  /* Default to full optimization (-O2) */

    if (!compiler_parse_args(argc, argv, options))
    {
        compiler_cleanup(options);
        exit(1);
    }

    symbol_table_init(&options->arena, &options->symbol_table);
}

void compiler_cleanup(CompilerOptions *options)
{
    if (!options)
    {
        return;
    }

    symbol_table_cleanup(&options->symbol_table);
    arena_free(&options->arena);

    options->source_file = NULL;
    options->output_file = NULL;
    options->source = NULL;
}

int compiler_parse_args(int argc, char **argv, CompilerOptions *options)
{
    if (argc < 2)
    {
        fprintf(stderr,
            "Usage: %s <source_file> [-o <output_file>] [-v] [-l <level>] [--unchecked] [-O<level>]\n"
            "  -o <output_file>   Specify output file (default is source_file.s)\n"
            "  -v                 Verbose mode\n"
            "  -l <level>         Set log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)\n"
            "  --unchecked        Use unchecked arithmetic (no overflow checking, faster)\n"
            "\n"
            "Optimization levels:\n"
            "  -O0                No optimization (for debugging)\n"
            "  -O1                Basic optimizations (dead code elimination, string merging)\n"
            "  -O2                Full optimizations (default: + tail call optimization)\n",
            argv[0]);
        return 0;
    }

    // First pass: parse options to set log level early
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
        {
            i++;
            int log_level = atoi(argv[i]);
            if (log_level < DEBUG_LEVEL_NONE || log_level > DEBUG_LEVEL_VERBOSE)
            {
                fprintf(stderr, "Invalid log level: %s (must be 0-4)\n", argv[i]);
                return 0;
            }
            options->log_level = log_level;
            init_debug(log_level);
        }
    }

    // Second pass: parse all arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            i++;
            options->output_file = arena_strdup(&options->arena, argv[i]);
            if (!options->output_file)
            {
                DEBUG_ERROR("Failed to allocate memory for output file path");
                return 0;
            }
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            options->verbose = 1;
        }
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
        {
            i++; // Skip the level value, already processed
        }
        else if (strcmp(argv[i], "--unchecked") == 0)
        {
            options->arithmetic_mode = ARITH_UNCHECKED;
        }
        else if (strcmp(argv[i], "-O0") == 0)
        {
            options->optimization_level = OPT_LEVEL_NONE;
        }
        else if (strcmp(argv[i], "-O1") == 0)
        {
            options->optimization_level = OPT_LEVEL_BASIC;
        }
        else if (strcmp(argv[i], "-O2") == 0)
        {
            options->optimization_level = OPT_LEVEL_FULL;
        }
        else if (strcmp(argv[i], "--no-opt") == 0)
        {
            /* Legacy flag - equivalent to -O0 */
            options->optimization_level = OPT_LEVEL_NONE;
        }
        else if (argv[i][0] == '-')
        {
            DEBUG_ERROR("Unknown option: %s", argv[i]);
            return 0;
        }
        else
        {
            // This is the source file
            if (options->source_file != NULL)
            {
                DEBUG_ERROR("Multiple source files specified: %s and %s", options->source_file, argv[i]);
                return 0;
            }
            options->source_file = arena_strdup(&options->arena, argv[i]);
            if (!options->source_file)
            {
                DEBUG_ERROR("Failed to allocate memory for source file path");
                return 0;
            }
        }
    }

    // Check that source file was provided
    if (options->source_file == NULL)
    {
        fprintf(stderr, "Error: No source file specified\n");
        return 0;
    }

    // Generate default output file name if not specified
    if (options->output_file == NULL)
    {
        const char *dot = strrchr(options->source_file, '.');
        size_t base_len = dot ? (size_t)(dot - options->source_file) : strlen(options->source_file);
        size_t out_len = base_len + 3; // ".s" + null terminator
        char *out = arena_alloc(&options->arena, out_len);
        if (!out)
        {
            DEBUG_ERROR("Failed to allocate memory for output file path");
            return 0;
        }
        strncpy(out, options->source_file, base_len);
        strcpy(out + base_len, ".s");
        options->output_file = out;
    }

    return 1;
}

Module* compiler_compile(CompilerOptions *options)
{
    char **imported = NULL;
    int imported_count = 0;
    int imported_capacity = 0;

    Module *module = parse_module_with_imports(&options->arena, &options->symbol_table, options->source_file, &imported, &imported_count, &imported_capacity);
    if (!module)
    {
        DEBUG_ERROR("Failed to parse module with imports");
        return NULL;
    }

    if (!type_check_module(module, &options->symbol_table))
    {
        DEBUG_ERROR("Type checking failed");
        return NULL;
    }

    /* Run optimization passes based on optimization level */
    if (options->optimization_level >= OPT_LEVEL_BASIC)
    {
        Optimizer opt;
        optimizer_init(&opt, &options->arena);

        /* -O1 and above: Dead code elimination */
        optimizer_dead_code_elimination(&opt, module);

        /* -O1 and above: String literal merging */
        optimizer_merge_string_literals(&opt, module);

        /* -O2 only: Tail call optimization */
        if (options->optimization_level >= OPT_LEVEL_FULL)
        {
            optimizer_tail_call_optimization(&opt, module);
        }

        int stmts_removed, vars_removed, noops_removed;
        optimizer_get_stats(&opt, &stmts_removed, &vars_removed, &noops_removed);

        if (options->verbose)
        {
            DEBUG_INFO("Optimization level: -O%d", options->optimization_level);
            if (stmts_removed > 0 || vars_removed > 0 || noops_removed > 0)
            {
                DEBUG_INFO("Optimizer: removed %d unreachable statements, %d unused variables, %d no-ops",
                           stmts_removed, vars_removed, noops_removed);
            }
            if (options->optimization_level >= OPT_LEVEL_FULL && opt.tail_calls_optimized > 0)
            {
                DEBUG_INFO("Optimizer: marked %d tail calls for optimization",
                           opt.tail_calls_optimized);
            }
            if (opt.string_literals_merged > 0)
            {
                DEBUG_INFO("Optimizer: merged %d adjacent string literals",
                           opt.string_literals_merged);
            }
        }
    }
    else if (options->verbose)
    {
        DEBUG_INFO("Optimization disabled (-O0)");
    }

    return module;
}