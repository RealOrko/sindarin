#ifndef RUNTIME_RANDOM_STATIC_H
#define RUNTIME_RANDOM_STATIC_H

#include "runtime_random_core.h"

/* ============================================================================
 * Static Random Methods (OS Entropy)
 * ============================================================================
 * This module contains static methods that use OS entropy for randomness.
 * These are the "batch" and "collection" operations on arrays.
 *
 * For basic value generation (int, double, bool, etc.), see runtime_random_basic.h.
 * For instance methods using seeded PRNG, see runtime_random.h.
 * ============================================================================ */

/* ============================================================================
 * Static Batch Generation Methods (OS Entropy)
 * ============================================================================
 * Generate multiple values in one call for performance.
 * ============================================================================ */

/* Array of random integers in range [min, max] inclusive - uses long long for 64-bit portability */
long long *rt_random_static_int_many(RtArena *arena, long long min, long long max, long count);

/* Array of random longs in range [min, max] inclusive */
long long *rt_random_static_long_many(RtArena *arena, long long min, long long max, long count);

/* Array of random doubles in range [min, max) */
double *rt_random_static_double_many(RtArena *arena, double min, double max, long count);

/* Array of random booleans */
int *rt_random_static_bool_many(RtArena *arena, long count);

/* Array of gaussian samples */
double *rt_random_static_gaussian_many(RtArena *arena, double mean, double stddev, long count);

/* ============================================================================
 * Static Collection Operations (OS Entropy)
 * ============================================================================ */

/* Random element from long array - uses long long for 64-bit portability */
long long rt_random_static_choice_long(long long *arr, long len);

/* Random element from double array */
double rt_random_static_choice_double(double *arr, long len);

/* Random element from string array */
char *rt_random_static_choice_string(char **arr, long len);

/* Random element from int (bool) array */
int rt_random_static_choice_bool(int *arr, long len);

/* Random element from byte array */
unsigned char rt_random_static_choice_byte(unsigned char *arr, long len);

/* ============================================================================
 * Weight Validation Helper
 * ============================================================================
 * Validates that weights array is suitable for weighted random selection.
 * Requirements:
 *   - All weights must be positive (> 0)
 *   - Sum of weights must be non-zero
 *   - Array must not be empty
 * Returns: 1 if valid, 0 if invalid
 * ============================================================================ */

/* Validate weights array for weighted random selection */
int rt_random_validate_weights(double *weights, long len);

/*
 * Build cumulative distribution array from weights.
 * Normalizes weights to sum to 1.0 and creates cumulative distribution.
 * Result array has same length as input weights.
 * Each cumulative[i] = sum(normalized_weights[0..i]).
 * Last element will be 1.0 (or very close due to floating point).
 * Returns NULL if arena is NULL, weights is NULL, or len <= 0.
 */
double *rt_random_build_cumulative(RtArena *arena, double *weights, long len);

/*
 * Select index from cumulative distribution using random value.
 * Takes a random double in [0, 1) and cumulative distribution array.
 * Returns index where cumulative[index] > random_val (first such index).
 * Uses binary search for O(log n) performance.
 * Returns 0 if random_val is 0.0 or inputs are invalid.
 * Returns len-1 if random_val is >= 1.0 (safety for edge case).
 */
long rt_random_select_weighted_index(double random_val, double *cumulative, long len);

/* Weighted random choice from long array - uses long long for 64-bit portability */
long long rt_random_static_weighted_choice_long(long long *arr, double *weights);

/* Weighted random choice from double array */
double rt_random_static_weighted_choice_double(double *arr, double *weights);

/* Weighted random choice from string array */
char *rt_random_static_weighted_choice_string(char **arr, double *weights);

/* Shuffle long array in place - uses long long for 64-bit portability */
void rt_random_static_shuffle_long(long long *arr);

/* Shuffle double array in place */
void rt_random_static_shuffle_double(double *arr);

/* Shuffle string array in place */
void rt_random_static_shuffle_string(char **arr);

/* Shuffle bool array in place */
void rt_random_static_shuffle_bool(int *arr);

/* Shuffle byte array in place */
void rt_random_static_shuffle_byte(unsigned char *arr);

/* Random sample without replacement from long array - uses long long for 64-bit portability */
long long *rt_random_static_sample_long(RtArena *arena, long long *arr, long count);

/* Random sample without replacement from double array */
double *rt_random_static_sample_double(RtArena *arena, double *arr, long count);

/* Random sample without replacement from string array */
char **rt_random_static_sample_string(RtArena *arena, char **arr, long count);

#endif /* RUNTIME_RANDOM_STATIC_H */
