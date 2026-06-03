#ifndef CAPY_AGENT_COMPONENT_MANIFEST_H
#define CAPY_AGENT_COMPONENT_MANIFEST_H

#include <stddef.h>
#include <stdint.h>

#include "component_index.h"

/*
 * Host-testable serializer for the line-oriented `key=value` manifest that the
 * CapyOS in-tree adapter `services/capypkg` consumes, plus the canonical
 * Ed25519 descriptor builder.
 *
 * The format is authoritative in
 * `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
 * and summarised in `docs/capypkg-publisher-guide.md`. This module is a pure,
 * deterministic, fail-closed function of its declared inputs: no allocation,
 * no I/O, no CapyOS headers.
 */

/* Hard limits mirrored from the CapyOS capypkg adapter contract. */
#define CAPY_MANIFEST_NAME_MAX 63u
#define CAPY_MANIFEST_SHA256_HEX_LEN 64u
#define CAPY_MANIFEST_SIGNATURE_HEX_LEN 128u
#define CAPY_MANIFEST_PAYLOAD_MAX (8u * 1024u * 1024u) /* CAPYPKG_PAYLOAD_MAX */
#define CAPY_MANIFEST_URL_MAX 2048u                    /* HTTP_MAX_URL */
#define CAPY_MANIFEST_DEPENDS_MAX 8u
#define CAPY_MANIFEST_CANONICAL_MAX 4096u
#define CAPY_MANIFEST_BLOCK_MAX 8192u

enum capy_manifest_status {
  CAPY_MANIFEST_OK = 0,
  CAPY_MANIFEST_INVALID_INPUT,
  CAPY_MANIFEST_NAME_INVALID,
  CAPY_MANIFEST_VERSION_INVALID,
  CAPY_MANIFEST_SHA256_INVALID,
  CAPY_MANIFEST_URL_INVALID,
  CAPY_MANIFEST_SIZE_INVALID,
  CAPY_MANIFEST_INSTALL_ROOT_INVALID,
  CAPY_MANIFEST_DEPENDS_INVALID,
  CAPY_MANIFEST_SIGNATURE_INVALID,
  CAPY_MANIFEST_SUMMARY_INVALID,
  CAPY_MANIFEST_OVERFLOW
};

struct capy_manifest_input {
  const char *name;           /* required; [a-zA-Z0-9._-], 1-63, not dot-only  */
  const char *version;        /* required; printable, no '|' (no leading v)    */
  const char *payload_url;    /* required; https:// only, <= 2048              */
  const char *payload_sha256; /* required; 64 lowercase hex                    */
  uint32_t payload_size;      /* 0 => omit; else <= CAPY_MANIFEST_PAYLOAD_MAX  */
  const char *install_root;   /* optional; under /var/capypkg/ or /opt/, no .. */
  const char *signature_hex;  /* optional; 128 lowercase hex                   */
  const char *summary;        /* optional; printable ASCII                     */
  const char *const *depends; /* optional; array of dependency names           */
  uint32_t depends_count;     /* <= 8                                          */
};

/* Field validators (fail-closed, deterministic). Return 1 if valid, 0 if not. */
int capy_manifest_name_valid(const char *name);
int capy_manifest_sha256_hex_valid(const char *sha256);
int capy_manifest_signature_hex_valid(const char *signature_hex);
int capy_manifest_https_url_valid(const char *url);
int capy_manifest_install_root_valid(const char *root);
int capy_manifest_value_printable(const char *value);

/* Copy a release tag into `out` stripped of a single leading 'v'/'V'.
 * Returns 1 on success, 0 on overflow/invalid input. */
int capy_manifest_version_from_tag(const char *tag, char *out, size_t out_size);

/* Build the canonical Ed25519 descriptor that the signature covers:
 *   name=<N>|version=<V>|payload_sha256=<H>|payload_url=<U>\n
 * Literal '|' separators, single '\n' terminator, fixed field order. */
enum capy_manifest_status capy_manifest_canonical_descriptor(
    const char *name, const char *version, const char *payload_sha256,
    const char *payload_url, char *out, size_t out_size, size_t *out_len);

/* Emit a full line-oriented manifest block terminated by "---\n". */
enum capy_manifest_status capy_manifest_emit(const struct capy_manifest_input *in,
                                             char *out, size_t out_size,
                                             size_t *out_len);

/* Convenience: derive a manifest from a validated component descriptor plus
 * release coordinates and emit it. payload_url = base_url + "/" + artifact;
 * version is the descriptor tag without its leading 'v'; name is the
 * descriptor id; depends come from the descriptor dependencies. install_root
 * defaults to /var/capypkg/<name> when NULL. */
enum capy_manifest_status capy_manifest_emit_descriptor(
    const struct capy_component_descriptor *descriptor, const char *base_url,
    const char *payload_sha256, uint32_t payload_size, const char *install_root,
    const char *signature_hex, char *out, size_t out_size, size_t *out_len);

#endif
