#include "component_plan.h"

static void capy_plan_zero(void *ptr, size_t len) {
  uint8_t *p = (uint8_t *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

/* Length-bounded equality over component ids (never reads past the id field). */
static int capy_plan_streq(const char *a, const char *b) {
  uint32_t i;
  if (!a || !b) {
    return 0;
  }
  for (i = 0u; i < CAPY_COMPONENT_ID_MAX; ++i) {
    if (a[i] != b[i]) {
      return 0;
    }
    if (a[i] == '\0') {
      return 1;
    }
  }
  return 0;
}

static void capy_plan_copy(char *dst, size_t dst_size, const char *src) {
  size_t i = 0u;
  if (!dst || dst_size == 0u) {
    return;
  }
  if (!src) {
    dst[0] = '\0';
    return;
  }
  while (i + 1u < dst_size && src[i]) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = '\0';
}

static void capy_plan_set_error(struct capy_component_plan *plan,
                                enum capy_component_plan_status status,
                                const char *id, const char *reason) {
  if (!plan) {
    return;
  }
  plan->status = status;
  capy_plan_copy(plan->failing_component, sizeof(plan->failing_component), id);
  capy_plan_copy(plan->reason, sizeof(plan->reason), reason);
}

static int capy_component_plan_add_id(struct capy_component_plan *plan,
                                      const char *id) {
  if (!plan || !id || !id[0]) {
    return -1;
  }
  if (capy_component_plan_contains(plan, id)) {
    return 0;
  }
  if (plan->item_count >= CAPY_COMPONENT_PLAN_MAX_ITEMS) {
    return -1;
  }
  capy_plan_copy(plan->items[plan->item_count++].id,
                 sizeof(plan->items[0].id), id);
  return 0;
}

static int capy_component_plan_resolve_depth(
    const struct capy_component_index *index,
    const struct capy_component_host_abi *host, const char *id,
    struct capy_component_plan *plan, uint32_t depth) {
  const struct capy_component_descriptor *item;
  if (!index || !host || !id || !plan) {
    capy_plan_set_error(plan, CAPY_COMPONENT_PLAN_INVALID_INPUT, id,
                        "invalid input");
    return -1;
  }
  if (depth > CAPY_COMPONENT_INDEX_MAX_ITEMS) {
    capy_plan_set_error(plan, CAPY_COMPONENT_PLAN_DEPENDENCY_CYCLE, id,
                        "dependency cycle");
    return -1;
  }
  if (capy_component_plan_contains(plan, id)) {
    return 0;
  }
  item = capy_component_index_find(index, id);
  if (!item) {
    capy_plan_set_error(plan, CAPY_COMPONENT_PLAN_NOT_FOUND, id,
                        "component not found");
    return -1;
  }
  if (!capy_component_descriptor_compatible(item, host)) {
    capy_plan_set_error(plan, CAPY_COMPONENT_PLAN_INCOMPATIBLE, id,
                        "incompatible ABI");
    return -1;
  }
  for (uint32_t i = 0; i < item->dependency_count; ++i) {
    if (capy_component_plan_resolve_depth(index, host, item->dependencies[i],
                                          plan, depth + 1u) != 0) {
      return -1;
    }
  }
  if (capy_component_plan_add_id(plan, id) != 0) {
    capy_plan_set_error(plan, CAPY_COMPONENT_PLAN_TOO_LARGE, id,
                        "plan too large");
    return -1;
  }
  plan->status = CAPY_COMPONENT_PLAN_OK;
  return 0;
}

void capy_component_plan_init(struct capy_component_plan *plan) {
  if (!plan) {
    return;
  }
  capy_plan_zero(plan, sizeof(*plan));
  plan->status = CAPY_COMPONENT_PLAN_OK;
}

int capy_component_plan_contains(const struct capy_component_plan *plan,
                                 const char *id) {
  if (!plan || !id) {
    return 0;
  }
  for (uint32_t i = 0; i < plan->item_count; ++i) {
    if (capy_plan_streq(plan->items[i].id, id)) {
      return 1;
    }
  }
  return 0;
}

int capy_component_plan_add_selected(const struct capy_component_index *index,
                                     const struct capy_component_host_abi *host,
                                     const char *id,
                                     struct capy_component_plan *plan) {
  if (!plan) {
    return -1;
  }
  if (plan->status != CAPY_COMPONENT_PLAN_OK) {
    return -1;
  }
  return capy_component_plan_resolve_depth(index, host, id, plan, 0u);
}
