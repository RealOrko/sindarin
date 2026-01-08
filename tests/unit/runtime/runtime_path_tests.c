// tests/unit/runtime/runtime_path_tests.c
// Tests for runtime path operations

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../runtime.h"
#include "../test_harness.h"

/* ============================================================================
 * Path Component Extraction Tests
 * ============================================================================ */

static void test_rt_path_directory(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Unix-style paths */
    char *dir = rt_path_directory(arena, "/home/user/file.txt");
    assert(strcmp(dir, "/home/user") == 0);

    dir = rt_path_directory(arena, "/home/user/subdir/file.txt");
    assert(strcmp(dir, "/home/user/subdir") == 0);

    dir = rt_path_directory(arena, "relative/path/file.txt");
    assert(strcmp(dir, "relative/path") == 0);

    /* Root file */
    dir = rt_path_directory(arena, "/file.txt");
    assert(strcmp(dir, "/") == 0 || strcmp(dir, "") == 0);

    /* No directory - returns "." for current directory */
    dir = rt_path_directory(arena, "file.txt");
    assert(strcmp(dir, ".") == 0);

    /* Trailing slash */
    dir = rt_path_directory(arena, "/home/user/");
    assert(strcmp(dir, "/home/user") == 0 || strcmp(dir, "/home") == 0);

    /* NULL input - returns "." */
    dir = rt_path_directory(arena, NULL);
    assert(strcmp(dir, ".") == 0);

    /* Empty string - returns "." */
    dir = rt_path_directory(arena, "");
    assert(strcmp(dir, ".") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_path_filename(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Basic filename extraction */
    char *name = rt_path_filename(arena, "/home/user/file.txt");
    assert(strcmp(name, "file.txt") == 0);

    name = rt_path_filename(arena, "/home/user/document.pdf");
    assert(strcmp(name, "document.pdf") == 0);

    name = rt_path_filename(arena, "relative/path/script.sh");
    assert(strcmp(name, "script.sh") == 0);

    /* Just filename */
    name = rt_path_filename(arena, "file.txt");
    assert(strcmp(name, "file.txt") == 0);

    /* No extension */
    name = rt_path_filename(arena, "/home/user/README");
    assert(strcmp(name, "README") == 0);

    /* Hidden file */
    name = rt_path_filename(arena, "/home/user/.hidden");
    assert(strcmp(name, ".hidden") == 0);

    /* NULL input */
    name = rt_path_filename(arena, NULL);
    assert(strcmp(name, "") == 0);

    /* Empty string */
    name = rt_path_filename(arena, "");
    assert(strcmp(name, "") == 0);

    rt_arena_destroy(arena);
}

static void test_rt_path_extension(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Basic extension extraction */
    char *ext = rt_path_extension(arena, "/home/user/file.txt");
    assert(strcmp(ext, "txt") == 0);

    ext = rt_path_extension(arena, "/home/user/document.pdf");
    assert(strcmp(ext, "pdf") == 0);

    ext = rt_path_extension(arena, "archive.tar.gz");
    assert(strcmp(ext, "gz") == 0);  /* Last extension only */

    /* No extension */
    ext = rt_path_extension(arena, "/home/user/README");
    assert(strcmp(ext, "") == 0);

    ext = rt_path_extension(arena, "Makefile");
    assert(strcmp(ext, "") == 0);

    /* Hidden file with extension */
    ext = rt_path_extension(arena, "/home/user/.config.json");
    assert(strcmp(ext, "json") == 0);

    /* Hidden file without extension - returns "" since leading dot is not extension */
    ext = rt_path_extension(arena, "/home/user/.hidden");
    assert(strcmp(ext, "") == 0);

    /* NULL input */
    ext = rt_path_extension(arena, NULL);
    assert(strcmp(ext, "") == 0);

    /* Empty string */
    ext = rt_path_extension(arena, "");
    assert(strcmp(ext, "") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Path Join Tests
 * ============================================================================ */

static void test_rt_path_join2(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Basic join */
    char *path = rt_path_join2(arena, "/home/user", "file.txt");
    assert(strcmp(path, "/home/user/file.txt") == 0);

    path = rt_path_join2(arena, "/home", "user");
    assert(strcmp(path, "/home/user") == 0);

    /* Trailing slash on first component */
    path = rt_path_join2(arena, "/home/user/", "file.txt");
    assert(strcmp(path, "/home/user/file.txt") == 0);

    /* Leading slash on second component - returns path2 since it's absolute */
    path = rt_path_join2(arena, "/home/user", "/file.txt");
    assert(strcmp(path, "/file.txt") == 0);

    /* Empty components */
    path = rt_path_join2(arena, "", "file.txt");
    assert(strcmp(path, "file.txt") == 0);

    path = rt_path_join2(arena, "/home/user", "");
    assert(strcmp(path, "/home/user/") == 0);  /* Adds trailing separator */

    /* Relative paths */
    path = rt_path_join2(arena, "relative", "path");
    assert(strcmp(path, "relative/path") == 0);

    /* NULL handling */
    path = rt_path_join2(arena, NULL, "file.txt");
    assert(strcmp(path, "file.txt") == 0);

    path = rt_path_join2(arena, "/home", NULL);
    assert(strcmp(path, "/home/") == 0);  /* NULL treated as empty string, adds separator */

    rt_arena_destroy(arena);
}

static void test_rt_path_join3(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Basic three-component join */
    char *path = rt_path_join3(arena, "/home", "user", "file.txt");
    assert(strcmp(path, "/home/user/file.txt") == 0);

    path = rt_path_join3(arena, "a", "b", "c");
    assert(strcmp(path, "a/b/c") == 0);

    /* With trailing slashes */
    path = rt_path_join3(arena, "/home/", "user/", "file.txt");
    assert(strcmp(path, "/home/user/file.txt") == 0);

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Path Query Tests
 * ============================================================================ */

static void test_rt_path_exists(void)
{
    /* Current directory should exist */
    assert(rt_path_exists(".") == 1);

    /* Root should exist */
    assert(rt_path_exists("/") == 1);

    /* Non-existent path */
    assert(rt_path_exists("/definitely/does/not/exist/12345") == 0);

    /* NULL input */
    assert(rt_path_exists(NULL) == 0);

    /* Empty string */
    assert(rt_path_exists("") == 0);
}

static void test_rt_path_is_file(void)
{
    /* Create a temporary file for testing */
    const char *test_file = "/tmp/rt_path_test_file.txt";
    FILE *f = fopen(test_file, "w");
    if (f) {
        fprintf(f, "test");
        fclose(f);

        assert(rt_path_is_file(test_file) == 1);
        assert(rt_path_is_directory(test_file) == 0);

        /* Clean up */
        unlink(test_file);
    }

    /* Directory is not a file */
    assert(rt_path_is_file("/tmp") == 0);
    assert(rt_path_is_file(".") == 0);

    /* Non-existent path */
    assert(rt_path_is_file("/definitely/does/not/exist") == 0);

    /* NULL input */
    assert(rt_path_is_file(NULL) == 0);
}

static void test_rt_path_is_directory(void)
{
    /* Known directories */
    assert(rt_path_is_directory("/tmp") == 1);
    assert(rt_path_is_directory(".") == 1);
    assert(rt_path_is_directory("/") == 1);

    /* Non-existent path */
    assert(rt_path_is_directory("/definitely/does/not/exist") == 0);

    /* NULL input */
    assert(rt_path_is_directory(NULL) == 0);
}

static void test_rt_path_absolute(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Relative path should become absolute */
    char *abs = rt_path_absolute(arena, ".");
    assert(abs != NULL);
    assert(abs[0] == '/');  /* Should start with / on Unix */

    /* Already absolute should stay absolute */
    abs = rt_path_absolute(arena, "/tmp");
    assert(abs != NULL);
    assert(strcmp(abs, "/tmp") == 0 || abs[0] == '/');

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Directory Operation Tests
 * ============================================================================ */

static void test_rt_directory_list(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* List /tmp - should work */
    char **files = rt_directory_list(arena, "/tmp");
    assert(files != NULL);

    /* Non-existent directory - should return empty array */
    files = rt_directory_list(arena, "/definitely/does/not/exist");
    assert(files != NULL);
    assert(rt_array_length(files) == 0);

    rt_arena_destroy(arena);
}

static void test_rt_directory_create_and_delete(void)
{
    const char *test_dir = "/tmp/rt_path_test_dir_12345";

    /* Ensure it doesn't exist first */
    if (rt_path_exists(test_dir)) {
        rmdir(test_dir);
    }

    /* Create directory */
    rt_directory_create(test_dir);
    assert(rt_path_exists(test_dir) == 1);
    assert(rt_path_is_directory(test_dir) == 1);

    /* Delete directory */
    rt_directory_delete(test_dir);
    assert(rt_path_exists(test_dir) == 0);
}

static void test_rt_directory_list_recursive(void)
{
    RtArena *arena = rt_arena_create(NULL);

    /* Test with /tmp - should return some entries */
    char **files = rt_directory_list_recursive(arena, "/tmp");
    assert(files != NULL);
    /* Don't assert on count since /tmp contents vary */

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_path_main(void)
{
    TEST_SECTION("Runtime Path");

    /* Component extraction */
    TEST_RUN("rt_path_directory", test_rt_path_directory);
    TEST_RUN("rt_path_filename", test_rt_path_filename);
    TEST_RUN("rt_path_extension", test_rt_path_extension);

    /* Path joining */
    TEST_RUN("rt_path_join2", test_rt_path_join2);
    TEST_RUN("rt_path_join3", test_rt_path_join3);

    /* Path queries */
    TEST_RUN("rt_path_exists", test_rt_path_exists);
    TEST_RUN("rt_path_is_file", test_rt_path_is_file);
    TEST_RUN("rt_path_is_directory", test_rt_path_is_directory);
    TEST_RUN("rt_path_absolute", test_rt_path_absolute);

    /* Directory operations */
    TEST_RUN("rt_directory_list", test_rt_directory_list);
    TEST_RUN("rt_directory_create_and_delete", test_rt_directory_create_and_delete);
    TEST_RUN("rt_directory_list_recursive", test_rt_directory_list_recursive);
}
