#ifndef GCC_BACKEND_H
#define GCC_BACKEND_H

#include <stdbool.h>

/* Check if GCC is available on the system.
 * Returns true if gcc is found and executable, false otherwise.
 * If verbose is true, prints diagnostic information about what was found.
 */
bool gcc_check_available(bool verbose);

/* Compile a C source file to an executable using GCC.
 *
 * Parameters:
 *   c_file        - Path to the C source file to compile
 *   output_exe    - Path for the output executable (if NULL, derives from c_file)
 *   compiler_dir  - Directory containing runtime objects (arena.o, debug.o, runtime.o)
 *   verbose       - If true, print the GCC command being executed
 *   debug_mode    - If true, include debug symbols and sanitizers
 *
 * Returns true on success, false on failure.
 */
bool gcc_compile(const char *c_file, const char *output_exe,
                 const char *compiler_dir, bool verbose, bool debug_mode);

/* Get the directory containing the compiler executable.
 * This is used to locate the runtime object files.
 * Returns a statically allocated string (do not free).
 */
const char *gcc_get_compiler_dir(const char *argv0);

#endif
