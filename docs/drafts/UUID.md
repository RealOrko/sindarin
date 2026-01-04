# UUID in Sindarin

> **DRAFT** - This specification is not yet implemented. This document explores design options for a `UUID` type in Sindarin's runtime.

## Overview

Universally Unique Identifiers (UUIDs) are 128-bit values used to identify resources without central coordination. [RFC 9562](https://datatracker.ietf.org/doc/rfc9562/) (May 2024) obsoletes the original RFC 4122 and introduces new UUID versions optimized for modern use cases.

```sindarin
// Generate a time-ordered UUID (v7) - recommended for most uses
var id: UUID = UUID.create()
print($"New record: {id}\n")

// Parse from string
var parsed: UUID = UUID.fromString("01912345-6789-7abc-8def-0123456789ab")

// Use as database key, map key, etc.
var users: Map<UUID, User> = {}
users[id] = currentUser
```

---

## Why a Dedicated Type?

UUIDs could be represented as strings, but a dedicated type provides:

1. **Type safety**: Can't accidentally pass an arbitrary string where a UUID is expected
2. **Validation**: Parsing catches malformed UUIDs at the point of entry
3. **Efficiency**: Internal 128-bit representation is more compact than 36-character strings
4. **Rich API**: Extract timestamps, compare versions, generate specific variants
5. **Version awareness**: v4 and v7 have different properties worth exposing

---

## The UUID Landscape

### Version History

| Version | Source | Sortable | Use Case |
|---------|--------|----------|----------|
| v1 | Timestamp + MAC address | Yes | Legacy, privacy concerns |
| v3 | MD5 hash of namespace + name | No | Deterministic from input |
| v4 | Random | No | Simple unique IDs |
| v5 | SHA-1 hash of namespace + name | No | Deterministic from input |
| v6 | Reordered v1 timestamp | Yes | Legacy compatibility |
| **v7** | **Timestamp + random** | **Yes** | **Modern default** |
| v8 | Custom | Varies | Application-specific |

### The v4 vs v7 Decision

**UUIDv4** (random):
```
f47ac10b-58cc-4372-a567-0e02b2c3d479
^^^^^^^^ ^^^^ ^^^^ ^^^^ ^^^^^^^^^^^^
   all random except version/variant bits
```

**UUIDv7** (time-ordered):
```
01912345-6789-7abc-8def-0123456789ab
^^^^^^^^ ^^^^ ^
   48-bit      |
   timestamp   version
```

### Why v7 is the New Default

[Buildkite reported](https://buildkite.com/resources/blog/goodbye-integers-hello-uuids/) real-world results after switching:

| Metric | UUIDv4 | UUIDv7 |
|--------|--------|--------|
| Write Ahead Log | Baseline | **50% reduction** |
| Index page splits | 5,000-10,000 per million | ~10-20 per million |
| Sort by creation | Requires extra column | Built-in |
| Index locality | Random (poor) | Sequential (excellent) |

The timestamp prefix means new UUIDs are inserted near each other in B-tree indexes, dramatically reducing fragmentation.

---

## Proposed Design for Sindarin

### The `UUID` Type

```sindarin
// Default: generate UUIDv7 (time-ordered, recommended)
var id: UUID = UUID.create()

// Explicit versions
var v7: UUID = UUID.v7()        // Time-ordered (same as create())
var v4: UUID = UUID.v4()        // Pure random

// Deterministic UUIDs from names
var ns: UUID = UUID.namespaceUrl()
var named: UUID = UUID.v5(ns, "https://example.com/users/alice")

// Parse from string
var parsed: UUID = UUID.fromString("01912345-6789-7abc-8def-0123456789ab")

// Parse from bytes
var fromBytes: UUID = UUID.fromBytes(byteArray)
```

### Static Methods

#### Factory Methods

| Method | Return | Description |
|--------|--------|-------------|
| `UUID.create()` | `UUID` | Generate UUIDv7 (recommended default) |
| `UUID.v7()` | `UUID` | Generate UUIDv7 (time-ordered) |
| `UUID.v4()` | `UUID` | Generate UUIDv4 (random) |
| `UUID.v5(namespace, name)` | `UUID` | Deterministic UUID from namespace + name |
| `UUID.fromString(str)` | `UUID` | Parse standard 36-char format |
| `UUID.fromHex(str)` | `UUID` | Parse 32-char hex format |
| `UUID.fromBase64(str)` | `UUID` | Parse 22-char base64 format |
| `UUID.fromBytes(bytes)` | `UUID` | Create from 16-byte array |

#### Namespace Constants

For v5 deterministic UUIDs, RFC 9562 defines standard namespaces:

| Method | Return | Description |
|--------|--------|-------------|
| `UUID.namespaceDns()` | `UUID` | `6ba7b810-9dad-11d1-80b4-00c04fd430c8` |
| `UUID.namespaceUrl()` | `UUID` | `6ba7b811-9dad-11d1-80b4-00c04fd430c8` |
| `UUID.namespaceOid()` | `UUID` | `6ba7b812-9dad-11d1-80b4-00c04fd430c8` |
| `UUID.namespaceX500()` | `UUID` | `6ba7b814-9dad-11d1-80b4-00c04fd430c8` |

#### Nil and Max

| Method | Return | Description |
|--------|--------|-------------|
| `UUID.nil()` | `UUID` | All zeros: `00000000-0000-0000-0000-000000000000` |
| `UUID.max()` | `UUID` | All ones: `ffffffff-ffff-ffff-ffff-ffffffffffff` |

### Instance Methods

#### Properties

| Method | Return | Description |
|--------|--------|-------------|
| `.version()` | `int` | UUID version (1-8) |
| `.variant()` | `int` | UUID variant |
| `.isNil()` | `bool` | True if nil UUID |

#### Time Extraction (v7 only)

| Method | Return | Description |
|--------|--------|-------------|
| `.timestamp()` | `long` | Unix timestamp in milliseconds (v7 only) |
| `.time()` | `Time` | Time when UUID was created (v7 only) |

```sindarin
var id: UUID = UUID.create()
var created: Time = id.time()
print($"Created at: {created.format("HH:mm:ss")}\n")
```

#### Conversion

| Method | Return | Description |
|--------|--------|-------------|
| `.toString()` | `str` | Standard 36-char format: `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx` |
| `.toHex()` | `str` | 32-char hex string (no dashes) |
| `.toBase64()` | `str` | 22-char URL-safe base64 |
| `.toBytes()` | `byte[]` | 16-byte array |

#### Comparison

UUIDs are comparable and orderable:

```sindarin
var a: UUID = UUID.create()
var b: UUID = UUID.create()

if a < b =>
    print("a was created before b\n")

// Works in sorted collections
var ids: UUID[] = {c, a, b}
ids.sort()  // Chronological order for v7
```

---

## Use Cases

### Database Primary Keys

```sindarin
fn createUser(name: str): User =>
    var user: User = User {
        id: UUID.create(),  // v7: time-ordered for index performance
        name: name,
        createdAt: Time.now()
    }
    db.insert(user)
    return user
```

### Deterministic IDs

```sindarin
// Same input always produces same UUID
var ns: UUID = UUID.namespaceUrl()
var userId: UUID = UUID.v5(ns, "https://myapp.com/users/alice")

// Useful for:
// - Idempotent operations
// - Content-addressable storage
// - Reproducible builds
```

### Correlation IDs

```sindarin
fn handleRequest(req: Request): Response =>
    var correlationId: UUID = UUID.create()
    log($"[{correlationId}] Processing request\n")

    // Pass through service calls
    var result: str = callService(req.data, correlationId)

    log($"[{correlationId}] Complete\n")
    return Response { body: result }
```

### When to Use v4

```sindarin
// Use v4 when timestamp leakage is a concern
var anonymousSession: UUID = UUID.v4()

// Or when you specifically need unpredictability
var secretToken: UUID = UUID.v4()
```

---

## Implementation Considerations

### Internal Representation

```c
typedef struct {
    uint64_t high;  // Most significant 64 bits
    uint64_t low;   // Least significant 64 bits
} SN_UUID;
```

### v7 Generation

```c
SN_UUID sn_uuid_v7(void) {
    SN_UUID uuid;

    // 48-bit timestamp (milliseconds since Unix epoch)
    uint64_t timestamp = current_time_millis();

    // Fill with random bytes
    uint8_t random[10];
    sn_random_fill(random, 10);

    // Construct: timestamp | version | random | variant | random
    uuid.high = (timestamp << 16) | 0x7000 | (random[0] << 8 | random[1]);
    uuid.low = (0x80 | (random[2] & 0x3F)) << 56;  // variant bits
    uuid.low |= /* remaining random bytes */;

    return uuid;
}
```

### v5 Generation

Uses SHA-1 hash of namespace UUID concatenated with name bytes, then sets version/variant bits.

### Monotonicity

For v7 UUIDs generated in the same millisecond, the random portion provides ordering. Some implementations add a counter for strict monotonicity within a millisecond - consider whether this is needed.

---

## Comparison with Other Languages

| Language | v7 Support | API Style |
|----------|------------|-----------|
| Python 3.14 | `uuid.uuid7()` | Module functions |
| .NET 9 | `Guid.CreateVersion7()` | Static methods |
| Go 1.24 | `uuid.NewV7()` | Package functions |
| PostgreSQL 18 | `gen_random_uuid_v7()` | SQL function |
| **Sindarin** | `UUID.create()` | Static methods, v7 default |

Sindarin's approach:
- v7 as the default (modern, performant)
- Dedicated type with rich API
- Explicit version selection when needed
- Time extraction for v7 UUIDs

---

## Design Decisions

### No Strict Monotonicity

UUIDv7 generation will **not** guarantee strict monotonicity within the same millisecond. Multiple UUIDs generated in the same millisecond share the timestamp prefix but have random suffixes - they are unique but not guaranteed to be ordered.

**Rationale:**

Strict monotonicity requires:
- State tracking (last generated UUID per thread)
- Counter or random-increment strategy within milliseconds
- Clock rollback handling (block, increment, or error)
- Synchronization overhead for thread safety

This complexity isn't justified for most use cases:
- Millisecond-level ordering is sufficient for typical applications
- Same-millisecond collisions are astronomically unlikely (random bits)
- Distributed systems can't guarantee cross-node ordering anyway (clock sync)
- High-throughput event sourcing is a niche requirement

Applications requiring strict monotonicity can implement their own sequencing layer.

### Timestamp Extraction Throws on Non-v7

Calling `.timestamp()` or `.time()` on a non-v7 UUID will throw a runtime error. These methods are only meaningful for v7 UUIDs where the timestamp is encoded.

**Rationale:**

- Returning 0 or a sentinel value would silently produce incorrect results
- An optional return type adds ceremony for the common case (v7)
- Throwing makes the programmer's assumption explicit - if you're extracting a timestamp, you expect a v7 UUID
- Use `.version()` to check before calling if the UUID version is uncertain

```sindarin
var id: UUID = getIdFromSomewhere()

// Safe pattern when version is uncertain
if id.version() == 7 =>
    var created: Time = id.time()
    print($"Created: {created}\n")
```

### Compact Format Support

UUIDs will support multiple string representations for different contexts:

| Method | Length | Example | Use Case |
|--------|--------|---------|----------|
| `.toString()` | 36 | `01912345-6789-7abc-8def-0123456789ab` | Standard, human-readable |
| `.toHex()` | 32 | `019123456789abcd8def0123456789ab` | No dashes, still hex |
| `.toBase64()` | 22 | `AZEjRWeJq82N7wEjRWeJqw` | URL-safe, compact |

Corresponding parse methods:

| Method | Description |
|--------|-------------|
| `UUID.fromString(str)` | Parse standard 36-char format |
| `UUID.fromHex(str)` | Parse 32-char hex |
| `UUID.fromBase64(str)` | Parse 22-char base64 |

```sindarin
var id: UUID = UUID.create()

// Standard format for logs, debugging
print($"Created: {id}\n")  // Uses toString()

// Compact format for URLs
var url: str = $"/users/{id.toBase64()}/profile"
// /users/AZEjRWeJq82N7wEjRWeJqw/profile

// Parse back
var parsed: UUID = UUID.fromBase64("AZEjRWeJq82N7wEjRWeJqw")
```

**Note**: Base64 encoding uses URL-safe alphabet (RFC 4648 §5): `A-Z`, `a-z`, `0-9`, `-`, `_` with no padding.

---

## References

- [RFC 9562 - Universally Unique IDentifiers (UUIDs)](https://datatracker.ietf.org/doc/rfc9562/)
- [UUIDv7 Benefits](https://uuid7.com/)
- [Buildkite: Goodbye Integers, Hello UUIDs](https://buildkite.com/resources/blog/goodbye-integers-hello-uuids/)
- [PostgreSQL 18 UUID v7 Support](https://betterstack.com/community/guides/databases/postgresql-18-uuid/)
- [UUID Versions Explained](https://www.uuidtools.com/uuid-versions-explained)
- [TIL: 8 Versions of UUID](https://ntietz.com/blog/til-uses-for-the-different-uuid-versions/)
