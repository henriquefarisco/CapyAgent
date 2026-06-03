#include "release_manifest.h"

struct capy_release_version_key {
  uint32_t major;
  uint32_t minor;
  uint32_t patch;
  uint32_t prerelease_number;
  int prerelease_rank;
};

static void capy_release_zero(void *ptr, size_t len) {
  uint8_t *p = (uint8_t *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

static int capy_release_is_digit(char c) {
  return c >= '0' && c <= '9';
}

static int capy_release_is_hex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

/* True iff `s` has a NUL within the first `max` bytes. */
static int capy_release_str_terminated(const char *s, uint32_t max) {
  uint32_t i;
  if (!s) {
    return 0;
  }
  for (i = 0u; i < max; ++i) {
    if (s[i] == '\0') {
      return 1;
    }
  }
  return 0;
}

static int capy_release_read_number(const char **cursor, uint32_t *out) {
  const char *p = cursor ? *cursor : 0;
  uint32_t value = 0u;
  if (!p || !out || !capy_release_is_digit(*p)) {
    return -1;
  }
  while (capy_release_is_digit(*p)) {
    value = value * 10u + (uint32_t)(*p - '0');
    ++p;
  }
  *cursor = p;
  *out = value;
  return 0;
}

static int capy_release_prerelease_rank(const char *start, size_t len) {
  if (len == 5u && start[0] == 'a' && start[1] == 'l' && start[2] == 'p' &&
      start[3] == 'h' && start[4] == 'a') {
    return 1;
  }
  if (len == 4u && start[0] == 'b' && start[1] == 'e' && start[2] == 't' &&
      start[3] == 'a') {
    return 2;
  }
  if (len == 2u && start[0] == 'r' && start[1] == 'c') {
    return 3;
  }
  return 0;
}

static int capy_release_parse_version(const char *version,
                                      struct capy_release_version_key *out) {
  const char *p = version;
  if (!version || !out) {
    return -1;
  }
  if (*p == 'v' || *p == 'V') {
    ++p;
  }
  out->major = 0u;
  out->minor = 0u;
  out->patch = 0u;
  out->prerelease_number = 0u;
  out->prerelease_rank = 4;
  if (capy_release_read_number(&p, &out->major) != 0 || *p++ != '.' ||
      capy_release_read_number(&p, &out->minor) != 0 || *p++ != '.' ||
      capy_release_read_number(&p, &out->patch) != 0) {
    return -1;
  }
  if (*p == '-') {
    const char *label = ++p;
    size_t label_len = 0u;
    /* Bounded: valid pre-release labels are alpha/beta/rc; cap the scan so an
     * unterminated buffer cannot be walked off the end. */
    while (label_len < 8u && p[label_len] && p[label_len] != '.' &&
           p[label_len] != '+') {
      ++label_len;
    }
    out->prerelease_rank = capy_release_prerelease_rank(label, label_len);
    if (out->prerelease_rank == 0) {
      return -1;
    }
    p += label_len;
    if (*p == '.') {
      ++p;
      if (capy_release_read_number(&p, &out->prerelease_number) != 0) {
        return -1;
      }
    }
  }
  if (*p == '+') {
    return 0;
  }
  return *p == '\0' ? 0 : -1;
}

static int capy_release_compare_u32(uint32_t a, uint32_t b) {
  if (a < b) {
    return -1;
  }
  if (a > b) {
    return 1;
  }
  return 0;
}

void capy_release_manifest_init(struct capy_release_manifest *manifest) {
  if (!manifest) {
    return;
  }
  capy_release_zero(manifest, sizeof(*manifest));
}

int capy_release_tag_valid(const char *tag) {
  uint32_t dots = 0u;
  uint32_t i;
  if (!tag || tag[0] != 'v' || !capy_release_is_digit(tag[1])) {
    return 0;
  }
  for (i = 1u; i < CAPY_RELEASE_TAG_MAX; ++i) {
    char c = tag[i];
    if (c == '\0') {
      return dots >= 2u ? 1 : 0;
    }
    if (c == '.') {
      ++dots;
    } else if (!(capy_release_is_digit(c) || c == '-' || c == '+' ||
                 (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
      return 0;
    }
  }
  return 0; /* no terminator within the tag field: reject */
}

int capy_release_sha256_valid(const char *sha256) {
  if (!sha256) {
    return 0;
  }
  for (uint32_t i = 0; i < CAPY_RELEASE_SHA256_HEX_LEN; ++i) {
    if (!capy_release_is_hex(sha256[i])) {
      return 0;
    }
  }
  return sha256[CAPY_RELEASE_SHA256_HEX_LEN] == '\0' ? 1 : 0;
}

int capy_release_manifest_valid(const struct capy_release_manifest *manifest) {
  struct capy_release_version_key key;
  if (!manifest || !manifest->version[0] || !manifest->repository[0] ||
      !manifest->artifact[0] ||
      !capy_release_str_terminated(manifest->version,
                                   CAPY_RELEASE_VERSION_MAX) ||
      !capy_release_str_terminated(manifest->repository,
                                   CAPY_RELEASE_REPOSITORY_MAX) ||
      !capy_release_str_terminated(manifest->artifact,
                                   CAPY_RELEASE_ARTIFACT_MAX) ||
      !capy_release_tag_valid(manifest->tag) ||
      !capy_release_sha256_valid(manifest->sha256) ||
      manifest->minimum_base_abi == 0u) {
    return 0;
  }
  return capy_release_parse_version(manifest->version, &key) == 0;
}

int capy_release_compare_versions(const char *candidate, const char *current,
                                  int *out_cmp) {
  struct capy_release_version_key a;
  struct capy_release_version_key b;
  int cmp;
  if (!out_cmp || capy_release_parse_version(candidate, &a) != 0 ||
      capy_release_parse_version(current, &b) != 0) {
    return -1;
  }
  cmp = capy_release_compare_u32(a.major, b.major);
  if (cmp == 0) cmp = capy_release_compare_u32(a.minor, b.minor);
  if (cmp == 0) cmp = capy_release_compare_u32(a.patch, b.patch);
  if (cmp == 0) cmp = capy_release_compare_u32((uint32_t)a.prerelease_rank,
                                               (uint32_t)b.prerelease_rank);
  if (cmp == 0) cmp = capy_release_compare_u32(a.prerelease_number,
                                               b.prerelease_number);
  *out_cmp = cmp;
  return 0;
}

int capy_release_manifest_compatible(const struct capy_release_manifest *manifest,
                                     uint32_t base_abi,
                                     uint32_t package_abi) {
  if (!capy_release_manifest_valid(manifest)) {
    return 0;
  }
  if (base_abi < manifest->minimum_base_abi) {
    return 0;
  }
  if (manifest->minimum_package_abi &&
      package_abi < manifest->minimum_package_abi) {
    return 0;
  }
  return 1;
}
