#ifndef RUNTIME_CAST_H
#define RUNTIME_CAST_H

/**
 * runtime_cast.h - Type Casting Inline Functions
 *
 * Provides clean type conversions between Sindarin's generated C types
 * and the types expected by C libraries. Used in SDK adapter functions
 * to ensure proper type matching at native function call sites.
 *
 * Sindarin type mappings:
 *   int/long  -> long long (64-bit signed)
 *   uint      -> uint64_t (64-bit unsigned)
 *   int32     -> int32_t
 *   uint32    -> uint32_t
 *   bool      -> bool (_Bool)
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * Integer Conversions: Sindarin int/long (long long) <-> C types
 * ============================================================================ */

/* long long -> narrower types */
static inline long rt_ll_to_long(long long x) { return (long)x; }
static inline int rt_ll_to_int(long long x) { return (int)x; }
static inline int32_t rt_ll_to_i32(long long x) { return (int32_t)x; }
static inline int16_t rt_ll_to_i16(long long x) { return (int16_t)x; }
static inline int8_t rt_ll_to_i8(long long x) { return (int8_t)x; }

/* Narrower types -> long long */
static inline long long rt_long_to_ll(long x) { return (long long)x; }
static inline long long rt_int_to_ll(int x) { return (long long)x; }
static inline long long rt_i32_to_ll(int32_t x) { return (long long)x; }
static inline long long rt_i16_to_ll(int16_t x) { return (long long)x; }
static inline long long rt_i8_to_ll(int8_t x) { return (long long)x; }

/* ============================================================================
 * Unsigned Integer Conversions: Sindarin uint (uint64_t) <-> C types
 * ============================================================================ */

/* uint64_t -> narrower types */
static inline unsigned long rt_ull_to_ulong(uint64_t x) { return (unsigned long)x; }
static inline unsigned int rt_ull_to_uint(uint64_t x) { return (unsigned int)x; }
static inline uint32_t rt_ull_to_u32(uint64_t x) { return (uint32_t)x; }
static inline uint16_t rt_ull_to_u16(uint64_t x) { return (uint16_t)x; }
static inline uint8_t rt_ull_to_u8(uint64_t x) { return (uint8_t)x; }
static inline size_t rt_ull_to_size(uint64_t x) { return (size_t)x; }

/* Narrower types -> uint64_t */
static inline uint64_t rt_ulong_to_ull(unsigned long x) { return (uint64_t)x; }
static inline uint64_t rt_uint_to_ull(unsigned int x) { return (uint64_t)x; }
static inline uint64_t rt_u32_to_ull(uint32_t x) { return (uint64_t)x; }
static inline uint64_t rt_u16_to_ull(uint16_t x) { return (uint16_t)x; }
static inline uint64_t rt_u8_to_ull(uint8_t x) { return (uint64_t)x; }
static inline uint64_t rt_size_to_ull(size_t x) { return (uint64_t)x; }

/* ============================================================================
 * Boolean Conversions: Sindarin bool (_Bool) <-> C int
 * ============================================================================ */

static inline int rt_bool_to_int(bool b) { return b ? 1 : 0; }
static inline bool rt_int_to_bool(int i) { return i != 0; }

/* ============================================================================
 * Signed/Unsigned Conversions (use with caution)
 * ============================================================================ */

static inline uint64_t rt_ll_to_ull(long long x) { return (uint64_t)x; }
static inline long long rt_ull_to_ll(uint64_t x) { return (long long)x; }

#endif /* RUNTIME_CAST_H */
