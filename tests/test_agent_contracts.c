#include "component_index.h"
#include "component_plan.h"
#include "package_model.h"
#include "release_manifest.h"

#include <string.h>

static int failures;

#define EXPECT(expr)      \
  do {                    \
    if (!(expr)) {        \
      ++failures;         \
      return;             \
    }                     \
  } while (0)

static void fill_sha(char out[CAPY_RELEASE_SHA256_HEX_MAX]) {
  for (unsigned i = 0; i < CAPY_RELEASE_SHA256_HEX_LEN; ++i) {
    out[i] = "0123456789abcdef"[i % 16u];
  }
  out[CAPY_RELEASE_SHA256_HEX_LEN] = '\0';
}

static void test_package_dependency_install(void) {
  struct capy_package_db db;
  struct capy_package_info runtime;
  struct capy_package_info app;
  struct capy_package_stats stats;
  capy_package_db_init(&db);
  memset(&runtime, 0, sizeof(runtime));
  memset(&app, 0, sizeof(app));
  strcpy(runtime.name, "capy-runtime");
  strcpy(runtime.version, "0.0.1");
  runtime.size_bytes = 10u;
  strcpy(app.name, "capy-agent");
  strcpy(app.version, "0.0.1");
  strcpy(app.deps[0], "capy-runtime");
  app.dep_count = 1u;
  app.size_bytes = 20u;
  EXPECT(capy_package_db_add_available(&db, &runtime) == 0);
  EXPECT(capy_package_db_add_available(&db, &app) == 0);
  EXPECT(capy_package_install(&db, "capy-agent") == 0);
  EXPECT(capy_package_check_deps(&db, "capy-agent") == 0);
  capy_package_stats_get(&db, &stats);
  EXPECT(stats.installed_count == 2u);
  EXPECT(stats.total_installed_bytes == 30u);
}

static void test_release_manifest(void) {
  struct capy_release_manifest manifest;
  int cmp = 0;
  capy_release_manifest_init(&manifest);
  strcpy(manifest.version, "0.0.1");
  manifest.channel = CAPY_RELEASE_CHANNEL_STABLE;
  strcpy(manifest.repository, "henriquefarisco/CapyAgent");
  strcpy(manifest.tag, "v0.0.1");
  strcpy(manifest.artifact, "capyagent-0.0.1.tar.gz");
  fill_sha(manifest.sha256);
  manifest.minimum_base_abi = 1u;
  EXPECT(capy_release_manifest_valid(&manifest) == 1);
  EXPECT(capy_release_compare_versions("0.0.1", "0.0.0", &cmp) == 0);
  EXPECT(cmp > 0);
  EXPECT(capy_release_manifest_compatible(&manifest, 1u, 0u) == 1);
}

static void test_component_plan(void) {
  struct capy_component_index index;
  struct capy_component_descriptor runtime;
  struct capy_component_descriptor app;
  struct capy_component_host_abi host;
  struct capy_component_plan plan;
  capy_component_index_init(&index);
  memset(&runtime, 0, sizeof(runtime));
  memset(&app, 0, sizeof(app));
  memset(&host, 0, sizeof(host));
  strcpy(runtime.id, "capy-runtime");
  strcpy(runtime.name, "Capy Runtime");
  runtime.kind = CAPY_COMPONENT_KIND_AGENT;
  runtime.channel = CAPY_COMPONENT_CHANNEL_STABLE;
  strcpy(runtime.tag, "v0.0.1");
  strcpy(runtime.artifact, "runtime.capypkg");
  fill_sha(runtime.sha256);
  runtime.activation_class = CAPY_COMPONENT_ACTIVATION_ATOMIC;
  strcpy(runtime.required_abis[0].name, "capyos-base");
  runtime.required_abis[0].minimum_version = 1u;
  runtime.required_abi_count = 1u;
  app = runtime;
  strcpy(app.id, "capy-agent");
  strcpy(app.name, "Capy Agent");
  strcpy(app.artifact, "agent.capypkg");
  strcpy(app.dependencies[0], "capy-runtime");
  app.dependency_count = 1u;
  strcpy(host.available[0].name, "capyos-base");
  host.available[0].minimum_version = 1u;
  host.available_count = 1u;
  EXPECT(capy_component_index_add(&index, &runtime) == 0);
  EXPECT(capy_component_index_add(&index, &app) == 0);
  capy_component_plan_init(&plan);
  EXPECT(capy_component_plan_add_selected(&index, &host, "capy-agent", &plan) == 0);
  EXPECT(plan.status == CAPY_COMPONENT_PLAN_OK);
  EXPECT(plan.item_count == 2u);
  EXPECT(capy_component_plan_contains(&plan, "capy-runtime") == 1);
  EXPECT(capy_component_plan_contains(&plan, "capy-agent") == 1);
}

int main(void) {
  test_package_dependency_install();
  test_release_manifest();
  test_component_plan();
  return failures == 0 ? 0 : 1;
}
