// tests/unit/runtime_random_tests_collection.c
// Tests for runtime random shuffle and sample operations

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../../src/arena.h"
#include "../../src/runtime/runtime_random.h"
#include "../../src/runtime/runtime_arena.h"
#include "../../src/runtime/runtime_array.h"
#include "../../src/debug.h"
#include "../test_utils.h"
#include "../test_harness.h"

/* ============================================================================
 * Shuffle Tests - Static Methods (OS Entropy)
 * ============================================================================
 * Tests for Fisher-Yates shuffle algorithm.
 * ============================================================================ */

static void test_rt_random_static_shuffle_long_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array {1, 2, 3, 4, 5} */
    long long data[] = {1, 2, 3, 4, 5};
    long long *arr = rt_array_create_long(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Calculate original sum */
    long long original_sum = 0;
    for (int i = 0; i < 5; i++) original_sum += arr[i];

    /* Shuffle multiple times and verify all elements present */
    for (int trial = 0; trial < 10; trial++) {
        rt_random_static_shuffle_long(arr);

        /* Verify all elements still present (sum unchanged) */
        long sum = 0;
        int found[5] = {0, 0, 0, 0, 0};
        for (int i = 0; i < 5; i++) {
            sum += arr[i];
            if (arr[i] >= 1 && arr[i] <= 5) {
                found[arr[i] - 1] = 1;
            }
        }

        TEST_ASSERT(sum == original_sum, "Sum should be unchanged after shuffle");

        int all_found = 1;
        for (int i = 0; i < 5; i++) {
            if (!found[i]) all_found = 0;
        }
        TEST_ASSERT(all_found, "All elements should be present after shuffle");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_shuffle_double_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    double *arr = rt_array_create_double(arena, 5, data);

    double original_sum = 0;
    for (int i = 0; i < 5; i++) original_sum += arr[i];

    rt_random_static_shuffle_double(arr);

    double sum = 0;
    for (int i = 0; i < 5; i++) sum += arr[i];

    TEST_ASSERT(fabs(sum - original_sum) < 0.001, "Sum should be unchanged after shuffle");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_shuffle_string_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    const char *data[] = {"apple", "banana", "cherry", "date", "elderberry"};
    char **arr = rt_array_create_string(arena, 5, data);

    rt_random_static_shuffle_string(arr);

    /* Verify all strings still present */
    int found[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 5; i++) {
        if (strcmp(arr[i], "apple") == 0) found[0] = 1;
        else if (strcmp(arr[i], "banana") == 0) found[1] = 1;
        else if (strcmp(arr[i], "cherry") == 0) found[2] = 1;
        else if (strcmp(arr[i], "date") == 0) found[3] = 1;
        else if (strcmp(arr[i], "elderberry") == 0) found[4] = 1;
    }

    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(found[i], "All strings should be present after shuffle");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_shuffle_bool_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    int data[] = {1, 1, 0, 0, 1};
    int *arr = rt_array_create_bool(arena, 5, data);

    int original_true_count = 0;
    for (int i = 0; i < 5; i++) if (arr[i]) original_true_count++;

    rt_random_static_shuffle_bool(arr);

    int true_count = 0;
    for (int i = 0; i < 5; i++) if (arr[i]) true_count++;

    TEST_ASSERT(true_count == original_true_count, "Bool count should be unchanged");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_shuffle_byte_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    unsigned char data[] = {10, 20, 30, 40, 50};
    unsigned char *arr = rt_array_create_byte(arena, 5, data);

    int original_sum = 0;
    for (int i = 0; i < 5; i++) original_sum += arr[i];

    rt_random_static_shuffle_byte(arr);

    int sum = 0;
    for (int i = 0; i < 5; i++) sum += arr[i];

    TEST_ASSERT(sum == original_sum, "Byte sum should be unchanged");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_shuffle_null_handling(void)
{

    /* These should not crash */
    rt_random_static_shuffle_long(NULL);
    rt_random_static_shuffle_double(NULL);
    rt_random_static_shuffle_string(NULL);
    rt_random_static_shuffle_bool(NULL);
    rt_random_static_shuffle_byte(NULL);

}

static void test_rt_random_static_shuffle_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);

    long long data[] = {42};
    long long *arr = rt_array_create_long(arena, 1, data);

    rt_random_static_shuffle_long(arr);

    TEST_ASSERT(arr[0] == 42, "Single element should be unchanged");


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Shuffle Tests - Instance Methods (Seeded PRNG)
 * ============================================================================ */

static void test_rt_random_shuffle_long_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    long long data[] = {1, 2, 3, 4, 5};
    long long *arr = rt_array_create_long(arena, 5, data);

    long long original_sum = 0;
    for (int i = 0; i < 5; i++) original_sum += arr[i];

    rt_random_shuffle_long(rng, arr);

    long sum = 0;
    int found[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 5; i++) {
        sum += arr[i];
        if (arr[i] >= 1 && arr[i] <= 5) found[arr[i] - 1] = 1;
    }

    TEST_ASSERT(sum == original_sum, "Sum should be unchanged");
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(found[i], "All elements present");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_shuffle_reproducible(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Two identical arrays with same seed should produce same shuffle */
    long long data1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long long data2[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long long *arr1 = rt_array_create_long(arena, 10, data1);
    long long *arr2 = rt_array_create_long(arena, 10, data2);

    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);

    rt_random_shuffle_long(rng1, arr1);
    rt_random_shuffle_long(rng2, arr2);

    int all_match = 1;
    for (int i = 0; i < 10; i++) {
        if (arr1[i] != arr2[i]) {
            all_match = 0;
            break;
        }
    }

    TEST_ASSERT(all_match, "Same seed must produce identical shuffle");


    rt_arena_destroy(arena);
}

static void test_rt_random_shuffle_null_rng(void)
{

    RtArena *arena = rt_arena_create(NULL);
    long long data[] = {1, 2, 3};
    long long *arr = rt_array_create_long(arena, 3, data);

    /* Should not crash */
    rt_random_shuffle_long(NULL, arr);

    /* Array should be unchanged */
    TEST_ASSERT(arr[0] == 1 && arr[1] == 2 && arr[2] == 3, "Array unchanged with NULL rng");


    rt_arena_destroy(arena);
}

static void test_rt_random_shuffle_all_types_seeded(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 54321);

    /* Double */
    double ddata[] = {1.1, 2.2, 3.3};
    double *darr = rt_array_create_double(arena, 3, ddata);
    rt_random_shuffle_double(rng, darr);

    /* String */
    const char *sdata[] = {"a", "b", "c"};
    char **sarr = rt_array_create_string(arena, 3, sdata);
    rt_random_shuffle_string(rng, sarr);

    /* Bool */
    int bdata[] = {1, 0, 1};
    int *barr = rt_array_create_bool(arena, 3, bdata);
    rt_random_shuffle_bool(rng, barr);

    /* Byte */
    unsigned char bydata[] = {1, 2, 3};
    unsigned char *byarr = rt_array_create_byte(arena, 3, bydata);
    rt_random_shuffle_byte(rng, byarr);


    rt_arena_destroy(arena);
}

/* Statistical test: Verify uniform permutation distribution */
static void test_rt_random_shuffle_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* For a 3-element array, there are 6 possible permutations.
     * Each should occur roughly 1/6 of the time.
     * We encode permutations as: arr[0]*100 + arr[1]*10 + arr[2]
     * 123, 132, 213, 231, 312, 321
     */
    int perm_counts[6] = {0, 0, 0, 0, 0, 0};
    int iterations = 6000;

    for (int iter = 0; iter < iterations; iter++) {
        long long data[] = {1, 2, 3};
        long long *arr = rt_array_create_long(arena, 3, data);

        rt_random_static_shuffle_long(arr);

        int perm = (int)(arr[0] * 100 + arr[1] * 10 + arr[2]);

        /* Map permutation to index */
        int idx = -1;
        switch (perm) {
            case 123: idx = 0; break;
            case 132: idx = 1; break;
            case 213: idx = 2; break;
            case 231: idx = 3; break;
            case 312: idx = 4; break;
            case 321: idx = 5; break;
        }

        if (idx >= 0 && idx < 6) {
            perm_counts[idx]++;
        }

        /* We need to free this array - but arena will handle cleanup */
    }

    int expected = iterations / 6;  /* ~1000 */
    int tolerance = expected / 2;   /* Allow 50% deviation */


    for (int i = 0; i < 6; i++) {
        int deviation = abs(perm_counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Permutation distribution should be uniform");
    }

    rt_arena_destroy(arena);
}

static void test_rt_random_shuffle_distribution_seeded(void)
{

    RtArena *arena = rt_arena_create(NULL);

    int perm_counts[6] = {0, 0, 0, 0, 0, 0};
    int iterations = 6000;

    /* Use different seeds to get variety while still being deterministic */
    for (int iter = 0; iter < iterations; iter++) {
        RtRandom *rng = rt_random_create_with_seed(arena, (long)(iter * 7919));  /* Different seed each time */

        long long data[] = {1, 2, 3};
        long long *arr = rt_array_create_long(arena, 3, data);

        rt_random_shuffle_long(rng, arr);

        int perm = (int)(arr[0] * 100 + arr[1] * 10 + arr[2]);

        int idx = -1;
        switch (perm) {
            case 123: idx = 0; break;
            case 132: idx = 1; break;
            case 213: idx = 2; break;
            case 231: idx = 3; break;
            case 312: idx = 4; break;
            case 321: idx = 5; break;
        }

        if (idx >= 0 && idx < 6) {
            perm_counts[idx]++;
        }
    }

    int expected = iterations / 6;
    int tolerance = expected / 2;


    for (int i = 0; i < 6; i++) {
        int deviation = abs(perm_counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Seeded permutation distribution should be uniform");
    }

    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Static (OS Entropy)
 * ============================================================================
 * Tests for Random.sample() which selects elements without replacement.
 * ============================================================================ */

static void test_rt_random_static_sample_long_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array {10, 20, 30, 40, 50} */
    long long data[] = {10, 20, 30, 40, 50};
    long long *arr = rt_array_create_long(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    long long *sample = rt_random_static_sample_long(arena, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (sample[i] == data[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_long_no_duplicates(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with unique values */
    long long data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long long *arr = rt_array_create_long(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        long long *sample = rt_random_static_sample_long(arena, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(sample[i] != sample[j], "Sample should not contain duplicates");
            }
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_long_full_array(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    long long data[] = {10, 20, 30, 40, 50};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Sample entire array (count == length) */
    long long *sample = rt_random_static_sample_long(arena, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present */
    long long original_sum = 10 + 20 + 30 + 40 + 50;
    long long sample_sum = 0;
    for (int i = 0; i < 5; i++) {
        sample_sum += sample[i];
    }
    TEST_ASSERT(sample_sum == original_sum, "Full sample should contain all original elements");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_long_single_element(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    long long data[] = {100, 200, 300, 400, 500};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Sample single element */
    long long *sample = rt_random_static_sample_long(arena, arr, 1);
    TEST_ASSERT_NOT_NULL(sample, "Single element sample should be created");
    TEST_ASSERT(rt_array_length(sample) == 1, "Sample should have 1 element");

    /* Verify element is from original array */
    int found = 0;
    for (int j = 0; j < 5; j++) {
        if (sample[0] == data[j]) {
            found = 1;
            break;
        }
    }
    TEST_ASSERT(found, "Single sampled element should be from original array");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_long_count_exceeds_length(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with 5 elements */
    long long data[] = {1, 2, 3, 4, 5};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    long long *sample = rt_random_static_sample_long(arena, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    /* Try to sample 10 elements (should return NULL) */
    sample = rt_random_static_sample_long(arena, arr, 10);
    TEST_ASSERT(sample == NULL, "Should return NULL when count >> array length");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_long_null_handling(void)
{

    RtArena *arena = rt_arena_create(NULL);

    long long data[] = {1, 2, 3};
    long long *arr = rt_array_create_long(arena, 3, data);

    /* NULL arena */
    long long *sample = rt_random_static_sample_long(NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL array */
    sample = rt_random_static_sample_long(arena, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_static_sample_long(arena, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_static_sample_long(arena, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_long_preserves_original(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    long long data[] = {100, 200, 300, 400, 500};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        long long *sample = rt_random_static_sample_long(arena, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(arr[i] == data[i], "Original array should be unchanged after sampling");
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_long_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array {1, 2, 3, 4, 5} */
    long long data[] = {1, 2, 3, 4, 5};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Track how often each element appears in samples */
    int element_counts[5] = {0, 0, 0, 0, 0};
    int iterations = 1000;

    for (int iter = 0; iter < iterations; iter++) {
        long long *sample = rt_random_static_sample_long(arena, arr, 2);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 5; j++) {
                if (sample[i] == data[j]) {
                    element_counts[j]++;
                }
            }
        }
    }

    /* Each element should appear roughly (2/5) * iterations = 400 times */
    int expected = (2 * iterations) / 5;
    int tolerance = expected / 3;  /* Allow ~33% deviation */

    for (int i = 0; i < 5; i++) {
        int deviation = abs(element_counts[i] - expected);
        TEST_ASSERT(deviation < tolerance, "Sample distribution should be roughly uniform");
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Static Double (OS Entropy)
 * ============================================================================
 * Tests for Random.sample() on double arrays.
 * ============================================================================ */

static void test_rt_random_static_sample_double_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array {1.1, 2.2, 3.3, 4.4, 5.5} */
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    double *arr = rt_array_create_double(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    double *sample = rt_random_static_sample_double(arena, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (fabs(sample[i] - data[j]) < 0.001) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_double_no_duplicates(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with unique values */
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.0};
    double *arr = rt_array_create_double(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        double *sample = rt_random_static_sample_double(arena, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(fabs(sample[i] - sample[j]) > 0.001, "Sample should not contain duplicates");
            }
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_double_full_array(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample entire array (count == length) */
    double *sample = rt_random_static_sample_double(arena, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present */
    double original_sum = 1.0 + 2.0 + 3.0 + 4.0 + 5.0;
    double sample_sum = 0.0;
    for (int i = 0; i < 5; i++) {
        sample_sum += sample[i];
    }
    TEST_ASSERT(fabs(sample_sum - original_sum) < 0.001, "Full sample should contain all original elements");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_double_count_exceeds_length(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with 5 elements */
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    double *sample = rt_random_static_sample_double(arena, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    /* Try to sample 10 elements (should return NULL) */
    sample = rt_random_static_sample_double(arena, arr, 10);
    TEST_ASSERT(sample == NULL, "Should return NULL when count >> array length");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_double_null_handling(void)
{

    RtArena *arena = rt_arena_create(NULL);

    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    /* NULL arena */
    double *sample = rt_random_static_sample_double(NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL array */
    sample = rt_random_static_sample_double(arena, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_static_sample_double(arena, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_static_sample_double(arena, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_double_preserves_original(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    double data[] = {10.5, 20.5, 30.5, 40.5, 50.5};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        double *sample = rt_random_static_sample_double(arena, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(fabs(arr[i] - data[i]) < 0.001, "Original array should be unchanged after sampling");
        }
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Static String (OS Entropy)
 * ============================================================================
 * Tests for Random.sample() on string arrays.
 * ============================================================================ */

static void test_rt_random_static_sample_string_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    /* Create array of strings */
    const char *data[] = {"apple", "banana", "cherry", "date", "elderberry"};
    char **arr = rt_array_create_string(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    char **sample = rt_random_static_sample_string(arena, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (strcmp(sample[i], data[j]) == 0) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_string_no_duplicates(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with unique strings */
    const char *data[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    char **arr = rt_array_create_string(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        char **sample = rt_random_static_sample_string(arena, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(strcmp(sample[i], sample[j]) != 0, "Sample should not contain duplicates");
            }
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_string_full_array(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    const char *data[] = {"one", "two", "three", "four", "five"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample entire array (count == length) */
    char **sample = rt_random_static_sample_string(arena, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present */
    int found[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (strcmp(sample[i], data[j]) == 0) {
                found[j] = 1;
            }
        }
    }
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(found[i], "All original elements should be in full sample");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_string_count_exceeds_length(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array with 5 elements */
    const char *data[] = {"a", "b", "c", "d", "e"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    char **sample = rt_random_static_sample_string(arena, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");

    /* Try to sample 10 elements (should return NULL) */
    sample = rt_random_static_sample_string(arena, arr, 10);
    TEST_ASSERT(sample == NULL, "Should return NULL when count >> array length");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_string_null_handling(void)
{

    RtArena *arena = rt_arena_create(NULL);

    const char *data[] = {"x", "y", "z"};
    char **arr = rt_array_create_string(arena, 3, data);

    /* NULL arena */
    char **sample = rt_random_static_sample_string(NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL array */
    sample = rt_random_static_sample_string(arena, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_static_sample_string(arena, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_static_sample_string(arena, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");


    rt_arena_destroy(arena);
}

static void test_rt_random_static_sample_string_preserves_original(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    const char *data[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        char **sample = rt_random_static_sample_string(arena, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(strcmp(arr[i], data[i]) == 0, "Original array should be unchanged after sampling");
        }
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Instance Long (Seeded PRNG)
 * ============================================================================
 * Tests for Random.sample() instance method on long arrays.
 * ============================================================================ */

static void test_rt_random_sample_long_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array {10, 20, 30, 40, 50} */
    long long data[] = {10, 20, 30, 40, 50};
    long long *arr = rt_array_create_long(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    long long *sample = rt_random_sample_long(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (sample[i] == data[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_long_no_duplicates(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 54321);

    /* Create array with unique values */
    long long data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long long *arr = rt_array_create_long(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        long long *sample = rt_random_sample_long(arena, rng, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(sample[i] != sample[j], "Sample should not contain duplicates");
            }
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_long_reproducible(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    long long data[] = {100, 200, 300, 400, 500};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Sample with same seed twice */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    long long *sample1 = rt_random_sample_long(arena, rng1, arr, 3);

    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);
    long long *sample2 = rt_random_sample_long(arena, rng2, arr, 3);

    TEST_ASSERT_NOT_NULL(sample1, "First sample should be created");
    TEST_ASSERT_NOT_NULL(sample2, "Second sample should be created");

    /* Verify samples are identical */
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(sample1[i] == sample2[i], "Samples with same seed should be identical");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_long_count_exceeds_length(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);

    /* Create array with 5 elements */
    long long data[] = {1, 2, 3, 4, 5};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    long long *sample = rt_random_sample_long(arena, rng, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_long_null_handling(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 22222);

    long long data[] = {1, 2, 3};
    long long *arr = rt_array_create_long(arena, 3, data);

    /* NULL arena */
    long long *sample = rt_random_sample_long(NULL, rng, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL rng */
    sample = rt_random_sample_long(arena, NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL rng");

    /* NULL array */
    sample = rt_random_sample_long(arena, rng, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_sample_long(arena, rng, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_sample_long(arena, rng, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_long_preserves_original(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 33333);

    /* Create array */
    long long data[] = {100, 200, 300, 400, 500};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        long long *sample = rt_random_sample_long(arena, rng, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(arr[i] == data[i], "Original array should be unchanged after sampling");
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_long_full_array(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 44444);

    /* Create array */
    long long data[] = {10, 20, 30, 40, 50};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Sample entire array (count == length) */
    long long *sample = rt_random_sample_long(arena, rng, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present (sum should match) */
    long long original_sum = 10 + 20 + 30 + 40 + 50;
    long long sample_sum = 0;
    for (int i = 0; i < 5; i++) {
        sample_sum += sample[i];
    }
    TEST_ASSERT(sample_sum == original_sum, "Full sample should contain all original elements");

    /* Verify no duplicates */
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT(sample[i] != sample[j], "Full array sample should have no duplicates");
        }
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Instance Double (Seeded PRNG)
 * ============================================================================
 * Tests for Random.sample() instance method on double arrays.
 * ============================================================================ */

static void test_rt_random_sample_double_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array {1.1, 2.2, 3.3, 4.4, 5.5} */
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5};
    double *arr = rt_array_create_double(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    double *sample = rt_random_sample_double(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (fabs(sample[i] - data[j]) < 0.001) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_double_no_duplicates(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 54321);

    /* Create array with unique values */
    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.0};
    double *arr = rt_array_create_double(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        double *sample = rt_random_sample_double(arena, rng, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(fabs(sample[i] - sample[j]) > 0.001, "Sample should not contain duplicates");
            }
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_double_reproducible(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    double data[] = {10.5, 20.5, 30.5, 40.5, 50.5};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample with same seed twice */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    double *sample1 = rt_random_sample_double(arena, rng1, arr, 3);

    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);
    double *sample2 = rt_random_sample_double(arena, rng2, arr, 3);

    TEST_ASSERT_NOT_NULL(sample1, "First sample should be created");
    TEST_ASSERT_NOT_NULL(sample2, "Second sample should be created");

    /* Verify samples are identical */
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(fabs(sample1[i] - sample2[i]) < 0.001, "Samples with same seed should be identical");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_double_count_exceeds_length(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);

    /* Create array with 5 elements */
    double data[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    double *sample = rt_random_sample_double(arena, rng, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_double_null_handling(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 22222);

    double data[] = {1.0, 2.0, 3.0};
    double *arr = rt_array_create_double(arena, 3, data);

    /* NULL arena */
    double *sample = rt_random_sample_double(NULL, rng, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL rng */
    sample = rt_random_sample_double(arena, NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL rng");

    /* NULL array */
    sample = rt_random_sample_double(arena, rng, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_sample_double(arena, rng, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_sample_double(arena, rng, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_double_preserves_original(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 33333);

    /* Create array */
    double data[] = {10.5, 20.5, 30.5, 40.5, 50.5};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        double *sample = rt_random_sample_double(arena, rng, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(fabs(arr[i] - data[i]) < 0.001, "Original array should be unchanged after sampling");
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_double_full_array(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 44444);

    /* Create array */
    double data[] = {10.5, 20.5, 30.5, 40.5, 50.5};
    double *arr = rt_array_create_double(arena, 5, data);

    /* Sample entire array (count == length) */
    double *sample = rt_random_sample_double(arena, rng, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present (sum should match) */
    double original_sum = 10.5 + 20.5 + 30.5 + 40.5 + 50.5;
    double sample_sum = 0.0;
    for (int i = 0; i < 5; i++) {
        sample_sum += sample[i];
    }
    TEST_ASSERT(fabs(sample_sum - original_sum) < 0.001, "Full sample should contain all original elements");

    /* Verify no duplicates */
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT(fabs(sample[i] - sample[j]) > 0.001, "Full array sample should have no duplicates");
        }
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Sample Tests - Instance String (Seeded PRNG)
 * ============================================================================
 * Tests for Random.sample() instance method on string arrays.
 * ============================================================================ */

static void test_rt_random_sample_string_basic(void)
{

    RtArena *arena = rt_arena_create(NULL);
    TEST_ASSERT_NOT_NULL(arena, "Arena should be created");

    RtRandom *rng = rt_random_create_with_seed(arena, 12345);
    TEST_ASSERT_NOT_NULL(rng, "RNG should be created");

    /* Create array of strings */
    const char *data[] = {"apple", "banana", "cherry", "date", "elderberry"};
    char **arr = rt_array_create_string(arena, 5, data);
    TEST_ASSERT_NOT_NULL(arr, "Array should be created");

    /* Sample 3 elements */
    char **sample = rt_random_sample_string(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

    /* Verify sample has correct length */
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original array */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (strcmp(sample[i], data[j]) == 0) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_string_no_duplicates(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 54321);

    /* Create array with unique strings */
    const char *data[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
    char **arr = rt_array_create_string(arena, 10, data);

    /* Sample 5 elements multiple times */
    for (int trial = 0; trial < 20; trial++) {
        char **sample = rt_random_sample_string(arena, rng, arr, 5);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Check for duplicates */
        for (int i = 0; i < 5; i++) {
            for (int j = i + 1; j < 5; j++) {
                TEST_ASSERT(strcmp(sample[i], sample[j]) != 0, "Sample should not contain duplicates");
            }
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_string_reproducible(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Create array */
    const char *data[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample with same seed twice */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 99999);
    char **sample1 = rt_random_sample_string(arena, rng1, arr, 3);

    RtRandom *rng2 = rt_random_create_with_seed(arena, 99999);
    char **sample2 = rt_random_sample_string(arena, rng2, arr, 3);

    TEST_ASSERT_NOT_NULL(sample1, "First sample should be created");
    TEST_ASSERT_NOT_NULL(sample2, "Second sample should be created");

    /* Verify samples are identical */
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT(strcmp(sample1[i], sample2[i]) == 0, "Samples with same seed should be identical");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_string_count_exceeds_length(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);

    /* Create array with 5 elements */
    const char *data[] = {"a", "b", "c", "d", "e"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Try to sample 6 elements (should return NULL) */
    char **sample = rt_random_sample_string(arena, rng, arr, 6);
    TEST_ASSERT(sample == NULL, "Should return NULL when count > array length");


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_string_null_handling(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 22222);

    const char *data[] = {"x", "y", "z"};
    char **arr = rt_array_create_string(arena, 3, data);

    /* NULL arena */
    char **sample = rt_random_sample_string(NULL, rng, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL arena");

    /* NULL rng */
    sample = rt_random_sample_string(arena, NULL, arr, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL rng");

    /* NULL array */
    sample = rt_random_sample_string(arena, rng, NULL, 2);
    TEST_ASSERT(sample == NULL, "Should return NULL with NULL array");

    /* Zero count */
    sample = rt_random_sample_string(arena, rng, arr, 0);
    TEST_ASSERT(sample == NULL, "Should return NULL with zero count");

    /* Negative count */
    sample = rt_random_sample_string(arena, rng, arr, -1);
    TEST_ASSERT(sample == NULL, "Should return NULL with negative count");


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_string_preserves_original(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 33333);

    /* Create array */
    const char *data[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample multiple times */
    for (int trial = 0; trial < 10; trial++) {
        char **sample = rt_random_sample_string(arena, rng, arr, 3);
        TEST_ASSERT_NOT_NULL(sample, "Sample should be created");

        /* Verify original array is unchanged */
        for (int i = 0; i < 5; i++) {
            TEST_ASSERT(strcmp(arr[i], data[i]) == 0, "Original array should be unchanged after sampling");
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_string_full_array(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 44444);

    /* Create array */
    const char *data[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Sample entire array (count == length) */
    char **sample = rt_random_sample_string(arena, rng, arr, 5);
    TEST_ASSERT_NOT_NULL(sample, "Sample should be created when count equals array length");
    TEST_ASSERT(rt_array_length(sample) == 5, "Sample should have all 5 elements");

    /* Verify all original elements are present (each must be found) */
    for (int i = 0; i < 5; i++) {
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (strcmp(sample[j], data[i]) == 0) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Full sample should contain all original elements");
    }

    /* Verify no duplicates */
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            TEST_ASSERT(strcmp(sample[i], sample[j]) != 0, "Full array sample should have no duplicates");
        }
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Comprehensive Edge Case Tests - Empty Arrays
 * ============================================================================ */

static void test_rt_random_shuffle_empty_array(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    /* Static shuffle of empty long array - should not crash */
    long long *empty_long = rt_array_alloc_long(arena, 0, 0);
    rt_random_static_shuffle_long(empty_long);
    TEST_ASSERT(rt_array_length(empty_long) == 0, "Empty long array should remain empty after shuffle");

    /* Static shuffle of empty double array */
    double *empty_double = rt_array_alloc_double(arena, 0, 0.0);
    rt_random_static_shuffle_double(empty_double);
    TEST_ASSERT(rt_array_length(empty_double) == 0, "Empty double array should remain empty after shuffle");

    /* Static shuffle of empty string array */
    char **empty_string = rt_array_alloc_string(arena, 0, NULL);
    rt_random_static_shuffle_string(empty_string);
    TEST_ASSERT(rt_array_length(empty_string) == 0, "Empty string array should remain empty after shuffle");

    /* Instance shuffle of empty arrays */
    long long *empty_long2 = rt_array_alloc_long(arena, 0, 0);
    rt_random_shuffle_long(rng, empty_long2);
    TEST_ASSERT(rt_array_length(empty_long2) == 0, "Empty long array should remain empty after seeded shuffle");

    double *empty_double2 = rt_array_alloc_double(arena, 0, 0.0);
    rt_random_shuffle_double(rng, empty_double2);
    TEST_ASSERT(rt_array_length(empty_double2) == 0, "Empty double array should remain empty after seeded shuffle");


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_empty_array(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 12345);

    /* Create empty arrays */
    long long *empty_long = rt_array_alloc_long(arena, 0, 0);
    double *empty_double = rt_array_alloc_double(arena, 0, 0.0);
    char **empty_string = rt_array_alloc_string(arena, 0, NULL);

    /* Static sample from empty arrays - should return NULL */
    long long *sample_long = rt_random_static_sample_long(arena, empty_long, 1);
    TEST_ASSERT(sample_long == NULL, "Sampling from empty long array should return NULL");

    double *sample_double = rt_random_static_sample_double(arena, empty_double, 1);
    TEST_ASSERT(sample_double == NULL, "Sampling from empty double array should return NULL");

    char **sample_string = rt_random_static_sample_string(arena, empty_string, 1);
    TEST_ASSERT(sample_string == NULL, "Sampling from empty string array should return NULL");

    /* Instance sample from empty arrays */
    sample_long = rt_random_sample_long(arena, rng, empty_long, 1);
    TEST_ASSERT(sample_long == NULL, "Seeded sampling from empty long array should return NULL");

    sample_double = rt_random_sample_double(arena, rng, empty_double, 1);
    TEST_ASSERT(sample_double == NULL, "Seeded sampling from empty double array should return NULL");

    sample_string = rt_random_sample_string(arena, rng, empty_string, 1);
    TEST_ASSERT(sample_string == NULL, "Seeded sampling from empty string array should return NULL");


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Comprehensive Edge Case Tests - Single Element
 * ============================================================================ */

static void test_rt_random_sample_single_element_all_types(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 55555);

    /* Static sample single from long array */
    long long long_data[] = {42};
    long long *long_arr = rt_array_create_long(arena, 1, long_data);
    long long *long_sample = rt_random_static_sample_long(arena, long_arr, 1);
    TEST_ASSERT_NOT_NULL(long_sample, "Single element long sample should succeed");
    TEST_ASSERT(rt_array_length(long_sample) == 1, "Single element sample should have length 1");
    TEST_ASSERT(long_sample[0] == 42, "Single element sample should be 42");

    /* Static sample single from double array */
    double double_data[] = {3.14};
    double *double_arr = rt_array_create_double(arena, 1, double_data);
    double *double_sample = rt_random_static_sample_double(arena, double_arr, 1);
    TEST_ASSERT_NOT_NULL(double_sample, "Single element double sample should succeed");
    TEST_ASSERT(rt_array_length(double_sample) == 1, "Single element sample should have length 1");
    TEST_ASSERT(fabs(double_sample[0] - 3.14) < 0.001, "Single element sample should be 3.14");

    /* Static sample single from string array */
    const char *string_data[] = {"hello"};
    char **string_arr = rt_array_create_string(arena, 1, string_data);
    char **string_sample = rt_random_static_sample_string(arena, string_arr, 1);
    TEST_ASSERT_NOT_NULL(string_sample, "Single element string sample should succeed");
    TEST_ASSERT(rt_array_length(string_sample) == 1, "Single element sample should have length 1");
    TEST_ASSERT(strcmp(string_sample[0], "hello") == 0, "Single element sample should be 'hello'");

    /* Instance sample single from long array */
    long_sample = rt_random_sample_long(arena, rng, long_arr, 1);
    TEST_ASSERT_NOT_NULL(long_sample, "Seeded single element long sample should succeed");
    TEST_ASSERT(long_sample[0] == 42, "Seeded single element sample should be 42");

    /* Instance sample single from double array */
    double_sample = rt_random_sample_double(arena, rng, double_arr, 1);
    TEST_ASSERT_NOT_NULL(double_sample, "Seeded single element double sample should succeed");
    TEST_ASSERT(fabs(double_sample[0] - 3.14) < 0.001, "Seeded single element sample should be 3.14");

    /* Instance sample single from string array */
    string_sample = rt_random_sample_string(arena, rng, string_arr, 1);
    TEST_ASSERT_NOT_NULL(string_sample, "Seeded single element string sample should succeed");
    TEST_ASSERT(strcmp(string_sample[0], "hello") == 0, "Seeded single element sample should be 'hello'");


    rt_arena_destroy(arena);
}

static void test_rt_random_shuffle_single_element_all_types(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 55555);

    /* Static shuffle single long */
    long long long_data[] = {99};
    long long *long_arr = rt_array_create_long(arena, 1, long_data);
    rt_random_static_shuffle_long(long_arr);
    TEST_ASSERT(long_arr[0] == 99, "Single element should remain unchanged after shuffle");

    /* Static shuffle single double */
    double double_data[] = {2.718};
    double *double_arr = rt_array_create_double(arena, 1, double_data);
    rt_random_static_shuffle_double(double_arr);
    TEST_ASSERT(fabs(double_arr[0] - 2.718) < 0.001, "Single double should remain unchanged after shuffle");

    /* Static shuffle single string */
    const char *string_data[] = {"world"};
    char **string_arr = rt_array_create_string(arena, 1, string_data);
    rt_random_static_shuffle_string(string_arr);
    TEST_ASSERT(strcmp(string_arr[0], "world") == 0, "Single string should remain unchanged after shuffle");

    /* Instance shuffle single long */
    long long long_data2[] = {77};
    long long *long_arr2 = rt_array_create_long(arena, 1, long_data2);
    rt_random_shuffle_long(rng, long_arr2);
    TEST_ASSERT(long_arr2[0] == 77, "Seeded single element should remain unchanged");

    /* Instance shuffle single double */
    double double_data2[] = {1.414};
    double *double_arr2 = rt_array_create_double(arena, 1, double_data2);
    rt_random_shuffle_double(rng, double_arr2);
    TEST_ASSERT(fabs(double_arr2[0] - 1.414) < 0.001, "Seeded single double should remain unchanged");

    /* Instance shuffle single string */
    const char *string_data2[] = {"test"};
    char **string_arr2 = rt_array_create_string(arena, 1, string_data2);
    rt_random_shuffle_string(rng, string_arr2);
    TEST_ASSERT(strcmp(string_arr2[0], "test") == 0, "Seeded single string should remain unchanged");


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Reproducibility Tests for Sample Operations
 * ============================================================================ */

static void test_rt_random_sample_double_reproducible_extended(void)
{

    RtArena *arena = rt_arena_create(NULL);

    double data[] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.0};
    double *arr = rt_array_create_double(arena, 10, data);

    /* Same seed should produce same samples across multiple calls */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 77777);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 77777);

    for (int trial = 0; trial < 5; trial++) {
        double *sample1 = rt_random_sample_double(arena, rng1, arr, 4);
        double *sample2 = rt_random_sample_double(arena, rng2, arr, 4);

        TEST_ASSERT_NOT_NULL(sample1, "Sample 1 should succeed");
        TEST_ASSERT_NOT_NULL(sample2, "Sample 2 should succeed");

        for (int i = 0; i < 4; i++) {
            TEST_ASSERT(fabs(sample1[i] - sample2[i]) < 0.001,
                "Samples with same seed should be identical");
        }
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_sample_string_reproducible_extended(void)
{

    RtArena *arena = rt_arena_create(NULL);

    const char *data[] = {"one", "two", "three", "four", "five", "six", "seven", "eight"};
    char **arr = rt_array_create_string(arena, 8, data);

    /* Same seed should produce same samples across multiple calls */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 88888);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 88888);

    for (int trial = 0; trial < 5; trial++) {
        char **sample1 = rt_random_sample_string(arena, rng1, arr, 3);
        char **sample2 = rt_random_sample_string(arena, rng2, arr, 3);

        TEST_ASSERT_NOT_NULL(sample1, "Sample 1 should succeed");
        TEST_ASSERT_NOT_NULL(sample2, "Sample 2 should succeed");

        for (int i = 0; i < 3; i++) {
            TEST_ASSERT(strcmp(sample1[i], sample2[i]) == 0,
                "String samples with same seed should be identical");
        }
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Statistical Distribution Tests
 * ============================================================================ */

static void test_rt_random_sample_distribution(void)
{

    RtArena *arena = rt_arena_create(NULL);

    long long data[] = {0, 1, 2, 3, 4};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Count how often each element appears in samples */
    int counts[5] = {0, 0, 0, 0, 0};
    int num_samples = 10000;

    for (int trial = 0; trial < num_samples; trial++) {
        long long *sample = rt_random_static_sample_long(arena, arr, 2);
        TEST_ASSERT_NOT_NULL(sample, "Sample should succeed");
        counts[sample[0]]++;
        counts[sample[1]]++;
    }

    /* Each element should appear roughly equally (40% each with 2 samples from 5) */
    /* With 10000 samples of 2, each element expected 4000 times */
    int expected = num_samples * 2 / 5;
    double tolerance = expected * 0.15; /* 15% tolerance */

    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(counts[i] > expected - tolerance && counts[i] < expected + tolerance,
            "Sample distribution should be approximately uniform");
    }


    rt_arena_destroy(arena);
}

static void test_rt_random_shuffle_distribution_extended(void)
{

    RtArena *arena = rt_arena_create(NULL);

    /* Count how often each value appears at each position */
    int position_counts[5][5] = {{0}}; /* [value][position] */
    int num_trials = 10000;

    for (int trial = 0; trial < num_trials; trial++) {
        long long data[] = {0, 1, 2, 3, 4};
        long long *arr = rt_array_create_long(arena, 5, data);
        rt_random_static_shuffle_long(arr);

        for (int pos = 0; pos < 5; pos++) {
            position_counts[arr[pos]][pos]++;
        }
    }

    /* Each value should appear at each position roughly 20% of the time */
    int expected = num_trials / 5;
    double tolerance = expected * 0.15;

    int failed = 0;
    for (int val = 0; val < 5; val++) {
        for (int pos = 0; pos < 5; pos++) {
            if (position_counts[val][pos] < expected - tolerance ||
                position_counts[val][pos] > expected + tolerance) {
                failed = 1;
            }
        }
    }

    TEST_ASSERT(!failed, "Shuffle should produce uniform position distribution");


    rt_arena_destroy(arena);
}


/* ============================================================================
 * Integration Tests - Combining Operations
 * ============================================================================ */

static void test_integration_shuffle_then_sample(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 99999);

    /* Create array, shuffle it, then sample */
    long long data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    long long *arr = rt_array_create_long(arena, 10, data);

    /* Shuffle in place */
    rt_random_shuffle_long(rng, arr);

    /* Sample from shuffled array */
    long long *sample = rt_random_sample_long(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample from shuffled array should succeed");
    TEST_ASSERT(rt_array_length(sample) == 3, "Sample should have 3 elements");

    /* Verify all sampled elements are from original set */
    for (int i = 0; i < 3; i++) {
        int found = 0;
        for (int j = 0; j < 10; j++) {
            if (sample[i] == data[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Sampled element should be from original array");
    }

    /* Verify no duplicates in sample */
    TEST_ASSERT(sample[0] != sample[1] && sample[1] != sample[2] && sample[0] != sample[2],
        "Sample should have no duplicates");


    rt_arena_destroy(arena);
}

static void test_integration_sample_then_choice(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 11111);

    /* Create array, sample from it, then choose from sample */
    long long data[] = {100, 200, 300, 400, 500};
    long long *arr = rt_array_create_long(arena, 5, data);

    /* Sample 3 elements */
    long long *sample = rt_random_sample_long(arena, rng, arr, 3);
    TEST_ASSERT_NOT_NULL(sample, "Sample should succeed");

    /* Choose from the sample multiple times */
    for (int i = 0; i < 10; i++) {
        long choice = rt_random_choice_long(rng, sample, 3);

        /* Verify choice is in original data */
        int found = 0;
        for (int j = 0; j < 5; j++) {
            if (choice == data[j]) {
                found = 1;
                break;
            }
        }
        TEST_ASSERT(found, "Choice from sample should be from original array");
    }


    rt_arena_destroy(arena);
}

static void test_integration_multiple_samples_different_seeds(void)
{

    RtArena *arena = rt_arena_create(NULL);

    const char *data[] = {"apple", "banana", "cherry", "date", "elderberry"};
    char **arr = rt_array_create_string(arena, 5, data);

    /* Create multiple RNGs with different seeds */
    RtRandom *rng1 = rt_random_create_with_seed(arena, 11111);
    RtRandom *rng2 = rt_random_create_with_seed(arena, 22222);
    RtRandom *rng3 = rt_random_create_with_seed(arena, 33333);

    /* Take samples with each RNG */
    char **sample1 = rt_random_sample_string(arena, rng1, arr, 2);
    char **sample2 = rt_random_sample_string(arena, rng2, arr, 2);
    char **sample3 = rt_random_sample_string(arena, rng3, arr, 2);

    TEST_ASSERT_NOT_NULL(sample1, "Sample 1 should succeed");
    TEST_ASSERT_NOT_NULL(sample2, "Sample 2 should succeed");
    TEST_ASSERT_NOT_NULL(sample3, "Sample 3 should succeed");

    /* At least one pair should differ (statistically almost certain) */
    int all_same = (strcmp(sample1[0], sample2[0]) == 0 && strcmp(sample1[1], sample2[1]) == 0) &&
                   (strcmp(sample2[0], sample3[0]) == 0 && strcmp(sample2[1], sample3[1]) == 0);
    TEST_ASSERT(!all_same, "Different seeds should produce different samples");


    rt_arena_destroy(arena);
}

static void test_integration_weighted_choice_after_shuffle(void)
{

    RtArena *arena = rt_arena_create(NULL);
    RtRandom *rng = rt_random_create_with_seed(arena, 44444);

    /* Create array with values and corresponding weights */
    long long data[] = {1, 2, 3, 4, 5};
    long long *arr = rt_array_create_long(arena, 5, data);
    double weights_data[] = {5.0, 4.0, 3.0, 2.0, 1.0};
    double *weights = rt_array_create_double(arena, 5, weights_data);

    /* Shuffle the array (weights stay matched by index) */
    /* Note: In real use, would need to shuffle weights too - this tests the API */
    rt_random_shuffle_long(rng, arr);

    /* Make weighted choices - weights still correspond to shuffled positions */
    int counts[5] = {0};
    for (int i = 0; i < 1000; i++) {
        long choice = rt_random_weighted_choice_long(rng, arr, weights);
        counts[choice - 1]++;
    }

    /* Just verify the function works without crashing */
    int total = counts[0] + counts[1] + counts[2] + counts[3] + counts[4];
    TEST_ASSERT(total == 1000, "All choices should be valid");


    rt_arena_destroy(arena);
}

static void test_integration_reproducible_workflow(void)
{

    RtArena *arena = rt_arena_create(NULL);

    long long data[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    long long *arr = rt_array_create_long(arena, 10, data);

    /* Run same workflow twice with same seed */
    for (int run = 0; run < 2; run++) {
        RtRandom *rng = rt_random_create_with_seed(arena, 55555);

        /* 1. Make some random choices */
        long long choice1 = rt_random_choice_long(rng, arr, 10);
        long long choice2 = rt_random_choice_long(rng, arr, 10);

        /* 2. Sample from array */
        long long *sample = rt_random_sample_long(arena, rng, arr, 3);

        /* 3. Shuffle a copy */
        long long copy_data[] = {1, 2, 3, 4, 5};
        long long *copy = rt_array_create_long(arena, 5, copy_data);
        rt_random_shuffle_long(rng, copy);

        /* Store results from first run */
        static long long first_choice1, first_choice2;
        static long long first_sample[3];
        static long long first_shuffled[5];

        if (run == 0) {
            first_choice1 = choice1;
            first_choice2 = choice2;
            for (int i = 0; i < 3; i++) first_sample[i] = sample[i];
            for (int i = 0; i < 5; i++) first_shuffled[i] = copy[i];
        } else {
            /* Compare with first run */
            TEST_ASSERT(choice1 == first_choice1, "Choice 1 should be reproducible");
            TEST_ASSERT(choice2 == first_choice2, "Choice 2 should be reproducible");
            for (int i = 0; i < 3; i++) {
                TEST_ASSERT(sample[i] == first_sample[i], "Sample should be reproducible");
            }
            for (int i = 0; i < 5; i++) {
                TEST_ASSERT(copy[i] == first_shuffled[i], "Shuffle should be reproducible");
            }
        }
    }


    rt_arena_destroy(arena);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */


/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

void test_rt_random_collection_main(void)
{
    TEST_SECTION("Runtime Random Collection");

    /* Shuffle tests - Static (OS Entropy) */
    TEST_RUN("static_shuffle_long_basic", test_rt_random_static_shuffle_long_basic);
    TEST_RUN("static_shuffle_double_basic", test_rt_random_static_shuffle_double_basic);
    TEST_RUN("static_shuffle_string_basic", test_rt_random_static_shuffle_string_basic);
    TEST_RUN("static_shuffle_bool_basic", test_rt_random_static_shuffle_bool_basic);
    TEST_RUN("static_shuffle_byte_basic", test_rt_random_static_shuffle_byte_basic);
    TEST_RUN("static_shuffle_null_handling", test_rt_random_static_shuffle_null_handling);
    TEST_RUN("static_shuffle_single_element", test_rt_random_static_shuffle_single_element);

    /* Shuffle tests - Instance (Seeded PRNG) */
    TEST_RUN("shuffle_long_basic", test_rt_random_shuffle_long_basic);
    TEST_RUN("shuffle_reproducible", test_rt_random_shuffle_reproducible);
    TEST_RUN("shuffle_null_rng", test_rt_random_shuffle_null_rng);
    TEST_RUN("shuffle_all_types_seeded", test_rt_random_shuffle_all_types_seeded);
    TEST_RUN("shuffle_distribution", test_rt_random_shuffle_distribution);
    TEST_RUN("shuffle_distribution_seeded", test_rt_random_shuffle_distribution_seeded);

    /* Sample tests - Static Long (OS Entropy) */
    TEST_RUN("static_sample_long_basic", test_rt_random_static_sample_long_basic);
    TEST_RUN("static_sample_long_no_duplicates", test_rt_random_static_sample_long_no_duplicates);
    TEST_RUN("static_sample_long_full_array", test_rt_random_static_sample_long_full_array);
    TEST_RUN("static_sample_long_single_element", test_rt_random_static_sample_long_single_element);
    TEST_RUN("static_sample_long_count_exceeds_length", test_rt_random_static_sample_long_count_exceeds_length);
    TEST_RUN("static_sample_long_null_handling", test_rt_random_static_sample_long_null_handling);
    TEST_RUN("static_sample_long_preserves_original", test_rt_random_static_sample_long_preserves_original);
    TEST_RUN("static_sample_long_distribution", test_rt_random_static_sample_long_distribution);

    /* Sample tests - Static Double (OS Entropy) */
    TEST_RUN("static_sample_double_basic", test_rt_random_static_sample_double_basic);
    TEST_RUN("static_sample_double_no_duplicates", test_rt_random_static_sample_double_no_duplicates);
    TEST_RUN("static_sample_double_full_array", test_rt_random_static_sample_double_full_array);
    TEST_RUN("static_sample_double_count_exceeds_length", test_rt_random_static_sample_double_count_exceeds_length);
    TEST_RUN("static_sample_double_null_handling", test_rt_random_static_sample_double_null_handling);
    TEST_RUN("static_sample_double_preserves_original", test_rt_random_static_sample_double_preserves_original);

    /* Sample tests - Static String (OS Entropy) */
    TEST_RUN("static_sample_string_basic", test_rt_random_static_sample_string_basic);
    TEST_RUN("static_sample_string_no_duplicates", test_rt_random_static_sample_string_no_duplicates);
    TEST_RUN("static_sample_string_full_array", test_rt_random_static_sample_string_full_array);
    TEST_RUN("static_sample_string_count_exceeds_length", test_rt_random_static_sample_string_count_exceeds_length);
    TEST_RUN("static_sample_string_null_handling", test_rt_random_static_sample_string_null_handling);
    TEST_RUN("static_sample_string_preserves_original", test_rt_random_static_sample_string_preserves_original);

    /* Sample tests - Instance Long (Seeded PRNG) */
    TEST_RUN("sample_long_basic", test_rt_random_sample_long_basic);
    TEST_RUN("sample_long_no_duplicates", test_rt_random_sample_long_no_duplicates);
    TEST_RUN("sample_long_reproducible", test_rt_random_sample_long_reproducible);
    TEST_RUN("sample_long_count_exceeds_length", test_rt_random_sample_long_count_exceeds_length);
    TEST_RUN("sample_long_null_handling", test_rt_random_sample_long_null_handling);
    TEST_RUN("sample_long_preserves_original", test_rt_random_sample_long_preserves_original);
    TEST_RUN("sample_long_full_array", test_rt_random_sample_long_full_array);

    /* Sample tests - Instance Double (Seeded PRNG) */
    TEST_RUN("sample_double_basic", test_rt_random_sample_double_basic);
    TEST_RUN("sample_double_no_duplicates", test_rt_random_sample_double_no_duplicates);
    TEST_RUN("sample_double_reproducible", test_rt_random_sample_double_reproducible);
    TEST_RUN("sample_double_count_exceeds_length", test_rt_random_sample_double_count_exceeds_length);
    TEST_RUN("sample_double_null_handling", test_rt_random_sample_double_null_handling);
    TEST_RUN("sample_double_preserves_original", test_rt_random_sample_double_preserves_original);
    TEST_RUN("sample_double_full_array", test_rt_random_sample_double_full_array);

    /* Sample tests - Instance String (Seeded PRNG) */
    TEST_RUN("sample_string_basic", test_rt_random_sample_string_basic);
    TEST_RUN("sample_string_no_duplicates", test_rt_random_sample_string_no_duplicates);
    TEST_RUN("sample_string_reproducible", test_rt_random_sample_string_reproducible);
    TEST_RUN("sample_string_count_exceeds_length", test_rt_random_sample_string_count_exceeds_length);
    TEST_RUN("sample_string_null_handling", test_rt_random_sample_string_null_handling);
    TEST_RUN("sample_string_preserves_original", test_rt_random_sample_string_preserves_original);
    TEST_RUN("sample_string_full_array", test_rt_random_sample_string_full_array);

    /* Comprehensive edge case tests - empty arrays */
    TEST_RUN("shuffle_empty_array", test_rt_random_shuffle_empty_array);
    TEST_RUN("sample_empty_array", test_rt_random_sample_empty_array);

    /* Comprehensive edge case tests - single element */
    TEST_RUN("sample_single_element_all_types", test_rt_random_sample_single_element_all_types);
    TEST_RUN("shuffle_single_element_all_types", test_rt_random_shuffle_single_element_all_types);

    /* Reproducibility tests for sample operations */
    TEST_RUN("sample_double_reproducible_extended", test_rt_random_sample_double_reproducible_extended);
    TEST_RUN("sample_string_reproducible_extended", test_rt_random_sample_string_reproducible_extended);

    /* Statistical distribution tests */
    TEST_RUN("sample_distribution", test_rt_random_sample_distribution);
    TEST_RUN("shuffle_distribution_extended", test_rt_random_shuffle_distribution_extended);

    /* Integration tests - combining operations */
    TEST_RUN("integration_shuffle_then_sample", test_integration_shuffle_then_sample);
    TEST_RUN("integration_sample_then_choice", test_integration_sample_then_choice);
    TEST_RUN("integration_multiple_samples_different_seeds", test_integration_multiple_samples_different_seeds);
    TEST_RUN("integration_weighted_choice_after_shuffle", test_integration_weighted_choice_after_shuffle);
    TEST_RUN("integration_reproducible_workflow", test_integration_reproducible_workflow);
}
