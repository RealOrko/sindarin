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

/* Backend type enumeration */
typedef enum {
    BACKEND_GCC,
    BACKEND_CLANG,
    BACKEND_TINYCC
} BackendType;

/* Detect backend type from compiler name */
static BackendType detect_backend(const char *cc)
{
    /* Check for tcc/tinycc first (before checking for 'cc' substring) */
    if (strstr(cc, "tcc") != NULL || strstr(cc, "tinycc") != NULL)
    {
        return BACKEND_TINYCC;
    }

    /* Check for clang (must be before gcc since some systems alias clang as gcc) */
    if (strstr(cc, "clang") != NULL)
    {
        return BACKEND_CLANG;
    }

    /* Default to gcc for gcc, cc, or unknown */
    return BACKEND_GCC;
}

/* Get library subdirectory for backend */
static const char *backend_lib_subdir(BackendType backend)
{
    switch (backend)
    {
        case BACKEND_CLANG:  return "lib/clang";
        case BACKEND_TINYCC: return "lib/tinycc";
        case BACKEND_GCC:
        default:             return "lib/gcc";
    }
}

/* Get backend name for error messages */
static const char *backend_name(BackendType backend)
{
    switch (backend)
    {
        case BACKEND_CLANG:  return "clang";
        case BACKEND_TINYCC: return "tinycc";
        case BACKEND_GCC:
        default:             return "gcc";
    }
}

/* Filter flags for TinyCC compatibility.
 * TinyCC doesn't support: -flto, -fsanitize=*, -fno-omit-frame-pointer
 * Returns pointer to filtered flags (may be buf or original flags).
 */
static const char *filter_tinycc_flags(const char *flags, char *buf, size_t buf_size)
{
    if (flags == NULL || flags[0] == '\0')
    {
        return flags;
    }

    char *out = buf;
    char *out_end = buf + buf_size - 1;
    const char *p = flags;

    while (*p && out < out_end)
    {
        /* Skip leading whitespace */
        while (*p == ' ') p++;
        if (!*p) break;

        /* Find end of token */
        const char *start = p;
        while (*p && *p != ' ') p++;
        size_t len = p - start;

        /* Check if this flag should be filtered out */
        if ((len >= 5 && strncmp(start, "-flto", 5) == 0) ||
            (len >= 10 && strncmp(start, "-fsanitize", 10) == 0) ||
            (len >= 23 && strncmp(start, "-fno-omit-frame-pointer", 23) == 0))
        {
            /* Skip this flag */
            continue;
        }

        /* Copy flag to output */
        if (out != buf && out < out_end)
        {
            *out++ = ' ';
        }
        size_t copy_len = len;
        if (out + copy_len > out_end)
        {
            copy_len = out_end - out;
        }
        memcpy(out, start, copy_len);
        out += copy_len;
    }
    *out = '\0';

    return buf;
}

/* Default values for backend configuration */
#define DEFAULT_STD "c99"
#define DEFAULT_DEBUG_CFLAGS_GCC "-no-pie -fsanitize=address -fno-omit-frame-pointer -g"
#define DEFAULT_RELEASE_CFLAGS_GCC "-O3 -flto"
#define DEFAULT_DEBUG_CFLAGS_CLANG "-fsanitize=address -fno-omit-frame-pointer -g"
#define DEFAULT_RELEASE_CFLAGS_CLANG "-O3 -flto"
#define DEFAULT_DEBUG_CFLAGS_TCC "-g"
#define DEFAULT_RELEASE_CFLAGS_TCC "-O2"

/* Static buffers for config file values (persisted for lifetime of process) */
static char cfg_cc[256];
static char cfg_std[64];
static char cfg_debug_cflags[1024];
static char cfg_release_cflags[1024];
static char cfg_cflags[1024];
static char cfg_ldflags[1024];
static char cfg_ldlibs[1024];
static bool cfg_loaded = false;

/* Detect backend type from executable name (sn-gcc, sn-clang, sn-tcc) */
static BackendType detect_backend_from_exe(void)
{
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
    {
        return BACKEND_GCC; /* Default to GCC */
    }
    exe_path[len] = '\0';

    /* Get basename */
    char *base = strrchr(exe_path, '/');
    if (base) base++; else base = exe_path;

    /* Check for backend suffix */
    if (strstr(base, "sn-tcc") != NULL || strstr(base, "sn-tinycc") != NULL)
    {
        return BACKEND_TINYCC;
    }
    if (strstr(base, "sn-clang") != NULL)
    {
        return BACKEND_CLANG;
    }
    return BACKEND_GCC;
}

/* Get config filename for a backend (e.g., "sn.gcc.cfg") */
static const char *get_config_filename(BackendType backend)
{
    switch (backend)
    {
        case BACKEND_CLANG:  return "sn.clang.cfg";
        case BACKEND_TINYCC: return "sn.tcc.cfg";
        case BACKEND_GCC:
        default:             return "sn.gcc.cfg";
    }
}

/* Parse a single line from config file (KEY=VALUE format) */
static void parse_config_line(const char *line)
{
    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') line++;

    /* Skip empty lines and comments */
    if (*line == '\0' || *line == '\n' || *line == '#')
        return;

    /* Find the = separator */
    const char *eq = strchr(line, '=');
    if (!eq) return;

    /* Extract key and value */
    size_t key_len = eq - line;
    const char *value = eq + 1;

    /* Trim trailing whitespace/newline from value */
    size_t value_len = strlen(value);
    while (value_len > 0 && (value[value_len-1] == '\n' || value[value_len-1] == '\r' ||
                              value[value_len-1] == ' ' || value[value_len-1] == '\t'))
    {
        value_len--;
    }

    /* Match key and copy value to appropriate buffer */
    if (key_len == 5 && strncmp(line, "SN_CC", 5) == 0)
    {
        if (value_len < sizeof(cfg_cc))
        {
            strncpy(cfg_cc, value, value_len);
            cfg_cc[value_len] = '\0';
        }
    }
    else if (key_len == 6 && strncmp(line, "SN_STD", 6) == 0)
    {
        if (value_len < sizeof(cfg_std))
        {
            strncpy(cfg_std, value, value_len);
            cfg_std[value_len] = '\0';
        }
    }
    else if (key_len == 15 && strncmp(line, "SN_DEBUG_CFLAGS", 15) == 0)
    {
        if (value_len < sizeof(cfg_debug_cflags))
        {
            strncpy(cfg_debug_cflags, value, value_len);
            cfg_debug_cflags[value_len] = '\0';
        }
    }
    else if (key_len == 17 && strncmp(line, "SN_RELEASE_CFLAGS", 17) == 0)
    {
        if (value_len < sizeof(cfg_release_cflags))
        {
            strncpy(cfg_release_cflags, value, value_len);
            cfg_release_cflags[value_len] = '\0';
        }
    }
    else if (key_len == 9 && strncmp(line, "SN_CFLAGS", 9) == 0)
    {
        if (value_len < sizeof(cfg_cflags))
        {
            strncpy(cfg_cflags, value, value_len);
            cfg_cflags[value_len] = '\0';
        }
    }
    else if (key_len == 10 && strncmp(line, "SN_LDFLAGS", 10) == 0)
    {
        if (value_len < sizeof(cfg_ldflags))
        {
            strncpy(cfg_ldflags, value, value_len);
            cfg_ldflags[value_len] = '\0';
        }
    }
    else if (key_len == 9 && strncmp(line, "SN_LDLIBS", 9) == 0)
    {
        if (value_len < sizeof(cfg_ldlibs))
        {
            strncpy(cfg_ldlibs, value, value_len);
            cfg_ldlibs[value_len] = '\0';
        }
    }
}

/* Load config file from compiler directory if it exists */
void cc_backend_load_config(const char *compiler_dir)
{
    if (cfg_loaded) return;
    cfg_loaded = true;

    /* Detect backend from executable name */
    BackendType backend = detect_backend_from_exe();
    const char *config_name = get_config_filename(backend);

    /* Build config file path */
    char config_path[PATH_MAX];
    snprintf(config_path, sizeof(config_path), "%s/%s", compiler_dir, config_name);

    /* Try to open config file */
    FILE *f = fopen(config_path, "r");
    if (!f) return; /* No config file, use defaults */

    /* Parse each line */
    char line[2048];
    while (fgets(line, sizeof(line), f))
    {
        parse_config_line(line);
    }
    fclose(f);
}

void cc_backend_init_config(CCBackendConfig *config)
{
    const char *env_val;

    /* Detect backend from executable name for defaults */
    BackendType backend = detect_backend_from_exe();

    /* Set backend-specific defaults */
    const char *default_cc;
    const char *default_debug_cflags;
    const char *default_release_cflags;

    switch (backend)
    {
        case BACKEND_CLANG:
            default_cc = "clang";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_CLANG;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_CLANG;
            break;
        case BACKEND_TINYCC:
            default_cc = "tcc";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_TCC;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_TCC;
            break;
        case BACKEND_GCC:
        default:
            default_cc = "gcc";
            default_debug_cflags = DEFAULT_DEBUG_CFLAGS_GCC;
            default_release_cflags = DEFAULT_RELEASE_CFLAGS_GCC;
            break;
    }

    /* Config file values are loaded by cc_backend_load_config() which should be
     * called before this function. If not called, cfg_* buffers remain empty. */

    /* Priority: Environment variable > Config file > Default
     * Config file values are in cfg_* buffers (empty if not set) */

    /* SN_CC: C compiler command */
    env_val = getenv("SN_CC");
    if (env_val && env_val[0])
        config->cc = env_val;
    else if (cfg_cc[0])
        config->cc = cfg_cc;
    else
        config->cc = default_cc;

    /* SN_STD: C standard */
    env_val = getenv("SN_STD");
    if (env_val && env_val[0])
        config->std = env_val;
    else if (cfg_std[0])
        config->std = cfg_std;
    else
        config->std = DEFAULT_STD;

    /* SN_DEBUG_CFLAGS: Debug mode flags */
    env_val = getenv("SN_DEBUG_CFLAGS");
    if (env_val && env_val[0])
        config->debug_cflags = env_val;
    else if (cfg_debug_cflags[0])
        config->debug_cflags = cfg_debug_cflags;
    else
        config->debug_cflags = default_debug_cflags;

    /* SN_RELEASE_CFLAGS: Release mode flags */
    env_val = getenv("SN_RELEASE_CFLAGS");
    if (env_val && env_val[0])
        config->release_cflags = env_val;
    else if (cfg_release_cflags[0])
        config->release_cflags = cfg_release_cflags;
    else
        config->release_cflags = default_release_cflags;

    /* SN_CFLAGS: Additional compiler flags (empty by default) */
    env_val = getenv("SN_CFLAGS");
    if (env_val && env_val[0])
        config->cflags = env_val;
    else if (cfg_cflags[0])
        config->cflags = cfg_cflags;
    else
        config->cflags = "";

    /* SN_LDFLAGS: Additional linker flags (empty by default) */
    env_val = getenv("SN_LDFLAGS");
    if (env_val && env_val[0])
        config->ldflags = env_val;
    else if (cfg_ldflags[0])
        config->ldflags = cfg_ldflags;
    else
        config->ldflags = "";

    /* SN_LDLIBS: Additional libraries (empty by default) */
    env_val = getenv("SN_LDLIBS");
    if (env_val && env_val[0])
        config->ldlibs = env_val;
    else if (cfg_ldlibs[0])
        config->ldlibs = cfg_ldlibs;
    else
        config->ldlibs = "";
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
    char lib_dir[PATH_MAX];
    char include_dir[PATH_MAX];
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
    char runtime_random_core_obj[PATH_MAX];
    char runtime_random_basic_obj[PATH_MAX];
    char runtime_random_static_obj[PATH_MAX];
    char runtime_random_choice_obj[PATH_MAX];
    char runtime_random_collection_obj[PATH_MAX];
    char runtime_random_obj[PATH_MAX];
    char runtime_uuid_obj[PATH_MAX];
    char runtime_sha1_obj[PATH_MAX];
    char runtime_env_obj[PATH_MAX];
    char command[PATH_MAX * 16];
    char extra_libs[PATH_MAX];
    char filtered_mode_cflags[1024];

    /* Detect backend type from compiler name */
    BackendType backend = detect_backend(config->cc);

    /* Build paths to library and include directories */
    snprintf(lib_dir, sizeof(lib_dir), "%s/%s", compiler_dir, backend_lib_subdir(backend));
    snprintf(include_dir, sizeof(include_dir), "%s/include", compiler_dir);

    if (verbose)
    {
        DEBUG_INFO("Using %s backend, lib_dir=%s", backend_name(backend), lib_dir);
    }

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

    /* Build paths to runtime object files (in backend-specific lib directory) */
    snprintf(arena_obj, sizeof(arena_obj), "%s/arena.o", lib_dir);
    snprintf(debug_obj, sizeof(debug_obj), "%s/debug.o", lib_dir);
    snprintf(runtime_obj, sizeof(runtime_obj), "%s/runtime.o", lib_dir);
    snprintf(runtime_arena_obj, sizeof(runtime_arena_obj), "%s/runtime_arena.o", lib_dir);
    snprintf(runtime_string_obj, sizeof(runtime_string_obj), "%s/runtime_string.o", lib_dir);
    snprintf(runtime_array_obj, sizeof(runtime_array_obj), "%s/runtime_array.o", lib_dir);
    snprintf(runtime_text_file_obj, sizeof(runtime_text_file_obj), "%s/runtime_text_file.o", lib_dir);
    snprintf(runtime_binary_file_obj, sizeof(runtime_binary_file_obj), "%s/runtime_binary_file.o", lib_dir);
    snprintf(runtime_io_obj, sizeof(runtime_io_obj), "%s/runtime_io.o", lib_dir);
    snprintf(runtime_byte_obj, sizeof(runtime_byte_obj), "%s/runtime_byte.o", lib_dir);
    snprintf(runtime_path_obj, sizeof(runtime_path_obj), "%s/runtime_path.o", lib_dir);
    snprintf(runtime_date_obj, sizeof(runtime_date_obj), "%s/runtime_date.o", lib_dir);
    snprintf(runtime_time_obj, sizeof(runtime_time_obj), "%s/runtime_time.o", lib_dir);
    snprintf(runtime_thread_obj, sizeof(runtime_thread_obj), "%s/runtime_thread.o", lib_dir);
    snprintf(runtime_process_obj, sizeof(runtime_process_obj), "%s/runtime_process.o", lib_dir);
    snprintf(runtime_net_obj, sizeof(runtime_net_obj), "%s/runtime_net.o", lib_dir);
    snprintf(runtime_random_core_obj, sizeof(runtime_random_core_obj), "%s/runtime_random_core.o", lib_dir);
    snprintf(runtime_random_basic_obj, sizeof(runtime_random_basic_obj), "%s/runtime_random_basic.o", lib_dir);
    snprintf(runtime_random_static_obj, sizeof(runtime_random_static_obj), "%s/runtime_random_static.o", lib_dir);
    snprintf(runtime_random_choice_obj, sizeof(runtime_random_choice_obj), "%s/runtime_random_choice.o", lib_dir);
    snprintf(runtime_random_collection_obj, sizeof(runtime_random_collection_obj), "%s/runtime_random_collection.o", lib_dir);
    snprintf(runtime_random_obj, sizeof(runtime_random_obj), "%s/runtime_random.o", lib_dir);
    snprintf(runtime_uuid_obj, sizeof(runtime_uuid_obj), "%s/runtime_uuid.o", lib_dir);
    snprintf(runtime_sha1_obj, sizeof(runtime_sha1_obj), "%s/runtime_sha1.o", lib_dir);
    snprintf(runtime_env_obj, sizeof(runtime_env_obj), "%s/runtime_env.o", lib_dir);

    /* Check that runtime objects exist */
    if (access(arena_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", arena_obj);
        fprintf(stderr, "The '%s' backend runtime is not built.\n", backend_name(backend));
        fprintf(stderr, "Run 'make build-%s' to build this backend.\n", backend_name(backend));
        return false;
    }
    if (access(debug_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", debug_obj);
        fprintf(stderr, "Run 'make build-%s' to build this backend.\n", backend_name(backend));
        return false;
    }
    if (access(runtime_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_obj);
        fprintf(stderr, "Run 'make build-%s' to build this backend.\n", backend_name(backend));
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
    if (access(runtime_random_core_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_random_core_obj);
        return false;
    }
    if (access(runtime_random_basic_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_random_basic_obj);
        return false;
    }
    if (access(runtime_random_static_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_random_static_obj);
        return false;
    }
    if (access(runtime_random_choice_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_random_choice_obj);
        return false;
    }
    if (access(runtime_random_collection_obj, R_OK) != 0)
    {
        fprintf(stderr, "Error: Runtime object not found: %s\n", runtime_random_collection_obj);
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

    /* Select mode-specific flags, filtering for TinyCC if needed */
    const char *mode_cflags = debug_mode ? config->debug_cflags : config->release_cflags;
    if (backend == BACKEND_TINYCC)
    {
        mode_cflags = filter_tinycc_flags(mode_cflags, filtered_mode_cflags, sizeof(filtered_mode_cflags));
    }

    /* Build the command - note: config->cflags, config->ldlibs, config->ldflags
     * may be empty strings, which is fine.
     * Use include_dir for headers (-I) and lib_dir for runtime objects.
     */
    snprintf(command, sizeof(command),
        "%s %s -w -std=%s -D_GNU_SOURCE %s -I\"%s\" "
        "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" "
        "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" "
        "-lpthread -lm%s %s %s -o \"%s\" 2>\"%s\"",
        config->cc, mode_cflags, config->std, config->cflags, include_dir,
        c_file, arena_obj, debug_obj, runtime_obj, runtime_arena_obj,
        runtime_string_obj, runtime_array_obj, runtime_text_file_obj,
        runtime_binary_file_obj, runtime_io_obj, runtime_byte_obj,
        runtime_path_obj, runtime_date_obj, runtime_time_obj, runtime_thread_obj,
        runtime_process_obj, runtime_net_obj, runtime_random_core_obj, runtime_random_basic_obj,
        runtime_random_static_obj, runtime_random_choice_obj, runtime_random_collection_obj, runtime_random_obj, runtime_uuid_obj, runtime_sha1_obj, runtime_env_obj,
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
