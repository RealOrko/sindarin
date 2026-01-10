#ifndef RUNTIME_RANDOM_CHOICE_H
#define RUNTIME_RANDOM_CHOICE_H

#include "runtime_random_core.h"

/* ============================================================================
 * Random Choice and Selection Functions
 * ============================================================================
 * This module contains choice and weighted choice functions for instance
 * methods (using seeded PRNG).
 *
 * Choice functions select a random element from an array.
 * Weighted choice functions select with probability proportional to weights.
 *
 * Shuffle and sample functions are in runtime_random_collection.h.
 * ============================================================================ */

/* ============================================================================
 * Instance Choice Functions (Seeded PRNG)
 * ============================================================================ */

/* Random element from long array - uses long long for 64-bit portability */
long long rt_random_choice_long(RtRandom *rng, long long *arr, long len);

/* Random element from double array */
double rt_random_choice_double(RtRandom *rng, double *arr, long len);

/* Random element from string array */
char *rt_random_choice_string(RtRandom *rng, char **arr, long len);

/* Random element from int (bool) array */
int rt_random_choice_bool(RtRandom *rng, int *arr, long len);

/* Random element from byte array */
unsigned char rt_random_choice_byte(RtRandom *rng, unsigned char *arr, long len);

/* ============================================================================
 * Instance Weighted Choice Functions (Seeded PRNG)
 * ============================================================================ */

/* Weighted random choice from long array - uses long long for 64-bit portability */
long long rt_random_weighted_choice_long(RtRandom *rng, long long *arr, double *weights);

/* Weighted random choice from double array */
double rt_random_weighted_choice_double(RtRandom *rng, double *arr, double *weights);

/* Weighted random choice from string array */
char *rt_random_weighted_choice_string(RtRandom *rng, char **arr, double *weights);

#endif /* RUNTIME_RANDOM_CHOICE_H */
