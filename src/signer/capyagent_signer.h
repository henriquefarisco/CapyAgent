#ifndef CAPY_AGENT_SIGNER_H
#define CAPY_AGENT_SIGNER_H

#include <stddef.h>
#include <stdint.h>

/*
 * CapyAgent Ed25519 signing/verification surface for the capypkg manifest.
 *
 * - The publisher path (`sign_descriptor`) builds the canonical descriptor
 *   `name=N|version=V|payload_sha256=H|payload_url=U\n` and signs it.
 * - The consumer path is `capyagent_ed25519_verifier`, whose signature matches
 *   the CapyOS `capypkg_verify_signature_fn` callback. A future CapyOS release
 *   registers it via `capypkg_set_signature_verifier`; until then the capypkg
 *   adapter keeps the slot NULL and fails closed on `signed` repos.
 *
 * Fail-closed: with no trusted key configured, the verifier rejects.
 */

#define CAPY_SIGNER_PUBLIC_KEY_LEN 32u
#define CAPY_SIGNER_SECRET_KEY_LEN 64u
#define CAPY_SIGNER_SEED_LEN 32u
#define CAPY_SIGNER_SIGNATURE_LEN 64u
#define CAPY_SIGNER_SIGNATURE_HEX_LEN 128u

/* Lowercase hex codec. Return 0 on success, -1 on invalid input/size. */
int capy_signer_hex_encode(const uint8_t *bytes, size_t len, char *out,
                           size_t out_size);
int capy_signer_hex_decode(const char *hex, uint8_t *out, size_t out_len);

/* Deterministic keypair from a 32-byte seed (RFC 8032). */
void capy_signer_keypair_from_seed(const uint8_t seed[CAPY_SIGNER_SEED_LEN],
                                   uint8_t public_key[CAPY_SIGNER_PUBLIC_KEY_LEN],
                                   uint8_t secret_key[CAPY_SIGNER_SECRET_KEY_LEN]);

/* Sign the canonical descriptor; writes 128 lowercase hex + NUL into
 * signature_hex_out (out_size >= 129). Returns 0 on success. */
int capy_signer_sign_descriptor(const uint8_t secret_key[CAPY_SIGNER_SECRET_KEY_LEN],
                                const char *name, const char *version,
                                const char *payload_sha256, const char *payload_url,
                                char *signature_hex_out, size_t out_size);

/* Verify a 128-hex signature over the canonical descriptor with an explicit
 * public key. Returns 0 if valid, non-zero otherwise. */
int capy_signer_verify_descriptor(const uint8_t public_key[CAPY_SIGNER_PUBLIC_KEY_LEN],
                                  const char *name, const char *version,
                                  const char *payload_sha256, const char *payload_url,
                                  const char *signature_hex);

/* Configure / query the trusted publisher public key used by the verifier. */
void capy_signer_set_trusted_public_key(
    const uint8_t public_key[CAPY_SIGNER_PUBLIC_KEY_LEN]);
void capy_signer_clear_trusted_public_key(void);
int capy_signer_has_trusted_public_key(void);

/* capypkg `capypkg_verify_signature_fn`-compatible verifier: returns 0 when the
 * 128-hex signature over `signed_text` (signed_len bytes) is valid under the
 * configured trusted key, non-zero otherwise (fail-closed). */
int capyagent_ed25519_verifier(const char *signed_text, size_t signed_len,
                               const char *signature_hex);

#endif
