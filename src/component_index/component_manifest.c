#include "component_manifest.h"

/* Bounded, overflow-safe string builder. No allocation. */
struct capy_buf {
  char *data;
  size_t cap;
  size_t len;
  int ok;
};

static void buf_init(struct capy_buf *b, char *data, size_t cap) {
  b->data = data;
  b->cap = cap;
  b->len = 0u;
  b->ok = (cap > 0u) ? 1 : 0;
  if (cap > 0u) {
    data[0] = '\0';
  }
}

static void buf_putc(struct capy_buf *b, char c) {
  if (!b->ok) {
    return;
  }
  if (b->len + 1u >= b->cap) {
    b->ok = 0;
    return;
  }
  b->data[b->len++] = c;
  b->data[b->len] = '\0';
}

static void buf_puts(struct capy_buf *b, const char *s) {
  if (!b->ok || !s) {
    return;
  }
  while (*s) {
    buf_putc(b, *s++);
    if (!b->ok) {
      return;
    }
  }
}

static void buf_putu32(struct capy_buf *b, uint32_t v) {
  char tmp[10];
  int n = 0;
  if (v == 0u) {
    buf_putc(b, '0');
    return;
  }
  while (v > 0u && n < (int)sizeof(tmp)) {
    tmp[n++] = (char)('0' + (int)(v % 10u));
    v /= 10u;
  }
  while (n-- > 0) {
    buf_putc(b, tmp[n]);
  }
}

static int is_lower_hex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static int has_pipe(const char *s) {
  for (; *s; ++s) {
    if (*s == '|') {
      return 1;
    }
  }
  return 0;
}

int capy_manifest_value_printable(const char *value) {
  size_t i;
  if (!value) {
    return 0;
  }
  for (i = 0u; value[i]; ++i) {
    unsigned char c = (unsigned char)value[i];
    if (c < 0x20u || c > 0x7Eu) {
      return 0;
    }
  }
  return 1;
}

int capy_manifest_name_valid(const char *name) {
  size_t i;
  int has_nondot = 0;
  if (!name) {
    return 0;
  }
  for (i = 0u; name[i]; ++i) {
    char c = name[i];
    if (i >= CAPY_MANIFEST_NAME_MAX) {
      return 0;
    }
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-')) {
      return 0;
    }
    if (c != '.') {
      has_nondot = 1;
    }
  }
  if (i == 0u || !has_nondot) {
    return 0;
  }
  return 1;
}

int capy_manifest_sha256_hex_valid(const char *sha256) {
  size_t i;
  if (!sha256) {
    return 0;
  }
  for (i = 0u; i < CAPY_MANIFEST_SHA256_HEX_LEN; ++i) {
    if (!is_lower_hex(sha256[i])) {
      return 0;
    }
  }
  return sha256[CAPY_MANIFEST_SHA256_HEX_LEN] == '\0' ? 1 : 0;
}

int capy_manifest_signature_hex_valid(const char *signature_hex) {
  size_t i;
  if (!signature_hex) {
    return 0;
  }
  for (i = 0u; i < CAPY_MANIFEST_SIGNATURE_HEX_LEN; ++i) {
    if (!is_lower_hex(signature_hex[i])) {
      return 0;
    }
  }
  return signature_hex[CAPY_MANIFEST_SIGNATURE_HEX_LEN] == '\0' ? 1 : 0;
}

int capy_manifest_https_url_valid(const char *url) {
  static const char prefix[] = "https://";
  size_t i;
  if (!url) {
    return 0;
  }
  for (i = 0u; prefix[i]; ++i) {
    if (url[i] != prefix[i]) {
      return 0;
    }
  }
  if (url[i] == '\0') {
    return 0; /* nothing after the scheme */
  }
  for (i = 0u; url[i]; ++i) {
    unsigned char c = (unsigned char)url[i];
    if (i >= CAPY_MANIFEST_URL_MAX) {
      return 0;
    }
    if (c <= 0x20u || c == 0x7Fu || c == '|') {
      return 0;
    }
  }
  return 1;
}

int capy_manifest_install_root_valid(const char *root) {
  static const char scope_var[] = "/var/capypkg/";
  static const char scope_opt[] = "/opt/";
  const char *p;
  size_t i;
  int under_var = 1;
  int under_opt = 1;
  if (!root || root[0] != '/') {
    return 0;
  }
  for (i = 0u; scope_var[i]; ++i) {
    if (root[i] != scope_var[i]) {
      under_var = 0;
      break;
    }
  }
  for (i = 0u; scope_opt[i]; ++i) {
    if (root[i] != scope_opt[i]) {
      under_opt = 0;
      break;
    }
  }
  if (!under_var && !under_opt) {
    return 0;
  }
  for (i = 0u; root[i]; ++i) {
    unsigned char c = (unsigned char)root[i];
    if (i >= 256u || c < 0x20u || c > 0x7Eu) {
      return 0;
    }
  }
  /* Reject any ".." path segment. */
  p = root;
  while (*p) {
    const char *seg = p;
    while (*p && *p != '/') {
      ++p;
    }
    if ((size_t)(p - seg) == 2u && seg[0] == '.' && seg[1] == '.') {
      return 0;
    }
    if (*p == '/') {
      ++p;
    }
  }
  return 1;
}

int capy_manifest_version_from_tag(const char *tag, char *out, size_t out_size) {
  size_t i = 0u;
  const char *p = tag;
  if (!tag || !out || out_size == 0u) {
    return 0;
  }
  if (*p == 'v' || *p == 'V') {
    ++p;
  }
  while (p[i]) {
    if (i + 1u >= out_size) {
      out[0] = '\0';
      return 0;
    }
    out[i] = p[i];
    ++i;
  }
  out[i] = '\0';
  return i > 0u ? 1 : 0;
}

enum capy_manifest_status capy_manifest_canonical_descriptor(
    const char *name, const char *version, const char *payload_sha256,
    const char *payload_url, char *out, size_t out_size, size_t *out_len) {
  struct capy_buf b;
  if (out_len) {
    *out_len = 0u;
  }
  if (!name || !version || !payload_sha256 || !payload_url || !out ||
      out_size == 0u) {
    return CAPY_MANIFEST_INVALID_INPUT;
  }
  if (!capy_manifest_name_valid(name)) {
    return CAPY_MANIFEST_NAME_INVALID;
  }
  if (version[0] == '\0' || !capy_manifest_value_printable(version) ||
      has_pipe(version)) {
    return CAPY_MANIFEST_VERSION_INVALID;
  }
  if (!capy_manifest_sha256_hex_valid(payload_sha256)) {
    return CAPY_MANIFEST_SHA256_INVALID;
  }
  if (!capy_manifest_https_url_valid(payload_url)) {
    return CAPY_MANIFEST_URL_INVALID;
  }
  buf_init(&b, out, out_size);
  buf_puts(&b, "name=");
  buf_puts(&b, name);
  buf_putc(&b, '|');
  buf_puts(&b, "version=");
  buf_puts(&b, version);
  buf_putc(&b, '|');
  buf_puts(&b, "payload_sha256=");
  buf_puts(&b, payload_sha256);
  buf_putc(&b, '|');
  buf_puts(&b, "payload_url=");
  buf_puts(&b, payload_url);
  buf_putc(&b, '\n');
  if (!b.ok) {
    return CAPY_MANIFEST_OVERFLOW;
  }
  if (out_len) {
    *out_len = b.len;
  }
  return CAPY_MANIFEST_OK;
}

static int names_equal(const char *a, const char *b) {
  size_t i = 0u;
  if (!a || !b) {
    return 0;
  }
  while (a[i] != '\0' && a[i] == b[i]) {
    ++i;
  }
  return a[i] == b[i];
}

enum capy_manifest_status capy_manifest_emit(const struct capy_manifest_input *in,
                                             char *out, size_t out_size,
                                             size_t *out_len) {
  struct capy_buf b;
  uint32_t i;
  if (out_len) {
    *out_len = 0u;
  }
  if (!in || !out || out_size == 0u || !in->name || !in->version ||
      !in->payload_url || !in->payload_sha256) {
    return CAPY_MANIFEST_INVALID_INPUT;
  }
  if (!capy_manifest_name_valid(in->name)) {
    return CAPY_MANIFEST_NAME_INVALID;
  }
  if (in->version[0] == '\0' || !capy_manifest_value_printable(in->version) ||
      has_pipe(in->version)) {
    return CAPY_MANIFEST_VERSION_INVALID;
  }
  if (!capy_manifest_https_url_valid(in->payload_url)) {
    return CAPY_MANIFEST_URL_INVALID;
  }
  if (!capy_manifest_sha256_hex_valid(in->payload_sha256)) {
    return CAPY_MANIFEST_SHA256_INVALID;
  }
  if (in->payload_size > CAPY_MANIFEST_PAYLOAD_MAX) {
    return CAPY_MANIFEST_SIZE_INVALID;
  }
  if (in->summary && !capy_manifest_value_printable(in->summary)) {
    return CAPY_MANIFEST_SUMMARY_INVALID;
  }
  if (in->signature_hex && !capy_manifest_signature_hex_valid(in->signature_hex)) {
    return CAPY_MANIFEST_SIGNATURE_INVALID;
  }
  if (in->install_root && !capy_manifest_install_root_valid(in->install_root)) {
    return CAPY_MANIFEST_INSTALL_ROOT_INVALID;
  }
  if (in->depends_count > CAPY_MANIFEST_DEPENDS_MAX) {
    return CAPY_MANIFEST_DEPENDS_INVALID;
  }
  for (i = 0u; i < in->depends_count; ++i) {
    uint32_t j;
    if (!in->depends || !in->depends[i] ||
        !capy_manifest_name_valid(in->depends[i])) {
      return CAPY_MANIFEST_DEPENDS_INVALID;
    }
    if (names_equal(in->depends[i], in->name)) {
      return CAPY_MANIFEST_DEPENDS_INVALID; /* self-dependency */
    }
    for (j = 0u; j < i; ++j) {
      if (names_equal(in->depends[i], in->depends[j])) {
        return CAPY_MANIFEST_DEPENDS_INVALID; /* duplicate dependency name */
      }
    }
  }

  buf_init(&b, out, out_size);
  buf_puts(&b, "name=");
  buf_puts(&b, in->name);
  buf_putc(&b, '\n');
  buf_puts(&b, "version=");
  buf_puts(&b, in->version);
  buf_putc(&b, '\n');
  if (in->summary && in->summary[0]) {
    buf_puts(&b, "summary=");
    buf_puts(&b, in->summary);
    buf_putc(&b, '\n');
  }
  buf_puts(&b, "payload_url=");
  buf_puts(&b, in->payload_url);
  buf_putc(&b, '\n');
  buf_puts(&b, "payload_sha256=");
  buf_puts(&b, in->payload_sha256);
  buf_putc(&b, '\n');
  if (in->payload_size > 0u) {
    buf_puts(&b, "payload_size=");
    buf_putu32(&b, in->payload_size);
    buf_putc(&b, '\n');
  }
  if (in->signature_hex && in->signature_hex[0]) {
    buf_puts(&b, "signature_ed25519=");
    buf_puts(&b, in->signature_hex);
    buf_putc(&b, '\n');
  }
  if (in->install_root && in->install_root[0]) {
    buf_puts(&b, "install_root=");
    buf_puts(&b, in->install_root);
    buf_putc(&b, '\n');
  }
  buf_puts(&b, "depends=");
  for (i = 0u; i < in->depends_count; ++i) {
    if (i > 0u) {
      buf_putc(&b, ',');
    }
    buf_puts(&b, in->depends[i]);
  }
  buf_putc(&b, '\n');
  buf_puts(&b, "---\n");
  if (!b.ok) {
    return CAPY_MANIFEST_OVERFLOW;
  }
  if (out_len) {
    *out_len = b.len;
  }
  return CAPY_MANIFEST_OK;
}

enum capy_manifest_status capy_manifest_emit_descriptor(
    const struct capy_component_descriptor *descriptor, const char *base_url,
    const char *payload_sha256, uint32_t payload_size, const char *install_root,
    const char *signature_hex, char *out, size_t out_size, size_t *out_len) {
  struct capy_manifest_input in;
  struct capy_buf ub;
  struct capy_buf rb;
  char version[CAPY_COMPONENT_TAG_MAX];
  char url[CAPY_MANIFEST_URL_MAX + 1u];
  char root[128];
  const char *deps[CAPY_COMPONENT_MAX_DEPENDENCIES];
  uint32_t i;
  size_t base_len = 0u;

  if (out_len) {
    *out_len = 0u;
  }
  if (!descriptor || !capy_component_descriptor_valid(descriptor)) {
    return CAPY_MANIFEST_INVALID_INPUT;
  }
  if (!base_url || !capy_manifest_https_url_valid(base_url)) {
    return CAPY_MANIFEST_URL_INVALID;
  }
  if (!capy_manifest_name_valid(descriptor->id)) {
    return CAPY_MANIFEST_NAME_INVALID;
  }
  if (!capy_manifest_version_from_tag(descriptor->tag, version, sizeof(version))) {
    return CAPY_MANIFEST_VERSION_INVALID;
  }

  /* payload_url = base_url (without trailing '/') + "/" + artifact */
  buf_init(&ub, url, sizeof(url));
  buf_puts(&ub, base_url);
  while (base_url[base_len]) {
    ++base_len;
  }
  if (base_len > 0u && base_url[base_len - 1u] == '/' && ub.ok && ub.len > 0u) {
    ub.len--;
    ub.data[ub.len] = '\0';
  }
  buf_putc(&ub, '/');
  buf_puts(&ub, descriptor->artifact);
  if (!ub.ok) {
    return CAPY_MANIFEST_OVERFLOW;
  }

  for (i = 0u; i < descriptor->dependency_count &&
              i < CAPY_COMPONENT_MAX_DEPENDENCIES;
       ++i) {
    deps[i] = descriptor->dependencies[i];
  }

  in.name = descriptor->id;
  in.version = version;
  in.payload_url = url;
  in.payload_sha256 = payload_sha256;
  in.payload_size = payload_size;
  in.signature_hex = signature_hex;
  in.summary = 0;
  in.depends = (descriptor->dependency_count > 0u) ? deps : 0;
  in.depends_count = descriptor->dependency_count;

  if (install_root) {
    in.install_root = install_root;
  } else {
    buf_init(&rb, root, sizeof(root));
    buf_puts(&rb, "/var/capypkg/");
    buf_puts(&rb, descriptor->id);
    if (!rb.ok) {
      return CAPY_MANIFEST_OVERFLOW;
    }
    in.install_root = root;
  }

  return capy_manifest_emit(&in, out, out_size, out_len);
}
