#ifndef RUNTIME_RANDOM_COLLECTION_H
#define RUNTIME_RANDOM_COLLECTION_H

#include "runtime_random_core.h"

/* ============================================================================
 * Random Collection Operations
 * ============================================================================
 * This module contains collection manipulation functions:
 * - Batch generation (*_many) functions for generating multiple values
 * - Shuffle functions for randomizing array order in-place
 * - Sample functions for selecting multiple elements without replacement
 *
 * All functions use seeded PRNG for reproducible results.
 * ============================================================================ */

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
 * Instance Shuffle Functions (Seeded PRNG)
 * ============================================================================ */

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

/* ============================================================================
 * Instance Sample Functions (Seeded PRNG)
 * ============================================================================ */

/* Random sample without replacement from long array */
long *rt_random_sample_long(RtArena *arena, RtRandom *rng, long *arr, long count);

/* Random sample without replacement from double array */
double *rt_random_sample_double(RtArena *arena, RtRandom *rng, double *arr, long count);

/* Random sample without replacement from string array */
char **rt_random_sample_string(RtArena *arena, RtRandom *rng, char **arr, long count);

#endif /* RUNTIME_RANDOM_COLLECTION_H */
