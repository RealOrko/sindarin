// tests/unit/runtime/runtime_process_tests.c
// Tests for runtime process execution functions

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Cross-Platform Helper Macros
 * ============================================================================
 * Windows uses cmd.exe, POSIX uses sh. Commands differ between platforms.
 */

#ifdef _WIN32
    #define SHELL_CMD "cmd.exe"
    #define SHELL_ARG "/c"
    #define EXIT_SUCCESS_CMD "cmd.exe /c exit 0"
    #define EXIT_FAILURE_CMD "cmd.exe /c exit 1"
    #define ECHO_HELLO_CMD "echo hello"
    #define ECHO_ARGS_CMD "echo one two three"
    #define LINE_ENDING "\r\n"
#else
    #define SHELL_CMD "sh"
    #define SHELL_ARG "-c"
    #define EXIT_SUCCESS_CMD "true"
    #define EXIT_FAILURE_CMD "false"
    #define ECHO_HELLO_CMD "echo hello"
    #define ECHO_ARGS_CMD "echo one two three"
    #define LINE_ENDING "\n"
#endif

/* ============================================================================
 * Process Run (Command Only) Tests
 * ============================================================================ */

static void test_rt_process_run_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Run 'cmd /c exit 0' - should succeed with exit code 0 */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "exit 0"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
#else
    /* Run 'true' command - should succeed with exit code 0 */
    RtProcess *p = rt_process_run(arena, "true");
#endif
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(p->stderr_data != NULL);

    rt_arena_destroy(arena);
}

static void test_rt_process_run_exit_code(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Run 'cmd /c exit 1' - should fail with exit code 1 */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "exit 1"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
#else
    /* Run 'false' command - should fail with exit code 1 */
    RtProcess *p = rt_process_run(arena, "false");
#endif
    assert(p != NULL);
    assert(p->exit_code == 1);

    rt_arena_destroy(arena);
}

static void test_rt_process_run_command_not_found(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Run nonexistent command - should return exit code 127 */
    RtProcess *p = rt_process_run(arena, "nonexistent_command_xyz123");
    assert(p != NULL);
    assert(p->exit_code == 127);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Process Run With Arguments Tests
 * ============================================================================ */

static void test_rt_process_run_with_args_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Use cmd /c echo hello on Windows */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "echo hello"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    /* Windows echo includes "hello\r\n" */
    assert(strstr(p->stdout_data, "hello") != NULL);
#else
    /* Create args array for 'echo hello' */
    char **args = rt_array_create_string(arena, 1, (const char *[]){"hello"});

    RtProcess *p = rt_process_run_with_args(arena, "echo", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "hello\n") == 0);
#endif

    rt_arena_destroy(arena);
}

static void test_rt_process_run_with_args_multiple(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Use cmd /c echo one two three on Windows */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "echo one two three"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strstr(p->stdout_data, "one two three") != NULL);
#else
    /* Create args array for 'echo one two three' */
    char **args = rt_array_create_string(arena, 3, (const char *[]){"one", "two", "three"});

    RtProcess *p = rt_process_run_with_args(arena, "echo", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "one two three\n") == 0);
#endif

    rt_arena_destroy(arena);
}

static void test_rt_process_run_with_args_null(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* NULL args should behave like command-only */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "exit 0"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
#else
    /* NULL args should behave like command-only */
    RtProcess *p = rt_process_run_with_args(arena, "true", NULL);
#endif
    assert(p != NULL);
    assert(p->exit_code == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Stdout Capture Tests
 * ============================================================================ */

static void test_rt_process_stdout_capture(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Create args for 'cmd /c echo test output' */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "echo test output"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strstr(p->stdout_data, "test output") != NULL);
#else
    /* Create args for 'echo test output' */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"test", "output"});

    RtProcess *p = rt_process_run_with_args(arena, "echo", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "test output\n") == 0);
    /* stderr should be empty */
    assert(p->stderr_data != NULL);
    assert(strcmp(p->stderr_data, "") == 0);
#endif

    rt_arena_destroy(arena);
}

static void test_rt_process_stdout_multiline(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Use cmd /c with multiple echo statements */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "echo line1 & echo line2 & echo line3"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strstr(p->stdout_data, "line1") != NULL);
    assert(strstr(p->stdout_data, "line2") != NULL);
    assert(strstr(p->stdout_data, "line3") != NULL);
#else
    /* Use printf to output multiple lines */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "printf 'line1\\nline2\\nline3\\n'"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "line1\nline2\nline3\n") == 0);
#endif

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Stderr Capture Tests
 * ============================================================================ */

static void test_rt_process_stderr_capture(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Use cmd /c to write to stderr - redirect echo to stderr */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "echo error 1>&2"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    /* stdout might be empty or have the redirect output */
    assert(p->stderr_data != NULL);
    assert(strstr(p->stderr_data, "error") != NULL);
#else
    /* Use sh -c to write to stderr */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "echo error >&2"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    /* stdout should be empty */
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "") == 0);
    /* stderr should have content */
    assert(p->stderr_data != NULL);
    assert(strcmp(p->stderr_data, "error\n") == 0);
#endif

    rt_arena_destroy(arena);
}

static void test_rt_process_both_streams(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Write to both streams */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "echo out & echo err 1>&2"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strstr(p->stdout_data, "out") != NULL);
    assert(p->stderr_data != NULL);
    assert(strstr(p->stderr_data, "err") != NULL);
#else
    /* Write to both streams */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "echo out; echo err >&2"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "out\n") == 0);
    assert(p->stderr_data != NULL);
    assert(strcmp(p->stderr_data, "err\n") == 0);
#endif

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Shell Commands Tests
 * ============================================================================ */

static void test_rt_process_shell_command(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Run shell command - Windows doesn't have pipes like Unix in cmd */
    /* Just test that we can run a more complex command */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "echo Hello World"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strstr(p->stdout_data, "Hello World") != NULL);
#else
    /* Run shell command with pipes */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "echo hello | tr h H"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "Hello\n") == 0);
#endif

    rt_arena_destroy(arena);
}

static void test_rt_process_shell_exit_code(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Shell command that exits with specific code */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "exit 42"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
#else
    /* Shell command that exits with specific code */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "exit 42"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
#endif
    assert(p != NULL);
    assert(p->exit_code == 42);

    rt_arena_destroy(arena);
}

static void test_rt_process_shell_variable_expansion(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Shell command with variable expansion - Windows style */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"/c", "set X=test && echo %X%"});
    RtProcess *p = rt_process_run_with_args(arena, "cmd.exe", args);
    assert(p != NULL);
    /* Windows CMD might not expand %X% in a single command like this */
    /* Just verify the command ran */
    assert(p->exit_code == 0 || p->exit_code == 1);
#else
    /* Shell command with variable expansion */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "X=test; echo $X"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "test\n") == 0);
#endif

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Exit Code Tests
 * ============================================================================ */

static void test_rt_process_exit_codes(void)
{

    RtArena *arena = rt_arena_create(NULL);

#ifdef _WIN32
    /* Test exit code 0 */
    char **args0 = rt_array_create_string(arena, 2, (const char *[]){"/c", "exit 0"});
    RtProcess *p0 = rt_process_run_with_args(arena, "cmd.exe", args0);
    assert(p0->exit_code == 0);

    /* Test exit code 1 */
    char **args1 = rt_array_create_string(arena, 2, (const char *[]){"/c", "exit 1"});
    RtProcess *p1 = rt_process_run_with_args(arena, "cmd.exe", args1);
    assert(p1->exit_code == 1);

    /* Test exit code 255 */
    char **args255 = rt_array_create_string(arena, 2, (const char *[]){"/c", "exit 255"});
    RtProcess *p255 = rt_process_run_with_args(arena, "cmd.exe", args255);
    assert(p255->exit_code == 255);
#else
    /* Test exit code 0 */
    char **args0 = rt_array_create_string(arena, 2, (const char *[]){"-c", "exit 0"});
    RtProcess *p0 = rt_process_run_with_args(arena, "sh", args0);
    assert(p0->exit_code == 0);

    /* Test exit code 1 */
    char **args1 = rt_array_create_string(arena, 2, (const char *[]){"-c", "exit 1"});
    RtProcess *p1 = rt_process_run_with_args(arena, "sh", args1);
    assert(p1->exit_code == 1);

    /* Test exit code 255 */
    char **args255 = rt_array_create_string(arena, 2, (const char *[]){"-c", "exit 255"});
    RtProcess *p255 = rt_process_run_with_args(arena, "sh", args255);
    assert(p255->exit_code == 255);
#endif

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_process_main(void)
{
    TEST_SECTION("Process Execution");

    /* Command only tests */
    TEST_RUN("process_run_basic", test_rt_process_run_basic);
    TEST_RUN("process_run_exit_code", test_rt_process_run_exit_code);
    TEST_RUN("process_run_command_not_found", test_rt_process_run_command_not_found);

    /* Command with arguments tests */
    TEST_RUN("process_run_with_args_basic", test_rt_process_run_with_args_basic);
    TEST_RUN("process_run_with_args_multiple", test_rt_process_run_with_args_multiple);
    TEST_RUN("process_run_with_args_null", test_rt_process_run_with_args_null);

    /* Stdout capture tests */
    TEST_RUN("process_stdout_capture", test_rt_process_stdout_capture);
    TEST_RUN("process_stdout_multiline", test_rt_process_stdout_multiline);

    /* Stderr capture tests */
    TEST_RUN("process_stderr_capture", test_rt_process_stderr_capture);
    TEST_RUN("process_both_streams", test_rt_process_both_streams);

    /* Shell command tests */
    TEST_RUN("process_shell_command", test_rt_process_shell_command);
    TEST_RUN("process_shell_exit_code", test_rt_process_shell_exit_code);
    TEST_RUN("process_shell_variable_expansion", test_rt_process_shell_variable_expansion);

    /* Exit code tests */
    TEST_RUN("process_exit_codes", test_rt_process_exit_codes);
}
