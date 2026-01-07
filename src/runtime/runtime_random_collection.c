#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "runtime_random_collection.h"
#include "runtime_random_basic.h"
#include "runtime_array.h"

/* ============================================================================
 * Random Collection Operations
 * ============================================================================
 * This module contains collection manipulation functions:
 * - Batch generation (*_many) for generating multiple values
 * - Shuffle functions for randomizing array order in-place
 * - Sample functions for selecting multiple elements without replacement
 * ============================================================================ */

/* ============================================================================
 * Instance Batch Generation Methods (Seeded PRNG)
 * ============================================================================
 * Generate multiple values in one call for performance.
 * Uses arena allocation for efficient array creation.
 * ============================================================================ */

/*
 * Instance: Array of random integers in range [min, max] inclusive.
 */
long *rt_random_int_many(RtArena *arena, RtRandom *rng, long min, long max, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    long *result = rt_arena_alloc(arena, (size_t)count * sizeof(long));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_int(rng, min, max);
    }

    return result;
}

/*
 * Instance: Array of random longs in range [min, max] inclusive.
 */
long long *rt_random_long_many(RtArena *arena, RtRandom *rng, long long min, long long max, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    long long *result = rt_arena_alloc(arena, (size_t)count * sizeof(long long));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_long(rng, min, max);
    }

    return result;
}

/*
 * Instance: Array of random doubles in range [min, max).
 */
double *rt_random_double_many(RtArena *arena, RtRandom *rng, double min, double max, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    double *result = rt_arena_alloc(arena, (size_t)count * sizeof(double));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_double(rng, min, max);
    }

    return result;
}

/*
 * Instance: Array of random booleans.
 */
int *rt_random_bool_many(RtArena *arena, RtRandom *rng, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    int *result = rt_arena_alloc(arena, (size_t)count * sizeof(int));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_bool(rng);
    }

    return result;
}

/*
 * Instance: Array of gaussian samples.
 */
double *rt_random_gaussian_many(RtArena *arena, RtRandom *rng, double mean, double stddev, long count) {
    if (arena == NULL || rng == NULL || count <= 0) {
        return NULL;
    }

    double *result = rt_arena_alloc(arena, (size_t)count * sizeof(double));

    for (long i = 0; i < count; i++) {
        result[i] = rt_random_gaussian(rng, mean, stddev);
    }

    return result;
}

/* ============================================================================
 * Instance Shuffle Functions (Seeded PRNG)
 * ============================================================================
 * Fisher-Yates (modern/Knuth) shuffle algorithm.
 * Shuffles array in-place with no allocation.
 * Uses seeded PRNG for reproducible shuffles.
 * ============================================================================ */

/*
 * Instance: Shuffle long array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_long(RtRandom *rng, long *arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        long temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Instance: Shuffle double array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_double(RtRandom *rng, double *arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        double temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Instance: Shuffle string array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_string(RtRandom *rng, char **arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        char *temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Instance: Shuffle bool array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_bool(RtRandom *rng, int *arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Instance: Shuffle byte array in-place using Fisher-Yates algorithm.
 */
void rt_random_shuffle_byte(RtRandom *rng, unsigned char *arr) {
    if (rng == NULL || arr == NULL) {
        return;
    }

    size_t n = rt_array_length(arr);
    if (n <= 1) {
        return;
    }

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)rt_random_int(rng, 0, (long)i);

        unsigned char temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/* ============================================================================
 * Instance Sample Functions (Seeded PRNG)
 * ============================================================================
 * Random sampling without replacement using seeded PRNG for reproducibility.
 * Returns a new array with `count` randomly selected elements from the input.
 * ============================================================================ */

/*
 * Instance: Random sample without replacement from long array.
 * Uses seeded PRNG for reproducible sampling.
 * Returns NULL if:
 *   - arena is NULL
 *   - rng is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected elements.
 */
long *rt_random_sample_long(RtArena *arena, RtRandom *rng, long *arr, long count) {
    /* Validate inputs */
    if (arena == NULL || rng == NULL || arr == NULL) {
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
        /* Generate random index j in [i, n-1] inclusive using seeded PRNG */
        size_t j = (size_t)rt_random_int(rng, (long)i, (long)(n - 1));

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
 * Instance: Random sample without replacement from double array.
 * Uses seeded PRNG for reproducible sampling.
 * Returns NULL if:
 *   - arena is NULL
 *   - rng is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected elements.
 */
double *rt_random_sample_double(RtArena *arena, RtRandom *rng, double *arr, long count) {
    /* Validate inputs */
    if (arena == NULL || rng == NULL || arr == NULL) {
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
        /* Generate random index j in [i, n-1] inclusive using seeded PRNG */
        size_t j = (size_t)rt_random_int(rng, (long)i, (long)(n - 1));

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
 * Instance: Random sample without replacement from string array.
 * Uses seeded PRNG for reproducible sampling.
 * Returns NULL if:
 *   - arena is NULL
 *   - rng is NULL
 *   - arr is NULL
 *   - count <= 0
 *   - count > array length (invalid sample size)
 *
 * Returns: Arena-allocated array of `count` randomly selected string pointers.
 */
char **rt_random_sample_string(RtArena *arena, RtRandom *rng, char **arr, long count) {
    /* Validate inputs */
    if (arena == NULL || rng == NULL || arr == NULL) {
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
        /* Generate random index j in [i, n-1] inclusive using seeded PRNG */
        size_t j = (size_t)rt_random_int(rng, (long)i, (long)(n - 1));

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
