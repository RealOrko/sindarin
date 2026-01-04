# Random Number Generation in Sindarin

> **DRAFT** - This specification is not yet implemented. This document explores design options for a `Random` type in Sindarin's runtime.

## The Problem with "Pseudo" Randomness

Most programming languages expose random number generation in a way that immediately reveals implementation details:

```python
# Python
import random
random.seed(42)  # Why do I need to know about seeds?
x = random.randint(1, 100)
```

```javascript
// JavaScript
Math.random()  // No seed control, but also not cryptographically secure
```

```go
// Go - the classic footgun
rand.Seed(time.Now().UnixNano())  // Forgot this? Enjoy the same sequence every run
```

The exposure of seeding as a primary API concern stems from historical and practical reasons, but it creates a poor developer experience and potential security issues.

---

## Why Do Languages Use PRNGs with Seeds?

### Historical Context

1. **Performance**: True random number generation from hardware was historically slow and unreliable. PRNGs like [Mersenne Twister](https://en.wikipedia.org/wiki/Pseudorandom_number_generator) offered fast generation with good statistical properties.

2. **Reproducibility**: Scientific computing, simulations, and testing require deterministic sequences. Seeding enables reproducing exact results.

3. **Portability**: Not all systems had hardware RNG support. PRNGs work identically across platforms.

4. **Resource Constraints**: Embedded systems and early computers couldn't afford the entropy collection overhead.

### The Seed Problem

A [random seed](https://en.wikipedia.org/wiki/Random_seed) completely determines a PRNG's output sequence. This creates several issues:

- **Security vulnerabilities**: Predictable seeds (like timestamps) make sequences guessable
- **API complexity**: Developers must understand seeding to use randomness correctly
- **Common mistakes**: Forgetting to seed, seeding with predictable values, or re-seeding incorrectly

---

## Modern Entropy Sources

Modern systems have access to high-quality entropy that wasn't available historically:

### Hardware Random Number Generators

[RDRAND](https://en.wikipedia.org/wiki/RDRAND) (Intel, 2012) and similar instructions provide hardware-backed randomness:
- Entropy from thermal noise in meta-stable circuits
- Cryptographically processed through AES-based conditioning
- Available on all modern x86-64 CPUs

### Operating System Facilities

Modern operating systems provide well-designed randomness interfaces:

| OS | Interface | Properties |
|----|-----------|------------|
| Linux | `getrandom()` syscall | Blocks until entropy available, no file descriptor needed |
| macOS | `SecRandomCopyBytes()` | Cryptographically secure |
| Windows | `BCryptGenRandom()` | Cryptographically secure |

These interfaces:
- Automatically seed from multiple entropy sources (hardware, interrupts, disk timing)
- Continuously reseed as new entropy becomes available
- Handle all the complexity that applications shouldn't need to know about

---

## How Modern Languages Are Evolving

### Rust's `rand` Crate

Rust's approach with [`thread_rng()`](https://docs.rs/rand/latest/rand/fn.thread_rng.html) (now `rand::rng()`) shows a better direction:

```rust
use rand::Rng;

// No seeding required - automatically uses OS entropy
let x: i32 = rand::rng().gen_range(1..=100);
```

Design properties:
- Thread-local generator, lazily initialized
- Automatically seeded from `OsRng`
- Uses ChaCha12 CSPRNG internally
- Periodically reseeds from OS entropy

### Python's `secrets` Module

Python added `secrets` for security-sensitive randomness:

```python
import secrets
token = secrets.token_hex(16)  # No seed, just entropy
```

### The Pattern

The trend is clear: **abstract away the PRNG machinery and default to OS entropy**.

---

## Proposed Design for Sindarin

### Design Principles

1. **Secure by default**: Use OS entropy, not PRNGs with manual seeding
2. **Simple API**: No seeds, no initialization, just get random values
3. **Type-safe**: Generate values of specific types directly
4. **Reproducible when needed**: Provide an explicit, separate API for deterministic sequences

### The `Random` Type

```sindarin
// Simple usage - just get random values
var dice: int = Random.int(1, 6)
var coin: bool = Random.bool()
var percentage: double = Random.double(0.0, 1.0)

// Generate bytes for cryptographic use
var key: byte[] = Random.bytes(32)

// Choose from collections
var colors: str[] = {"red", "green", "blue"}
var pick: str = Random.choice(colors)

// Weighted selection (e.g., loot drops)
var items: str[] = {"common", "rare", "legendary"}
var weights: double[] = {0.7, 0.25, 0.05}
var loot: str = Random.weightedChoice(items, weights)

// Statistical sampling
var height: double = Random.gaussian(170.0, 10.0)  // mean=170cm, stddev=10cm

// Batch generation for hot loops
var samples: double[] = Random.gaussianMany(0.0, 1.0, 1000)

// Shuffle in place
var deck: int[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
Random.shuffle(deck)
```

### Static Methods

#### Factory Methods

| Method | Return | Description |
|--------|--------|-------------|
| `Random.create()` | `Random` | Create an OS-entropy backed instance |
| `Random.createWithSeed(seed)` | `Random` | Create a seeded PRNG instance |

#### Value Generation

| Method | Return | Description |
|--------|--------|-------------|
| `Random.int(min, max)` | `int` | Random integer in range [min, max] inclusive |
| `Random.long(min, max)` | `long` | Random long in range [min, max] inclusive |
| `Random.double(min, max)` | `double` | Random double in range [min, max) |
| `Random.bool()` | `bool` | Random boolean (50/50) |
| `Random.byte()` | `byte` | Random byte (0-255) |
| `Random.bytes(count)` | `byte[]` | Array of random bytes |
| `Random.gaussian(mean, stddev)` | `double` | Sample from normal distribution |

#### Batch Generation

For performance-sensitive loops, batch methods generate multiple values in one call:

| Method | Return | Description |
|--------|--------|-------------|
| `Random.intMany(min, max, count)` | `int[]` | Array of random integers |
| `Random.longMany(min, max, count)` | `long[]` | Array of random longs |
| `Random.doubleMany(min, max, count)` | `double[]` | Array of random doubles |
| `Random.boolMany(count)` | `bool[]` | Array of random booleans |
| `Random.gaussianMany(mean, stddev, count)` | `double[]` | Array of gaussian samples |

#### Collection Operations

| Method | Return | Description |
|--------|--------|-------------|
| `Random.choice(array)` | `T` | Random element from array |
| `Random.weightedChoice(array, weights)` | `T` | Random element with weights |
| `Random.shuffle(array)` | `void` | Shuffle array in place |
| `Random.sample(array, count)` | `T[]` | Random sample without replacement |

### Why No Seeding on Static Methods?

The static methods intentionally have no seed parameter. Every call draws from OS entropy (via `getrandom()` on Linux, equivalent on other platforms).

**Benefits:**
- Cannot accidentally use predictable sequences
- No initialization ceremony required
- Cryptographically suitable by default
- Simpler mental model

---

## Reproducible Sequences: Instance Mode

For testing, simulations, and procedural generation where reproducibility matters, create a `Random` instance with an explicit seed:

```sindarin
// Explicit: "I want a deterministic sequence"
var rng: Random = Random.createWithSeed(42)

// Same seed = same sequence, guaranteed
var a: int = rng.int(1, 100)  // Always the same value for seed 42
var b: int = rng.int(1, 100)  // Always the same second value

// For procedural generation
fn generateWorld(seed: long): World =>
    var rng: Random = Random.createWithSeed(seed)
    // World generation is now reproducible
    ...
```

### Instance Methods

When you have a `Random` instance (from `create()` or `createWithSeed()`), use the same methods as instance methods:

#### Value Generation

| Method | Return | Description |
|--------|--------|-------------|
| `.int(min, max)` | `int` | Next random integer |
| `.long(min, max)` | `long` | Next random long |
| `.double(min, max)` | `double` | Next random double |
| `.bool()` | `bool` | Next random boolean |
| `.byte()` | `byte` | Next random byte |
| `.bytes(count)` | `byte[]` | Next random bytes |
| `.gaussian(mean, stddev)` | `double` | Sample from normal distribution |

#### Batch Generation

| Method | Return | Description |
|--------|--------|-------------|
| `.intMany(min, max, count)` | `int[]` | Array of random integers |
| `.longMany(min, max, count)` | `long[]` | Array of random longs |
| `.doubleMany(min, max, count)` | `double[]` | Array of random doubles |
| `.boolMany(count)` | `bool[]` | Array of random booleans |
| `.gaussianMany(mean, stddev, count)` | `double[]` | Array of gaussian samples |

#### Collection Operations

| Method | Return | Description |
|--------|--------|-------------|
| `.choice(array)` | `T` | Random element |
| `.weightedChoice(array, weights)` | `T` | Random element with weights |
| `.shuffle(array)` | `void` | Shuffle with this generator |
| `.sample(array, count)` | `T[]` | Random sample without replacement |

This design means implementations can be swapped seamlessly - a function that accepts a `Random` instance works identically whether it was created with `create()` or `createWithSeed()`.

---

## Implementation Considerations

### Runtime Implementation

The `Random` type would be implemented in `runtime.c`:

```c
// Uses getrandom() on Linux, arc4random() on BSD/macOS, BCryptGenRandom() on Windows
static void sn_random_fill(uint8_t* buf, size_t len) {
#if defined(__linux__)
    // getrandom() - blocks until entropy available, no EINTR issues
    while (len > 0) {
        ssize_t ret = getrandom(buf, len, 0);
        if (ret < 0) {
            // Handle error
        }
        buf += ret;
        len -= ret;
    }
#elif defined(__APPLE__)
    arc4random_buf(buf, len);
#elif defined(_WIN32)
    BCryptGenRandom(NULL, buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
#endif
}
```

### SeededRandom Algorithm

For `SeededRandom`, use a well-analyzed PRNG:
- **ChaCha8/12**: Fast, high-quality, used by Rust's `rand`
- **PCG**: Compact state, good statistical properties
- **xoshiro256**: Fast, passes BigCrush

---

## Comparison with Other Approaches

| Language | Default Random | Secure Random | Seeded Random |
|----------|---------------|---------------|---------------|
| Python | `random.random()` (Mersenne) | `secrets` | `random.seed()` |
| Go | `rand.Int()` (needs seed!) | `crypto/rand` | Same as default |
| Rust | `rand::rng()` (ChaCha, OS-seeded) | `OsRng` | `StdRng::seed_from_u64()` |
| Java | `Random()` (needs seed) | `SecureRandom` | `Random(seed)` |
| **Sindarin** | `Random.int()` (OS entropy) | Same as default | `Random.createWithSeed(seed)` |

Sindarin's approach:
- Defaults to the secure option (like Rust's modern API)
- Makes the insecure option (seeded PRNG) explicit via factory method
- Single type with unified interface - implementations swap seamlessly

---

## Out of Scope

**UUID generation**: UUIDs will be a separate runtime type. The evolution from v4 to v7 (RFC 9562, May 2024) with time-ordering, database index locality benefits, and methods like `.timestamp()` justifies a dedicated `UUID` type rather than a simple string return from `Random`.

---

## References

- [Paragon Initiative: Secure Random Numbers](https://paragonie.com/blog/2016/05/how-generate-secure-random-numbers-in-various-programming-languages)
- [PRNG Recommendations for Applications](https://peteroupc.github.io/random.html)
- [Rust rand crate documentation](https://docs.rs/rand)
- [Linux getrandom() syscall](https://lwn.net/Articles/800509/)
- [Intel RDRAND Implementation Guide](https://www.intel.com/content/www/us/en/developer/articles/guide/intel-digital-random-number-generator-drng-software-implementation-guide.html)
