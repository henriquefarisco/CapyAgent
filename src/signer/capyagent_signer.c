#include "capyagent_signer.h"

#include "component_manifest.h"
#include "ed25519.h"

static const char k_hex_digits[] = "0123456789abcdef";

static uint8_t g_trusted_pk[CAPY_SIGNER_PUBLIC_KEY_LEN];
static int g_has_trusted;

static int hex_value(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  return -1;
}

int capy_signer_hex_encode(const uint8_t *bytes, size_t len, char *out,
                           size_t out_size) {
  size_t i;
  /* Overflow-safe capacity check: need 2*len + 1 bytes, computed without
   * the len*2 multiplication that could wrap for a hostile len. */
  if (!bytes || !out || out_size == 0u || len > (out_size - 1u) / 2u) {
    return -1;
  }
  for (i = 0u; i < len; ++i) {
    out[2u * i] = k_hex_digits[(bytes[i] >> 4) & 0x0fu];
    out[2u * i + 1u] = k_hex_digits[bytes[i] & 0x0fu];
  }
  out[2u * len] = '\0';
  return 0;
}

int capy_signer_hex_decode(const char *hex, uint8_t *out, size_t out_len) {
  size_t i;
  if (!hex || !out) {
    return -1;
  }
  for (i = 0u; i < out_len; ++i) {
    int hi = hex_value(hex[2u * i]);
    int lo;
    if (hi < 0) {
      return -1;
    }
    lo = hex_value(hex[2u * i + 1u]);
    if (lo < 0) {
      return -1;
    }
    out[i] = (uint8_t)((hi << 4) | lo);
  }
  if (hex[2u * out_len] != '\0') {
    return -1; /* trailing characters */
  }
  return 0;
}

void capy_signer_keypair_from_seed(const uint8_t seed[CAPY_SIGNER_SEED_LEN],
                                   uint8_t public_key[CAPY_SIGNER_PUBLIC_KEY_LEN],
                                   uint8_t secret_key[CAPY_SIGNER_SECRET_KEY_LEN]) {
  capy_ed25519_keypair_from_seed(seed, public_key, secret_key);
}

int capy_signer_sign_descriptor(const uint8_t secret_key[CAPY_SIGNER_SECRET_KEY_LEN],
                                const char *name, const char *version,
                                const char *payload_sha256, const char *payload_url,
                                char *signature_hex_out, size_t out_size) {
  char descriptor[CAPY_MANIFEST_CANONICAL_MAX];
  uint8_t sig[CAPY_SIGNER_SIGNATURE_LEN];
  size_t descriptor_len = 0u;
  if (!secret_key || !signature_hex_out ||
      out_size < (CAPY_SIGNER_SIGNATURE_HEX_LEN + 1u)) {
    return -1;
  }
  if (capy_manifest_canonical_descriptor(name, version, payload_sha256,
                                         payload_url, descriptor,
                                         sizeof(descriptor),
                                         &descriptor_len) != CAPY_MANIFEST_OK) {
    return -1;
  }
  capy_ed25519_sign(sig, (const uint8_t *)descriptor, descriptor_len, secret_key);
  return capy_signer_hex_encode(sig, CAPY_SIGNER_SIGNATURE_LEN, signature_hex_out,
                                out_size);
}

int capy_signer_verify_descriptor(const uint8_t public_key[CAPY_SIGNER_PUBLIC_KEY_LEN],
                                  const char *name, const char *version,
                                  const char *payload_sha256, const char *payload_url,
                                  const char *signature_hex) {
  char descriptor[CAPY_MANIFEST_CANONICAL_MAX];
  uint8_t sig[CAPY_SIGNER_SIGNATURE_LEN];
  size_t descriptor_len = 0u;
  if (!public_key || !signature_hex) {
    return -1;
  }
  if (capy_manifest_canonical_descriptor(name, version, payload_sha256,
                                         payload_url, descriptor,
                                         sizeof(descriptor),
                                         &descriptor_len) != CAPY_MANIFEST_OK) {
    return -1;
  }
  if (capy_signer_hex_decode(signature_hex, sig, CAPY_SIGNER_SIGNATURE_LEN) != 0) {
    return -1;
  }
  return capy_ed25519_verify(sig, (const uint8_t *)descriptor, descriptor_len,
                             public_key);
}

void capy_signer_set_trusted_public_key(
    const uint8_t public_key[CAPY_SIGNER_PUBLIC_KEY_LEN]) {
  size_t i;
  if (!public_key) {
    return;
  }
  for (i = 0u; i < CAPY_SIGNER_PUBLIC_KEY_LEN; ++i) {
    g_trusted_pk[i] = public_key[i];
  }
  g_has_trusted = 1;
}

void capy_signer_clear_trusted_public_key(void) {
  size_t i;
  for (i = 0u; i < CAPY_SIGNER_PUBLIC_KEY_LEN; ++i) {
    g_trusted_pk[i] = 0u;
  }
  g_has_trusted = 0;
}

int capy_signer_has_trusted_public_key(void) { return g_has_trusted; }

int capyagent_ed25519_verifier(const char *signed_text, size_t signed_len,
                               const char *signature_hex) {
  uint8_t sig[CAPY_SIGNER_SIGNATURE_LEN];
  if (!g_has_trusted || !signed_text || !signature_hex) {
    return -1; /* fail closed */
  }
  if (capy_signer_hex_decode(signature_hex, sig, CAPY_SIGNER_SIGNATURE_LEN) != 0) {
    return -1;
  }
  return capy_ed25519_verify(sig, (const uint8_t *)signed_text, signed_len,
                             g_trusted_pk);
}
