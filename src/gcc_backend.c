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

/* Default values for backend configuration */
#define DEFAULT_CC "gcc"
#define DEFAULT_STD "c99"
#define DEFAULT_DEBUG_CFLAGS "-no-pie -fsanitize=address -fno-omit-frame-pointer -g"
#define DEFAULT_RELEASE_CFLAGS "-O3 -flto"

void cc_backend_init_config(CCBackendConfig *config)
{
    const char *env_val;

    /* SN_CC: C compiler command */
    env_val = getenv("SN_CC");
    config->cc = (env_val && env_val[0]) ? env_val : DEFAULT_CC;

    /* SN_STD: C standard */
    env_val = getenv("SN_STD");
    config->std = (env_val && env_val[0]) ? env_val : DEFAULT_STD;

    /* SN_DEBUG_CFLAGS: Debug mode flags */
    env_val = getenv("SN_DEBUG_CFLAGS");
    config->debug_cflags = (env_val && env_val[0]) ? env_val : DEFAULT_DEBUG_CFLAGS;

    /* SN_RELEASE_CFLAGS: Release mode flags */
    env_val = getenv("SN_RELEASE_CFLAGS");
    config->release_cflags = (env_val && env_val[0]) ? env_val : DEFAULT_RELEASE_CFLAGS;

    /* SN_CFLAGS: Additional compiler flags (empty by default) */
    env_val = getenv("SN_CFLAGS");
    config->cflags = (env_val && env_val[0]) ? env_val : "";

    /* SN_LDFLAGS: Additional linker flags (empty by default) */
    env_val = getenv("SN_LDFLAGS");
    config->ldflags = (env_val && env_val[0]) ? env_val : "";

    /* SN_LDLIBS: Additional libraries (empty by default) */
    env_val = getenv("SN_LDLIBS");
    config->ldlibs = (env_val && env_val[0]) ? env_val : "";
}

bool gcc_check_available(const CCBackendConfig *config, bool verbose)
{
    /* Build command to check if compiler is available */
    char check_cmd[PATH_MAX];
    snprintf(check_cmd, sizeof(check_cmd), "%s --version > /dev/null 2>&1", config->cc);

    int result = system(check_cmd);

    if (result == 0)
    {
        if (verbose)
        {
            DEBUG_INFO("C compiler '%s' found and available", config->cc);
        }
        return true;
    }

    fprintf(stderr, "Error: C compiler '%s' is not installed or not in PATH.\n", config->cc);
    if (strcmp(config->cc, "gcc") == 0)
    {
        fprintf(stderr, "To compile Sn programs to executables, please install GCC:\n");
        fprintf(stderr, "  Ubuntu/Debian: sudo apt install gcc\n");
        fprintf(stderr, "  Fedora/RHEL:   sudo dnf install gcc\n");
        fprintf(stderr, "  Arch Linux:    sudo pacman -S gcc\n");
    }
    else
    {
        fprintf(stderr, "Ensure '%s' is installed and in your PATH.\n", config->cc);
        fprintf(stderr, "Or set SN_CC to a different compiler.\n");
    }
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

bool gcc_compile(const CCBackendConfig *config, const char *c_file,
                 const char *output_exe, const char *compiler_dir,
                 bool verbose, bool debug_mode,
                 char **link_libs, int link_lib_count)
{
    char exe_path[PATH_MAX];
    char arena_obj[PATH_MAX];
    char debug_obj[PATH_MAX];
    char runtime_obj[PATH_MAX];
    char runtime_arena_obj[PATH_MAX];
    char runtime_string_obj[PATH_MAX];
    char runtime_array_obj[PATH_MAX];
    char runtime_text_file_obj[PATH_MAX];
    char runtime_binary_file_obj[PATH_MAX];
    char runtime_io_obj[PATH_MAX];
    char runtime_byte_obj[PATH_MAX];
    char runtime_path_obj[PATH_MAX];
    char runtime_date_obj[PATH_MAX];
    char runtime_time_obj[PATH_MAX];
    char runtime_thread_obj[PATH_MAX];
    char runtime_process_obj[PATH_MAX];
    char runtime_net_obj[PATH_MAX];
    char runtime_random_obj[PATH_MAX];
    char runtime_uuid_obj[PATH_MAX];
    char runtime_sha1_obj[PATH_MAX];
    char runtime_env_obj[PATH_MAX];
    char command[PATH_MAX * 16];
    char extra_libs[PATH_MAX];

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
    snprintf(runtime_arena_obj, sizeof(runtime_arena_obj), "%s/runtime_arena.o", compiler_dir);
    snprintf(runtime_string_obj, sizeof(runtime_string_obj), "%s/runtime_string.o", compiler_dir);
    snprintf(runtime_array_obj, sizeof(runtime_array_obj), "%s/runtime_array.o", compiler_dir);
    snprintf(runtime_text_file_obj, sizeof(runtime_text_file_obj), "%s/runtime_text_file.o", compiler_dir);
    snprintf(runtime_binary_file_obj, sizeof(runtime_binary_file_obj), "%s/runtime_binary_file.o", compiler_dir);
    snprintf(runtime_io_obj, sizeof(runtime_io_obj), "%s/runtime_io.o", compiler_dir);
    snprintf(runtime_byte_obj, sizeof(runtime_byte_obj), "%s/runtime_byte.o", compiler_dir);
    snprintf(runtime_path_obj, sizeof(runtime_path_obj), "%s/runtime_path.o", compiler_dir);
    snprintf(runtime_date_obj, sizeof(runtime_date_obj), "%s/runtime_date.o", compiler_dir);
    snprintf(runtime_time_obj, sizeof(runtime_time_obj), "%s/runtime_time.o", compiler_dir);
    snprintf(runtime_thread_obj, sizeof(runtime_thread_obj), "%s/runtime_thread.o", compiler_dir);
    snprintf(runtime_process_obj, sizeof(runtime_process_obj), "%s/runtime_process.o", compiler_dir);
    snprintf(runtime_net_obj, sizeof(runtime_net_obj), "%s/runtime_net.o", compiler_dir);
    snprintf(runtime_random_obj, sizeof(runtime_random_obj), "%s/runtime_random.o", compiler_dir);
    snprintf(runtime_uuid_obj, sizeof(runtime_uuid_obj), "%s/runtime_uuid.o", compiler_dir);
    snprintf(runtime_sha1_obj, sizeof(runtime_sha1_obj), "%s/runtime_sha1.o", compiler_dir);
    snprintf(runtime_env_obj, sizeof(runtime_env_obj), "%s/runtime_env.o", compiler_dir);

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
    if (access(runtime_arena_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_arena_obj);
        return false;
    }
    if (access(runtime_string_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_string_obj);
        return false;
    }
    if (access(runtime_array_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_array_obj);
        return false;
    }
    if (access(runtime_text_file_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_text_file_obj);
        return false;
    }
    if (access(runtime_binary_file_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_binary_file_obj);
        return false;
    }
    if (access(runtime_io_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_io_obj);
        return false;
    }
    if (access(runtime_byte_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_byte_obj);
        return false;
    }
    if (access(runtime_path_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_path_obj);
        return false;
    }
    if (access(runtime_date_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_date_obj);
        return false;
    }
    if (access(runtime_time_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_time_obj);
        return false;
    }
    if (access(runtime_thread_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_thread_obj);
        return false;
    }
    if (access(runtime_process_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_process_obj);
        return false;
    }
    if (access(runtime_net_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_net_obj);
        return false;
    }
    if (access(runtime_random_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_random_obj);
        return false;
    }
    if (access(runtime_uuid_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_uuid_obj);
        return false;
    }
    if (access(runtime_sha1_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_sha1_obj);
        return false;
    }
    if (access(runtime_env_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_env_obj);
        return false;
    }

    /* Build C compiler command using configuration.
     *
     * Command structure:
     *   $CC $MODE_CFLAGS -w -std=$STD -D_GNU_SOURCE $CFLAGS -I"dir" <sources>
     *        -lpthread -lm $extra_libs $LDLIBS $LDFLAGS -o "output" 2>"errors"
     *
     * We use -w to suppress all warnings on generated code - any issues should
     * be caught by the Sn type checker, not the C compiler. We redirect stderr
     * to capture any errors for display only if compilation actually fails.
     */

    /* Build extra library flags from pragma link directives */
    extra_libs[0] = '\0';
    if (link_libs != NULL && link_lib_count > 0)
    {
        int offset = 0;
        for (int i = 0; i < link_lib_count && offset < (int)sizeof(extra_libs) - 8; i++)
        {
            int written = snprintf(extra_libs + offset, sizeof(extra_libs) - offset, " -l%s", link_libs[i]);
            if (written > 0)
            {
                offset += written;
            }
        }
    }

    char error_file[] = "/tmp/sn_cc_errors_XXXXXX";
    int error_fd = mkstemp(error_file);
    if (error_fd == -1)
    {
        /* Fallback: use a fixed name */
        strcpy(error_file, "/tmp/sn_cc_errors.txt");
    }
    else
    {
        close(error_fd);
    }

    /* Select mode-specific flags */
    const char *mode_cflags = debug_mode ? config->debug_cflags : config->release_cflags;

    /* Build the command - note: config->cflags, config->ldlibs, config->ldflags
     * may be empty strings, which is fine */
    snprintf(command, sizeof(command),
        "%s %s -w -std=%s -D_GNU_SOURCE %s -I\"%s\" "
        "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" "
        "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" "
        "-lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
        config->cc, mode_cflags, config->std, config->cflags, compiler_dir,
        c_file, arena_obj, debug_obj, runtime_obj, runtime_arena_obj,
        runtime_string_obj, runtime_array_obj, runtime_text_file_obj,
        runtime_binary_file_obj, runtime_io_obj, runtime_byte_obj,
        runtime_path_obj, runtime_date_obj, runtime_time_obj, runtime_thread_obj,
        runtime_process_obj, runtime_net_obj, runtime_random_obj, runtime_uuid_obj,
        runtime_sha1_obj, runtime_env_obj,
        extra_libs, config->ldlibs, config->ldflags, exe_path, error_file);

    if (verbose)
    {
        DEBUG_INFO("Executing: %s", command);
    }

    /* Execute C compiler */
    int result = system(command);

    if (result != 0)
    {
        /* Show compiler error output */
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
