// tests/unit/runtime/runtime_process_tests.c
// Tests for runtime process execution functions

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Process Run (Command Only) Tests
 * ============================================================================ */

static void test_rt_process_run_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Run 'true' command - should succeed with exit code 0 */
    RtProcess *p = rt_process_run(arena, "true");
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(p->stderr_data != NULL);

    rt_arena_destroy(arena);
}

static void test_rt_process_run_exit_code(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Run 'false' command - should fail with exit code 1 */
    RtProcess *p = rt_process_run(arena, "false");
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

    /* Create args array for 'echo hello' */
    char **args = rt_array_create_string(arena, 1, (const char *[]){"hello"});

    RtProcess *p = rt_process_run_with_args(arena, "echo", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "hello\n") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_process_run_with_args_multiple(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create args array for 'echo one two three' */
    char **args = rt_array_create_string(arena, 3, (const char *[]){"one", "two", "three"});

    RtProcess *p = rt_process_run_with_args(arena, "echo", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "one two three\n") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_process_run_with_args_null(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* NULL args should behave like command-only */
    RtProcess *p = rt_process_run_with_args(arena, "true", NULL);
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

    rt_arena_destroy(arena);
}

static void test_rt_process_stdout_multiline(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Use printf to output multiple lines */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "printf 'line1\\nline2\\nline3\\n'"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "line1\nline2\nline3\n") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Stderr Capture Tests
 * ============================================================================ */

static void test_rt_process_stderr_capture(void)
{

    RtArena *arena = rt_arena_create(NULL);

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

    rt_arena_destroy(arena);
}

static void test_rt_process_both_streams(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Write to both streams */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "echo out; echo err >&2"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "out\n") == 0);
    assert(p->stderr_data != NULL);
    assert(strcmp(p->stderr_data, "err\n") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Shell Commands via sh -c Tests
 * ============================================================================ */

static void test_rt_process_shell_command(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Run shell command with pipes */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "echo hello | tr h H"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "Hello\n") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_process_shell_exit_code(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Shell command that exits with specific code */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "exit 42"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 42);

    rt_arena_destroy(arena);
}

static void test_rt_process_shell_variable_expansion(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Shell command with variable expansion */
    char **args = rt_array_create_string(arena, 2, (const char *[]){"-c", "X=test; echo $X"});

    RtProcess *p = rt_process_run_with_args(arena, "sh", args);
    assert(p != NULL);
    assert(p->exit_code == 0);
    assert(p->stdout_data != NULL);
    assert(strcmp(p->stdout_data, "test\n") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Exit Code Tests
 * ============================================================================ */

static void test_rt_process_exit_codes(void)
{

    RtArena *arena = rt_arena_create(NULL);

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
