#include "capyagent_signer.h"
#include "component_manifest.h"
#include "ed25519.h"
#include "sha512.h"

#include <string.h>

static int fails;

#define EXPECT(expr) \
  do {               \
    if (!(expr)) {   \
      ++fails;       \
      return;        \
    }                \
  } while (0)

/* FIPS 180-4 SHA-512 known-answer tests. */
static void test_sha512_kat(void) {
  uint8_t digest[CAPY_SHA512_DIGEST_LEN];
  char hex[2u * CAPY_SHA512_DIGEST_LEN + 1u];

  capy_sha512((const uint8_t *)"", 0u, digest);
  EXPECT(capy_signer_hex_encode(digest, sizeof(digest), hex, sizeof(hex)) == 0);
  EXPECT(strcmp(hex,
                "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36c"
                "e9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327a"
                "f927da3e") == 0);

  capy_sha512((const uint8_t *)"abc", 3u, digest);
  EXPECT(capy_signer_hex_encode(digest, sizeof(digest), hex, sizeof(hex)) == 0);
  EXPECT(strcmp(hex,
                "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55"
                "d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94f"
                "a54ca49f") == 0);
}

/* RFC 8032 section 7.1, Test 1: verify a real Ed25519 vector (no seed needed,
 * so this anchors the verify path to the genuine curve constants). */
static void test_ed25519_rfc8032_verify(void) {
  uint8_t pk[CAPY_ED25519_PUBLIC_KEY_LEN];
  uint8_t sig[CAPY_ED25519_SIGNATURE_LEN];
  EXPECT(capy_signer_hex_decode(
             "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a",
             pk, CAPY_ED25519_PUBLIC_KEY_LEN) == 0);
  EXPECT(capy_signer_hex_decode(
             "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e0652249015"
             "55fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a10"
             "0b",
             sig, CAPY_ED25519_SIGNATURE_LEN) == 0);
  EXPECT(capy_ed25519_verify(sig, (const uint8_t *)"", 0u, pk) == 0);
  /* Any tamper must be rejected. */
  sig[0] ^= 0x01u;
  EXPECT(capy_ed25519_verify(sig, (const uint8_t *)"", 0u, pk) != 0);
}

/* sign -> verify roundtrip and tamper rejection (proves sign/verify are
 * mutually consistent; combined with the RFC verify KAT this implies the sign
 * path also produces genuine Ed25519 signatures). */
static void test_ed25519_roundtrip(void) {
  uint8_t seed[CAPY_ED25519_SEED_LEN];
  uint8_t pk[CAPY_ED25519_PUBLIC_KEY_LEN];
  uint8_t sk[CAPY_ED25519_SECRET_KEY_LEN];
  uint8_t sig[CAPY_ED25519_SIGNATURE_LEN];
  const char *msg = "name=org.capyos.agent.core|version=0.0.6";
  size_t mlen = strlen(msg);
  size_t i;
  for (i = 0u; i < CAPY_ED25519_SEED_LEN; ++i) {
    seed[i] = (uint8_t)(i + 1u);
  }
  capy_ed25519_keypair_from_seed(seed, pk, sk);
  capy_ed25519_sign(sig, (const uint8_t *)msg, mlen, sk);
  EXPECT(capy_ed25519_verify(sig, (const uint8_t *)msg, mlen, pk) == 0);

  sig[5] ^= 0x80u; /* tamper signature */
  EXPECT(capy_ed25519_verify(sig, (const uint8_t *)msg, mlen, pk) != 0);
  sig[5] ^= 0x80u; /* restore */

  EXPECT(capy_ed25519_verify(sig, (const uint8_t *)"different", 9u, pk) != 0);

  pk[0] ^= 0x01u; /* wrong key */
  EXPECT(capy_ed25519_verify(sig, (const uint8_t *)msg, mlen, pk) != 0);
}

static void test_hex_codec(void) {
  uint8_t in[4] = {0x00u, 0x7fu, 0xabu, 0xffu};
  uint8_t out[4];
  char hex[9];
  EXPECT(capy_signer_hex_encode(in, 4u, hex, sizeof(hex)) == 0);
  EXPECT(strcmp(hex, "007fabff") == 0);
  EXPECT(capy_signer_hex_decode(hex, out, 4u) == 0);
  EXPECT(memcmp(in, out, 4u) == 0);
  EXPECT(capy_signer_hex_encode(in, 4u, hex, 8u) == -1);   /* buffer too small */
  EXPECT(capy_signer_hex_decode("00xy", out, 2u) == -1);   /* invalid hex */
  EXPECT(capy_signer_hex_decode("00FF", out, 2u) == -1);   /* uppercase rejected */
  EXPECT(capy_signer_hex_decode("007fabff00", out, 4u) == -1); /* trailing */
}

static void test_signer_descriptor(void) {
  uint8_t seed[CAPY_SIGNER_SEED_LEN];
  uint8_t pk[CAPY_SIGNER_PUBLIC_KEY_LEN];
  uint8_t sk[CAPY_SIGNER_SECRET_KEY_LEN];
  char sha[CAPY_MANIFEST_SHA256_HEX_LEN + 1u];
  char hex[CAPY_SIGNER_SIGNATURE_HEX_LEN + 1u];
  char desc[CAPY_MANIFEST_CANONICAL_MAX];
  size_t desc_len = 0u;
  size_t i;
  const char *name = "org.capyos.agent.core";
  const char *version = "0.0.6";
  const char *url = "https://h/r/a.bin";

  for (i = 0u; i < CAPY_SIGNER_SEED_LEN; ++i) {
    seed[i] = (uint8_t)(0x42u + i);
  }
  for (i = 0u; i < CAPY_MANIFEST_SHA256_HEX_LEN; ++i) {
    sha[i] = 'a';
  }
  sha[CAPY_MANIFEST_SHA256_HEX_LEN] = '\0';

  capy_signer_keypair_from_seed(seed, pk, sk);
  EXPECT(capy_signer_sign_descriptor(sk, name, version, sha, url, hex,
                                     sizeof(hex)) == 0);
  EXPECT(strlen(hex) == CAPY_SIGNER_SIGNATURE_HEX_LEN);
  EXPECT(capy_signer_verify_descriptor(pk, name, version, sha, url, hex) == 0);
  /* Mutating any signed field invalidates the signature. */
  EXPECT(capy_signer_verify_descriptor(pk, name, "0.0.7", sha, url, hex) != 0);

  EXPECT(capy_manifest_canonical_descriptor(name, version, sha, url, desc,
                                            sizeof(desc),
                                            &desc_len) == CAPY_MANIFEST_OK);
  capy_signer_clear_trusted_public_key();
  EXPECT(capy_signer_has_trusted_public_key() == 0);
  /* Fail closed when no trusted key is configured. */
  EXPECT(capyagent_ed25519_verifier(desc, desc_len, hex) != 0);

  capy_signer_set_trusted_public_key(pk);
  EXPECT(capy_signer_has_trusted_public_key() == 1);
  EXPECT(capyagent_ed25519_verifier(desc, desc_len, hex) == 0);
  EXPECT(capyagent_ed25519_verifier(desc, desc_len, "abcd") != 0); /* bad hex */
  desc[0] ^= 0x01u;                                                /* tamper */
  EXPECT(capyagent_ed25519_verifier(desc, desc_len, hex) != 0);
  capy_signer_clear_trusted_public_key();
}

/*
 * Canonical-descriptor known-answer test.
 *
 * The sign->verify roundtrip above only proves the sign and verify paths agree
 * with each other; a symmetric change to the canonical layout (reordering
 * fields, swapping a separator) would slip through it unnoticed. This test
 * instead pins the EXACT canonical descriptor bytes and the EXACT Ed25519
 * signature over them to fixed, externally reproducible values.
 *
 * The expected public key and signature were produced by an independent RFC
 * 8032 oracle (OpenSSL via Python `cryptography`) from the documented 32-byte
 * seed 0x00..0x1f, so this anchors three guarantees the `capy-agent-component-
 * index` ABI and the pending verifier registration depend on:
 *   (a) seed -> Ed25519 keypair derivation,
 *   (b) the canonical descriptor field order / separators / `\n` terminator,
 *   (c) the signing path produces a genuine, standard Ed25519 signature.
 *
 * The same vector is mirrored in docs/compatibility.md so the CapyOS-side
 * `capypkg_set_signature_verifier` registration can confirm it wired up
 * `capyagent_ed25519_verifier` correctly. Frozen vector: do not edit.
 */
static void test_canonical_descriptor_kat(void) {
  static const char k_name[] = "org.capyos.agent.core";
  static const char k_version[] = "1.2.3";
  static const char k_sha256[] =
      "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08";
  static const char k_url[] =
      "https://github.com/henriquefarisco/CapyAgent/releases/download/"
      "v1.2.3/org.capyos.agent.core-1.2.3.bin";
  static const char k_expected_desc[] =
      "name=org.capyos.agent.core|version=1.2.3|payload_sha256="
      "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08|"
      "payload_url=https://github.com/henriquefarisco/CapyAgent/releases/"
      "download/v1.2.3/org.capyos.agent.core-1.2.3.bin\n";
  static const char k_pubkey_hex[] =
      "03a107bff3ce10be1d70dd18e74bc09967e4d6309ba50d5f1ddc8664125531b8";
  static const char k_signature_hex[] =
      "9788539478ef8b7d0a64339047a98f9a5833f7b069ef29d9a17cd25f8a664280"
      "6a80afe64708c4eece3d6d80eb3ebb415bedde868f5de01d9f8ae30a199c3d0a";

  uint8_t seed[CAPY_SIGNER_SEED_LEN];
  uint8_t pk[CAPY_SIGNER_PUBLIC_KEY_LEN];
  uint8_t sk[CAPY_SIGNER_SECRET_KEY_LEN];
  char pk_hex[2u * CAPY_SIGNER_PUBLIC_KEY_LEN + 1u];
  char sig_hex[CAPY_SIGNER_SIGNATURE_HEX_LEN + 1u];
  char desc[CAPY_MANIFEST_CANONICAL_MAX];
  size_t desc_len = 0u;
  size_t i;

  for (i = 0u; i < CAPY_SIGNER_SEED_LEN; ++i) {
    seed[i] = (uint8_t)i; /* documented vector seed 0x00..0x1f */
  }
  capy_signer_keypair_from_seed(seed, pk, sk);

  /* (a) seed -> public key is the known answer. */
  EXPECT(capy_signer_hex_encode(pk, sizeof(pk), pk_hex, sizeof(pk_hex)) == 0);
  EXPECT(strcmp(pk_hex, k_pubkey_hex) == 0);

  /* (b) canonical descriptor bytes are exactly the ABI-fixed layout. */
  EXPECT(capy_manifest_canonical_descriptor(k_name, k_version, k_sha256, k_url,
                                            desc, sizeof(desc), &desc_len) ==
         CAPY_MANIFEST_OK);
  EXPECT(strcmp(desc, k_expected_desc) == 0);
  EXPECT(desc_len == strlen(k_expected_desc));

  /* (c) Ed25519 signature over those bytes is the known answer. */
  EXPECT(capy_signer_sign_descriptor(sk, k_name, k_version, k_sha256, k_url,
                                     sig_hex, sizeof(sig_hex)) == 0);
  EXPECT(strcmp(sig_hex, k_signature_hex) == 0);

  /* The fixed signature verifies through both the explicit-key and the
   * registered-verifier paths, and any tamper fails closed. */
  EXPECT(capy_signer_verify_descriptor(pk, k_name, k_version, k_sha256, k_url,
                                       k_signature_hex) == 0);
  capy_signer_set_trusted_public_key(pk);
  EXPECT(capyagent_ed25519_verifier(desc, desc_len, k_signature_hex) == 0);
  desc[0] ^= 0x01u; /* flip one signed byte */
  EXPECT(capyagent_ed25519_verifier(desc, desc_len, k_signature_hex) != 0);
  capy_signer_clear_trusted_public_key();
}

int run_signer_tests(void) {
  fails = 0;
  test_sha512_kat();
  test_ed25519_rfc8032_verify();
  test_ed25519_roundtrip();
  test_hex_codec();
  test_signer_descriptor();
  test_canonical_descriptor_kat();
  return fails;
}
