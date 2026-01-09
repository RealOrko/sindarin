// tests/unit/runtime_random_tests_many.c
// Tests for runtime random batch generation (*_many) functions

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../../src/arena.h"
#include "../../src/runtime/runtime_random.h"
#include "../../src/runtime/runtime_arena.h"
#include "../../src/debug.h"
#include "../test_utils.h"
#include "../test_harness.h"

/* ============================================================================
 * Static Batch Generation Tests
 * ============================================================================ */

static void test_rt_random_static_int_many_count_and_range(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 1000;
    long long min = 10;
    long long max = 100;

    long long *arr = rt_random_static_int_many(arena, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] <= max,
                   "All values should be in range [min, max]");
    }

    /* Verify distribution is reasonable */
    int range_size = (int)(max - min + 1);
    int *buckets = rt_arena_calloc(arena, (size_t)range_size, sizeof(int));
    for (long i = 0; i < count; i++) {
        buckets[arr[i] - min]++;
    }

    /* Each bucket should have some values (expect ~11 per bucket for 91 buckets, 1000 samples) */
    int empty_buckets = 0;
    for (int i = 0; i < range_size; i++) {
        if (buckets[i] == 0) empty_buckets++;
    }
    TEST_ASSERT(empty_buckets < range_size / 4, "Distribution should cover most of range");

    rt_arena_destroy(arena);
}

static void test_rt_random_static_int_many_null_arena(void)
{
    long long *arr = rt_random_static_int_many(NULL, 0, 100, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");
}

static void test_rt_random_static_int_many_zero_count(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long long *arr1 = rt_random_static_int_many(arena, 0, 100, 0);
    TEST_ASSERT(arr1 == NULL, "Zero count should return NULL");

    long long *arr2 = rt_random_static_int_many(arena, 0, 100, -5);
    TEST_ASSERT(arr2 == NULL, "Negative count should return NULL");

    rt_arena_destroy(arena);
}

static void test_rt_random_static_long_many_count_and_range(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 500;
    long long min = 1000000000LL;
    long long max = 2000000000LL;

    long long *arr = rt_random_static_long_many(arena, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] <= max,
                   "All longs should be in range [min, max]");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_static_long_many_null_arena(void)
{
    long long *arr = rt_random_static_long_many(NULL, 0, 100, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");
}

static void test_rt_random_static_double_many_count_and_range(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 1000;
    double min = 0.0;
    double max = 1.0;

    double *arr = rt_random_static_double_many(arena, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range [min, max) */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] < max,
                   "All doubles should be in range [min, max)");
    }

    /* Verify distribution - check mean is roughly 0.5 */
    double sum = 0.0;
    for (long i = 0; i < count; i++) {
        sum += arr[i];
    }
    double mean = sum / count;
    TEST_ASSERT(mean > 0.4 && mean < 0.6, "Mean should be approximately 0.5");

    rt_arena_destroy(arena);
}

static void test_rt_random_static_double_many_null_arena(void)
{
    double *arr = rt_random_static_double_many(NULL, 0.0, 1.0, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");
}

static void test_rt_random_static_bool_many_count(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 1000;
    int *arr = rt_random_static_bool_many(arena, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are 0 or 1, count trues */
    int true_count = 0;
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] == 0 || arr[i] == 1, "All bools should be 0 or 1");
        if (arr[i]) true_count++;
    }

    /* Should be roughly 50/50 */
    TEST_ASSERT(true_count > 400 && true_count < 600,
               "Bool distribution should be roughly 50/50");

    rt_arena_destroy(arena);
}

static void test_rt_random_static_bool_many_null_arena(void)
{
    int *arr = rt_random_static_bool_many(NULL, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");
}

static void test_rt_random_static_gaussian_many_count_and_distribution(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    long count = 10000;
    double mean = 100.0;
    double stddev = 15.0;

    double *arr = rt_random_static_gaussian_many(arena, mean, stddev, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Calculate actual mean and stddev */
    double sum = 0.0;
    double sum_sq = 0.0;
    for (long i = 0; i < count; i++) {
        sum += arr[i];
        sum_sq += arr[i] * arr[i];
    }

    double actual_mean = sum / count;
    double variance = (sum_sq / count) - (actual_mean * actual_mean);
    double actual_stddev = sqrt(variance);

    /* Mean should be close to target */
    double mean_error = fabs(actual_mean - mean);
    TEST_ASSERT(mean_error < 1.0, "Mean should be close to target");

    /* Standard deviation should be close to target */
    double stddev_error = fabs(actual_stddev - stddev);
    TEST_ASSERT(stddev_error < 1.0, "Stddev should be close to target");

    rt_arena_destroy(arena);
}

static void test_rt_random_static_gaussian_many_null_arena(void)
{
    double *arr = rt_random_static_gaussian_many(NULL, 0.0, 1.0, 10);
    TEST_ASSERT(arr == NULL, "NULL arena should return NULL");
}

/* ============================================================================
 * Instance Batch Generation Tests
 * ============================================================================ */

static void test_rt_random_int_many_count_and_range(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 1000;
    long long min = 10;
    long long max = 100;

    long long *arr = rt_random_int_many(arena, rng, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] <= max,
                   "All values should be in range [min, max]");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_int_many_null_args(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long long *arr1 = rt_random_int_many(NULL, rng, 0, 100, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    long long *arr2 = rt_random_int_many(arena, NULL, 0, 100, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    rt_arena_destroy(arena);
}

static void test_rt_random_int_many_reproducibility(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    long long *arr1 = rt_random_int_many(arena, rng1, 0, 1000, count);
    long long *arr2 = rt_random_int_many(arena, rng2, 0, 1000, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    /* Same seed should produce identical arrays */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_long_many_count_and_range(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 500;
    long long min = 1000000000LL;
    long long max = 2000000000LL;

    long long *arr = rt_random_long_many(arena, rng, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] <= max,
                   "All longs should be in range [min, max]");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_long_many_null_args(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long long *arr1 = rt_random_long_many(NULL, rng, 0, 100, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    long long *arr2 = rt_random_long_many(arena, NULL, 0, 100, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    rt_arena_destroy(arena);
}

static void test_rt_random_long_many_reproducibility(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    long long *arr1 = rt_random_long_many(arena, rng1, 0, 1000000000LL, count);
    long long *arr2 = rt_random_long_many(arena, rng2, 0, 1000000000LL, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_double_many_count_and_range(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 1000;
    double min = 0.0;
    double max = 1.0;

    double *arr = rt_random_double_many(arena, rng, min, max, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are in range [min, max) */
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] >= min && arr[i] < max,
                   "All doubles should be in range [min, max)");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_double_many_null_args(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    double *arr1 = rt_random_double_many(NULL, rng, 0.0, 1.0, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    double *arr2 = rt_random_double_many(arena, NULL, 0.0, 1.0, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    rt_arena_destroy(arena);
}

static void test_rt_random_double_many_reproducibility(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    double *arr1 = rt_random_double_many(arena, rng1, 0.0, 1.0, count);
    double *arr2 = rt_random_double_many(arena, rng2, 0.0, 1.0, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_bool_many_count(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 1000;
    int *arr = rt_random_bool_many(arena, rng, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Verify all values are 0 or 1, count trues */
    int true_count = 0;
    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr[i] == 0 || arr[i] == 1, "All bools should be 0 or 1");
        if (arr[i]) true_count++;
    }

    /* Should be roughly 50/50 */
    TEST_ASSERT(true_count > 400 && true_count < 600,
               "Bool distribution should be roughly 50/50");

    rt_arena_destroy(arena);
}

static void test_rt_random_bool_many_null_args(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    int *arr1 = rt_random_bool_many(NULL, rng, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    int *arr2 = rt_random_bool_many(arena, NULL, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    rt_arena_destroy(arena);
}

static void test_rt_random_bool_many_reproducibility(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    int *arr1 = rt_random_bool_many(arena, rng1, count);
    int *arr2 = rt_random_bool_many(arena, rng2, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_gaussian_many_count_and_distribution(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    long count = 10000;
    double mean = 0.0;
    double stddev = 1.0;

    double *arr = rt_random_gaussian_many(arena, rng, mean, stddev, count);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Calculate actual mean and stddev */
    double sum = 0.0;
    double sum_sq = 0.0;
    for (long i = 0; i < count; i++) {
        sum += arr[i];
        sum_sq += arr[i] * arr[i];
    }

    double actual_mean = sum / count;
    double variance = (sum_sq / count) - (actual_mean * actual_mean);
    double actual_stddev = sqrt(variance);

    /* Standard normal should have mean ~0 and stddev ~1 */
    TEST_ASSERT(fabs(actual_mean) < 0.1, "Mean should be close to 0");
    TEST_ASSERT(fabs(actual_stddev - 1.0) < 0.1, "Stddev should be close to 1");

    rt_arena_destroy(arena);
}

static void test_rt_random_gaussian_many_null_args(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    double *arr1 = rt_random_gaussian_many(NULL, rng, 0.0, 1.0, 10);
    TEST_ASSERT(arr1 == NULL, "NULL arena should return NULL");

    double *arr2 = rt_random_gaussian_many(arena, NULL, 0.0, 1.0, 10);
    TEST_ASSERT(arr2 == NULL, "NULL rng should return NULL");

    rt_arena_destroy(arena);
}

static void test_rt_random_gaussian_many_reproducibility(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng1 = rt_random_create_with_seed(arena, 42);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 42);

    long count = 100;
    double *arr1 = rt_random_gaussian_many(arena, rng1, 0.0, 1.0, count);
    double *arr2 = rt_random_gaussian_many(arena, rng2, 0.0, 1.0, count);

    TEST_ASSERT_NOT_NULL(arr1, "arr1 should be created");
    TEST_ASSERT_NOT_NULL(arr2, "arr2 should be created");

    for (long i = 0; i < count; i++) {
        TEST_ASSERT(arr1[i] == arr2[i], "Same seed should produce identical arrays");
    }

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Performance Tests for Large Batches
 * ============================================================================ */

static void test_rt_random_batch_large_count(void)
{
    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Generate 100,000 values of each type */
    long large_count = 100000;

    long *ints = rt_random_int_many(arena, rng, 0, 1000000, large_count);
    TEST_ASSERT_NOT_NULL(ints, "Large int array should be created");

    long long *longs = rt_random_long_many(arena, rng, 0, 1000000000LL, large_count);
    TEST_ASSERT_NOT_NULL(longs, "Large long array should be created");

    double *doubles = rt_random_double_many(arena, rng, 0.0, 1.0, large_count);
    TEST_ASSERT_NOT_NULL(doubles, "Large double array should be created");

    int *bools = rt_random_bool_many(arena, rng, large_count);
    TEST_ASSERT_NOT_NULL(bools, "Large bool array should be created");

    double *gaussians = rt_random_gaussian_many(arena, rng, 0.0, 1.0, large_count);
    TEST_ASSERT_NOT_NULL(gaussians, "Large gaussian array should be created");

    /* Verify a few random samples are in range */
    TEST_ASSERT(ints[0] >= 0 && ints[0] <= 1000000, "First int in range");
    TEST_ASSERT(ints[large_count-1] >= 0 && ints[large_count-1] <= 1000000, "Last int in range");

    TEST_ASSERT(longs[0] >= 0 && longs[0] <= 1000000000LL, "First long in range");
    TEST_ASSERT(longs[large_count-1] >= 0 && longs[large_count-1] <= 1000000000LL, "Last long in range");

    TEST_ASSERT(doubles[0] >= 0.0 && doubles[0] < 1.0, "First double in range");
    TEST_ASSERT(doubles[large_count-1] >= 0.0 && doubles[large_count-1] < 1.0, "Last double in range");

    TEST_ASSERT(bools[0] == 0 || bools[0] == 1, "First bool valid");
    TEST_ASSERT(bools[large_count-1] == 0 || bools[large_count-1] == 1, "Last bool valid");

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_random_many_main(void)
{
    TEST_SECTION("Runtime Random Many");

    /* Static batch generation tests */
    TEST_RUN("static_int_many_count_and_range", test_rt_random_static_int_many_count_and_range);
    TEST_RUN("static_int_many_null_arena", test_rt_random_static_int_many_null_arena);
    TEST_RUN("static_int_many_zero_count", test_rt_random_static_int_many_zero_count);
    TEST_RUN("static_long_many_count_and_range", test_rt_random_static_long_many_count_and_range);
    TEST_RUN("static_long_many_null_arena", test_rt_random_static_long_many_null_arena);
    TEST_RUN("static_double_many_count_and_range", test_rt_random_static_double_many_count_and_range);
    TEST_RUN("static_double_many_null_arena", test_rt_random_static_double_many_null_arena);
    TEST_RUN("static_bool_many_count", test_rt_random_static_bool_many_count);
    TEST_RUN("static_bool_many_null_arena", test_rt_random_static_bool_many_null_arena);
    TEST_RUN("static_gaussian_many_count_and_distribution", test_rt_random_static_gaussian_many_count_and_distribution);
    TEST_RUN("static_gaussian_many_null_arena", test_rt_random_static_gaussian_many_null_arena);

    /* Instance batch generation tests */
    TEST_RUN("int_many_count_and_range", test_rt_random_int_many_count_and_range);
    TEST_RUN("int_many_null_args", test_rt_random_int_many_null_args);
    TEST_RUN("int_many_reproducibility", test_rt_random_int_many_reproducibility);
    TEST_RUN("long_many_count_and_range", test_rt_random_long_many_count_and_range);
    TEST_RUN("long_many_null_args", test_rt_random_long_many_null_args);
    TEST_RUN("long_many_reproducibility", test_rt_random_long_many_reproducibility);
    TEST_RUN("double_many_count_and_range", test_rt_random_double_many_count_and_range);
    TEST_RUN("double_many_null_args", test_rt_random_double_many_null_args);
    TEST_RUN("double_many_reproducibility", test_rt_random_double_many_reproducibility);
    TEST_RUN("bool_many_count", test_rt_random_bool_many_count);
    TEST_RUN("bool_many_null_args", test_rt_random_bool_many_null_args);
    TEST_RUN("bool_many_reproducibility", test_rt_random_bool_many_reproducibility);
    TEST_RUN("gaussian_many_count_and_distribution", test_rt_random_gaussian_many_count_and_distribution);
    TEST_RUN("gaussian_many_null_args", test_rt_random_gaussian_many_null_args);
    TEST_RUN("gaussian_many_reproducibility", test_rt_random_gaussian_many_reproducibility);

    /* Large batch performance test */
    TEST_RUN("batch_large_count", test_rt_random_batch_large_count);
}
