# Tag release component index

This alpha-stage flow intentionally uses GitHub release tags plus a compatibility index without certificate/signature enforcement.

## Trust model for this stage

CapyOS treats GitHub release tags as versioned distribution coordinates and uses sha256 to detect accidental or unexpected artifact changes. This is not a final release-security model.

Deferred until a later hardening slice:

- signed index;
- signed artifacts;
- release root key rotation;
- vendor trust store.

## Minimum index fields

Each component entry must declare:

- component id;
- human name;
- kind;
- channel;
- release tag, such as `v0.0.1`;
- release artifact name;
- sha256 of the artifact;
- required ABI names and minimum versions;
- dependencies;
- permissions;
- rollback/staging compatibility class.

## Update flow

1. CapyOS downloads a component index from a known GitHub release/tag URL.
2. CapyAgent parses the index into descriptors.
3. CapyAgent filters descriptors by ABI compatibility.
4. The installer/update UI offers basic, recommended or custom selections.
5. CapyAgent resolves dependencies and produces a dependency-ordered dry-run install/update plan.
6. CapyOS downloads artifacts from release-tag URLs.
7. CapyOS verifies sha256 before staging.
8. CapyOS activates staged components with rollback/fallback owned by the base system.

The runtime installation boundary is defined in
`CapyOS/docs/reference/integration/modular-installation-architecture.md`.

## Security limitation

Without signatures, this model is suitable for early alpha testing only. It does not protect against a compromised GitHub account or malicious release replacement with a matching updated index. The final release model must add signed indexes or signed artifacts before this becomes an official user-facing update trust chain.

## Current core files

- `src/component_index/component_index.h`
- `src/component_index/component_index.c`
- `src/component_index/component_plan.h`
- `src/component_index/component_plan.c`
- `src/update_core/release_manifest.h`
- `src/update_core/release_manifest.c`
