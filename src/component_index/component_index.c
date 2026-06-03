#include "component_index.h"

static void capy_component_zero(void *ptr, size_t len) {
  uint8_t *p = (uint8_t *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

/* Length-bounded string equality. Reads at most `max` bytes of each operand,
 * so it never runs past a fixed-size field that lacks a terminator. For
 * properly terminated strings shorter than `max` it is identical to strcmp==0. */
static int capy_component_streqn(const char *a, const char *b, uint32_t max) {
  uint32_t i;
  if (!a || !b) {
    return 0;
  }
  for (i = 0u; i < max; ++i) {
    if (a[i] != b[i]) {
      return 0;
    }
    if (a[i] == '\0') {
      return 1;
    }
  }
  return 0;
}

/* True iff `s` contains a NUL within the first `max` bytes (i.e. is a valid
 * bounded C string for a field of that size). */
static int capy_component_str_terminated(const char *s, uint32_t max) {
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

static int capy_component_is_digit(char c) {
  return c >= '0' && c <= '9';
}

static int capy_component_is_hex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static const struct capy_component_abi_requirement *capy_component_find_host_abi(
    const struct capy_component_host_abi *host, const char *name) {
  if (!host || !name) {
    return 0;
  }
  for (uint32_t i = 0; i < host->available_count; ++i) {
    if (capy_component_streqn(host->available[i].name, name,
                              CAPY_COMPONENT_ABI_NAME_MAX)) {
      return &host->available[i];
    }
  }
  return 0;
}

void capy_component_index_init(struct capy_component_index *index) {
  if (!index) {
    return;
  }
  capy_component_zero(index, sizeof(*index));
}

int capy_component_index_add(struct capy_component_index *index,
                             const struct capy_component_descriptor *item) {
  if (!index || !capy_component_descriptor_valid(item) ||
      index->item_count >= CAPY_COMPONENT_INDEX_MAX_ITEMS ||
      capy_component_index_find(index, item->id)) {
    return -1;
  }
  index->items[index->item_count++] = *item;
  return 0;
}

const struct capy_component_descriptor *capy_component_index_find(
    const struct capy_component_index *index, const char *id) {
  if (!index || !id) {
    return 0;
  }
  for (uint32_t i = 0; i < index->item_count; ++i) {
    if (capy_component_streqn(index->items[i].id, id, CAPY_COMPONENT_ID_MAX)) {
      return &index->items[i];
    }
  }
  return 0;
}

int capy_component_tag_valid(const char *tag) {
  uint32_t dots = 0u;
  uint32_t i;
  if (!tag || tag[0] != 'v' || !capy_component_is_digit(tag[1])) {
    return 0;
  }
  for (i = 1u; i < CAPY_COMPONENT_TAG_MAX; ++i) {
    char c = tag[i];
    if (c == '\0') {
      return dots >= 2u ? 1 : 0;
    }
    if (c == '.') {
      ++dots;
    } else if (!(capy_component_is_digit(c) || c == '-' || c == '+' ||
                 (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
      return 0;
    }
  }
  return 0; /* no terminator within the tag field: reject */
}

int capy_component_sha256_valid(const char *sha256) {
  if (!sha256) {
    return 0;
  }
  for (uint32_t i = 0; i < CAPY_COMPONENT_SHA256_HEX_LEN; ++i) {
    if (!capy_component_is_hex(sha256[i])) {
      return 0;
    }
  }
  return sha256[CAPY_COMPONENT_SHA256_HEX_LEN] == '\0' ? 1 : 0;
}

int capy_component_descriptor_valid(
    const struct capy_component_descriptor *item) {
  uint32_t i;
  if (!item || !item->id[0] || !item->name[0] || !item->artifact[0] ||
      !capy_component_str_terminated(item->id, CAPY_COMPONENT_ID_MAX) ||
      !capy_component_str_terminated(item->name, CAPY_COMPONENT_NAME_MAX) ||
      !capy_component_str_terminated(item->artifact,
                                     CAPY_COMPONENT_ARTIFACT_MAX) ||
      item->kind < CAPY_COMPONENT_KIND_APP ||
      item->kind >= CAPY_COMPONENT_KIND_UNKNOWN ||
      item->channel < CAPY_COMPONENT_CHANNEL_STABLE ||
      item->channel > CAPY_COMPONENT_CHANNEL_CUSTOM ||
      item->activation_class <= CAPY_COMPONENT_ACTIVATION_UNKNOWN ||
      item->activation_class > CAPY_COMPONENT_ACTIVATION_MANUAL ||
      !capy_component_tag_valid(item->tag) ||
      !capy_component_sha256_valid(item->sha256) ||
      item->required_abi_count > CAPY_COMPONENT_MAX_ABIS ||
      item->dependency_count > CAPY_COMPONENT_MAX_DEPENDENCIES ||
      item->permission_count > CAPY_COMPONENT_MAX_PERMISSIONS) {
    return 0;
  }
  for (i = 0u; i < item->required_abi_count; ++i) {
    if (!item->required_abis[i].name[0] ||
        !capy_component_str_terminated(item->required_abis[i].name,
                                       CAPY_COMPONENT_ABI_NAME_MAX) ||
        item->required_abis[i].minimum_version == 0u) {
      return 0;
    }
  }
  for (i = 0u; i < item->dependency_count; ++i) {
    if (!capy_component_str_terminated(item->dependencies[i],
                                       CAPY_COMPONENT_ID_MAX)) {
      return 0;
    }
  }
  for (i = 0u; i < item->permission_count; ++i) {
    if (!capy_component_str_terminated(item->permissions[i],
                                       CAPY_COMPONENT_ID_MAX)) {
      return 0;
    }
  }
  return 1;
}

int capy_component_descriptor_compatible(
    const struct capy_component_descriptor *item,
    const struct capy_component_host_abi *host) {
  if (!capy_component_descriptor_valid(item) || !host) {
    return 0;
  }
  for (uint32_t i = 0; i < item->required_abi_count; ++i) {
    const struct capy_component_abi_requirement *available =
        capy_component_find_host_abi(host, item->required_abis[i].name);
    if (!available ||
        available->minimum_version < item->required_abis[i].minimum_version) {
      return 0;
    }
  }
  return 1;
}
