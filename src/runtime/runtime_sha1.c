// src/runtime/runtime_sha1.c
// SHA-1 hash algorithm implementation (RFC 3174) for UUIDv5 generation

#include <string.h>
#include "runtime_sha1.h"

/* ============================================================================
 * SHA-1 Constants (RFC 3174)
 * ============================================================================ */

/* Initial hash values (H0-H4) */
#define SHA1_H0 0x67452301
#define SHA1_H1 0xEFCDAB89
#define SHA1_H2 0x98BADCFE
#define SHA1_H3 0x10325476
#define SHA1_H4 0xC3D2E1F0

/* Round constants (K) */
#define SHA1_K0 0x5A827999  /* Rounds  0-19 */
#define SHA1_K1 0x6ED9EBA1  /* Rounds 20-39 */
#define SHA1_K2 0x8F1BBCDC  /* Rounds 40-59 */
#define SHA1_K3 0xCA62C1D6  /* Rounds 60-79 */

/* ============================================================================
 * SHA-1 Bit Operations
 * ============================================================================ */

/* Circular left shift (rotate left) */
#define SHA1_ROTL(X, n) (((X) << (n)) | ((X) >> (32 - (n))))

/* Round functions f(t, B, C, D) */
#define SHA1_F0(B, C, D) (((B) & (C)) | ((~(B)) & (D)))       /* Ch: 0-19 */
#define SHA1_F1(B, C, D) ((B) ^ (C) ^ (D))                     /* Parity: 20-39 */
#define SHA1_F2(B, C, D) (((B) & (C)) | ((B) & (D)) | ((C) & (D)))  /* Maj: 40-59 */
#define SHA1_F3(B, C, D) ((B) ^ (C) ^ (D))                     /* Parity: 60-79 */

/* ============================================================================
 * Message Padding (RFC 3174 Section 4)
 * ============================================================================
 * SHA-1 padding ensures the message length is a multiple of 512 bits:
 *   1. Append '1' bit followed by zeros
 *   2. Final 64 bits contain the original message length (big-endian)
 * ============================================================================ */

int sha1_pad_message(uint8_t *block, int *block_count,
                     const uint8_t *data, size_t data_len, uint64_t total_len) {
    if (block == NULL || block_count == NULL) {
        return -1;
    }

    /* data_len must be < 64 (a partial block) */
    if (data_len >= SHA1_BLOCK_SIZE) {
        return -1;
    }

    /* Copy remaining data to block */
    if (data != NULL && data_len > 0) {
        memcpy(block, data, data_len);
    }

    /* Append 0x80 (the '1' bit followed by 7 zero bits) */
    block[data_len] = 0x80;

    /* Calculate bit length for padding */
    uint64_t bit_len = total_len * 8;

    /*
     * If data_len < 56, we can fit padding + length in one block.
     * Otherwise, we need two blocks.
     */
    if (data_len < 56) {
        /* Single block: zero fill from data_len+1 to byte 55 */
        memset(block + data_len + 1, 0, 55 - data_len);

        /* Append 64-bit message length in big-endian at bytes 56-63 */
        block[56] = (uint8_t)(bit_len >> 56);
        block[57] = (uint8_t)(bit_len >> 48);
        block[58] = (uint8_t)(bit_len >> 40);
        block[59] = (uint8_t)(bit_len >> 32);
        block[60] = (uint8_t)(bit_len >> 24);
        block[61] = (uint8_t)(bit_len >> 16);
        block[62] = (uint8_t)(bit_len >> 8);
        block[63] = (uint8_t)(bit_len);

        *block_count = 1;
    } else {
        /* Two blocks needed */
        /* First block: zero fill remainder after 0x80 */
        memset(block + data_len + 1, 0, 63 - data_len);

        /* Second block: all zeros except final 8 bytes for length */
        memset(block + 64, 0, 56);

        /* Append 64-bit message length in big-endian at bytes 120-127 */
        block[120] = (uint8_t)(bit_len >> 56);
        block[121] = (uint8_t)(bit_len >> 48);
        block[122] = (uint8_t)(bit_len >> 40);
        block[123] = (uint8_t)(bit_len >> 32);
        block[124] = (uint8_t)(bit_len >> 24);
        block[125] = (uint8_t)(bit_len >> 16);
        block[126] = (uint8_t)(bit_len >> 8);
        block[127] = (uint8_t)(bit_len);

        *block_count = 2;
    }

    return 0;
}

/* ============================================================================
 * Block Processing (RFC 3174 Section 6)
 * ============================================================================ */

void sha1_process_block(uint32_t H[5], const uint8_t *block) {
    uint32_t W[80];
    uint32_t A, B, C, D, E, TEMP;
    int t;

    /* Step 1: Expand message block into 80 words */

    /* W[0..15]: Copy from message block (big-endian) */
    for (t = 0; t < 16; t++) {
        W[t] = ((uint32_t)block[t * 4] << 24) |
               ((uint32_t)block[t * 4 + 1] << 16) |
               ((uint32_t)block[t * 4 + 2] << 8) |
               ((uint32_t)block[t * 4 + 3]);
    }

    /* W[16..79]: Expand using XOR and rotate */
    for (t = 16; t < 80; t++) {
        W[t] = SHA1_ROTL(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);
    }

    /* Step 2: Initialize working variables */
    A = H[0];
    B = H[1];
    C = H[2];
    D = H[3];
    E = H[4];

    /* Step 3: 80 rounds of compression */
    for (t = 0; t < 80; t++) {
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

    /* Step 4: Update hash values */
    H[0] += A;
    H[1] += B;
    H[2] += C;
    H[3] += D;
    H[4] += E;
}

/* ============================================================================
 * SHA-1 API Implementation
 * ============================================================================ */

void sha1_init(SHA1_Context *ctx) {
    if (ctx == NULL) {
        return;
    }

    /* Set initial hash values */
    ctx->H[0] = SHA1_H0;
    ctx->H[1] = SHA1_H1;
    ctx->H[2] = SHA1_H2;
    ctx->H[3] = SHA1_H3;
    ctx->H[4] = SHA1_H4;

    ctx->buffer_len = 0;
    ctx->total_len = 0;
}

void sha1_update(SHA1_Context *ctx, const uint8_t *data, size_t len) {
    if (ctx == NULL || data == NULL || len == 0) {
        return;
    }

    ctx->total_len += len;

    /* If we have buffered data, try to complete a block */
    if (ctx->buffer_len > 0) {
        size_t to_copy = SHA1_BLOCK_SIZE - ctx->buffer_len;
        if (to_copy > len) {
            to_copy = len;
        }

        memcpy(ctx->buffer + ctx->buffer_len, data, to_copy);
        ctx->buffer_len += to_copy;
        data += to_copy;
        len -= to_copy;

        if (ctx->buffer_len == SHA1_BLOCK_SIZE) {
            sha1_process_block(ctx->H, ctx->buffer);
            ctx->buffer_len = 0;
        }
    }

    /* Process complete blocks directly from input */
    while (len >= SHA1_BLOCK_SIZE) {
        sha1_process_block(ctx->H, data);
        data += SHA1_BLOCK_SIZE;
        len -= SHA1_BLOCK_SIZE;
    }

    /* Buffer remaining data */
    if (len > 0) {
        memcpy(ctx->buffer, data, len);
        ctx->buffer_len = len;
    }
}

void sha1_final(SHA1_Context *ctx, uint8_t digest[SHA1_DIGEST_SIZE]) {
    if (ctx == NULL || digest == NULL) {
        return;
    }

    /* Pad and process final block(s) */
    uint8_t final_blocks[128];
    int block_count;

    sha1_pad_message(final_blocks, &block_count,
                     ctx->buffer, ctx->buffer_len, ctx->total_len);

    /* Process final block(s) */
    sha1_process_block(ctx->H, final_blocks);
    if (block_count == 2) {
        sha1_process_block(ctx->H, final_blocks + 64);
    }

    /* Output digest in big-endian format */
    for (int i = 0; i < 5; i++) {
        digest[i * 4]     = (uint8_t)(ctx->H[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->H[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->H[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)(ctx->H[i]);
    }
}

void sha1_hash(const uint8_t *data, size_t len, uint8_t digest[SHA1_DIGEST_SIZE]) {
    SHA1_Context ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, len);
    sha1_final(&ctx, digest);
}
