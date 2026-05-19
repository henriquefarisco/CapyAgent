# CapyAgent compatibility and integration contract

CapyAgent modules must remain compatible with the CapyOS modular installation boundary.

## CapyOS reference version

- CapyOS core pinned for this contract: `0.8.0-alpha.240+20260519`
- Authoritative cross-repo matrix: `CapyOS/docs/reference/integration/compatibility-matrix.md`
- Canonical manifest format consumed by the in-tree `services/capypkg` adapter: `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
- Manual deploy runbook: `CapyOS/docs/operations/manual-module-deploy-runbook.md`

## Authoritative CapyOS references

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/tag-release-component-index.md`
- `CapyOS/docs/reference/integration/package-format-integration-contract.md`
- `CapyOS/docs/reference/integration/compatibility-matrix.md`
- `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
- `CapyOS/docs/architecture/capypkg-adapter.md`

## Owned ABI

CapyAgent owns the `capy-agent-component-index` ABI.

This ABI covers:

- component descriptor fields;
- tag and sha256 validation rules;
- ABI requirement matching;
- dependency-ordered dry-run planning;
- activation class representation.

CapyAgent does not own the `capyos-package-apply` ABI. Real package application belongs to CapyOS.

## Compatibility rules

- Descriptor changes must be additive unless a future CapyOS integration stage explicitly permits a breaking migration.
- Unknown critical fields must be rejected by parsers once serialization exists.
- `activation_class` must be explicit; zero/unknown values are invalid.
- ABI requirements must use registered ABI names, not repository names.
- The dependency plan must be deterministic for the same index, host ABI set and selected component.

## Install/update boundary

CapyAgent may produce a dry-run plan. CapyOS must perform:

- network fetch;
- sha256 calculation;
- signature or trust policy;
- staging;
- activation;
- rollback;
- user prompts.

The CapyOS in-tree adapter `services/capypkg` is the active receiving
boundary today. CapyAgent must publish, alongside its high-level JSON
index, a line-oriented `key=value` manifest in the exact format
documented in `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`.
The Ed25519 signature must cover exactly the canonical descriptor
`name=N|version=V|payload_sha256=H|payload_url=U\n` (literal `|`
separators, single `\n` terminator). The CapyAgent verifier must be
registered with `capypkg_set_signature_verifier` before the first
`pkg-install` against a `signed` repo; otherwise the adapter fails
closed with `CAPYPKG_ERR_SIGNATURE`.

## Required descriptor fields

Every installable component descriptor must include:

- component id;
- display name;
- kind;
- channel;
- release tag;
- artifact name or URL;
- lowercase sha256;
- activation class;
- required ABI names and minimum versions;
- dependencies;
- permissions;
- rollback/staging compatibility class.

## Validation before CapyOS integration

Before CapyOS consumes a CapyAgent release, externally validate:

- valid and invalid descriptors;
- invalid tags and sha256 strings;
- incompatible ABI rejection;
- missing dependency rejection;
- dependency ordering;
- cycle detection;
- unknown/invalid activation class rejection;
- round-trip of high-level JSON descriptor to the line-oriented manifest format consumed by the in-tree adapter;
- Ed25519 signature over the canonical descriptor recognised by the adapter;
- HTTPS publishing of both index and payload with valid certificates against the CapyOS trust anchors.

CapyOS runtime integration is gated by Etapas 8-9. The CapyOS
`services/capypkg` adapter is already in place as an Etapa 9 alpha
receiver; integration is unblocked only when CapyAgent ships the
Ed25519 signer and registers its verifier through
`capypkg_set_signature_verifier`.

Until that point, repositories declared with `require_signature=1`
(default `stable`) will refuse every CapyAgent install with
`CAPYPKG_ERR_SIGNATURE`. Lab installs with `--unsigned` repositories
are possible but must never be promoted to user-facing release.
