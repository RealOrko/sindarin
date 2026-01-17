#ifndef RUNTIME_UUID_H
#define RUNTIME_UUID_H

#include "runtime_arena.h"
#include <stdint.h>

/* ============================================================================
 * UUID Type Definition
 * ============================================================================
 * Universally Unique Identifiers (UUIDs) are 128-bit values used to identify
 * resources without central coordination. This implementation follows RFC 9562.
 *
 * Supported versions:
 * - v4: Random UUID (simple unique IDs)
 * - v5: SHA-1 hash of namespace + name (deterministic from input)
 * - v7: Timestamp + random (time-ordered, modern default)
 *
 * The v7 UUID is the recommended default for most use cases as it provides
 * excellent database index performance due to its time-ordered nature.
 * ============================================================================ */

/* UUID handle - 128-bit value stored as two 64-bit parts */
typedef struct RtUuid {
    uint64_t high;  /* Most significant 64 bits */
    uint64_t low;   /* Least significant 64 bits */
} RtUuid;

/* ============================================================================
 * Namespace Constants (RFC 9562)
 * ============================================================================
 * Standard namespaces for generating v5 deterministic UUIDs.
 * ============================================================================ */

/* DNS namespace: 6ba7b810-9dad-11d1-80b4-00c04fd430c8 */
extern const RtUuid RT_UUID_NAMESPACE_DNS;

/* URL namespace: 6ba7b811-9dad-11d1-80b4-00c04fd430c8 */
extern const RtUuid RT_UUID_NAMESPACE_URL;

/* OID namespace: 6ba7b812-9dad-11d1-80b4-00c04fd430c8 */
extern const RtUuid RT_UUID_NAMESPACE_OID;

/* X.500 namespace: 6ba7b814-9dad-11d1-80b4-00c04fd430c8 */
extern const RtUuid RT_UUID_NAMESPACE_X500;

/* ============================================================================
 * Factory Methods - UUID Generation
 * ============================================================================ */

/* Create a UUIDv7 (time-ordered) - recommended default */
RtUuid *rt_uuid_create(RtArena *arena);

/* Generate a UUIDv4 (random) */
RtUuid *rt_uuid_v4(RtArena *arena);

/* Generate a UUIDv5 (SHA-1 hash of namespace + name) */
RtUuid *rt_uuid_v5(RtArena *arena, RtUuid *namespace_uuid, const char *name);

/* Generate a UUIDv7 (timestamp + random) */
RtUuid *rt_uuid_v7(RtArena *arena);

/* ============================================================================
 * Factory Methods - Parsing
 * ============================================================================ */

/* Parse from standard 36-char format (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx) */
RtUuid *rt_uuid_from_string(RtArena *arena, const char *str);

/* Parse from 32-char hex format (no dashes) */
RtUuid *rt_uuid_from_hex(RtArena *arena, const char *str);

/* Parse from 22-char URL-safe base64 format */
RtUuid *rt_uuid_from_base64(RtArena *arena, const char *str);

/* Create from 16-byte array */
RtUuid *rt_uuid_from_bytes(RtArena *arena, const unsigned char *bytes);

/* ============================================================================
 * Special UUIDs
 * ============================================================================ */

/* Get nil UUID (all zeros: 00000000-0000-0000-0000-000000000000) */
RtUuid *rt_uuid_nil(RtArena *arena);

/* Get max UUID (all ones: ffffffff-ffff-ffff-ffff-ffffffffffff) */
RtUuid *rt_uuid_max(RtArena *arena);

/* ============================================================================
 * Namespace Accessors
 * ============================================================================ */

/* Get DNS namespace UUID */
RtUuid *rt_uuid_namespace_dns(RtArena *arena);

/* Get URL namespace UUID */
RtUuid *rt_uuid_namespace_url(RtArena *arena);

/* Get OID namespace UUID */
RtUuid *rt_uuid_namespace_oid(RtArena *arena);

/* Get X.500 namespace UUID */
RtUuid *rt_uuid_namespace_x500(RtArena *arena);

/* ============================================================================
 * Property Getters
 * ============================================================================ */

/* Get UUID version (1-8) */
long rt_uuid_get_version(RtUuid *uuid);

/* Get UUID variant */
long rt_uuid_get_variant(RtUuid *uuid);

/* Check if UUID is nil (all zeros) */
int rt_uuid_is_nil(RtUuid *uuid);

/* ============================================================================
 * Time Extraction (v7 only)
 * ============================================================================
 * These methods only work on v7 UUIDs. Calling on non-v7 UUIDs will cause
 * a runtime error.
 * ============================================================================ */

/* Get Unix timestamp in milliseconds (v7 only, throws on non-v7) */
long long rt_uuid_get_timestamp(RtUuid *uuid);

/* ============================================================================
 * Conversion Methods
 * ============================================================================ */

/* Format as standard 36-char string (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx) */
char *rt_uuid_to_string(RtArena *arena, RtUuid *uuid);

/* Format as 32-char hex string (no dashes) */
char *rt_uuid_to_hex(RtArena *arena, RtUuid *uuid);

/* Format as 22-char URL-safe base64 string */
char *rt_uuid_to_base64(RtArena *arena, RtUuid *uuid);

/* Convert to 16-byte array */
unsigned char *rt_uuid_to_bytes(RtArena *arena, RtUuid *uuid);

/* ============================================================================
 * Comparison Methods
 * ============================================================================ */

/* Check if two UUIDs are equal */
int rt_uuid_equals(RtUuid *uuid, RtUuid *other);

/* Compare two UUIDs (returns -1, 0, or 1) */
int rt_uuid_compare(RtUuid *uuid, RtUuid *other);

/* Check if uuid < other */
int rt_uuid_is_less_than(RtUuid *uuid, RtUuid *other);

/* Check if uuid > other */
int rt_uuid_is_greater_than(RtUuid *uuid, RtUuid *other);

#endif /* RUNTIME_UUID_H */
