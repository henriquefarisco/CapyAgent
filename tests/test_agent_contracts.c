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

static void make_valid_descriptor(struct capy_component_descriptor *d) {
  memset(d, 0, sizeof(*d));
  strcpy(d->id, "org.capyos.agent.core");
  strcpy(d->name, "CapyAgent Core");
  d->kind = CAPY_COMPONENT_KIND_AGENT;
  d->channel = CAPY_COMPONENT_CHANNEL_STABLE;
  strcpy(d->tag, "v0.0.1");
  strcpy(d->artifact, "agent.capypkg");
  fill_sha(d->sha256);
  d->activation_class = CAPY_COMPONENT_ACTIVATION_ATOMIC;
  strcpy(d->required_abis[0].name, "capyos-base");
  d->required_abis[0].minimum_version = 1u;
  d->required_abi_count = 1u;
}

static void test_descriptor_validation_negatives(void) {
  struct capy_component_descriptor d;
  make_valid_descriptor(&d);
  EXPECT(capy_component_descriptor_valid(&d) == 1);
  EXPECT(capy_component_descriptor_valid(0) == 0);

  make_valid_descriptor(&d);
  d.id[0] = '\0';
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.name[0] = '\0';
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.artifact[0] = '\0';
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.kind = CAPY_COMPONENT_KIND_UNKNOWN;
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.activation_class = CAPY_COMPONENT_ACTIVATION_UNKNOWN;
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  strcpy(d.tag, "0.0.1"); /* missing leading v */
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.sha256[0] = 'A'; /* uppercase not allowed */
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.required_abi_count = CAPY_COMPONENT_MAX_ABIS + 1u;
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.required_abis[0].name[0] = '\0'; /* abi without name */
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.required_abis[0].minimum_version = 0u; /* abi min version zero */
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.dependency_count = CAPY_COMPONENT_MAX_DEPENDENCIES + 1u;
  EXPECT(capy_component_descriptor_valid(&d) == 0);

  make_valid_descriptor(&d);
  d.permission_count = CAPY_COMPONENT_MAX_PERMISSIONS + 1u;
  EXPECT(capy_component_descriptor_valid(&d) == 0);
}

static void test_tag_and_sha_validation(void) {
  char sha[CAPY_COMPONENT_SHA256_HEX_MAX];
  EXPECT(capy_component_tag_valid("v0.0.1") == 1);
  EXPECT(capy_component_tag_valid("v1.2.3-alpha.4+build") == 1);
  EXPECT(capy_component_tag_valid(0) == 0);
  EXPECT(capy_component_tag_valid("0.0.1") == 0);  /* no leading v */
  EXPECT(capy_component_tag_valid("v") == 0);      /* no digit */
  EXPECT(capy_component_tag_valid("vx.y.z") == 0); /* no digit after v */
  EXPECT(capy_component_tag_valid("v1.2") == 0);   /* needs >= 2 dots */
  EXPECT(capy_component_tag_valid("v1.2.3 ") == 0); /* space rejected */

  fill_sha(sha);
  EXPECT(capy_component_sha256_valid(sha) == 1);
  EXPECT(capy_component_sha256_valid(0) == 0);
  sha[0] = 'A';
  EXPECT(capy_component_sha256_valid(sha) == 0); /* uppercase */
  fill_sha(sha);
  sha[10] = 'g';
  EXPECT(capy_component_sha256_valid(sha) == 0); /* non-hex */
  fill_sha(sha);
  sha[63] = '\0';
  EXPECT(capy_component_sha256_valid(sha) == 0); /* too short */
}

static void test_plan_error_paths(void) {
  struct capy_component_index index;
  struct capy_component_descriptor runtime;
  struct capy_component_descriptor app;
  struct capy_component_host_abi host;
  struct capy_component_plan plan;

  /* Missing dependency: app depends on a runtime that is not in the index. */
  capy_component_index_init(&index);
  memset(&host, 0, sizeof(host));
  strcpy(host.available[0].name, "capyos-base");
  host.available[0].minimum_version = 1u;
  host.available_count = 1u;
  make_valid_descriptor(&app);
  strcpy(app.id, "capy-agent");
  strcpy(app.dependencies[0], "capy-runtime");
  app.dependency_count = 1u;
  EXPECT(capy_component_index_add(&index, &app) == 0);
  capy_component_plan_init(&plan);
  EXPECT(capy_component_plan_add_selected(&index, &host, "capy-agent", &plan) != 0);
  EXPECT(plan.status == CAPY_COMPONENT_PLAN_NOT_FOUND);

  /* Selecting an id absent from the index. */
  capy_component_plan_init(&plan);
  EXPECT(capy_component_plan_add_selected(&index, &host, "absent", &plan) != 0);
  EXPECT(plan.status == CAPY_COMPONENT_PLAN_NOT_FOUND);

  /* Incompatible ABI: host version lower than required. */
  capy_component_index_init(&index);
  make_valid_descriptor(&runtime);
  strcpy(runtime.id, "capy-runtime");
  runtime.required_abis[0].minimum_version = 3u; /* requires capyos-base v3 */
  runtime.required_abi_count = 1u;
  EXPECT(capy_component_index_add(&index, &runtime) == 0);
  capy_component_plan_init(&plan);
  EXPECT(capy_component_plan_add_selected(&index, &host, "capy-runtime", &plan) != 0);
  EXPECT(plan.status == CAPY_COMPONENT_PLAN_INCOMPATIBLE);
}

static void test_release_version_ordering(void) {
  int cmp = 0;
  EXPECT(capy_release_compare_versions("0.0.1", "0.0.0", &cmp) == 0 && cmp > 0);
  EXPECT(capy_release_compare_versions("1.0.0", "0.9.9", &cmp) == 0 && cmp > 0);
  EXPECT(capy_release_compare_versions("0.1.0", "0.0.9", &cmp) == 0 && cmp > 0);
  EXPECT(capy_release_compare_versions("1.2.3", "1.2.3", &cmp) == 0 && cmp == 0);
  EXPECT(capy_release_compare_versions("1.0.0-alpha", "1.0.0-beta", &cmp) == 0 &&
         cmp < 0);
  EXPECT(capy_release_compare_versions("1.0.0-rc", "1.0.0", &cmp) == 0 && cmp < 0);
  EXPECT(capy_release_compare_versions("1.0.0-alpha.1", "1.0.0-alpha.2", &cmp) ==
             0 &&
         cmp < 0);
  /* Invalid version strings fail closed. */
  EXPECT(capy_release_compare_versions("x", "1.0.0", &cmp) != 0);
  EXPECT(capy_release_compare_versions("1.0", "1.0.0", &cmp) != 0);

  EXPECT(capy_release_tag_valid("v1.2.3") == 1);
  EXPECT(capy_release_tag_valid("1.2.3") == 0);
  EXPECT(capy_release_sha256_valid(0) == 0);
}

static void test_package_error_paths(void) {
  struct capy_package_db db;
  struct capy_package_info app;
  struct capy_package_info out;
  capy_package_db_init(&db);
  memset(&app, 0, sizeof(app));
  strcpy(app.name, "capy-agent");
  strcpy(app.version, "0.0.1");
  strcpy(app.deps[0], "capy-runtime");
  app.dep_count = 1u;
  EXPECT(capy_package_db_add_available(&db, &app) == 0);
  /* Install fails because the dependency is not available. */
  EXPECT(capy_package_install(&db, "capy-agent") != 0);
  /* check_deps fails while the dependency is not installed. */
  EXPECT(capy_package_check_deps(&db, "capy-agent") != 0);
  /* Removing / querying an unknown package fails closed. */
  EXPECT(capy_package_remove(&db, "absent") != 0);
  EXPECT(capy_package_info_get(&db, "absent", &out) != 0);
  EXPECT(capy_package_install(&db, "absent") != 0);
}

int run_manifest_tests(void);
int run_signer_tests(void);

int main(void) {
  test_package_dependency_install();
  test_release_manifest();
  test_component_plan();
  test_descriptor_validation_negatives();
  test_tag_and_sha_validation();
  test_plan_error_paths();
  test_release_version_ordering();
  test_package_error_paths();
  failures += run_manifest_tests();
  failures += run_signer_tests();
  return failures == 0 ? 0 : 1;
}
