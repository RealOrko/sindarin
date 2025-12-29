#include "gcc_backend.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <libgen.h>
#include <limits.h>

/* Static buffer for compiler directory path */
static char compiler_dir_buf[PATH_MAX];

bool gcc_check_available(bool verbose)
{
    /* Try to run 'gcc --version' to check if GCC is available */
    int result = system("gcc --version > /dev/null 2>&1");

    if (result == 0)
    {
        if (verbose)
        {
            DEBUG_INFO("GCC found and available");
        }
        return true;
    }

    fprintf(stderr, "Error: GCC is not installed or not in PATH.\n");
    fprintf(stderr, "To compile Sn programs to executables, please install GCC:\n");
    fprintf(stderr, "  Ubuntu/Debian: sudo apt install gcc\n");
    fprintf(stderr, "  Fedora/RHEL:   sudo dnf install gcc\n");
    fprintf(stderr, "  Arch Linux:    sudo pacman -S gcc\n");
    fprintf(stderr, "\nAlternatively, use --emit-c to output C code only.\n");

    return false;
}

const char *gcc_get_compiler_dir(const char *argv0)
{
    /* First, try to resolve via /proc/self/exe (Linux-specific but reliable) */
    ssize_t len = readlink("/proc/self/exe", compiler_dir_buf, sizeof(compiler_dir_buf) - 1);
    if (len != -1)
    {
        compiler_dir_buf[len] = '\0';
        /* Get the directory part */
        char *dir = dirname(compiler_dir_buf);
        /* Copy back since dirname may modify in place */
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }

    /* Fallback: use argv[0] */
    if (argv0 != NULL)
    {
        /* Make a copy since dirname may modify the string */
        strncpy(compiler_dir_buf, argv0, sizeof(compiler_dir_buf) - 1);
        compiler_dir_buf[sizeof(compiler_dir_buf) - 1] = '\0';
        char *dir = dirname(compiler_dir_buf);
        memmove(compiler_dir_buf, dir, strlen(dir) + 1);
        return compiler_dir_buf;
    }

    /* Last resort: assume current directory */
    strcpy(compiler_dir_buf, ".");
    return compiler_dir_buf;
}

bool gcc_compile(const char *c_file, const char *output_exe,
                 const char *compiler_dir, bool verbose, bool debug_mode)
{
    char exe_path[PATH_MAX];
    char arena_obj[PATH_MAX];
    char debug_obj[PATH_MAX];
    char runtime_obj[PATH_MAX];
    char command[PATH_MAX * 5 + 256];

    /* Determine output executable path if not specified */
    if (output_exe == NULL || output_exe[0] == '\0')
    {
        /* Derive from c_file by removing .c extension */
        strncpy(exe_path, c_file, sizeof(exe_path) - 1);
        exe_path[sizeof(exe_path) - 1] = '\0';

        char *dot = strrchr(exe_path, '.');
        if (dot != NULL && strcmp(dot, ".c") == 0)
        {
            *dot = '\0';
        }
        /* else: just use the filename as-is */
    }
    else
    {
        strncpy(exe_path, output_exe, sizeof(exe_path) - 1);
        exe_path[sizeof(exe_path) - 1] = '\0';
    }

    /* Build paths to runtime object files */
    snprintf(arena_obj, sizeof(arena_obj), "%s/arena.o", compiler_dir);
    snprintf(debug_obj, sizeof(debug_obj), "%s/debug.o", compiler_dir);
    snprintf(runtime_obj, sizeof(runtime_obj), "%s/runtime.o", compiler_dir);

    /* Check that runtime objects exist */
    if (access(arena_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", arena_obj);
        fprintf(stderr, "Make sure the compiler was built correctly with ./scripts/build.sh\n");
        return false;
    }
    if (access(debug_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", debug_obj);
        return false;
    }
    if (access(runtime_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_obj);
        return false;
    }

    /* Build GCC command
     * Note: Runtime objects are compiled with address sanitizer, so we always
     * need to link with -fsanitize=address. The debug_mode flag controls whether
     * we include extra debug info (-g) and frame pointers.
     *
     * We use -w to suppress all warnings on generated code - any issues should
     * be caught by the Sn type checker, not GCC. We redirect stderr to capture
     * any errors for display only if compilation actually fails.
     */
    char error_file[] = "/tmp/sn_gcc_errors_XXXXXX";
    int error_fd = mkstemp(error_file);
    if (error_fd == -1)
    {
        /* Fallback: use a fixed name */
        strcpy(error_file, "/tmp/sn_gcc_errors.txt");
    }
    else
    {
        close(error_fd);
    }

    if (debug_mode)
    {
        snprintf(command, sizeof(command),
            "gcc -no-pie -fsanitize=address -fno-omit-frame-pointer -g "
            "-w -std=c99 -D_GNU_SOURCE "
            "\"%s\" \"%s\" \"%s\" \"%s\" -o \"%s\" 2>\"%s\"",
            c_file, arena_obj, debug_obj, runtime_obj, exe_path, error_file);
    }
    else
    {
        /* Still need -fsanitize=address because runtime objects require it */
        snprintf(command, sizeof(command),
            "gcc -O2 -fsanitize=address -w -std=c99 -D_GNU_SOURCE "
            "\"%s\" \"%s\" \"%s\" \"%s\" -o \"%s\" 2>\"%s\"",
            c_file, arena_obj, debug_obj, runtime_obj, exe_path, error_file);
    }

    if (verbose)
    {
        DEBUG_INFO("Executing: %s", command);
    }

    /* Execute GCC */
    int result = system(command);

    if (result != 0)
    {
        /* Show GCC error output */
        FILE *errfile = fopen(error_file, "r");
        if (errfile)
        {
            char line[1024];
            fprintf(stderr, "\n");
            while (fgets(line, sizeof(line), errfile))
            {
                fprintf(stderr, "%s", line);
            }
            fclose(errfile);
        }
        unlink(error_file);
        return false;
    }

    /* Clean up error file on success */
    unlink(error_file);

    if (verbose)
    {
        DEBUG_INFO("Successfully compiled to: %s", exe_path);
    }

    return true;
}
