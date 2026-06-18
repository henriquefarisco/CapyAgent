# CapyAgent documentation

CapyAgent is the external home for package-format, resolver and agent-side policy cores that must remain decoupled from CapyOS internals.

## CapyOS reference version

Pinned for this release: `0.8.0-alpha.262+20260602`. Update this section together with `docs/compatibility.md` whenever the CapyOS core version, ABI or canonical manifest format changes.

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

## Delivered (host-side, pending external validation)

These land as portable, host-testable C with regression and known-answer
tests. They must pass `make validate` on an external machine/CI before being
trusted (this workspace is review/edit only and does not execute the suite).

- Semver-like version comparison (`src/update_core/release_manifest.c`).
- Line-oriented manifest serializer + canonical Ed25519 descriptor builder
  (`src/component_index/component_manifest.{h,c}`), compatible with the in-tree
  `services/capypkg` adapter format (see `docs/capypkg-publisher-guide.md`).
- Ed25519 signer/verifier for the canonical descriptor
  `name=N|version=V|payload_sha256=H|payload_url=U\n` plus SHA-512
  (`src/signer/`). Correctness is gated by RFC 8032 + FIPS 180-4 known-answer
  tests in `tests/test_signer.c`, including a canonical-descriptor KAT whose
  public key and signature come from an independent RFC 8032 oracle and are
  mirrored in `docs/compatibility.md` for the pending CapyOS verifier
  registration.
- `capyagent_ed25519_verifier`, whose signature matches the CapyOS
  `capypkg_verify_signature_fn` callback, ready to be registered via
  `capypkg_set_signature_verifier`.

## Pending work

1. Manifest v1 parser (reading the line-oriented `key=value` form back into
   descriptors; only emission exists today).
2. Component index JSON parser (high-level `capyos-component-index-v1`).
3. Conflict-aware resolver (the current planner detects cycles and missing
   dependencies but not version conflicts).
4. Declarative install/remove/rollback plan.
5. Trusted publisher key provisioning + rotation policy for the verifier.
6. CapyOS-side registration of the verifier and the install-plan applier when
   Etapa 9 opens (CapyOS owns this; until then `signed` repos fail closed).
