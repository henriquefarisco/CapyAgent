#ifndef CAPY_AGENT_RELEASE_MANIFEST_H
#define CAPY_AGENT_RELEASE_MANIFEST_H

#include <stddef.h>
#include <stdint.h>

#define CAPY_RELEASE_VERSION_MAX 40u
#define CAPY_RELEASE_CHANNEL_MAX 16u
#define CAPY_RELEASE_REPOSITORY_MAX 96u
#define CAPY_RELEASE_TAG_MAX 32u
#define CAPY_RELEASE_ARTIFACT_MAX 128u
#define CAPY_RELEASE_SHA256_HEX_LEN 64u
#define CAPY_RELEASE_SHA256_HEX_MAX (CAPY_RELEASE_SHA256_HEX_LEN + 1u)

enum capy_release_channel {
  CAPY_RELEASE_CHANNEL_STABLE = 0,
  CAPY_RELEASE_CHANNEL_TESTING,
  CAPY_RELEASE_CHANNEL_EXPERIMENTAL,
  CAPY_RELEASE_CHANNEL_CUSTOM
};

struct capy_release_manifest {
  char version[CAPY_RELEASE_VERSION_MAX];
  enum capy_release_channel channel;
  char repository[CAPY_RELEASE_REPOSITORY_MAX];
  char tag[CAPY_RELEASE_TAG_MAX];
  char artifact[CAPY_RELEASE_ARTIFACT_MAX];
  char sha256[CAPY_RELEASE_SHA256_HEX_MAX];
  uint32_t minimum_base_abi;
  uint32_t minimum_package_abi;
};

void capy_release_manifest_init(struct capy_release_manifest *manifest);
int capy_release_manifest_valid(const struct capy_release_manifest *manifest);
int capy_release_tag_valid(const char *tag);
int capy_release_sha256_valid(const char *sha256);
int capy_release_compare_versions(const char *candidate, const char *current,
                                  int *out_cmp);
int capy_release_manifest_compatible(const struct capy_release_manifest *manifest,
                                     uint32_t base_abi,
                                     uint32_t package_abi);

#endif
