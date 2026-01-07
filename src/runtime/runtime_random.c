/* ============================================================================
 * Random Module - Main Entry Point
 * ============================================================================
 * This file serves as the main entry point for the random module.
 * All functionality has been split into separate files:
 *
 * - runtime_random_core.c: Core RNG type, seeding, entropy
 * - runtime_random_basic.c: Basic value generation (int, double, bool, etc.)
 * - runtime_random_static.c: Static methods using OS entropy
 * - runtime_random_choice.c: Choice and weighted choice functions
 * - runtime_random_collection.c: Shuffle, sample, and batch generation
 *
 * This file is kept for backwards compatibility and as an umbrella include.
 * ============================================================================ */

#include "runtime_random.h"

/* No additional functions needed - all functionality is in sub-modules */
