#ifndef RUNTIME_RANDOM_H
#define RUNTIME_RANDOM_H

#include "runtime_arena.h"
#include <stdint.h>

/* ============================================================================
 * Random Number Generator Type
 * ============================================================================
 * Sindarin provides the Random type for generating random values. The default
 * static methods use OS entropy (secure by default). For reproducible sequences,
 * create an instance with createWithSeed().
 *
 * Two modes of operation:
 * 1. Static methods - Use OS entropy (cryptographically secure)
 * 2. Instance methods - Use seeded PRNG (reproducible sequences)
 * ============================================================================ */

/* Random generator handle */
typedef struct RtRandom {
    int is_seeded;          /* 0 = OS entropy, 1 = seeded PRNG */
    uint64_t state[4];      /* PRNG state (xoshiro256** algorithm) */
} RtRandom;

/* ============================================================================
 * Factory Methods
 * ============================================================================ */

/* Create an OS-entropy backed random instance */
RtRandom *rt_random_create(RtArena *arena);

/* Create a seeded PRNG instance for reproducible sequences */
RtRandom *rt_random_create_with_seed(RtArena *arena, long seed);

/* ============================================================================
 * Core Entropy Functions (Internal)
 * ============================================================================ */

/* Fill buffer with random bytes from OS entropy source */
void rt_random_fill_entropy(uint8_t *buf, size_t len);

/* ============================================================================
 * Static Value Generation Methods (OS Entropy)
 * ============================================================================
 * These methods use OS entropy directly - no seeding required.
 * Suitable for cryptographic and security-sensitive use cases.
 * ============================================================================ */

/* Random integer in range [min, max] inclusive */
long rt_random_static_int(long min, long max);

/* Random long in range [min, max] inclusive */
long long rt_random_static_long(long long min, long long max);

/* Random double in range [min, max) */
double rt_random_static_double(double min, double max);

/* Random boolean (50/50) */
int rt_random_static_bool(void);

/* Random byte (0-255) */
unsigned char rt_random_static_byte(void);

/* Array of random bytes */
unsigned char *rt_random_static_bytes(RtArena *arena, long count);

/* Sample from normal distribution */
double rt_random_static_gaussian(double mean, double stddev);

/* ============================================================================
 * Static Batch Generation Methods (OS Entropy)
 * ============================================================================
 * Generate multiple values in one call for performance.
 * ============================================================================ */

/* Array of random integers in range [min, max] inclusive */
long *rt_random_static_int_many(RtArena *arena, long min, long max, long count);

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

/* Random element from long array */
long rt_random_static_choice_long(long *arr, long len);

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

/* Weighted random choice from long array */
long rt_random_static_weighted_choice_long(long *arr, double *weights);

/* Weighted random choice from double array */
double rt_random_static_weighted_choice_double(double *arr, double *weights);

/* Weighted random choice from string array */
char *rt_random_static_weighted_choice_string(char **arr, double *weights);

/* Shuffle long array in place */
void rt_random_static_shuffle_long(long *arr);

/* Shuffle double array in place */
void rt_random_static_shuffle_double(double *arr);

/* Shuffle string array in place */
void rt_random_static_shuffle_string(char **arr);

/* Shuffle bool array in place */
void rt_random_static_shuffle_bool(int *arr);

/* Shuffle byte array in place */
void rt_random_static_shuffle_byte(unsigned char *arr);

/* Random sample without replacement from long array */
long *rt_random_static_sample_long(RtArena *arena, long *arr, long count);

/* Random sample without replacement from double array */
double *rt_random_static_sample_double(RtArena *arena, double *arr, long count);

/* Random sample without replacement from string array */
char **rt_random_static_sample_string(RtArena *arena, char **arr, long count);

/* ============================================================================
 * Instance Value Generation Methods (Seeded PRNG)
 * ============================================================================
 * These methods use the instance's internal PRNG state.
 * Reproducible when created with createWithSeed().
 * ============================================================================ */

/* Random integer in range [min, max] inclusive */
long rt_random_int(RtRandom *rng, long min, long max);

/* Random long in range [min, max] inclusive */
long long rt_random_long(RtRandom *rng, long long min, long long max);

/* Random double in range [min, max) */
double rt_random_double(RtRandom *rng, double min, double max);

/* Random boolean (50/50) */
int rt_random_bool(RtRandom *rng);

/* Random byte (0-255) */
unsigned char rt_random_byte(RtRandom *rng);

/* Array of random bytes */
unsigned char *rt_random_bytes(RtArena *arena, RtRandom *rng, long count);

/* Sample from normal distribution */
double rt_random_gaussian(RtRandom *rng, double mean, double stddev);

/* ============================================================================
 * Instance Batch Generation Methods (Seeded PRNG)
 * ============================================================================ */

/* Array of random integers in range [min, max] inclusive */
long *rt_random_int_many(RtArena *arena, RtRandom *rng, long min, long max, long count);

/* Array of random longs in range [min, max] inclusive */
long long *rt_random_long_many(RtArena *arena, RtRandom *rng, long long min, long long max, long count);

/* Array of random doubles in range [min, max) */
double *rt_random_double_many(RtArena *arena, RtRandom *rng, double min, double max, long count);

/* Array of random booleans */
int *rt_random_bool_many(RtArena *arena, RtRandom *rng, long count);

/* Array of gaussian samples */
double *rt_random_gaussian_many(RtArena *arena, RtRandom *rng, double mean, double stddev, long count);

/* ============================================================================
 * Instance Collection Operations (Seeded PRNG)
 * ============================================================================ */

/* Random element from long array */
long rt_random_choice_long(RtRandom *rng, long *arr, long len);

/* Random element from double array */
double rt_random_choice_double(RtRandom *rng, double *arr, long len);

/* Random element from string array */
char *rt_random_choice_string(RtRandom *rng, char **arr, long len);

/* Random element from int (bool) array */
int rt_random_choice_bool(RtRandom *rng, int *arr, long len);

/* Random element from byte array */
unsigned char rt_random_choice_byte(RtRandom *rng, unsigned char *arr, long len);

/* Weighted random choice from long array */
long rt_random_weighted_choice_long(RtRandom *rng, long *arr, double *weights);

/* Weighted random choice from double array */
double rt_random_weighted_choice_double(RtRandom *rng, double *arr, double *weights);

/* Weighted random choice from string array */
char *rt_random_weighted_choice_string(RtRandom *rng, char **arr, double *weights);

/* Shuffle long array in place */
void rt_random_shuffle_long(RtRandom *rng, long *arr);

/* Shuffle double array in place */
void rt_random_shuffle_double(RtRandom *rng, double *arr);

/* Shuffle string array in place */
void rt_random_shuffle_string(RtRandom *rng, char **arr);

/* Shuffle bool array in place */
void rt_random_shuffle_bool(RtRandom *rng, int *arr);

/* Shuffle byte array in place */
void rt_random_shuffle_byte(RtRandom *rng, unsigned char *arr);

/* Random sample without replacement from long array */
long *rt_random_sample_long(RtArena *arena, RtRandom *rng, long *arr, long count);

/* Random sample without replacement from double array */
double *rt_random_sample_double(RtArena *arena, RtRandom *rng, double *arr, long count);

/* Random sample without replacement from string array */
char **rt_random_sample_string(RtArena *arena, RtRandom *rng, char **arr, long count);

#endif /* RUNTIME_RANDOM_H */
