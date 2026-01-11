#ifndef RUNTIME_PROCESS_H
#define RUNTIME_PROCESS_H

#include "runtime_arena.h"
#include "runtime_string.h"

/* ============================================================================
 * Process Execution Types
 * ============================================================================
 * Sindarin provides the Process type for spawning and managing external
 * processes. Processes can run synchronously (blocking) or asynchronously
 * using & for spawning and ! for synchronization.
 * ============================================================================ */

/* Forward declaration */
typedef struct RtProcess RtProcess;

/* ============================================================================
 * Process Structure
 * ============================================================================
 * Represents a completed or pending process execution. Contains the exit code
 * and captured stdout/stderr streams.
 * ============================================================================ */

typedef struct RtProcess {
    int exit_code;          /* Process exit code (0 typically means success) */
    char *stdout_data;      /* Captured standard output */
    char *stderr_data;      /* Captured standard error */
} RtProcess;

/* ============================================================================
 * Process Function Declarations
 * ============================================================================ */

/* Run a command with no arguments.
 * Blocks until the process completes.
 * Returns a Process with exit_code, stdout_data, and stderr_data populated.
 * If the command is not found, exit_code is set to 127.
 */
RtProcess *rt_process_run(RtArena *arena, const char *cmd);

/* Run a command with arguments.
 * The args array contains command-line arguments (NULL for no arguments).
 * Blocks until the process completes.
 * Returns a Process with exit_code, stdout_data, and stderr_data populated.
 * If the command is not found, exit_code is set to 127.
 * Note: args may be NULL, in which case only the command is executed.
 */
RtProcess *rt_process_run_with_args(RtArena *arena, const char *cmd, char **args);

/* Exit the program with the specified exit code.
 * This is a wrapper around the C exit() function for Sindarin programs.
 */
void rt_exit(int code);

#endif /* RUNTIME_PROCESS_H */
