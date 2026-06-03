#include "component_index.h"
#include "component_manifest.h"

#include <string.h>

static int fails;

#define EXPECT(expr) \
  do {               \
    if (!(expr)) {   \
      ++fails;       \
      return;        \
    }                \
  } while (0)

static void fill_hex(char *out, size_t hex_len) {
  size_t i;
  for (i = 0u; i < hex_len; ++i) {
    out[i] = 'a';
  }
  out[hex_len] = '\0';
}

static void test_canonical_descriptor(void) {
  char sha[CAPY_MANIFEST_SHA256_HEX_LEN + 1u];
  char out[CAPY_MANIFEST_CANONICAL_MAX];
  char expected[512];
  size_t len = 0u;
  fill_hex(sha, CAPY_MANIFEST_SHA256_HEX_LEN);

  EXPECT(capy_manifest_canonical_descriptor(
             "org.capyos.agent.core", "0.0.6", sha, "https://h/r/a.bin", out,
             sizeof(out), &len) == CAPY_MANIFEST_OK);

  strcpy(expected, "name=org.capyos.agent.core|version=0.0.6|payload_sha256=");
  strcat(expected, sha);
  strcat(expected, "|payload_url=https://h/r/a.bin\n");
  EXPECT(strcmp(out, expected) == 0);
  EXPECT(len == strlen(expected));

  /* Fail-closed inputs. */
  EXPECT(capy_manifest_canonical_descriptor("bad name", "0.0.6", sha,
                                            "https://h/a", out, sizeof(out),
                                            &len) == CAPY_MANIFEST_NAME_INVALID);
  EXPECT(capy_manifest_canonical_descriptor(
             "org.capyos.agent.core", "0.0|6", sha, "https://h/a", out,
             sizeof(out), &len) == CAPY_MANIFEST_VERSION_INVALID);
  EXPECT(capy_manifest_canonical_descriptor(
             "org.capyos.agent.core", "0.0.6", "tooshort", "https://h/a", out,
             sizeof(out), &len) == CAPY_MANIFEST_SHA256_INVALID);
  EXPECT(capy_manifest_canonical_descriptor(
             "org.capyos.agent.core", "0.0.6", sha, "http://h/a", out,
             sizeof(out), &len) == CAPY_MANIFEST_URL_INVALID);
  /* Tiny output buffer overflows fail closed. */
  EXPECT(capy_manifest_canonical_descriptor("org.capyos.agent.core", "0.0.6",
                                            sha, "https://h/a", out, 8u,
                                            &len) == CAPY_MANIFEST_OVERFLOW);
}

static void test_emit_minimal(void) {
  char sha[CAPY_MANIFEST_SHA256_HEX_LEN + 1u];
  char out[CAPY_MANIFEST_BLOCK_MAX];
  struct capy_manifest_input in;
  size_t len = 0u;
  fill_hex(sha, CAPY_MANIFEST_SHA256_HEX_LEN);
  memset(&in, 0, sizeof(in));
  in.name = "org.capyos.agent.core";
  in.version = "0.0.6";
  in.payload_url = "https://h/r/a.bin";
  in.payload_sha256 = sha;

  EXPECT(capy_manifest_emit(&in, out, sizeof(out), &len) == CAPY_MANIFEST_OK);
  EXPECT(len == strlen(out));
  EXPECT(strstr(out, "name=org.capyos.agent.core\n") != 0);
  EXPECT(strstr(out, "version=0.0.6\n") != 0);
  EXPECT(strstr(out, "payload_url=https://h/r/a.bin\n") != 0);
  EXPECT(strstr(out, "depends=\n") != 0);
  /* Block terminator present. */
  EXPECT(strstr(out, "---\n") != 0);
  /* Omitted optional fields must not appear. */
  EXPECT(strstr(out, "summary=") == 0);
  EXPECT(strstr(out, "payload_size=") == 0);
  EXPECT(strstr(out, "signature_ed25519=") == 0);
  EXPECT(strstr(out, "install_root=") == 0);
}

static void test_emit_full(void) {
  char sha[CAPY_MANIFEST_SHA256_HEX_LEN + 1u];
  char sig[CAPY_MANIFEST_SIGNATURE_HEX_LEN + 1u];
  char out[CAPY_MANIFEST_BLOCK_MAX];
  const char *deps[1];
  struct capy_manifest_input in;
  size_t len = 0u;
  fill_hex(sha, CAPY_MANIFEST_SHA256_HEX_LEN);
  fill_hex(sig, CAPY_MANIFEST_SIGNATURE_HEX_LEN);
  deps[0] = "org.capyos.codecs.image-basic";
  memset(&in, 0, sizeof(in));
  in.name = "org.capyos.agent.core";
  in.version = "0.0.6";
  in.payload_url = "https://h/r/a.bin";
  in.payload_sha256 = sha;
  in.payload_size = 524288u;
  in.signature_hex = sig;
  in.summary = "CapyAgent core";
  in.install_root = "/var/capypkg/org.capyos.agent.core";
  in.depends = deps;
  in.depends_count = 1u;

  EXPECT(capy_manifest_emit(&in, out, sizeof(out), &len) == CAPY_MANIFEST_OK);
  EXPECT(strstr(out, "summary=CapyAgent core\n") != 0);
  EXPECT(strstr(out, "payload_size=524288\n") != 0);
  EXPECT(strstr(out, "signature_ed25519=") != 0);
  EXPECT(strstr(out, "install_root=/var/capypkg/org.capyos.agent.core\n") != 0);
  EXPECT(strstr(out, "depends=org.capyos.codecs.image-basic\n") != 0);
}

static void test_emit_negatives(void) {
  char sha[CAPY_MANIFEST_SHA256_HEX_LEN + 1u];
  char out[CAPY_MANIFEST_BLOCK_MAX];
  const char *bad_deps[1];
  struct capy_manifest_input in;
  fill_hex(sha, CAPY_MANIFEST_SHA256_HEX_LEN);

  memset(&in, 0, sizeof(in));
  in.version = "0.0.6";
  in.payload_url = "https://h/a";
  in.payload_sha256 = sha;

  in.name = "bad name";
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) == CAPY_MANIFEST_NAME_INVALID);
  in.name = ".."; /* dot-only */
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) == CAPY_MANIFEST_NAME_INVALID);

  in.name = "org.capyos.agent.core";
  in.payload_url = "http://h/a"; /* not https */
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) == CAPY_MANIFEST_URL_INVALID);
  in.payload_url = "https://h/a";

  in.payload_sha256 = "short";
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) == CAPY_MANIFEST_SHA256_INVALID);
  in.payload_sha256 = sha;

  in.payload_size = CAPY_MANIFEST_PAYLOAD_MAX + 1u;
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) == CAPY_MANIFEST_SIZE_INVALID);
  in.payload_size = 0u;

  in.install_root = "/etc/passwd"; /* outside scope */
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) ==
         CAPY_MANIFEST_INSTALL_ROOT_INVALID);
  in.install_root = "/var/capypkg/../escape"; /* .. segment */
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) ==
         CAPY_MANIFEST_INSTALL_ROOT_INVALID);
  in.install_root = 0;

  in.signature_hex = "abcd"; /* wrong length */
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) ==
         CAPY_MANIFEST_SIGNATURE_INVALID);
  in.signature_hex = 0;

  in.summary = "bad\tsummary"; /* control byte */
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) ==
         CAPY_MANIFEST_SUMMARY_INVALID);
  in.summary = 0;

  bad_deps[0] = "bad dep"; /* space in dependency */
  in.depends = bad_deps;
  in.depends_count = 1u;
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) ==
         CAPY_MANIFEST_DEPENDS_INVALID);
  in.depends_count = CAPY_MANIFEST_DEPENDS_MAX + 1u; /* too many */
  EXPECT(capy_manifest_emit(&in, out, sizeof(out), 0) ==
         CAPY_MANIFEST_DEPENDS_INVALID);
}

static void test_version_from_tag(void) {
  char out[CAPY_COMPONENT_TAG_MAX];
  EXPECT(capy_manifest_version_from_tag("v0.0.6", out, sizeof(out)) == 1);
  EXPECT(strcmp(out, "0.0.6") == 0);
  EXPECT(capy_manifest_version_from_tag("0.0.6", out, sizeof(out)) == 1);
  EXPECT(strcmp(out, "0.0.6") == 0);
  EXPECT(capy_manifest_version_from_tag("V1.2.3", out, sizeof(out)) == 1);
  EXPECT(strcmp(out, "1.2.3") == 0);
}

static void make_valid_descriptor(struct capy_component_descriptor *d) {
  memset(d, 0, sizeof(*d));
  strcpy(d->id, "org.capyos.agent.core");
  strcpy(d->name, "CapyAgent Core");
  d->kind = CAPY_COMPONENT_KIND_AGENT;
  d->channel = CAPY_COMPONENT_CHANNEL_STABLE;
  strcpy(d->tag, "v0.0.1");
  strcpy(d->artifact, "agent.capypkg");
  {
    size_t i;
    for (i = 0u; i < CAPY_COMPONENT_SHA256_HEX_LEN; ++i) {
      d->sha256[i] = 'a';
    }
    d->sha256[CAPY_COMPONENT_SHA256_HEX_LEN] = '\0';
  }
  d->activation_class = CAPY_COMPONENT_ACTIVATION_ATOMIC;
  strcpy(d->required_abis[0].name, "capyos-base");
  d->required_abis[0].minimum_version = 1u;
  d->required_abi_count = 1u;
}

static void test_emit_descriptor(void) {
  char sha[CAPY_MANIFEST_SHA256_HEX_LEN + 1u];
  char out[CAPY_MANIFEST_BLOCK_MAX];
  struct capy_component_descriptor d;
  size_t len = 0u;
  fill_hex(sha, CAPY_MANIFEST_SHA256_HEX_LEN);
  make_valid_descriptor(&d);

  EXPECT(capy_manifest_emit_descriptor(&d, "https://h/r", sha, 100u, 0, 0, out,
                                       sizeof(out), &len) == CAPY_MANIFEST_OK);
  EXPECT(strstr(out, "name=org.capyos.agent.core\n") != 0);
  EXPECT(strstr(out, "version=0.0.1\n") != 0);
  EXPECT(strstr(out, "payload_url=https://h/r/agent.capypkg\n") != 0);
  EXPECT(strstr(out, "payload_size=100\n") != 0);
  EXPECT(strstr(out, "install_root=/var/capypkg/org.capyos.agent.core\n") != 0);

  /* Trailing slash on the base URL must not double up. */
  EXPECT(capy_manifest_emit_descriptor(&d, "https://h/r/", sha, 100u, 0, 0, out,
                                       sizeof(out), &len) == CAPY_MANIFEST_OK);
  EXPECT(strstr(out, "payload_url=https://h/r/agent.capypkg\n") != 0);

  /* A non-https base URL fails closed. */
  EXPECT(capy_manifest_emit_descriptor(&d, "http://h/r", sha, 100u, 0, 0, out,
                                       sizeof(out), &len) ==
         CAPY_MANIFEST_URL_INVALID);
}

int run_manifest_tests(void) {
  fails = 0;
  test_canonical_descriptor();
  test_emit_minimal();
  test_emit_full();
  test_emit_negatives();
  test_version_from_tag();
  test_emit_descriptor();
  return fails;
}
