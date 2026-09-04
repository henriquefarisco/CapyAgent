#include "publish_resolver.h"

#include "component_manifest.h"
#include "component_plan.h"
#include "release_manifest.h"

static void resolver_zero(void *ptr, size_t len) {
  uint8_t *p = (uint8_t *)ptr;
  while (len--) *p++ = 0u;
}

static void resolver_copy(char *dst, size_t cap, const char *src) {
  size_t i = 0u;
  if (!dst || cap == 0u) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  while (i + 1u < cap && src[i]) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = '\0';
}

static int resolver_equal(const char *a, const char *b, size_t cap) {
  size_t i;
  if (!a || !b) return 0;
  for (i = 0u; i < cap; ++i) {
    if (a[i] != b[i]) return 0;
    if (a[i] == '\0') return 1;
  }
  return 0;
}

static int token_matches_core(const char *token, uint32_t version) {
  static const char prefix[] = "capyos-base-v";
  size_t i = 0u;
  uint32_t parsed = 0u;
  if (!token || version == 0u) return 0;
  while (prefix[i]) {
    if (token[i] != prefix[i]) return 0;
    ++i;
  }
  if (token[i] < '1' || token[i] > '9') return 0;
  while (token[i]) {
    uint32_t digit;
    if (token[i] < '0' || token[i] > '9') return 0;
    digit = (uint32_t)(token[i] - '0');
    if (parsed > (UINT32_MAX - digit) / 10u) return 0;
    parsed = parsed * 10u + digit;
    ++i;
  }
  return parsed == version;
}

static int publish_candidate_valid(
    const struct capy_component_descriptor *item,
    const struct capy_publish_request *request) {
  if (!capy_component_descriptor_valid(item) || !item->provides_abi[0] ||
      !capy_manifest_name_valid(item->provides_abi) || !item->abi_version[0] ||
      item->core_abi_min == 0u ||
      item->core_abi_max < item->core_abi_min || !item->known_good ||
      item->channel != request->channel) {
    return 0;
  }
  return request->core_abi_version >= item->core_abi_min &&
         request->core_abi_version <= item->core_abi_max;
}

static void resolve_fail(struct capy_publish_result *out,
                         enum capy_publish_resolve_status status,
                         const char *component) {
  out->status = status;
  resolver_copy(out->failing_component, sizeof(out->failing_component),
                component);
}

int capy_publish_resolve_latest(
    const struct capy_component_descriptor *candidates,
    uint32_t candidate_count,
    const struct capy_publish_request *request,
    struct capy_publish_result *out) {
  uint8_t consumed[CAPY_COMPONENT_INDEX_MAX_ITEMS];
  uint32_t i;
  if (!out) return -1;
  resolver_zero(out, sizeof(*out));
  capy_component_index_init(&out->index);
  if (!candidates || !request || candidate_count == 0u ||
      candidate_count > CAPY_COMPONENT_INDEX_MAX_ITEMS ||
      request->channel < CAPY_COMPONENT_CHANNEL_STABLE ||
      request->channel > CAPY_COMPONENT_CHANNEL_CUSTOM) {
    resolve_fail(out, CAPY_PUBLISH_RESOLVE_INVALID_INPUT, 0);
    return -1;
  }
  if (!token_matches_core(request->abi_token, request->core_abi_version)) {
    resolve_fail(out, CAPY_PUBLISH_RESOLVE_INVALID_TOKEN, 0);
    return -1;
  }
  resolver_zero(consumed, sizeof(consumed));

  for (i = 0u; i < candidate_count; ++i) {
    const struct capy_component_descriptor *best = 0;
    uint32_t best_index = 0u;
    uint32_t j;
    if (consumed[i]) continue;
    for (j = i; j < candidate_count; ++j) {
      int cmp = 0;
      if (!resolver_equal(candidates[i].id, candidates[j].id,
                          CAPY_COMPONENT_ID_MAX)) {
        continue;
      }
      consumed[j] = 1u;
      if (!publish_candidate_valid(&candidates[j], request)) continue;
      if (!best) {
        best = &candidates[j];
        best_index = j;
        continue;
      }
      if (capy_release_compare_versions(&candidates[j].tag[1], &best->tag[1],
                                        &cmp) != 0) {
        resolve_fail(out, CAPY_PUBLISH_RESOLVE_INVALID_INPUT,
                     candidates[j].id);
        return -1;
      }
      if (cmp == 0) {
        resolve_fail(out, CAPY_PUBLISH_RESOLVE_AMBIGUOUS_CANDIDATE,
                     candidates[j].id);
        return -1;
      }
      if (cmp > 0) {
        best = &candidates[j];
        best_index = j;
      }
    }
    (void)best_index;
    if (!best) {
      resolve_fail(out, CAPY_PUBLISH_RESOLVE_NO_COMPATIBLE_CANDIDATE,
                   candidates[i].id);
      return -1;
    }
    if (capy_component_index_add(&out->index, best) != 0) {
      resolve_fail(out, CAPY_PUBLISH_RESOLVE_INVALID_INPUT, best->id);
      return -1;
    }
  }

  /* The selected set must be dependency-closed and planable. */
  for (i = 0u; i < out->index.item_count; ++i) {
    uint32_t d;
    const struct capy_component_descriptor *item = &out->index.items[i];
    for (d = 0u; d < item->dependency_count; ++d) {
      if (!capy_component_index_find(&out->index, item->dependencies[d])) {
        resolve_fail(out, CAPY_PUBLISH_RESOLVE_DEPENDENCY_MISSING, item->id);
        return -1;
      }
    }
  }
  out->status = CAPY_PUBLISH_RESOLVE_OK;
  return 0;
}
