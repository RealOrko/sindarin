#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "runtime_process.h"
#include "runtime_array.h"

/* ============================================================================
 * Process Implementation
 * ============================================================================
 *
 * This module provides process execution for the Sindarin runtime.
 * Processes are spawned using fork/exec on POSIX systems and capture
 * stdout/stderr output along with the exit code.
 */

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Initial buffer size for reading from file descriptors */
#define READ_BUFFER_INITIAL_SIZE 4096

/* Read all data from a file descriptor until EOF.
 * Returns a null-terminated string allocated in the arena.
 * Closes the file descriptor after reading.
 * On error, returns an empty string.
 */
static char *read_fd_to_string(RtArena *arena, int fd)
{
    if (arena == NULL || fd < 0) {
        if (fd >= 0) {
            close(fd);
        }
        return rt_arena_strdup(arena, "");
    }

    /* Start with initial buffer size */
    size_t capacity = READ_BUFFER_INITIAL_SIZE;
    size_t length = 0;
    char *buffer = rt_arena_alloc(arena, capacity);
    if (buffer == NULL) {
        close(fd);
        return rt_arena_strdup(arena, "");
    }

    /* Read in a loop until EOF or error */
    while (1) {
        /* Ensure we have room for more data plus null terminator */
        if (length + 1 >= capacity) {
            /* Need to grow the buffer - allocate new larger buffer */
            size_t new_capacity = capacity * 2;
            char *new_buffer = rt_arena_alloc(arena, new_capacity);
            if (new_buffer == NULL) {
                /* Allocation failed, return what we have */
                buffer[length] = '\0';
                close(fd);
                return buffer;
            }
            memcpy(new_buffer, buffer, length);
            buffer = new_buffer;
            capacity = new_capacity;
        }

        /* Read into remaining buffer space */
        ssize_t bytes_read = read(fd, buffer + length, capacity - length - 1);

        if (bytes_read < 0) {
            /* Read error - return what we have so far */
            buffer[length] = '\0';
            close(fd);
            return buffer;
        }

        if (bytes_read == 0) {
            /* EOF reached */
            break;
        }

        length += (size_t)bytes_read;
    }

    /* Null-terminate the result */
    buffer[length] = '\0';

    /* Close the file descriptor */
    close(fd);

    return buffer;
}

/* Build argv array for execvp from command and optional args array.
 * First element is the command name.
 * Subsequent elements come from args array (if provided).
 * Array is NULL-terminated as required by execvp.
 * Returns malloc'd array (caller must free if needed, typically not needed
 * since child process will exec or exit).
 */
static char **build_argv(const char *cmd, char **args)
{
    /* Count the number of arguments */
    size_t argc = 1; /* Start with 1 for the command itself */
    if (args != NULL) {
        argc += rt_array_length(args);
    }

    /* Allocate argv array with space for NULL terminator */
    char **argv = malloc((argc + 1) * sizeof(char *));
    if (argv == NULL) {
        return NULL;
    }

    /* First element is the command */
    argv[0] = (char *)cmd;

    /* Copy arguments from args array */
    if (args != NULL) {
        size_t args_len = rt_array_length(args);
        for (size_t i = 0; i < args_len; i++) {
            argv[i + 1] = args[i];
        }
    }

    /* NULL-terminate the array as required by execvp */
    argv[argc] = NULL;

    return argv;
}

/* Helper function to create RtProcess with given values */
static RtProcess *rt_process_create(RtArena *arena, int exit_code,
                                     const char *stdout_str, const char *stderr_str)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_process_create: NULL arena\n");
        return NULL;
    }

    RtProcess *proc = rt_arena_alloc(arena, sizeof(RtProcess));
    if (proc == NULL) {
        fprintf(stderr, "rt_process_create: allocation failed\n");
        exit(1);
    }

    proc->exit_code = exit_code;
    proc->stdout_data = rt_arena_strdup(arena, stdout_str ? stdout_str : "");
    proc->stderr_data = rt_arena_strdup(arena, stderr_str ? stderr_str : "");

    return proc;
}

/* ============================================================================
 * Process Execution Functions
 * ============================================================================ */

/* Run a command with arguments.
 * Uses fork/exec to spawn child process, captures stdout and stderr, and waits for completion.
 * Returns RtProcess with exit code and captured stdout/stderr.
 */
RtProcess *rt_process_run_with_args(RtArena *arena, const char *cmd, char **args)
{
    if (arena == NULL) {
        fprintf(stderr, "rt_process_run_with_args: NULL arena\n");
        return NULL;
    }
    if (cmd == NULL) {
        fprintf(stderr, "rt_process_run_with_args: NULL command\n");
        return NULL;
    }

    /* Build argv array for execvp */
    char **argv = build_argv(cmd, args);
    if (argv == NULL) {
        fprintf(stderr, "rt_process_run_with_args: failed to build argv\n");
        return rt_process_create(arena, 127, "", "");
    }

    /* Create pipe for capturing stdout */
    int stdout_pipe[2];
    if (pipe(stdout_pipe) < 0) {
        free(argv);
        fprintf(stderr, "rt_process_run_with_args: stdout pipe failed\n");
        return rt_process_create(arena, 127, "", "");
    }

    /* Create pipe for capturing stderr */
    int stderr_pipe[2];
    if (pipe(stderr_pipe) < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        free(argv);
        fprintf(stderr, "rt_process_run_with_args: stderr pipe failed\n");
        return rt_process_create(arena, 127, "", "");
    }

    /* Fork the process */
    pid_t pid = fork();

    if (pid < 0) {
        /* Fork failed */
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        free(argv);
        fprintf(stderr, "rt_process_run_with_args: fork failed\n");
        return rt_process_create(arena, 127, "", "");
    }

    if (pid == 0) {
        /* Child process */

        /* Redirect stdout to pipe write end */
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0) {
            _exit(127);
        }

        /* Redirect stderr to pipe write end */
        if (dup2(stderr_pipe[1], STDERR_FILENO) < 0) {
            _exit(127);
        }

        /* Close all pipe file descriptors - no longer needed after dup2 */
        close(stdout_pipe[0]); /* Read end not needed in child */
        close(stdout_pipe[1]); /* Write end now duplicated to stdout */
        close(stderr_pipe[0]); /* Read end not needed in child */
        close(stderr_pipe[1]); /* Write end now duplicated to stderr */

        /* Execute the command */
        execvp(cmd, argv);

        /* If execvp returns, it failed - write error message and exit with 127 */
        /* Use dprintf to write directly to stderr fd (already redirected to pipe) */
        dprintf(STDERR_FILENO, "%s: command not found\n", cmd);
        _exit(127);
    }

    /* Parent process */
    free(argv); /* No longer needed in parent */

    /* Close write ends of pipes (we only read from them) */
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    /* Read stdout from child */
    char *stdout_data = read_fd_to_string(arena, stdout_pipe[0]);
    /* Note: read_fd_to_string closes stdout_pipe[0] */

    /* Read stderr from child */
    char *stderr_data = read_fd_to_string(arena, stderr_pipe[0]);
    /* Note: read_fd_to_string closes stderr_pipe[0] */

    /* Wait for child to complete */
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        /* waitpid failed */
        fprintf(stderr, "rt_process_run_with_args: waitpid failed\n");
        return rt_process_create(arena, 127, stdout_data, stderr_data);
    }

    /* Extract exit code from status */
    int exit_code;
    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        /* Killed by signal - use 128 + signal number as exit code */
        exit_code = 128 + WTERMSIG(status);
    } else {
        /* Unknown status */
        exit_code = 127;
    }

    return rt_process_create(arena, exit_code, stdout_data, stderr_data);
}

/* Run a command with no arguments.
 * Delegates to rt_process_run_with_args with NULL args.
 */
RtProcess *rt_process_run(RtArena *arena, const char *cmd)
{
    return rt_process_run_with_args(arena, cmd, NULL);
}
