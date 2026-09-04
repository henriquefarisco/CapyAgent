#include "publish_resolver.h"

#include <string.h>

static int failures;

#define EXPECT(expr) do { if (!(expr)) { ++failures; return; } } while (0)

static void fill_sha(char out[CAPY_COMPONENT_SHA256_HEX_MAX]) {
  unsigned i;
  for (i = 0u; i < CAPY_COMPONENT_SHA256_HEX_LEN; ++i) out[i] = 'a';
  out[CAPY_COMPONENT_SHA256_HEX_LEN] = '\0';
}

static void candidate(struct capy_component_descriptor *d, const char *id,
                      const char *tag, uint32_t min, uint32_t max,
                      uint8_t known_good) {
  memset(d, 0, sizeof(*d));
  strcpy(d->id, id);
  strcpy(d->name, id);
  d->kind = CAPY_COMPONENT_KIND_APP;
  d->channel = CAPY_COMPONENT_CHANNEL_STABLE;
  strcpy(d->tag, tag);
  strcpy(d->artifact, "payload.capypkg");
  fill_sha(d->sha256);
  d->activation_class = CAPY_COMPONENT_ACTIVATION_ATOMIC;
  strcpy(d->required_abis[0].name, "capyos-base");
  d->required_abis[0].minimum_version = min;
  d->required_abi_count = 1u;
  strcpy(d->provides_abi, "capy-test-app");
  strcpy(d->abi_version, "1");
  d->core_abi_min = min;
  d->core_abi_max = max;
  d->known_good = known_good;
}

static struct capy_publish_request request(void) {
  struct capy_publish_request r;
  memset(&r, 0, sizeof(r));
  strcpy(r.abi_token, "capyos-base-v3");
  r.core_abi_version = 3u;
  r.channel = CAPY_COMPONENT_CHANNEL_STABLE;
  return r;
}

static void test_newest_compatible_known_good_wins(void) {
  struct capy_component_descriptor c[4];
  struct capy_publish_request r = request();
  struct capy_publish_result out;
  candidate(&c[0], "org.capyos.test", "v1.0.0", 1u, 3u, 1u);
  candidate(&c[1], "org.capyos.test", "v1.2.0", 1u, 3u, 1u);
  candidate(&c[2], "org.capyos.test", "v2.0.0", 4u, 5u, 1u);
  candidate(&c[3], "org.capyos.test", "v1.3.0", 1u, 3u, 0u);
  EXPECT(capy_publish_resolve_latest(c, 4u, &r, &out) == 0);
  EXPECT(out.status == CAPY_PUBLISH_RESOLVE_OK);
  EXPECT(out.index.item_count == 1u);
  EXPECT(strcmp(out.index.items[0].tag, "v1.2.0") == 0);
}

static void test_fail_closed_cases(void) {
  struct capy_component_descriptor c[2];
  struct capy_publish_request r = request();
  struct capy_publish_result out;
  candidate(&c[0], "org.capyos.test", "v1.0.0", 1u, 3u, 1u);
  c[1] = c[0];
  EXPECT(capy_publish_resolve_latest(c, 2u, &r, &out) != 0);
  EXPECT(out.status == CAPY_PUBLISH_RESOLVE_AMBIGUOUS_CANDIDATE);

  strcpy(r.abi_token, "capyos-base-v2");
  EXPECT(capy_publish_resolve_latest(c, 1u, &r, &out) != 0);
  EXPECT(out.status == CAPY_PUBLISH_RESOLVE_INVALID_TOKEN);

  r = request();
  c[0].known_good = 0u;
  EXPECT(capy_publish_resolve_latest(c, 1u, &r, &out) != 0);
  EXPECT(out.status == CAPY_PUBLISH_RESOLVE_NO_COMPATIBLE_CANDIDATE);
}

static void test_dependency_closure(void) {
  struct capy_component_descriptor c[1];
  struct capy_publish_request r = request();
  struct capy_publish_result out;
  candidate(&c[0], "org.capyos.app", "v1.0.0", 1u, 3u, 1u);
  strcpy(c[0].dependencies[0], "org.capyos.missing");
  c[0].dependency_count = 1u;
  EXPECT(capy_publish_resolve_latest(c, 1u, &r, &out) != 0);
  EXPECT(out.status == CAPY_PUBLISH_RESOLVE_DEPENDENCY_MISSING);
}

int run_publish_resolver_tests(void) {
  failures = 0;
  test_newest_compatible_known_good_wins();
  test_fail_closed_cases();
  test_dependency_closure();
  return failures;
}
