#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #if defined(__MINGW32__) || defined(__MINGW64__)
    /* MinGW provides Windows headers but not fork/wait (Windows uses CreateProcess) */
    #include <windows.h>
    #else
    #include "../platform/compat_windows.h"
    #endif
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "runtime_process.h"
#include "runtime_array.h"

/* ============================================================================
 * Process Implementation
 * ============================================================================
 *
 * This module provides process execution for the Sindarin runtime.
 * On POSIX systems, processes are spawned using fork/exec.
 * On Windows, processes are spawned using CreateProcess.
 * Both implementations capture stdout/stderr output along with the exit code.
 */

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Initial buffer size for reading from file descriptors */
#define READ_BUFFER_INITIAL_SIZE 4096

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

#ifdef _WIN32
/* ============================================================================
 * Windows Implementation using CreateProcess
 * ============================================================================ */

/* Read all data from a Windows HANDLE until EOF.
 * Returns a null-terminated string allocated in the arena.
 * Closes the handle after reading.
 */
static char *read_handle_to_string(RtArena *arena, HANDLE handle)
{
    if (arena == NULL || handle == NULL || handle == INVALID_HANDLE_VALUE) {
        if (handle != NULL && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
        return rt_arena_strdup(arena, "");
    }

    size_t capacity = READ_BUFFER_INITIAL_SIZE;
    size_t length = 0;
    char *buffer = rt_arena_alloc(arena, capacity);
    if (buffer == NULL) {
        CloseHandle(handle);
        return rt_arena_strdup(arena, "");
    }

    while (1) {
        if (length + 1 >= capacity) {
            size_t new_capacity = capacity * 2;
            char *new_buffer = rt_arena_alloc(arena, new_capacity);
            if (new_buffer == NULL) {
                buffer[length] = '\0';
                CloseHandle(handle);
                return buffer;
            }
            memcpy(new_buffer, buffer, length);
            buffer = new_buffer;
            capacity = new_capacity;
        }

        DWORD bytes_read = 0;
        BOOL success = ReadFile(handle, buffer + length,
                                (DWORD)(capacity - length - 1), &bytes_read, NULL);

        if (!success || bytes_read == 0) {
            break;
        }

        length += bytes_read;
    }

    buffer[length] = '\0';
    CloseHandle(handle);
    return buffer;
}

/* Build a command line string from command and args for CreateProcess.
 * Windows uses a single command line string, not argv.
 * Returns malloc'd string (caller must free).
 */
static char *build_command_line(const char *cmd, char **args)
{
    /* Calculate required buffer size */
    size_t total_len = strlen(cmd) + 3; /* cmd + quotes + space + null */

    if (args != NULL) {
        size_t args_len = rt_array_length(args);
        for (size_t i = 0; i < args_len; i++) {
            total_len += strlen(args[i]) + 3; /* arg + quotes + space */
        }
    }

    char *cmdline = malloc(total_len);
    if (cmdline == NULL) {
        return NULL;
    }

    /* Build the command line */
    char *ptr = cmdline;

    /* Add command (quote if contains spaces) */
    if (strchr(cmd, ' ') != NULL) {
        ptr += sprintf(ptr, "\"%s\"", cmd);
    } else {
        ptr += sprintf(ptr, "%s", cmd);
    }

    /* Add arguments */
    if (args != NULL) {
        size_t args_len = rt_array_length(args);
        for (size_t i = 0; i < args_len; i++) {
            /* Quote arguments that contain spaces */
            if (strchr(args[i], ' ') != NULL) {
                ptr += sprintf(ptr, " \"%s\"", args[i]);
            } else {
                ptr += sprintf(ptr, " %s", args[i]);
            }
        }
    }

    return cmdline;
}

/* Windows implementation of process execution */
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

    /* Build command line string */
    char *cmdline = build_command_line(cmd, args);
    if (cmdline == NULL) {
        fprintf(stderr, "rt_process_run_with_args: failed to build command line\n");
        return rt_process_create(arena, 127, "", "");
    }

    /* Create pipes for stdout and stderr */
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE stdout_read = NULL, stdout_write = NULL;
    HANDLE stderr_read = NULL, stderr_write = NULL;

    if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
        free(cmdline);
        fprintf(stderr, "rt_process_run_with_args: stdout pipe failed\n");
        return rt_process_create(arena, 127, "", "");
    }
    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
        CloseHandle(stdout_read);
        CloseHandle(stdout_write);
        free(cmdline);
        fprintf(stderr, "rt_process_run_with_args: stderr pipe failed\n");
        return rt_process_create(arena, 127, "", "");
    }
    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);

    /* Set up process startup info */
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    /* Create the child process */
    BOOL success = CreateProcessA(
        NULL,           /* No module name (use command line) */
        cmdline,        /* Command line */
        NULL,           /* Process handle not inheritable */
        NULL,           /* Thread handle not inheritable */
        TRUE,           /* Inherit handles */
        0,              /* No creation flags */
        NULL,           /* Use parent's environment */
        NULL,           /* Use parent's current directory */
        &si,            /* Pointer to STARTUPINFO */
        &pi             /* Pointer to PROCESS_INFORMATION */
    );

    /* Close write ends of pipes - we only read from them */
    CloseHandle(stdout_write);
    CloseHandle(stderr_write);

    if (!success) {
        /* CreateProcess failed */
        DWORD error = GetLastError();
        CloseHandle(stdout_read);
        CloseHandle(stderr_read);
        free(cmdline);

        /* Format error message for stderr */
        char error_msg[256];
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            snprintf(error_msg, sizeof(error_msg), "%s: command not found\n", cmd);
            return rt_process_create(arena, 127, "", error_msg);
        } else {
            snprintf(error_msg, sizeof(error_msg),
                     "CreateProcess failed with error %lu\n", error);
            return rt_process_create(arena, 127, "", error_msg);
        }
    }

    free(cmdline);

    /* Read stdout and stderr from child */
    char *stdout_data = read_handle_to_string(arena, stdout_read);
    char *stderr_data = read_handle_to_string(arena, stderr_read);

    /* Wait for child process to complete */
    WaitForSingleObject(pi.hProcess, INFINITE);

    /* Get exit code */
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    /* Clean up handles */
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return rt_process_create(arena, (int)exit_code, stdout_data, stderr_data);
}

#else
/* ============================================================================
 * POSIX Implementation using fork/exec
 * ============================================================================ */

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

#endif /* _WIN32 / POSIX */

/* ============================================================================
 * Common Functions (both platforms)
 * ============================================================================ */

/* Run a command with no arguments.
 * Delegates to rt_process_run_with_args with NULL args.
 */
RtProcess *rt_process_run(RtArena *arena, const char *cmd)
{
    return rt_process_run_with_args(arena, cmd, NULL);
}

/* Exit the program with the specified exit code. */
void rt_exit(int code)
{
    exit(code);
}

/* Assert that a condition is true. */
void rt_assert(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}
