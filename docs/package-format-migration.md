# Package format migration

This repository now owns the package-format and resolver core that should evolve outside CapyOS.

## Source origin

Initial model extracted from the coupled CapyOS package manager:

- `CapyOS/include/services/package_manager.h`
- `CapyOS/src/services/package_manager.c`

The extracted core keeps only deterministic package database and dependency operations. It intentionally does not include CapyOS VFS, HTTP, Ed25519, update-agent, boot slot or filesystem apply logic.

## Current extracted files

- `src/package_format/package_model.h`
- `src/package_format/package_model.c`

## Ownership boundary

CapyAgent owns:

- package identity and version fields;
- package dependency graph model;
- installed/available database model;
- resolver behavior that can be tested on a host;
- declarative install/remove/update plan in future slices.

CapyOS keeps:

- real filesystem writes;
- signature verification;
- release/update policy;
- boot-slot rollback;
- download transport;
- package application under system permissions.

## Integration rule

CapyOS must consume this repository through the contract in:

- `CapyOS/docs/reference/integration/package-format-integration-contract.md`

The current in-tree CapyOS package manager remains a legacy compatibility/adaptation point until Etapa 9 is active and the external package format has host-side tests.

## Next migration slices

1. Add manifest v1 parser with golden fixtures.
2. Add semver-like version comparison.
3. Add dependency resolver with conflict reporting.
4. Add declarative install/remove/rollback plan.
5. Add CapyOS adapter only when Etapa 9 is active.
