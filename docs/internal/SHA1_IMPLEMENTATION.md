# SHA-1 Implementation for UUIDv5

This document describes the SHA-1 algorithm implementation approach based on RFC 3174, specifically for use in UUIDv5 generation.

## Overview

SHA-1 (Secure Hash Algorithm 1) produces a 160-bit (20-byte) message digest. For UUIDv5, we hash the concatenation of a namespace UUID (16 bytes) and a name string, then use the first 128 bits of the resulting digest (with version and variant bits set).

## RFC 3174 Specification Summary

### Initial Hash Values (H0-H4)

The five 32-bit words that form the initial hash state:

```c
#define SHA1_H0 0x67452301
#define SHA1_H1 0xEFCDAB89
#define SHA1_H2 0x98BADCFE
#define SHA1_H3 0x10325476
#define SHA1_H4 0xC3D2E1F0
```

### Round Constants (K)

Four different constants used during the 80 rounds:

```c
#define SHA1_K0 0x5A827999  /* Rounds  0-19 */
#define SHA1_K1 0x6ED9EBA1  /* Rounds 20-39 */
#define SHA1_K2 0x8F1BBCDC  /* Rounds 40-59 */
#define SHA1_K3 0xCA62C1D6  /* Rounds 60-79 */
```

### Round Functions (f)

Four logical functions used in different round ranges:

| Rounds | Function f(B, C, D) | Description |
|--------|---------------------|-------------|
| 0-19   | `(B AND C) OR ((NOT B) AND D)` | Ch (Choose) |
| 20-39  | `B XOR C XOR D` | Parity |
| 40-59  | `(B AND C) OR (B AND D) OR (C AND D)` | Maj (Majority) |
| 60-79  | `B XOR C XOR D` | Parity |

C implementation:

```c
/* f functions for each round range */
#define SHA1_F0(B, C, D) (((B) & (C)) | ((~(B)) & (D)))       /* 0-19: Ch */
#define SHA1_F1(B, C, D) ((B) ^ (C) ^ (D))                     /* 20-39: Parity */
#define SHA1_F2(B, C, D) (((B) & (C)) | ((B) & (D)) | ((C) & (D)))  /* 40-59: Maj */
#define SHA1_F3(B, C, D) ((B) ^ (C) ^ (D))                     /* 60-79: Parity */
```

### Circular Left Shift (S^n)

Rotates a 32-bit word left by n positions:

```c
#define SHA1_ROTL(X, n) (((X) << (n)) | ((X) >> (32 - (n))))
```

## Message Processing

### Padding

Messages must be padded to a multiple of 512 bits (64 bytes):

1. Append a single '1' bit (0x80 byte)
2. Append zero bits until length ≡ 448 (mod 512)
3. Append the original message length as a 64-bit big-endian integer

```c
/* Padding structure for a message block */
/* Original message + 1 + zeros + 64-bit length = multiple of 512 bits */

void sha1_pad_message(uint8_t *block, size_t msg_len, size_t block_offset) {
    /* Add 0x80 byte after message */
    block[block_offset] = 0x80;

    /* Zero fill until 56 bytes (leave room for 8-byte length) */
    memset(block + block_offset + 1, 0, 56 - block_offset - 1);

    /* Append message length in bits as big-endian 64-bit */
    uint64_t bit_len = (uint64_t)msg_len * 8;
    block[56] = (uint8_t)(bit_len >> 56);
    block[57] = (uint8_t)(bit_len >> 48);
    block[58] = (uint8_t)(bit_len >> 40);
    block[59] = (uint8_t)(bit_len >> 32);
    block[60] = (uint8_t)(bit_len >> 24);
    block[61] = (uint8_t)(bit_len >> 16);
    block[62] = (uint8_t)(bit_len >> 8);
    block[63] = (uint8_t)(bit_len);
}
```

### Message Schedule Expansion

Each 512-bit (64-byte) block is expanded into 80 32-bit words:

1. Words 0-15: Directly from the message block (big-endian)
2. Words 16-79: `W[t] = ROTL(W[t-3] XOR W[t-8] XOR W[t-14] XOR W[t-16], 1)`

```c
void sha1_expand_block(uint32_t W[80], const uint8_t *block) {
    /* W[0..15]: Copy from message block (big-endian) */
    for (int i = 0; i < 16; i++) {
        W[i] = ((uint32_t)block[i*4] << 24) |
               ((uint32_t)block[i*4 + 1] << 16) |
               ((uint32_t)block[i*4 + 2] << 8) |
               ((uint32_t)block[i*4 + 3]);
    }

    /* W[16..79]: Expand using XOR and rotate */
    for (int t = 16; t < 80; t++) {
        W[t] = SHA1_ROTL(W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16], 1);
    }
}
```

## Compression Function

The core SHA-1 algorithm processes each 512-bit block:

```c
void sha1_process_block(uint32_t H[5], const uint8_t *block) {
    uint32_t W[80];
    uint32_t A, B, C, D, E, TEMP;

    /* Expand message block into 80 words */
    sha1_expand_block(W, block);

    /* Initialize working variables */
    A = H[0];
    B = H[1];
    C = H[2];
    D = H[3];
    E = H[4];

    /* 80 rounds */
    for (int t = 0; t < 80; t++) {
        uint32_t f, K;

        if (t < 20) {
            f = SHA1_F0(B, C, D);
            K = SHA1_K0;
        } else if (t < 40) {
            f = SHA1_F1(B, C, D);
            K = SHA1_K1;
        } else if (t < 60) {
            f = SHA1_F2(B, C, D);
            K = SHA1_K2;
        } else {
            f = SHA1_F3(B, C, D);
            K = SHA1_K3;
        }

        TEMP = SHA1_ROTL(A, 5) + f + E + W[t] + K;
        E = D;
        D = C;
        C = SHA1_ROTL(B, 30);
        B = A;
        A = TEMP;
    }

    /* Update hash values */
    H[0] += A;
    H[1] += B;
    H[2] += C;
    H[3] += D;
    H[4] += E;
}
```

## Full SHA-1 Implementation Structure

```c
typedef struct {
    uint32_t H[5];           /* Hash state */
    uint8_t buffer[64];      /* Block buffer */
    size_t buffer_len;       /* Bytes in buffer */
    uint64_t total_len;      /* Total message length in bytes */
} SHA1_Context;

void sha1_init(SHA1_Context *ctx) {
    ctx->H[0] = SHA1_H0;
    ctx->H[1] = SHA1_H1;
    ctx->H[2] = SHA1_H2;
    ctx->H[3] = SHA1_H3;
    ctx->H[4] = SHA1_H4;
    ctx->buffer_len = 0;
    ctx->total_len = 0;
}

void sha1_update(SHA1_Context *ctx, const uint8_t *data, size_t len) {
    ctx->total_len += len;

    /* If we have buffered data, try to complete a block */
    if (ctx->buffer_len > 0) {
        size_t to_copy = 64 - ctx->buffer_len;
        if (to_copy > len) to_copy = len;
        memcpy(ctx->buffer + ctx->buffer_len, data, to_copy);
        ctx->buffer_len += to_copy;
        data += to_copy;
        len -= to_copy;

        if (ctx->buffer_len == 64) {
            sha1_process_block(ctx->H, ctx->buffer);
            ctx->buffer_len = 0;
        }
    }

    /* Process complete blocks directly */
    while (len >= 64) {
        sha1_process_block(ctx->H, data);
        data += 64;
        len -= 64;
    }

    /* Buffer remaining data */
    if (len > 0) {
        memcpy(ctx->buffer, data, len);
        ctx->buffer_len = len;
    }
}

void sha1_final(SHA1_Context *ctx, uint8_t digest[20]) {
    /* Pad and process final block(s) */
    uint8_t final_block[128];  /* May need 2 blocks */
    size_t padding_start = ctx->buffer_len;

    /* Copy buffered data */
    memcpy(final_block, ctx->buffer, ctx->buffer_len);

    /* Add 0x80 */
    final_block[padding_start] = 0x80;

    /* Zero fill */
    if (padding_start < 56) {
        /* Fits in one block */
        memset(final_block + padding_start + 1, 0, 55 - padding_start);
    } else {
        /* Need two blocks */
        memset(final_block + padding_start + 1, 0, 63 - padding_start);
        sha1_process_block(ctx->H, final_block);
        memset(final_block, 0, 56);
    }

    /* Append length in bits (big-endian) */
    uint64_t bit_len = ctx->total_len * 8;
    final_block[56] = (uint8_t)(bit_len >> 56);
    final_block[57] = (uint8_t)(bit_len >> 48);
    final_block[58] = (uint8_t)(bit_len >> 40);
    final_block[59] = (uint8_t)(bit_len >> 32);
    final_block[60] = (uint8_t)(bit_len >> 24);
    final_block[61] = (uint8_t)(bit_len >> 16);
    final_block[62] = (uint8_t)(bit_len >> 8);
    final_block[63] = (uint8_t)(bit_len);

    sha1_process_block(ctx->H, final_block);

    /* Output digest (big-endian) */
    for (int i = 0; i < 5; i++) {
        digest[i*4]     = (uint8_t)(ctx->H[i] >> 24);
        digest[i*4 + 1] = (uint8_t)(ctx->H[i] >> 16);
        digest[i*4 + 2] = (uint8_t)(ctx->H[i] >> 8);
        digest[i*4 + 3] = (uint8_t)(ctx->H[i]);
    }
}
```

## UUIDv5 Integration

For UUIDv5, SHA-1 is used to hash the namespace UUID concatenated with the name:

```c
RtUuid *rt_uuid_v5(RtArena *arena, RtUuid *namespace_uuid, const char *name) {
    /* 1. Convert namespace UUID to 16 bytes (big-endian) */
    uint8_t ns_bytes[16];
    /* ... fill from namespace_uuid->high and namespace_uuid->low ... */

    /* 2. Hash namespace + name */
    SHA1_Context ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, ns_bytes, 16);
    sha1_update(&ctx, (const uint8_t *)name, strlen(name));

    uint8_t digest[20];
    sha1_final(&ctx, digest);

    /* 3. Use first 16 bytes as UUID */
    RtUuid *uuid = rt_arena_alloc(arena, sizeof(RtUuid));

    /* Build high 64 bits from digest[0..7] */
    uuid->high = ((uint64_t)digest[0] << 56) |
                 ((uint64_t)digest[1] << 48) |
                 ((uint64_t)digest[2] << 40) |
                 ((uint64_t)digest[3] << 32) |
                 ((uint64_t)digest[4] << 24) |
                 ((uint64_t)digest[5] << 16) |
                 ((uint64_t)digest[6] << 8)  |
                 ((uint64_t)digest[7]);

    /* Build low 64 bits from digest[8..15] */
    uuid->low = ((uint64_t)digest[8] << 56)  |
                ((uint64_t)digest[9] << 48)  |
                ((uint64_t)digest[10] << 40) |
                ((uint64_t)digest[11] << 32) |
                ((uint64_t)digest[12] << 24) |
                ((uint64_t)digest[13] << 16) |
                ((uint64_t)digest[14] << 8)  |
                ((uint64_t)digest[15]);

    /* 4. Set version (5) and variant (RFC 4122) bits */
    /* Version 5 in bits 15-12 of high word */
    uuid->high = (uuid->high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000005000ULL;

    /* Variant 1 in bits 63-62 of low word */
    uuid->low = (uuid->low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    return uuid;
}
```

## Test Vectors (RFC 3174)

Test vectors from RFC 3174 Section 7.3:

| Input | SHA-1 Digest |
|-------|--------------|
| "abc" | A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D |
| "" (empty) | DA39A3EE 5E6B4B0D 3255BFEF 95601890 AFD80709 |

UUIDv5 test vectors (from various implementations):

| Namespace | Name | Expected UUID |
|-----------|------|---------------|
| DNS | "www.example.com" | 2ed6657d-e927-568b-95e1-2665a8aea6a2 |
| URL | "http://example.com" | 6e73b88a-4c0e-5e42-9e49-c4f5b8c77d7c |

## Summary

Key implementation points:

1. **Constants**: H0-H4 (initial state), K0-K3 (round constants)
2. **Bit operations**: Circular left shift (ROTL), AND, OR, XOR, NOT
3. **Round functions**: Ch (0-19), Parity (20-39, 60-79), Maj (40-59)
4. **Message schedule**: 16 words from input, 64 words expanded
5. **Compression**: 80 rounds updating A-E working variables
6. **Padding**: 0x80 + zeros + 64-bit length (big-endian)
7. **Byte order**: All multi-byte values are big-endian

The implementation is self-contained with no external cryptographic library dependencies.
