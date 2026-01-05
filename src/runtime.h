#ifndef RUNTIME_H
#define RUNTIME_H

/* ============================================================================
 * Sindarin Runtime Library - Main Header
 * ============================================================================
 * This header provides all runtime functionality for compiled Sindarin programs.
 * Include order matters for dependencies - arena must come first.
 * ============================================================================ */

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* Core modules - arena must be first as other modules depend on it */
#include "runtime/runtime_arena.h"

/* Data type modules - depend on arena */
#include "runtime/runtime_string.h"
#include "runtime/runtime_array.h"

/* I/O modules - depend on arena and string */
#include "runtime/runtime_file.h"
#include "runtime/runtime_io.h"

/* Utility modules - depend on arena */
#include "runtime/runtime_byte.h"
#include "runtime/runtime_path.h"
#include "runtime/runtime_date.h"
#include "runtime/runtime_time.h"

/* Threading module - depends on arena */
#include "runtime/runtime_thread.h"

/* Process module - depends on arena and string */
#include "runtime/runtime_process.h"

/* Network module - depends on arena */
#include "runtime/runtime_net.h"

/* Random module - depends on arena and array */
#include "runtime/runtime_random.h"

/* ============================================================================
 * Arithmetic operations
 * ============================================================================ */

/* Long arithmetic (with overflow checking) */
long rt_add_long(long a, long b);
long rt_sub_long(long a, long b);
long rt_mul_long(long a, long b);
long rt_div_long(long a, long b);
long rt_mod_long(long a, long b);
long rt_neg_long(long a);

/* Long comparisons - inlined for performance */
static inline int rt_eq_long(long a, long b) { return a == b; }
static inline int rt_ne_long(long a, long b) { return a != b; }
static inline int rt_lt_long(long a, long b) { return a < b; }
static inline int rt_le_long(long a, long b) { return a <= b; }
static inline int rt_gt_long(long a, long b) { return a > b; }
static inline int rt_ge_long(long a, long b) { return a >= b; }

/* Double arithmetic (with overflow checking) */
double rt_add_double(double a, double b);
double rt_sub_double(double a, double b);
double rt_mul_double(double a, double b);
double rt_div_double(double a, double b);
double rt_neg_double(double a);

/* Double comparisons - inlined for performance */
static inline int rt_eq_double(double a, double b) { return a == b; }
static inline int rt_ne_double(double a, double b) { return a != b; }
static inline int rt_lt_double(double a, double b) { return a < b; }
static inline int rt_le_double(double a, double b) { return a <= b; }
static inline int rt_gt_double(double a, double b) { return a > b; }
static inline int rt_ge_double(double a, double b) { return a >= b; }

/* Boolean operations - inlined for performance */
static inline int rt_not_bool(int a) { return !a; }

/* Increment/decrement */
long rt_post_inc_long(long *p);
long rt_post_dec_long(long *p);

/* String comparisons - inlined for performance */
static inline int rt_eq_string(const char *a, const char *b) { return strcmp(a, b) == 0; }
static inline int rt_ne_string(const char *a, const char *b) { return strcmp(a, b) != 0; }
static inline int rt_lt_string(const char *a, const char *b) { return strcmp(a, b) < 0; }
static inline int rt_le_string(const char *a, const char *b) { return strcmp(a, b) <= 0; }
static inline int rt_gt_string(const char *a, const char *b) { return strcmp(a, b) > 0; }
static inline int rt_ge_string(const char *a, const char *b) { return strcmp(a, b) >= 0; }

/* Check if string is empty or contains only whitespace */
int rt_str_is_blank(const char *str);

/* Split string on whitespace */
char **rt_str_split_whitespace(RtArena *arena, const char *str);

/* Split string on line endings */
char **rt_str_split_lines(RtArena *arena, const char *str);

#endif /* RUNTIME_H */
