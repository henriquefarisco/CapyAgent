#ifndef CAPY_AGENT_SHA512_H
#define CAPY_AGENT_SHA512_H

#include <stddef.h>
#include <stdint.h>

/*
 * FIPS 180-4 SHA-512. Host-testable, no allocation, no I/O. Used by the
 * Ed25519 signer/verifier. Validated externally by NIST known-answer tests in
 * tests/test_signer.c (this machine is review/edit only).
 */

#define CAPY_SHA512_DIGEST_LEN 64u
#define CAPY_SHA512_BLOCK_LEN 128u

struct capy_sha512_ctx {
  uint64_t state[8];
  uint64_t bitlen_hi;
  uint64_t bitlen_lo;
  uint8_t buffer[CAPY_SHA512_BLOCK_LEN];
  size_t buffer_len;
};

void capy_sha512_init(struct capy_sha512_ctx *ctx);
void capy_sha512_update(struct capy_sha512_ctx *ctx, const uint8_t *data,
                        size_t len);
void capy_sha512_final(struct capy_sha512_ctx *ctx,
                       uint8_t out[CAPY_SHA512_DIGEST_LEN]);
void capy_sha512(const uint8_t *data, size_t len,
                 uint8_t out[CAPY_SHA512_DIGEST_LEN]);

#endif
