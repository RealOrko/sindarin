// tests/code_gen_tests_util.c
// Helper functions and basic code gen tests

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../arena.h"
#include "../debug.h"
#include "../code_gen.h"
#include "../ast.h"
#include "../token.h"
#include "../symbol_table.h"
#include "../file.h"
#include "test_utils.h"

static const char *test_output_path = "test_output.c";
static const char *expected_output_path = "expected_output.c";

/* Use shared header from test_utils.h */
const char *get_expected(Arena *arena, const char *expected)
{
    return build_expected_output(arena, expected);
}

void create_expected_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    if (content)
    {
        size_t len = strlen(content);
        size_t written = fwrite(content, 1, len, file);
        assert(written == len);
    }
    fclose(file);
}

void remove_test_file(const char *path)
{
    remove(path);
}

void compare_output_files(const char *actual_path, const char *expected_path)
{
    DEBUG_VERBOSE("Entering compare_output_files with actual_path=%s, expected_path=%s", actual_path, expected_path);

    Arena read_arena;
    DEBUG_VERBOSE("Initializing arena with size=1MB");
    arena_init(&read_arena, 1024 * 1024);

    DEBUG_VERBOSE("Reading actual file: %s", actual_path);
    char *actual = file_read(&read_arena, actual_path);
    DEBUG_VERBOSE("Actual file contents: %s", actual ? actual : "NULL");

    DEBUG_VERBOSE("Reading expected file: %s", expected_path);
    char *expected = file_read(&read_arena, expected_path);
    DEBUG_VERBOSE("Expected file contents: %s", expected ? expected : "NULL");

    DEBUG_VERBOSE("Checking if file contents are non-null");
    assert(actual != NULL && expected != NULL);

    DEBUG_VERBOSE("Comparing file contents");
    assert(strcmp(actual, expected) == 0);

    DEBUG_VERBOSE("Freeing arena");
    arena_free(&read_arena);
}

void setup_basic_token(Token *token, TokenType type, const char *lexeme)
{
    token_init(token, type, lexeme, (int)strlen(lexeme), 1, "test.sn");
}

void test_code_gen_init_invalid_output_file()
{
    DEBUG_INFO("Starting test_code_gen_init_invalid_output_file");
    printf("Testing code_gen_init with invalid output path...\n");

    Arena arena;
    arena_init(&arena, 1024);
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);

    const char *invalid_path = "/invalid/path/test.c";
    code_gen_init(&arena, &gen, &sym_table, invalid_path);
    assert(gen.output == NULL); // fopen fails

    symbol_table_cleanup(&sym_table);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_init_invalid_output_file");
}

void test_code_gen_cleanup_null_output()
{
    DEBUG_INFO("Starting test_code_gen_cleanup_null_output");
    printf("Testing code_gen_cleanup with NULL output...\n");

    Arena arena;
    arena_init(&arena, 1024);
    CodeGen gen;
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    gen.output = NULL; // Simulate

    code_gen_cleanup(&gen); // Should do nothing

    symbol_table_cleanup(&sym_table);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_cleanup_null_output");
}

void test_code_gen_headers_and_externs()
{
    DEBUG_INFO("Starting test_code_gen_headers_and_externs");
    printf("Testing code_gen_headers and code_gen_externs...\n");

    Arena arena;
    arena_init(&arena, 4096);
    SymbolTable sym_table;
    symbol_table_init(&arena, &sym_table);
    CodeGen gen;
    code_gen_init(&arena, &gen, &sym_table, test_output_path);
    Module module;
    ast_init_module(&arena, &module, "test.sn");
    code_gen_module(&gen, &module);

    // Expected with full headers and externs + dummy main
    const char *expected = get_expected(&arena,
                                  "int main() {\n"
                                  "    return 0;\n"
                                  "}\n");

    code_gen_cleanup(&gen);
    symbol_table_cleanup(&sym_table);

    create_expected_file(expected_output_path, expected);
    compare_output_files(test_output_path, expected_output_path);
    remove_test_file(test_output_path);
    remove_test_file(expected_output_path);

    arena_free(&arena);

    DEBUG_INFO("Finished test_code_gen_headers_and_externs");
}

void test_code_gen_util_main()
{
    test_code_gen_cleanup_null_output();
    test_code_gen_headers_and_externs();
}
