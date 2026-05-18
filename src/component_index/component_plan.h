#ifndef CAPY_AGENT_COMPONENT_PLAN_H
#define CAPY_AGENT_COMPONENT_PLAN_H

#include "component_index.h"

#define CAPY_COMPONENT_PLAN_MAX_ITEMS 128u
#define CAPY_COMPONENT_PLAN_REASON_MAX 96u

enum capy_component_plan_status {
  CAPY_COMPONENT_PLAN_OK = 0,
  CAPY_COMPONENT_PLAN_INVALID_INPUT,
  CAPY_COMPONENT_PLAN_NOT_FOUND,
  CAPY_COMPONENT_PLAN_INCOMPATIBLE,
  CAPY_COMPONENT_PLAN_TOO_LARGE,
  CAPY_COMPONENT_PLAN_DEPENDENCY_CYCLE
};

struct capy_component_plan_item {
  char id[CAPY_COMPONENT_ID_MAX];
};

struct capy_component_plan {
  struct capy_component_plan_item items[CAPY_COMPONENT_PLAN_MAX_ITEMS];
  uint32_t item_count;
  enum capy_component_plan_status status;
  char failing_component[CAPY_COMPONENT_ID_MAX];
  char reason[CAPY_COMPONENT_PLAN_REASON_MAX];
};

void capy_component_plan_init(struct capy_component_plan *plan);
int capy_component_plan_contains(const struct capy_component_plan *plan,
                                 const char *id);
int capy_component_plan_add_selected(const struct capy_component_index *index,
                                     const struct capy_component_host_abi *host,
                                     const char *id,
                                     struct capy_component_plan *plan);

#endif
