#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "runtime_random_static.h"
#include "runtime_random_basic.h"
#include "runtime_array.h"

/* ============================================================================
 * Static Random Methods (OS Entropy)
 * ============================================================================
 * This module contains static methods that use OS entropy for randomness.
 * These are the "batch" and "collection" operations on arrays.
 *
 * For basic value generation (int, double, bool, etc.), see runtime_random_basic.c.
 * For instance methods using seeded PRNG, see runtime_random.c.
 * ============================================================================ */

/* ============================================================================
 * Static Batch Generation Methods (OS Entropy)
 * ============================================================================
 * Generate multiple values in one call for performance.
 * Uses arena allocation for efficient array creation.
 * ============================================================================ */

/*
 * Static: Array of random integers in range [min, max] inclusive.
 */
long *rt_random_static_int_many(RtArena *arena, long min, long max, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    long *result = rt_arena_alloc(arena, (size_t)count * sizeof(long));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_int(min, max);
    }

    return result;
}

/*
 * Static: Array of random longs in range [min, max] inclusive.
 */
long long *rt_random_static_long_many(RtArena *arena, long long min, long long max, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    long long *result = rt_arena_alloc(arena, (size_t)count * sizeof(long long));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_long(min, max);
    }

    return result;
}

/*
 * Static: Array of random doubles in range [min, max).
 */
double *rt_random_static_double_many(RtArena *arena, double min, double max, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    double *result = rt_arena_alloc(arena, (size_t)count * sizeof(double));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_double(min, max);
    }

    return result;
}

/*
 * Static: Array of random booleans.
 */
int *rt_random_static_bool_many(RtArena *arena, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    int *result = rt_arena_alloc(arena, (size_t)count * sizeof(int));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_bool();
    }

    return result;
}

/*
 * Static: Array of gaussian samples.
 */
double *rt_random_static_gaussian_many(RtArena *arena, double mean, double stddev, long count) {
    if (arena == NULL || count <= 0) {
        return NULL;
    }

    double *result = rt_arena_alloc(arena, (size_t)count * sizeof(double));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_static_gaussian(mean, stddev);
    }

    return result;
}

/* ============================================================================
 * Static Collection Operations (OS Entropy)
 * ============================================================================
 * Random selection from arrays using OS entropy.
 * ============================================================================ */

/*
 * Static: Random element from long array.
 * Returns 0 if array is NULL or empty.
 */
long rt_random_static_choice_long(long *arr, long len) {
    if (arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/*
 * Static: Random element from double array.
 * Returns 0.0 if array is NULL or empty.
 */
double rt_random_static_choice_double(double *arr, long len) {
    if (arr == NULL || len <= 0) {
        return 0.0;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/*
 * Static: Random element from string array.
 * Returns NULL if array is NULL or empty.
 */
char *rt_random_static_choice_string(char **arr, long len) {
    if (arr == NULL || len <= 0) {
        return NULL;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/*
 * Static: Random element from bool (int) array.
 * Returns 0 if array is NULL or empty.
 */
int rt_random_static_choice_bool(int *arr, long len) {
    if (arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/*
 * Static: Random element from byte array.
 * Returns 0 if array is NULL or empty.
 */
unsigned char rt_random_static_choice_byte(unsigned char *arr, long len) {
    if (arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_static_int(0, len - 1);
    return arr[index];
}

/* ============================================================================
 * Weight Validation Helper
 * ============================================================================
 * Validates that a weights array is suitable for weighted random selection.
 * ============================================================================ */

/*
 * Validate weights array for weighted random selection.
 * Requirements:
 *   - weights must not be NULL
 *   - len must be > 0 (non-empty array)
 *   - All weights must be positive (> 0)
 *   - Sum of weights must be non-zero
 *
 * Returns: 1 if valid, 0 if invalid
 */
int rt_random_validate_weights(double *weights, long len) {
    /* Check for NULL pointer */
    if (weights == NULL) {
        return 0;
    }

    /* Check for empty array */
    if (len <= 0) {
        return 0;
    }

    /* Check all weights are positive and calculate sum */
    double sum = 0.0;
    for (long i = 0; i < len; i++) {
        if (weights[i] <= 0.0) {
            /* Weight must be strictly positive */
            return 0;
        }
        sum += weights[i];
    }

    /* Check sum is non-zero (technically covered by positive check, but explicit) */
    if (sum <= 0.0) {
        return 0;
    }

    return 1;
}

/*
 * Build cumulative distribution array from weights.
 * Normalizes weights to sum to 1.0 and creates cumulative distribution.
 *
 * For weights [0.7, 0.25, 0.05], produces cumulative [0.7, 0.95, 1.0].
 * For single-element weight [1.0], produces cumulative [1.0].
 *
 * Returns: Arena-allocated array of cumulative probabilities, or NULL on error.
 */
double *rt_random_build_cumulative(RtArena *arena, double *weights, long len) {
    /* Validate inputs */
    if (arena == NULL || weights == NULL || len <= 0) {
        return NULL;
    }

    /* Calculate sum of all weights */
    double sum = 0.0;
    for (long i = 0; i < len; i++) {
        sum += weights[i];
    }

    /* Avoid division by zero */
    if (sum <= 0.0) {
        return NULL;
    }

    /* Allocate cumulative array using arena */
    double *cumulative = rt_arena_alloc(arena, (size_t)len * sizeof(double));

    /* Build cumulative distribution with normalization */
    double running_sum = 0.0;
    for (long i = 0; i < len; i++) {
        running_sum += weights[i] / sum;  /* Normalize each weight */
        cumulative[i] = running_sum;
    }

    /* Ensure last element is exactly 1.0 to handle floating point errors */
    cumulative[len - 1] = 1.0;

    return cumulative;
}

/*
 * Select index from cumulative distribution using a random value.
 * Uses binary search to find the first index where cumulative[index] > random_val.
 *
 * For cumulative [0.7, 0.95, 1.0]:
 *   - random_val in [0, 0.7) returns 0
 *   - random_val in [0.7, 0.95) returns 1
 *   - random_val in [0.95, 1.0) returns 2
 *
 * Returns: Index into the original array (0 to len-1)
 */
long rt_random_select_weighted_index(double random_val, double *cumulative, long len) {
    /* Handle invalid inputs */
    if (cumulative == NULL || len <= 0) {
        return 0;
    }

    /* Handle single element case - always return 0 */
    if (len == 1) {
        return 0;
    }

    /* Handle edge case: random_val >= 1.0 (shouldn't happen but be safe) */
    if (random_val >= 1.0) {
        return len - 1;
    }

    /* Handle edge case: random_val <= 0.0 */
    if (random_val <= 0.0) {
        return 0;
    }

    /* Binary search for the first index where cumulative[index] > random_val */
    long left = 0;
    long right = len - 1;

    while (left < right) {
        long mid = left + (right - left) / 2;

        if (cumulative[mid] > random_val) {
            /* Found a valid index, but there might be a smaller one */
            right = mid;
        } else {
            /* cumulative[mid] <= random_val, need to look right */
            left = mid + 1;
        }
    }

    return left;
}

/* ============================================================================
 * Static Weighted Choice Functions (OS Entropy)
 * ============================================================================
 * Weighted random selection from arrays using OS entropy.
 * Uses the helper functions: validate_weights, build_cumulative, select_weighted_index.
 * ============================================================================ */

/*
 * Static: Weighted random choice from long array.
 * Returns a random element with probability proportional to its weight.
 * Returns 0 if inputs are invalid (NULL arrays, invalid weights).
 */
long rt_random_static_weighted_choice_long(long *arr, double *weights) {
    /* Handle NULL inputs */
    if (arr == NULL || weights == NULL) {
        return 0;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return 0;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return 0;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return 0;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return 0;
    }

    /* Generate random value in [0, 1) using OS entropy */
    double random_val = rt_random_static_double(0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    long result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Static: Weighted random choice from double array.
 * Returns a random element with probability proportional to its weight.
 * Returns 0.0 if inputs are invalid (NULL arrays, invalid weights).
 */
double rt_random_static_weighted_choice_double(double *arr, double *weights) {
    /* Handle NULL inputs */
    if (arr == NULL || weights == NULL) {
        return 0.0;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return 0.0;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return 0.0;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return 0.0;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return 0.0;
    }

    /* Generate random value in [0, 1) using OS entropy */
    double random_val = rt_random_static_double(0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    double result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Static: Weighted random choice from string array.
 * Returns a random element with probability proportional to its weight.
 * Returns NULL if inputs are invalid (NULL arrays, invalid weights).
 */
char *rt_random_static_weighted_choice_string(char **arr, double *weights) {
    /* Handle NULL inputs */
    if (arr == NULL || weights == NULL) {
        return NULL;
    }

    /* Get array length */
    long len = (long)rt_array_length(arr);
    if (len <= 0) {
        return NULL;
    }

    /* Validate weights using helper */
    if (!rt_random_validate_weights(weights, len)) {
        return NULL;
    }

    /* Create temporary arena for cumulative distribution */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    /* Build cumulative distribution using helper */
    double *cumulative = rt_random_build_cumulative(temp_arena, weights, len);
    if (cumulative == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Generate random value in [0, 1) using OS entropy */
    double random_val = rt_random_static_double(0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    char *result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/* ============================================================================
 * Static Shuffle Functions (OS Entropy)
 * ============================================================================
 * Fisher-Yates (modern/Knuth) shuffle algorithm.
 * Shuffles array in-place with no allocation.
 * Uses OS entropy for true randomness.
 * ============================================================================ */

/*
 * Static: Shuffle long array in-place using Fisher-Yates algorithm.
 * For i from n-1 down to 1:
 *   j = random index in [0, i]
 *   swap arr[i] and arr[j]
 */
void rt_random_static_shuffle_long(long *arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    /* Fisher-Yates shuffle (modern variant) */
    for (size_t i = n - 1; i > 0; i--) {
        /* Generate random index j in [0, i] inclusive */
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        /* Swap arr[i] and arr[j] */
        long temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Static: Shuffle double array in-place using Fisher-Yates algorithm.
 */
void rt_random_static_shuffle_double(double *arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        double temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Static: Shuffle string array in-place using Fisher-Yates algorithm.
 */
void rt_random_static_shuffle_string(char **arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        char *temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Static: Shuffle bool array in-place using Fisher-Yates algorithm.
 */
void rt_random_static_shuffle_bool(int *arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Static: Shuffle byte array in-place using Fisher-Yates algorithm.
 */
void rt_random_static_shuffle_byte(unsigned char *arr) {
    if (arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_static_int(0, (long)i);

        unsigned char temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/* ============================================================================
 * Static Sample Functions (OS Entropy)
 * ============================================================================
 * Random sampling without replacement using partial Fisher-Yates shuffle.
 * Returns a new array with `count` randomly selected elements from the input.
 * ============================================================================ */

/*
 * Static: Random sample without replacement from long array.
 * Uses partial Fisher-Yates shuffle for efficiency.
 * Returns NULL if:
 *   - arena is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected elements.
 */
long *rt_random_static_sample_long(RtArena *arena, long *arr, long count) {
    /* Validate arena and array */
    if (arena == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    long *temp = rt_arena_alloc(temp_arena, n * sizeof(long));
    memcpy(temp, arr, n * sizeof(long));

    /* Allocate result array using arena */
    long *result = rt_array_create_long(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive */
        size_t j = (size_t)rt_random_static_int((long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        long swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Static: Random sample without replacement from double array.
 * Uses partial Fisher-Yates shuffle for efficiency.
 * Returns NULL if:
 *   - arena is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected elements.
 */
double *rt_random_static_sample_double(RtArena *arena, double *arr, long count) {
    /* Validate arena and array */
    if (arena == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    double *temp = rt_arena_alloc(temp_arena, n * sizeof(double));
    memcpy(temp, arr, n * sizeof(double));

    /* Allocate result array using arena */
    double *result = rt_array_create_double(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive */
        size_t j = (size_t)rt_random_static_int((long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        double swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Static: Random sample without replacement from string array.
 * Uses partial Fisher-Yates shuffle for efficiency.
 * Returns NULL if:
 *   - arena is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected string pointers.
 */
char **rt_random_static_sample_string(RtArena *arena, char **arr, long count) {
    /* Validate arena and array */
    if (arena == NULL || arr == NULL) {
        return NULL;
    }

    /* Validate count */
    if (count <= 0) {
        return NULL;
    }

    /* Get array length */
    size_t n = rt_array_length(arr);

    /* Validate count <= array length */
    if (count > (long)n) {
        return NULL;
    }

    /* Create temporary copy of the array for shuffling */
    RtArena *temp_arena = rt_arena_create(NULL);
    if (temp_arena == NULL) {
        return NULL;
    }

    char **temp = rt_arena_alloc(temp_arena, n * sizeof(char *));
    memcpy(temp, arr, n * sizeof(char *));

    /* Allocate result array using arena */
    char **result = rt_array_create_string(arena, count, NULL);
    if (result == NULL) {
        rt_arena_destroy(temp_arena);
        return NULL;
    }

    /* Partial Fisher-Yates shuffle: only shuffle the first `count` elements */
    for (long i = 0; i < count; i++) {
        /* Generate random index j in [i, n-1] inclusive */
        size_t j = (size_t)rt_random_static_int((long)i, (long)(n - 1));

        /* Swap temp[i] and temp[j] */
        char *swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;

        /* Copy selected element to result */
        result[i] = temp[i];
    }

    rt_arena_destroy(temp_arena);

    return result;
}
