# CapyAgent documentation

CapyAgent is the external home for package-format, resolver and agent-side policy cores that must remain decoupled from CapyOS internals.

## CapyOS reference version

Pinned for this release: `0.8.0-alpha.244+20260520`. Update this section together with `docs/compatibility.md` whenever the CapyOS core version, ABI or canonical manifest format changes.

## Migrated content

- `src/package_format/package_model.h`
- `src/package_format/package_model.c`
- `src/component_index/component_index.h`
- `src/component_index/component_index.c`
- `src/component_index/component_plan.h`
- `src/component_index/component_plan.c`
- `src/update_core/release_manifest.h`
- `src/update_core/release_manifest.c`
- `docs/package-format-migration.md`
- `docs/tag-release-index.md`
- `docs/component-index-example.md`
- `docs/compatibility.md`

## Source origin

The initial package model was extracted from:

- `CapyOS/include/services/package_manager.h`
- `CapyOS/src/services/package_manager.c`

## Ownership

CapyAgent owns:

- package metadata model;
- dependency graph behavior;
- host-testable resolver logic;
- future manifest parsing;
- tag-release component index compatibility filtering;
- dependency-ordered component install/update planning;
- future declarative install/remove/rollback plan.

CapyOS owns:

- actual filesystem mutation;
- update/release signing policy;
- HTTP/TLS downloads;
- boot-slot transactions;
- privilege, sandbox and package application.

## Integration contract

- `CapyOS/docs/reference/integration/package-format-integration-contract.md`
- `CapyOS/docs/reference/integration/tag-release-component-index.md`
- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/compatibility-matrix.md`
- `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
- `CapyOS/docs/architecture/capypkg-adapter.md`
- `CapyOS/docs/operations/manual-module-deploy-runbook.md`

## Pending work

1. Manifest v1 parser.
2. Component index parser/serializer.
3. Version comparison.
4. Conflict-aware resolver.
5. Declarative rollback plan.
6. CapyOS adapter when Etapa 9 is active.
7. Line-oriented manifest serializer compatible with the in-tree `services/capypkg` adapter (see `docs/capypkg-publisher-guide.md` and `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`).
8. Ed25519 signer for the canonical descriptor `name=N|version=V|payload_sha256=H|payload_url=U\n`.
9. Verifier adapter that registers via `capypkg_set_signature_verifier` at CapyOS boot.
