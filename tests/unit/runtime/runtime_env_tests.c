// tests/unit/runtime/runtime_env_tests.c
// Tests for the runtime environment variable system

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/runtime/runtime_env.h"
#include "../../src/runtime/runtime_arena.h"
#include "../../src/runtime/runtime_array.h"
#include "../test_utils.h"
#include "../debug.h"

/* ============================================================================
 * rt_env_get() Tests
 * ============================================================================ */

void test_rt_env_get_existing_variable()
{
    printf("Testing rt_env_get with existing variable...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Set a test variable */
    rt_env_set("TEST_ENV_GET", "hello world");

    char *value = rt_env_get(arena, "TEST_ENV_GET");
    TEST_ASSERT_NOT_NULL(value, "rt_env_get should return non-NULL for existing variable");
    TEST_ASSERT_STR_EQ(value, "hello world", "rt_env_get should return correct value");

    /* Clean up */
    rt_env_remove("TEST_ENV_GET");
    rt_arena_destroy(arena);
}

void test_rt_env_get_missing_variable()
{
    printf("Testing rt_env_get with missing variable...\n");

    RtArena *arena = rt_arena_create(NULL);

    char *value = rt_env_get(arena, "NONEXISTENT_VAR_12345");
    TEST_ASSERT_NULL(value, "rt_env_get should return NULL for missing variable");

    rt_arena_destroy(arena);
}

void test_rt_env_get_empty_value()
{
#ifdef _WIN32
    /* Skip on Windows - SetEnvironmentVariableA with empty string removes the variable */
    printf("Testing rt_env_get with empty value... (skipped on Windows)\n");
#else
    printf("Testing rt_env_get with empty value...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Set a test variable with empty string */
    rt_env_set("TEST_ENV_EMPTY", "");

    char *value = rt_env_get(arena, "TEST_ENV_EMPTY");
    TEST_ASSERT_NOT_NULL(value, "rt_env_get should return non-NULL for empty value");
    TEST_ASSERT_STR_EQ(value, "", "rt_env_get should return empty string");

    /* Clean up */
    rt_env_remove("TEST_ENV_EMPTY");
    rt_arena_destroy(arena);
#endif
}

/* ============================================================================
 * rt_env_get_default() Tests
 * ============================================================================ */

void test_rt_env_get_default_existing_variable()
{
    printf("Testing rt_env_get_default with existing variable...\n");

    RtArena *arena = rt_arena_create(NULL);

    rt_env_set("TEST_ENV_DEFAULT", "actual_value");

    char *value = rt_env_get_default(arena, "TEST_ENV_DEFAULT", "default_value");
    TEST_ASSERT_NOT_NULL(value, "rt_env_get_default should return non-NULL");
    TEST_ASSERT_STR_EQ(value, "actual_value", "rt_env_get_default should return actual value when set");

    rt_env_remove("TEST_ENV_DEFAULT");
    rt_arena_destroy(arena);
}

void test_rt_env_get_default_missing_variable()
{
    printf("Testing rt_env_get_default with missing variable...\n");

    RtArena *arena = rt_arena_create(NULL);

    char *value = rt_env_get_default(arena, "NONEXISTENT_VAR_67890", "default_value");
    TEST_ASSERT_NOT_NULL(value, "rt_env_get_default should return non-NULL");
    TEST_ASSERT_STR_EQ(value, "default_value", "rt_env_get_default should return default when not set");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_env_set() Tests
 * ============================================================================ */

void test_rt_env_set_new_variable()
{
    printf("Testing rt_env_set with new variable...\n");

    int result = rt_env_set("TEST_SET_NEW", "new_value");
    TEST_ASSERT(result == 1, "rt_env_set should return 1 on success");

    RtArena *arena = rt_arena_create(NULL);
    char *value = rt_env_get(arena, "TEST_SET_NEW");
    TEST_ASSERT_STR_EQ(value, "new_value", "Variable should have been set correctly");

    rt_env_remove("TEST_SET_NEW");
    rt_arena_destroy(arena);
}

void test_rt_env_set_overwrite_variable()
{
    printf("Testing rt_env_set overwriting existing variable...\n");

    rt_env_set("TEST_SET_OVERWRITE", "original");
    rt_env_set("TEST_SET_OVERWRITE", "updated");

    RtArena *arena = rt_arena_create(NULL);
    char *value = rt_env_get(arena, "TEST_SET_OVERWRITE");
    TEST_ASSERT_STR_EQ(value, "updated", "Variable should have been updated");

    rt_env_remove("TEST_SET_OVERWRITE");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_env_remove() Tests
 * ============================================================================ */

void test_rt_env_remove_existing_variable()
{
    printf("Testing rt_env_remove with existing variable...\n");

    rt_env_set("TEST_REMOVE_EXISTS", "value");

    int result = rt_env_remove("TEST_REMOVE_EXISTS");
    TEST_ASSERT(result == 1, "rt_env_remove should return 1 when variable existed");

    TEST_ASSERT(rt_env_has("TEST_REMOVE_EXISTS") == 0, "Variable should be removed");
}

void test_rt_env_remove_missing_variable()
{
    printf("Testing rt_env_remove with missing variable...\n");

    int result = rt_env_remove("NONEXISTENT_VAR_REMOVE_99999");
    TEST_ASSERT(result == 0, "rt_env_remove should return 0 when variable didn't exist");
}

/* ============================================================================
 * rt_env_has() Tests
 * ============================================================================ */

void test_rt_env_has_existing_variable()
{
    printf("Testing rt_env_has with existing variable...\n");

    rt_env_set("TEST_HAS_EXISTS", "value");

    int result = rt_env_has("TEST_HAS_EXISTS");
    TEST_ASSERT(result == 1, "rt_env_has should return 1 for existing variable");

    rt_env_remove("TEST_HAS_EXISTS");
}

void test_rt_env_has_empty_variable()
{
    printf("Testing rt_env_has with empty variable...\n");

    rt_env_set("TEST_HAS_EMPTY", "");

    int result = rt_env_has("TEST_HAS_EMPTY");
    TEST_ASSERT(result == 1, "rt_env_has should return 1 for empty variable");

    rt_env_remove("TEST_HAS_EMPTY");
}

void test_rt_env_has_missing_variable()
{
    printf("Testing rt_env_has with missing variable...\n");

    int result = rt_env_has("NONEXISTENT_VAR_HAS_88888");
    TEST_ASSERT(result == 0, "rt_env_has should return 0 for missing variable");
}

/* ============================================================================
 * rt_env_get_int() Tests
 * ============================================================================ */

void test_rt_env_get_int_valid()
{
    printf("Testing rt_env_get_int with valid integer...\n");

    rt_env_set("TEST_INT", "42");

    int success;
    long value = rt_env_get_int("TEST_INT", &success);
    TEST_ASSERT(success == 1, "rt_env_get_int should succeed for valid integer");
    TEST_ASSERT_EQ(value, 42, "rt_env_get_int should return correct value");

    rt_env_remove("TEST_INT");
}

void test_rt_env_get_int_negative()
{
    printf("Testing rt_env_get_int with negative integer...\n");

    rt_env_set("TEST_INT_NEG", "-123");

    int success;
    long value = rt_env_get_int("TEST_INT_NEG", &success);
    TEST_ASSERT(success == 1, "rt_env_get_int should succeed for negative integer");
    TEST_ASSERT_EQ(value, -123, "rt_env_get_int should return correct negative value");

    rt_env_remove("TEST_INT_NEG");
}

void test_rt_env_get_int_invalid()
{
    printf("Testing rt_env_get_int with invalid integer...\n");

    rt_env_set("TEST_INT_INVALID", "not_a_number");

    int success;
    rt_env_get_int("TEST_INT_INVALID", &success);
    TEST_ASSERT(success == 0, "rt_env_get_int should fail for invalid integer");

    rt_env_remove("TEST_INT_INVALID");
}

void test_rt_env_get_int_missing()
{
    printf("Testing rt_env_get_int with missing variable...\n");

    int success;
    rt_env_get_int("NONEXISTENT_VAR_INT", &success);
    TEST_ASSERT(success == 0, "rt_env_get_int should fail for missing variable");
}

void test_rt_env_get_int_default()
{
    printf("Testing rt_env_get_int_default...\n");

    /* Test with missing variable */
    long value = rt_env_get_int_default("NONEXISTENT_VAR_INT_DEFAULT", 100);
    TEST_ASSERT_EQ(value, 100, "rt_env_get_int_default should return default for missing variable");

    /* Test with existing valid variable */
    rt_env_set("TEST_INT_DEFAULT", "200");
    value = rt_env_get_int_default("TEST_INT_DEFAULT", 100);
    TEST_ASSERT_EQ(value, 200, "rt_env_get_int_default should return actual value when set");

    rt_env_remove("TEST_INT_DEFAULT");
}

/* ============================================================================
 * rt_env_get_long() Tests
 * ============================================================================ */

void test_rt_env_get_long_valid()
{
    printf("Testing rt_env_get_long with valid long...\n");

    rt_env_set("TEST_LONG", "9223372036854775807");  /* LLONG_MAX */

    int success;
    long long value = rt_env_get_long("TEST_LONG", &success);
    TEST_ASSERT(success == 1, "rt_env_get_long should succeed for valid long");
    TEST_ASSERT(value == 9223372036854775807LL, "rt_env_get_long should return correct value");

    rt_env_remove("TEST_LONG");
}

void test_rt_env_get_long_default()
{
    printf("Testing rt_env_get_long_default...\n");

    long long value = rt_env_get_long_default("NONEXISTENT_VAR_LONG", 1234567890LL);
    TEST_ASSERT(value == 1234567890LL, "rt_env_get_long_default should return default");
}

/* ============================================================================
 * rt_env_get_double() Tests
 * ============================================================================ */

void test_rt_env_get_double_valid()
{
    printf("Testing rt_env_get_double with valid double...\n");

    rt_env_set("TEST_DOUBLE", "3.14159");

    int success;
    double value = rt_env_get_double("TEST_DOUBLE", &success);
    TEST_ASSERT(success == 1, "rt_env_get_double should succeed for valid double");
    TEST_ASSERT(value > 3.14 && value < 3.15, "rt_env_get_double should return correct value");

    rt_env_remove("TEST_DOUBLE");
}

void test_rt_env_get_double_integer()
{
    printf("Testing rt_env_get_double with integer value...\n");

    rt_env_set("TEST_DOUBLE_INT", "42");

    int success;
    double value = rt_env_get_double("TEST_DOUBLE_INT", &success);
    TEST_ASSERT(success == 1, "rt_env_get_double should succeed for integer");
    TEST_ASSERT(value == 42.0, "rt_env_get_double should return correct value");

    rt_env_remove("TEST_DOUBLE_INT");
}

void test_rt_env_get_double_default()
{
    printf("Testing rt_env_get_double_default...\n");

    double value = rt_env_get_double_default("NONEXISTENT_VAR_DOUBLE", 2.71828);
    TEST_ASSERT(value > 2.71 && value < 2.72, "rt_env_get_double_default should return default");
}

/* ============================================================================
 * rt_env_get_bool() Tests - Boolean Parsing
 * ============================================================================ */

void test_rt_env_get_bool_true_values()
{
    printf("Testing rt_env_get_bool with truthy values...\n");

    const char *truthy[] = {"true", "TRUE", "True", "1", "yes", "YES", "on", "ON"};

    for (size_t i = 0; i < sizeof(truthy) / sizeof(truthy[0]); i++) {
        char var_name[64];
        snprintf(var_name, sizeof(var_name), "TEST_BOOL_TRUE_%zu", i);

        rt_env_set(var_name, truthy[i]);

        int success;
        int value = rt_env_get_bool(var_name, &success);

        if (success != 1 || value != 1) {
            printf("  FAILED for '%s'\n", truthy[i]);
        }
        TEST_ASSERT(success == 1, "rt_env_get_bool should succeed for truthy value");
        TEST_ASSERT(value == 1, "rt_env_get_bool should return 1 for truthy value");

        rt_env_remove(var_name);
    }

    printf("  All truthy values parsed correctly\n");
}

void test_rt_env_get_bool_false_values()
{
    printf("Testing rt_env_get_bool with falsy values...\n");

    const char *falsy[] = {"false", "FALSE", "False", "0", "no", "NO", "off", "OFF"};

    for (size_t i = 0; i < sizeof(falsy) / sizeof(falsy[0]); i++) {
        char var_name[64];
        snprintf(var_name, sizeof(var_name), "TEST_BOOL_FALSE_%zu", i);

        rt_env_set(var_name, falsy[i]);

        int success;
        int value = rt_env_get_bool(var_name, &success);

        if (success != 1 || value != 0) {
            printf("  FAILED for '%s'\n", falsy[i]);
        }
        TEST_ASSERT(success == 1, "rt_env_get_bool should succeed for falsy value");
        TEST_ASSERT(value == 0, "rt_env_get_bool should return 0 for falsy value");

        rt_env_remove(var_name);
    }

    printf("  All falsy values parsed correctly\n");
}

void test_rt_env_get_bool_invalid()
{
    printf("Testing rt_env_get_bool with invalid values...\n");

    const char *invalid[] = {"maybe", "2", "", "truthy", "faux"};

    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        char var_name[64];
        snprintf(var_name, sizeof(var_name), "TEST_BOOL_INVALID_%zu", i);

        rt_env_set(var_name, invalid[i]);

        int success;
        rt_env_get_bool(var_name, &success);

        if (success != 0) {
            printf("  FAILED for '%s' - should not parse as bool\n", invalid[i]);
        }
        TEST_ASSERT(success == 0, "rt_env_get_bool should fail for invalid value");

        rt_env_remove(var_name);
    }

    printf("  All invalid values rejected correctly\n");
}

void test_rt_env_get_bool_default()
{
    printf("Testing rt_env_get_bool_default...\n");

    /* Test with missing variable */
    int value = rt_env_get_bool_default("NONEXISTENT_VAR_BOOL", 1);
    TEST_ASSERT(value == 1, "rt_env_get_bool_default should return default for missing variable");

    /* Test with existing variable */
    rt_env_set("TEST_BOOL_DEFAULT", "false");
    value = rt_env_get_bool_default("TEST_BOOL_DEFAULT", 1);
    TEST_ASSERT(value == 0, "rt_env_get_bool_default should return actual value when set");

    rt_env_remove("TEST_BOOL_DEFAULT");
}

/* ============================================================================
 * rt_env_list() Tests
 * ============================================================================ */

void test_rt_env_list_basic()
{
    printf("Testing rt_env_list basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Set some test variables */
    rt_env_set("TEST_LIST_A", "value_a");
    rt_env_set("TEST_LIST_B", "value_b");

    char ***list = rt_env_list(arena);
    TEST_ASSERT_NOT_NULL(list, "rt_env_list should return non-NULL");

    size_t count = rt_array_length(list);
    TEST_ASSERT(count >= 2, "rt_env_list should return at least our test variables");

    /* Check that our test variables are in the list */
    int found_a = 0, found_b = 0;
    for (size_t i = 0; i < count; i++) {
        char **pair = list[i];
        TEST_ASSERT_NOT_NULL(pair, "Each entry should be non-NULL");

        if (strcmp(pair[0], "TEST_LIST_A") == 0) {
            TEST_ASSERT_STR_EQ(pair[1], "value_a", "TEST_LIST_A should have correct value");
            found_a = 1;
        }
        if (strcmp(pair[0], "TEST_LIST_B") == 0) {
            TEST_ASSERT_STR_EQ(pair[1], "value_b", "TEST_LIST_B should have correct value");
            found_b = 1;
        }
    }

    TEST_ASSERT(found_a, "TEST_LIST_A should be found in list");
    TEST_ASSERT(found_b, "TEST_LIST_B should be found in list");

    /* Clean up */
    rt_env_remove("TEST_LIST_A");
    rt_env_remove("TEST_LIST_B");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_env_names() Tests
 * ============================================================================ */

void test_rt_env_names_basic()
{
    printf("Testing rt_env_names basic functionality...\n");

    RtArena *arena = rt_arena_create(NULL);

    /* Set a test variable */
    rt_env_set("TEST_NAMES_VAR", "value");

    char **names = rt_env_names(arena);
    TEST_ASSERT_NOT_NULL(names, "rt_env_names should return non-NULL");

    size_t count = rt_array_length(names);
    TEST_ASSERT(count >= 1, "rt_env_names should return at least our test variable");

    /* Check that our test variable is in the list */
    int found = 0;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(names[i], "TEST_NAMES_VAR") == 0) {
            found = 1;
            break;
        }
    }

    TEST_ASSERT(found, "TEST_NAMES_VAR should be found in names list");

    /* Clean up */
    rt_env_remove("TEST_NAMES_VAR");
    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_env_main()
{
    printf("\n=== Runtime Environment Tests ===\n\n");

    /* rt_env_get tests */
    test_rt_env_get_existing_variable();
    test_rt_env_get_missing_variable();
    test_rt_env_get_empty_value();

    /* rt_env_get_default tests */
    test_rt_env_get_default_existing_variable();
    test_rt_env_get_default_missing_variable();

    /* rt_env_set tests */
    test_rt_env_set_new_variable();
    test_rt_env_set_overwrite_variable();

    /* rt_env_remove tests */
    test_rt_env_remove_existing_variable();
    test_rt_env_remove_missing_variable();

    /* rt_env_has tests */
    test_rt_env_has_existing_variable();
    test_rt_env_has_empty_variable();
    test_rt_env_has_missing_variable();

    /* rt_env_get_int tests */
    test_rt_env_get_int_valid();
    test_rt_env_get_int_negative();
    test_rt_env_get_int_invalid();
    test_rt_env_get_int_missing();
    test_rt_env_get_int_default();

    /* rt_env_get_long tests */
    test_rt_env_get_long_valid();
    test_rt_env_get_long_default();

    /* rt_env_get_double tests */
    test_rt_env_get_double_valid();
    test_rt_env_get_double_integer();
    test_rt_env_get_double_default();

    /* rt_env_get_bool tests - comprehensive boolean parsing */
    test_rt_env_get_bool_true_values();
    test_rt_env_get_bool_false_values();
    test_rt_env_get_bool_invalid();
    test_rt_env_get_bool_default();

    /* rt_env_list tests */
    test_rt_env_list_basic();

    /* rt_env_names tests */
    test_rt_env_names_basic();

    printf("\n=== All Runtime Environment Tests Passed ===\n\n");
}
