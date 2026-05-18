# CapyAgent compatibility and integration contract

CapyAgent modules must remain compatible with the CapyOS modular installation boundary.

Authoritative CapyOS references:

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/tag-release-component-index.md`
- `CapyOS/docs/reference/integration/package-format-integration-contract.md`

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
- unknown/invalid activation class rejection.

CapyOS runtime integration is gated by Etapas 8-9.
