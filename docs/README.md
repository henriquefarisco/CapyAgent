# CapyAgent documentation

CapyAgent is the external home for package-format, resolver and agent-side policy cores that must remain decoupled from CapyOS internals.

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

## Pending work

1. Manifest v1 parser.
2. Component index parser/serializer.
3. Version comparison.
4. Conflict-aware resolver.
5. Declarative rollback plan.
6. CapyOS adapter when Etapa 9 is active.
