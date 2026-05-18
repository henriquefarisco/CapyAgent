#ifndef CAPY_AGENT_COMPONENT_INDEX_H
#define CAPY_AGENT_COMPONENT_INDEX_H

#include <stddef.h>
#include <stdint.h>

#define CAPY_COMPONENT_ID_MAX 80u
#define CAPY_COMPONENT_NAME_MAX 80u
#define CAPY_COMPONENT_TAG_MAX 32u
#define CAPY_COMPONENT_ARTIFACT_MAX 128u
#define CAPY_COMPONENT_SHA256_HEX_LEN 64u
#define CAPY_COMPONENT_SHA256_HEX_MAX (CAPY_COMPONENT_SHA256_HEX_LEN + 1u)
#define CAPY_COMPONENT_MAX_DEPENDENCIES 8u
#define CAPY_COMPONENT_MAX_PERMISSIONS 8u
#define CAPY_COMPONENT_ABI_NAME_MAX 32u
#define CAPY_COMPONENT_MAX_ABIS 8u
#define CAPY_COMPONENT_INDEX_MAX_ITEMS 128u

enum capy_component_kind {
  CAPY_COMPONENT_KIND_APP = 0,
  CAPY_COMPONENT_KIND_AGENT,
  CAPY_COMPONENT_KIND_BROWSER_CORE,
  CAPY_COMPONENT_KIND_CODEC,
  CAPY_COMPONENT_KIND_UI,
  CAPY_COMPONENT_KIND_LANG_RUNTIME,
  CAPY_COMPONENT_KIND_BENCHMARK,
  CAPY_COMPONENT_KIND_DRIVER,
  CAPY_COMPONENT_KIND_UNKNOWN
};

enum capy_component_channel {
  CAPY_COMPONENT_CHANNEL_STABLE = 0,
  CAPY_COMPONENT_CHANNEL_TESTING,
  CAPY_COMPONENT_CHANNEL_EXPERIMENTAL,
  CAPY_COMPONENT_CHANNEL_CUSTOM
};

enum capy_component_activation_class {
  CAPY_COMPONENT_ACTIVATION_UNKNOWN = 0,
  CAPY_COMPONENT_ACTIVATION_ATOMIC,
  CAPY_COMPONENT_ACTIVATION_REQUIRES_REBOOT,
  CAPY_COMPONENT_ACTIVATION_MANUAL
};

struct capy_component_abi_requirement {
  char name[CAPY_COMPONENT_ABI_NAME_MAX];
  uint32_t minimum_version;
};

struct capy_component_descriptor {
  char id[CAPY_COMPONENT_ID_MAX];
  char name[CAPY_COMPONENT_NAME_MAX];
  enum capy_component_kind kind;
  enum capy_component_channel channel;
  char tag[CAPY_COMPONENT_TAG_MAX];
  char artifact[CAPY_COMPONENT_ARTIFACT_MAX];
  char sha256[CAPY_COMPONENT_SHA256_HEX_MAX];
  enum capy_component_activation_class activation_class;
  struct capy_component_abi_requirement required_abis[CAPY_COMPONENT_MAX_ABIS];
  uint32_t required_abi_count;
  char dependencies[CAPY_COMPONENT_MAX_DEPENDENCIES][CAPY_COMPONENT_ID_MAX];
  uint32_t dependency_count;
  char permissions[CAPY_COMPONENT_MAX_PERMISSIONS][CAPY_COMPONENT_ID_MAX];
  uint32_t permission_count;
};

struct capy_component_index {
  struct capy_component_descriptor items[CAPY_COMPONENT_INDEX_MAX_ITEMS];
  uint32_t item_count;
};

struct capy_component_host_abi {
  struct capy_component_abi_requirement available[CAPY_COMPONENT_MAX_ABIS];
  uint32_t available_count;
};

void capy_component_index_init(struct capy_component_index *index);
int capy_component_index_add(struct capy_component_index *index,
                             const struct capy_component_descriptor *item);
const struct capy_component_descriptor *capy_component_index_find(
    const struct capy_component_index *index, const char *id);
int capy_component_descriptor_valid(
    const struct capy_component_descriptor *item);
int capy_component_descriptor_compatible(
    const struct capy_component_descriptor *item,
    const struct capy_component_host_abi *host);
int capy_component_tag_valid(const char *tag);
int capy_component_sha256_valid(const char *sha256);

#endif
