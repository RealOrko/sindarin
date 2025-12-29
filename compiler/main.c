#include "compiler.h"
#include "debug.h"
#include "type_checker.h"
#include "gcc_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    CompilerOptions options;
    Module *module = NULL;
    int result = 0;

    compiler_init(&options, argc, argv);
    init_debug(options.log_level);

    /* Check for GCC availability early (unless --emit-c mode) */
    if (!options.emit_c_only)
    {
        if (!gcc_check_available(options.verbose))
        {
            compiler_cleanup(&options);
            return 1;
        }
    }

    /* Compile Sn source to AST */
    module = compiler_compile(&options);

    if (module == NULL) {
        compiler_cleanup(&options);
        return 1;
    }

    /* Generate C code */
    if (options.verbose)
    {
        DEBUG_INFO("Generating C code: %s", options.output_file);
    }

    CodeGen gen;
    code_gen_init(&options.arena, &gen, &options.symbol_table, options.output_file);
    gen.arithmetic_mode = options.arithmetic_mode;
    code_gen_module(&gen, module);
    code_gen_cleanup(&gen);

    /* If --emit-c mode, we're done */
    if (options.emit_c_only)
    {
        if (options.verbose)
        {
            DEBUG_INFO("C code written to: %s", options.output_file);
        }
        compiler_cleanup(&options);
        return 0;
    }

    /* Compile C to executable using GCC */
    if (options.verbose)
    {
        DEBUG_INFO("Compiling to executable: %s", options.executable_file);
    }

    if (!gcc_compile(options.output_file, options.executable_file,
                     options.compiler_dir, options.verbose, options.debug_build))
    {
        fprintf(stderr, "Error: Failed to compile to executable\n");
        result = 1;
    }

    /* Delete intermediate C file unless --keep-c was specified */
    if (!options.keep_c && result == 0)
    {
        if (options.verbose)
        {
            DEBUG_INFO("Removing intermediate C file: %s", options.output_file);
        }
        if (unlink(options.output_file) != 0)
        {
            /* Non-fatal: just warn if we couldn't delete */
            DEBUG_WARNING("Could not remove intermediate C file: %s", options.output_file);
        }
    }
    else if (options.keep_c && options.verbose)
    {
        DEBUG_INFO("Keeping intermediate C file: %s", options.output_file);
    }

    compiler_cleanup(&options);

    return result;
}
