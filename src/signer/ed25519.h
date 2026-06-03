#ifndef CAPY_AGENT_ED25519_H
#define CAPY_AGENT_ED25519_H

#include <stddef.h>
#include <stdint.h>

/*
 * Ed25519 (RFC 8032) signature primitive owned by CapyAgent.
 *
 * The field arithmetic and point operations are a faithful reproduction of the
 * public-domain TweetNaCl reference (D. J. Bernstein et al.), adapted to this
 * repository's fixed-width style and wired to the in-repo SHA-512. It is
 * deterministic and host-testable; correctness is gated externally by the
 * RFC 8032 known-answer tests in tests/test_signer.c, because this machine is
 * review/edit only and cannot execute the suite.
 *
 * Secret key layout matches RFC 8032 / TweetNaCl: 64 bytes = 32-byte seed
 * followed by the 32-byte public key.
 */

#define CAPY_ED25519_PUBLIC_KEY_LEN 32u
#define CAPY_ED25519_SECRET_KEY_LEN 64u
#define CAPY_ED25519_SEED_LEN 32u
#define CAPY_ED25519_SIGNATURE_LEN 64u

/* Derive a public/secret keypair deterministically from a 32-byte seed. */
void capy_ed25519_keypair_from_seed(const uint8_t seed[CAPY_ED25519_SEED_LEN],
                                    uint8_t public_key[CAPY_ED25519_PUBLIC_KEY_LEN],
                                    uint8_t secret_key[CAPY_ED25519_SECRET_KEY_LEN]);

/* Produce a 64-byte detached signature over message m (length n). */
void capy_ed25519_sign(uint8_t sig[CAPY_ED25519_SIGNATURE_LEN], const uint8_t *m,
                       size_t n, const uint8_t secret_key[CAPY_ED25519_SECRET_KEY_LEN]);

/* Verify a 64-byte detached signature. Returns 0 if valid, -1 otherwise. */
int capy_ed25519_verify(const uint8_t sig[CAPY_ED25519_SIGNATURE_LEN],
                        const uint8_t *m, size_t n,
                        const uint8_t public_key[CAPY_ED25519_PUBLIC_KEY_LEN]);

#endif
