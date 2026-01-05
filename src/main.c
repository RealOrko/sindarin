#include "compiler.h"
#include "diagnostic.h"
#include "debug.h"
#include "type_checker.h"
#include "gcc_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

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

    /* Phase 3: Code generation */
    diagnostic_phase_start(PHASE_CODE_GEN);

    CodeGen gen;
    code_gen_init(&options.arena, &gen, &options.symbol_table, options.output_file);
    gen.arithmetic_mode = options.arithmetic_mode;
    code_gen_module(&gen, module);

    /* Copy link libraries from CodeGen to options for GCC backend */
    options.link_libs = gen.pragma_links;
    options.link_lib_count = gen.pragma_link_count;

    code_gen_cleanup(&gen);

    diagnostic_phase_done(PHASE_CODE_GEN, 0);

    /* If --emit-c mode, we're done */
    if (options.emit_c_only)
    {
        /* Get file size for the success message */
        struct stat st;
        long file_size = 0;
        if (stat(options.output_file, &st) == 0)
        {
            file_size = st.st_size;
        }
        diagnostic_compile_success(options.output_file, file_size, 0);
        compiler_cleanup(&options);
        return 0;
    }

    /* Phase 4: Linking (GCC compilation) */
    diagnostic_phase_start(PHASE_LINKING);

    if (!gcc_compile(options.output_file, options.executable_file,
                     options.compiler_dir, options.verbose, options.debug_build,
                     options.link_libs, options.link_lib_count))
    {
        diagnostic_phase_failed(PHASE_LINKING);
        diagnostic_compile_failed();
        result = 1;
    }
    else
    {
        diagnostic_phase_done(PHASE_LINKING, 0);

        /* Get file size for the success message */
        struct stat st;
        long file_size = 0;
        if (stat(options.executable_file, &st) == 0)
        {
            file_size = st.st_size;
        }
        diagnostic_compile_success(options.executable_file, file_size, 0);
    }

    /* Delete intermediate C file unless --keep-c was specified */
    if (!options.keep_c && result == 0)
    {
        if (unlink(options.output_file) != 0)
        {
            /* Non-fatal: just warn if we couldn't delete */
            DEBUG_WARNING("Could not remove intermediate C file: %s", options.output_file);
        }
    }

    compiler_cleanup(&options);

    return result;
}
