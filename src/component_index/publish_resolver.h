#ifndef CAPY_AGENT_PUBLISH_RESOLVER_H
#define CAPY_AGENT_PUBLISH_RESOLVER_H

#include "component_index.h"

#define CAPY_PUBLISH_ABI_TOKEN_MAX 48u

enum capy_publish_resolve_status {
  CAPY_PUBLISH_RESOLVE_OK = 0,
  CAPY_PUBLISH_RESOLVE_INVALID_INPUT,
  CAPY_PUBLISH_RESOLVE_INVALID_TOKEN,
  CAPY_PUBLISH_RESOLVE_NO_COMPATIBLE_CANDIDATE,
  CAPY_PUBLISH_RESOLVE_AMBIGUOUS_CANDIDATE,
  CAPY_PUBLISH_RESOLVE_DEPENDENCY_MISSING,
  CAPY_PUBLISH_RESOLVE_PLAN_INVALID
};

struct capy_publish_request {
  char abi_token[CAPY_PUBLISH_ABI_TOKEN_MAX];
  uint32_t core_abi_version;
  enum capy_component_channel channel;
};

struct capy_publish_result {
  struct capy_component_index index;
  enum capy_publish_resolve_status status;
  char failing_component[CAPY_COMPONENT_ID_MAX];
};

/* Resolve one deterministic, signed-publish candidate per component id.
 * Candidates must be stable descriptor records with publish ABI metadata,
 * known_good=1, and a core ABI range containing request->core_abi_version.
 * The numerically newest SemVer tag wins. Equal id+version candidates are
 * rejected as ambiguous instead of depending on input order. */
int capy_publish_resolve_latest(
    const struct capy_component_descriptor *candidates,
    uint32_t candidate_count,
    const struct capy_publish_request *request,
    struct capy_publish_result *out);

#endif

