#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "runtime_random_choice.h"
#include "runtime_random_basic.h"
#include "runtime_random_static.h"
#include "runtime_array.h"

/* ============================================================================
 * Random Choice and Selection Functions
 * ============================================================================
 * This module contains choice and weighted choice functions for instance
 * methods (using seeded PRNG).
 *
 * Shuffle and sample functions are in runtime_random_collection.c.
 * ============================================================================ */

/* ============================================================================
 * Instance Collection Operations (Seeded PRNG)
 * ============================================================================
 * Random selection from arrays using the instance's PRNG state.
 * ============================================================================ */

/*
 * Instance: Random element from long array.
 * Returns 0 if array is NULL or empty.
 */
long rt_random_choice_long(RtRandom *rng, long *arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/*
 * Instance: Random element from double array.
 * Returns 0.0 if array is NULL or empty.
 */
double rt_random_choice_double(RtRandom *rng, double *arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return 0.0;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/*
 * Instance: Random element from string array.
 * Returns NULL if array is NULL or empty.
 */
char *rt_random_choice_string(RtRandom *rng, char **arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return NULL;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/*
 * Instance: Random element from bool (int) array.
 * Returns 0 if array is NULL or empty.
 */
int rt_random_choice_bool(RtRandom *rng, int *arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/*
 * Instance: Random element from byte array.
 * Returns 0 if array is NULL or empty.
 */
unsigned char rt_random_choice_byte(RtRandom *rng, unsigned char *arr, long len) {
    if (rng == NULL || arr == NULL || len <= 0) {
        return 0;
    }

    long index = rt_random_int(rng, 0, len - 1);
    return arr[index];
}

/* ============================================================================
 * Instance Weighted Choice Functions (Seeded PRNG)
 * ============================================================================
 * Weighted random selection from arrays using seeded PRNG.
 * Uses the helper functions from runtime_random_static.c:
 * validate_weights, build_cumulative, select_weighted_index.
 * ============================================================================ */

/*
 * Instance: Weighted random choice from long array.
 * Returns a random element with probability proportional to its weight.
 * Returns 0 if inputs are invalid (NULL rng, NULL arrays, invalid weights).
 */
long rt_random_weighted_choice_long(RtRandom *rng, long *arr, double *weights) {
    /* Handle NULL inputs */
    if (rng == NULL || arr == NULL || weights == NULL) {
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

    /* Generate random value in [0, 1) using seeded PRNG */
    double random_val = rt_random_double(rng, 0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    long result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Instance: Weighted random choice from double array.
 * Returns a random element with probability proportional to its weight.
 * Returns 0.0 if inputs are invalid (NULL rng, NULL arrays, invalid weights).
 */
double rt_random_weighted_choice_double(RtRandom *rng, double *arr, double *weights) {
    /* Handle NULL inputs */
    if (rng == NULL || arr == NULL || weights == NULL) {
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

    /* Generate random value in [0, 1) using seeded PRNG */
    double random_val = rt_random_double(rng, 0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    double result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

/*
 * Instance: Weighted random choice from string array.
 * Returns a random element with probability proportional to its weight.
 * Returns NULL if inputs are invalid (NULL rng, NULL arrays, invalid weights).
 */
char *rt_random_weighted_choice_string(RtRandom *rng, char **arr, double *weights) {
    /* Handle NULL inputs */
    if (rng == NULL || arr == NULL || weights == NULL) {
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

    /* Generate random value in [0, 1) using seeded PRNG */
    double random_val = rt_random_double(rng, 0.0, 1.0);

    /* Select index using helper */
    long index = rt_random_select_weighted_index(random_val, cumulative, len);

    /* Get the selected value */
    char *result = arr[index];

    /* Clean up temporary arena */
    rt_arena_destroy(temp_arena);

    return result;
}

