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
#include "../test_harness.h"
#include "../debug.h"

/* ============================================================================
 * rt_env_get() Tests
 * ============================================================================ */

static void test_rt_env_get_existing_variable(void)
{
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

static void test_rt_env_get_missing_variable(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *value = rt_env_get(arena, "NONEXISTENT_VAR_12345");
    TEST_ASSERT_NULL(value, "rt_env_get should return NULL for missing variable");

    rt_arena_destroy(arena);
}

static void test_rt_env_get_empty_value(void)
{
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
}

/* ============================================================================
 * rt_env_get_default() Tests
 * ============================================================================ */

static void test_rt_env_get_default_existing_variable(void)
{
    RtArena *arena = rt_arena_create(NULL);

    rt_env_set("TEST_ENV_DEFAULT", "actual_value");

    char *value = rt_env_get_default(arena, "TEST_ENV_DEFAULT", "default_value");
    TEST_ASSERT_NOT_NULL(value, "rt_env_get_default should return non-NULL");
    TEST_ASSERT_STR_EQ(value, "actual_value", "rt_env_get_default should return actual value when set");

    rt_env_remove("TEST_ENV_DEFAULT");
    rt_arena_destroy(arena);
}

static void test_rt_env_get_default_missing_variable(void)
{
    RtArena *arena = rt_arena_create(NULL);

    char *value = rt_env_get_default(arena, "NONEXISTENT_VAR_67890", "default_value");
    TEST_ASSERT_NOT_NULL(value, "rt_env_get_default should return non-NULL");
    TEST_ASSERT_STR_EQ(value, "default_value", "rt_env_get_default should return default when not set");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * rt_env_set() Tests
 * ============================================================================ */

static void test_rt_env_set_new_variable(void)
{
    int result = rt_env_set("TEST_SET_NEW", "new_value");
    TEST_ASSERT(result == 1, "rt_env_set should return 1 on success");

    RtArena *arena = rt_arena_create(NULL);
    char *value = rt_env_get(arena, "TEST_SET_NEW");
    TEST_ASSERT_STR_EQ(value, "new_value", "Variable should have been set correctly");

    rt_env_remove("TEST_SET_NEW");
    rt_arena_destroy(arena);
}

static void test_rt_env_set_overwrite_variable(void)
{
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

static void test_rt_env_remove_existing_variable(void)
{
    rt_env_set("TEST_REMOVE_EXISTS", "value");

    int result = rt_env_remove("TEST_REMOVE_EXISTS");
    TEST_ASSERT(result == 1, "rt_env_remove should return 1 when variable existed");

    TEST_ASSERT(rt_env_has("TEST_REMOVE_EXISTS") == 0, "Variable should be removed");
}

static void test_rt_env_remove_missing_variable(void)
{
    int result = rt_env_remove("NONEXISTENT_VAR_REMOVE_99999");
    TEST_ASSERT(result == 0, "rt_env_remove should return 0 when variable didn't exist");
}

/* ============================================================================
 * rt_env_has() Tests
 * ============================================================================ */

static void test_rt_env_has_existing_variable(void)
{
    rt_env_set("TEST_HAS_EXISTS", "value");

    int result = rt_env_has("TEST_HAS_EXISTS");
    TEST_ASSERT(result == 1, "rt_env_has should return 1 for existing variable");

    rt_env_remove("TEST_HAS_EXISTS");
}

static void test_rt_env_has_empty_variable(void)
{
    rt_env_set("TEST_HAS_EMPTY", "");

    int result = rt_env_has("TEST_HAS_EMPTY");
    TEST_ASSERT(result == 1, "rt_env_has should return 1 for empty variable");

    rt_env_remove("TEST_HAS_EMPTY");
}

static void test_rt_env_has_missing_variable(void)
{
    int result = rt_env_has("NONEXISTENT_VAR_HAS_88888");
    TEST_ASSERT(result == 0, "rt_env_has should return 0 for missing variable");
}

/* ============================================================================
 * rt_env_get_int() Tests
 * ============================================================================ */

static void test_rt_env_get_int_valid(void)
{
    rt_env_set("TEST_INT", "42");

    int success;
    long value = rt_env_get_int("TEST_INT", &success);
    TEST_ASSERT(success == 1, "rt_env_get_int should succeed for valid integer");
    TEST_ASSERT_EQ(value, 42, "rt_env_get_int should return correct value");

    rt_env_remove("TEST_INT");
}

static void test_rt_env_get_int_negative(void)
{
    rt_env_set("TEST_INT_NEG", "-123");

    int success;
    long value = rt_env_get_int("TEST_INT_NEG", &success);
    TEST_ASSERT(success == 1, "rt_env_get_int should succeed for negative integer");
    TEST_ASSERT_EQ(value, -123, "rt_env_get_int should return correct negative value");

    rt_env_remove("TEST_INT_NEG");
}

static void test_rt_env_get_int_invalid(void)
{
    rt_env_set("TEST_INT_INVALID", "not_a_number");

    int success;
    rt_env_get_int("TEST_INT_INVALID", &success);
    TEST_ASSERT(success == 0, "rt_env_get_int should fail for invalid integer");

    rt_env_remove("TEST_INT_INVALID");
}

static void test_rt_env_get_int_missing(void)
{
    int success;
    rt_env_get_int("NONEXISTENT_VAR_INT", &success);
    TEST_ASSERT(success == 0, "rt_env_get_int should fail for missing variable");
}

static void test_rt_env_get_int_default(void)
{
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

static void test_rt_env_get_long_valid(void)
{
    rt_env_set("TEST_LONG", "9223372036854775807");  /* LLONG_MAX */

    int success;
    long long value = rt_env_get_long("TEST_LONG", &success);
    TEST_ASSERT(success == 1, "rt_env_get_long should succeed for valid long");
    TEST_ASSERT(value == 9223372036854775807LL, "rt_env_get_long should return correct value");

    rt_env_remove("TEST_LONG");
}

static void test_rt_env_get_long_default(void)
{
    long long value = rt_env_get_long_default("NONEXISTENT_VAR_LONG", 1234567890LL);
    TEST_ASSERT(value == 1234567890LL, "rt_env_get_long_default should return default");
}

/* ============================================================================
 * rt_env_get_double() Tests
 * ============================================================================ */

static void test_rt_env_get_double_valid(void)
{
    rt_env_set("TEST_DOUBLE", "3.14159");

    int success;
    double value = rt_env_get_double("TEST_DOUBLE", &success);
    TEST_ASSERT(success == 1, "rt_env_get_double should succeed for valid double");
    TEST_ASSERT(value > 3.14 && value < 3.15, "rt_env_get_double should return correct value");

    rt_env_remove("TEST_DOUBLE");
}

static void test_rt_env_get_double_integer(void)
{
    rt_env_set("TEST_DOUBLE_INT", "42");

    int success;
    double value = rt_env_get_double("TEST_DOUBLE_INT", &success);
    TEST_ASSERT(success == 1, "rt_env_get_double should succeed for integer");
    TEST_ASSERT(value == 42.0, "rt_env_get_double should return correct value");

    rt_env_remove("TEST_DOUBLE_INT");
}

static void test_rt_env_get_double_default(void)
{
    double value = rt_env_get_double_default("NONEXISTENT_VAR_DOUBLE", 2.71828);
    TEST_ASSERT(value > 2.71 && value < 2.72, "rt_env_get_double_default should return default");
}

/* ============================================================================
 * rt_env_get_bool() Tests - Boolean Parsing
 * ============================================================================ */

static void test_rt_env_get_bool_true_values(void)
{
    const char *truthy[] = {"true", "TRUE", "True", "1", "yes", "YES", "on", "ON"};

    for (size_t i = 0; i < sizeof(truthy) / sizeof(truthy[0]); i++) {
        char var_name[64];
        snprintf(var_name, sizeof(var_name), "TEST_BOOL_TRUE_%zu", i);

        rt_env_set(var_name, truthy[i]);

        int success;
        int value = rt_env_get_bool(var_name, &success);

        TEST_ASSERT(success == 1, "rt_env_get_bool should succeed for truthy value");
        TEST_ASSERT(value == 1, "rt_env_get_bool should return 1 for truthy value");

        rt_env_remove(var_name);
    }
}

static void test_rt_env_get_bool_false_values(void)
{
    const char *falsy[] = {"false", "FALSE", "False", "0", "no", "NO", "off", "OFF"};

    for (size_t i = 0; i < sizeof(falsy) / sizeof(falsy[0]); i++) {
        char var_name[64];
        snprintf(var_name, sizeof(var_name), "TEST_BOOL_FALSE_%zu", i);

        rt_env_set(var_name, falsy[i]);

        int success;
        int value = rt_env_get_bool(var_name, &success);

        TEST_ASSERT(success == 1, "rt_env_get_bool should succeed for falsy value");
        TEST_ASSERT(value == 0, "rt_env_get_bool should return 0 for falsy value");

        rt_env_remove(var_name);
    }
}

static void test_rt_env_get_bool_invalid(void)
{
    const char *invalid[] = {"maybe", "2", "", "truthy", "faux"};

    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        char var_name[64];
        snprintf(var_name, sizeof(var_name), "TEST_BOOL_INVALID_%zu", i);

        rt_env_set(var_name, invalid[i]);

        int success;
        rt_env_get_bool(var_name, &success);

        TEST_ASSERT(success == 0, "rt_env_get_bool should fail for invalid value");

        rt_env_remove(var_name);
    }
}

static void test_rt_env_get_bool_default(void)
{
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

static void test_rt_env_list_basic(void)
{
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

static void test_rt_env_names_basic(void)
{
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

void test_rt_env_main(void)
{
    TEST_SECTION("Runtime Environment");

    /* rt_env_get tests */
    TEST_RUN("get_existing_variable", test_rt_env_get_existing_variable);
    TEST_RUN("get_missing_variable", test_rt_env_get_missing_variable);
    TEST_RUN("get_empty_value", test_rt_env_get_empty_value);

    /* rt_env_get_default tests */
    TEST_RUN("get_default_existing_variable", test_rt_env_get_default_existing_variable);
    TEST_RUN("get_default_missing_variable", test_rt_env_get_default_missing_variable);

    /* rt_env_set tests */
    TEST_RUN("set_new_variable", test_rt_env_set_new_variable);
    TEST_RUN("set_overwrite_variable", test_rt_env_set_overwrite_variable);

    /* rt_env_remove tests */
    TEST_RUN("remove_existing_variable", test_rt_env_remove_existing_variable);
    TEST_RUN("remove_missing_variable", test_rt_env_remove_missing_variable);

    /* rt_env_has tests */
    TEST_RUN("has_existing_variable", test_rt_env_has_existing_variable);
    TEST_RUN("has_empty_variable", test_rt_env_has_empty_variable);
    TEST_RUN("has_missing_variable", test_rt_env_has_missing_variable);

    /* rt_env_get_int tests */
    TEST_RUN("get_int_valid", test_rt_env_get_int_valid);
    TEST_RUN("get_int_negative", test_rt_env_get_int_negative);
    TEST_RUN("get_int_invalid", test_rt_env_get_int_invalid);
    TEST_RUN("get_int_missing", test_rt_env_get_int_missing);
    TEST_RUN("get_int_default", test_rt_env_get_int_default);

    /* rt_env_get_long tests */
    TEST_RUN("get_long_valid", test_rt_env_get_long_valid);
    TEST_RUN("get_long_default", test_rt_env_get_long_default);

    /* rt_env_get_double tests */
    TEST_RUN("get_double_valid", test_rt_env_get_double_valid);
    TEST_RUN("get_double_integer", test_rt_env_get_double_integer);
    TEST_RUN("get_double_default", test_rt_env_get_double_default);

    /* rt_env_get_bool tests - comprehensive boolean parsing */
    TEST_RUN("get_bool_true_values", test_rt_env_get_bool_true_values);
    TEST_RUN("get_bool_false_values", test_rt_env_get_bool_false_values);
    TEST_RUN("get_bool_invalid", test_rt_env_get_bool_invalid);
    TEST_RUN("get_bool_default", test_rt_env_get_bool_default);

    /* rt_env_list tests */
    TEST_RUN("list_basic", test_rt_env_list_basic);

    /* rt_env_names tests */
    TEST_RUN("names_basic", test_rt_env_names_basic);
}
