# CapyAgent compatibility and integration contract

CapyAgent owns the **package format, component-index, resolver and
future Ed25519 signer** that produce the artefacts consumed by the
CapyOS in-tree adapter `services/capypkg`. CapyAgent modules must
remain compatible with the CapyOS modular installation boundary.

## CapyOS reference version

- CapyOS core pinned for this contract: `0.8.0-alpha.261+20260529`
- Authoritative cross-repo matrix: [`CapyOS/docs/reference/integration/compatibility-matrix.md`](../../CapyOS/docs/reference/integration/compatibility-matrix.md)
- Canonical manifest format consumed by the in-tree `services/capypkg` adapter: [`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md)
- Manual deploy runbook: [`CapyOS/docs/operations/manual-module-deploy-runbook.md`](../../CapyOS/docs/operations/manual-module-deploy-runbook.md)
- Current cross-repo audit: [`CapyOS/docs/reference/integration/compatibility-audit-2026-05-23.md`](../../CapyOS/docs/reference/integration/compatibility-audit-2026-05-23.md)

## Authoritative CapyOS references

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/tag-release-component-index.md`
- `CapyOS/docs/reference/integration/package-format-integration-contract.md`
- `CapyOS/docs/reference/integration/external-core-repositories.md`
- `CapyOS/docs/architecture/capypkg-adapter.md`

## Owned ABI

CapyAgent owns the `capy-agent-component-index` ABI (v1).

This ABI covers:

- component descriptor fields (high-level JSON form);
- tag and sha256 validation rules;
- ABI requirement matching;
- dependency-ordered dry-run planning;
- activation class representation;
- mapping between the high-level JSON index and the line-oriented
  `key=value` manifest consumed by the in-tree adapter (documented in
  `capypkg-publisher-manifest-format.md §10`).

CapyAgent does **not** own:

- the `capyos-package-apply` ABI (belongs to CapyOS — real package
  application, staging, activation and rollback);
- the network transport (CapyOS `net/services/http`);
- the SHA-256 verification (CapyOS `security/sha256`);
- the Ed25519 verifier slot in the kernel binder (CapyOS owns the
  callable; CapyAgent owns the implementation behind it);
- the filesystem scope enforcement (CapyOS `services/capypkg`);
- the user-facing CLI (CapyOS `capysh` `pkg-*` commands);
- the first-boot wizard (CapyOS `src/config/first_boot/modules.c`).

## Compatibility rules

- Descriptor changes must be additive unless a future CapyOS
  integration stage explicitly permits a breaking migration.
- Unknown critical fields must be rejected by parsers once
  serialization exists.
- `activation_class` must be explicit; zero/unknown values are invalid.
- ABI requirements must use registered ABI names (`capy-*`), not
  repository names.
- The dependency plan must be deterministic for the same index,
  host ABI set and selected component.
- The Ed25519 signature must be computed over the canonical descriptor
  exactly as documented in
  `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md §5`:
  `name=N|version=V|payload_sha256=H|payload_url=U\n` (literal `|`
  separators, single `\n` terminator, no extra whitespace).

## Error model

| Code family | Trigger | Adapter behaviour |
|---|---|---|
| Descriptor field invalid (alphabet, hex length, non-printable byte) | parser rejects | `CAPYPKG_ERR_DENIED` or `CAPYPKG_ERR_PARSE`; install aborts and audit trail records WARN |
| Missing required field (`name`, `version`, `payload_url`, `payload_sha256`) | parser rejects | `CAPYPKG_ERR_PARSE` |
| `payload_size` overflows `uint32_t` or exceeds `CAPYPKG_PAYLOAD_MAX` | parser/install rejects | `CAPYPKG_ERR_PARSE` or `CAPYPKG_ERR_QUOTA` |
| `install_root` outside `/var/capypkg` or `/opt/`, or contains `..` | install rejects | `CAPYPKG_ERR_DENIED` |
| SHA-256 mismatch on downloaded payload | install rejects | `CAPYPKG_ERR_DIGEST` |
| Signature mandatory but absent | install rejects | `CAPYPKG_ERR_SIGNATURE` |
| Ed25519 signature verification fails | install rejects | `CAPYPKG_ERR_SIGNATURE` |
| Dependency missing in index | install rejects | `CAPYPKG_ERR_DENIED` |
| Dependency cycle | install rejects | `CAPYPKG_ERR_DENIED` |
| Repository quota exhausted | install rejects | `CAPYPKG_ERR_QUOTA` |

All adapter rejections emit a deterministic
`[audit] [capypkg] WARN ...` line in klog with a distinct sub-cause
(digest / signature / dependency / fetch / write / quota /
persistence). Publishers must avoid producing descriptors that hit
these errors without triage.

## Resource and performance limits

| Limit | Value | Owner |
|---|---|---|
| Payload size | ≤ 1 MiB (alpha static buffer); `CAPYPKG_PAYLOAD_MAX = 8 MiB` reached when CapyOS streaming writer lands | CapyOS adapter |
| `name` length | 1-63 chars | CapyAgent + CapyOS |
| `name` alphabet | `[a-zA-Z0-9._-]`, no dot-only names | CapyAgent + CapyOS |
| Dependencies per package | ≤ 8 | CapyOS adapter |
| Installed packages | ≤ 64 (`CAPYPKG_MAX_INSTALLED`) | CapyOS adapter |
| Available packages | ≤ 128 (`CAPYPKG_MAX_AVAILABLE`) | CapyOS adapter |
| Configured repositories | ≤ 4 (`CAPYPKG_MAX_REPOS`) | CapyOS adapter |
| Recursive dependency resolution depth | ≤ 8 | CapyOS adapter |
| Index size | bounded by `HTTP_MAX_URL = 2048` × entry count + `payload_size` of index file | CapyOS adapter |

## Install/update boundary

CapyAgent **may produce** a dry-run plan and a signed manifest.
CapyOS **performs**:

- HTTPS network fetch (`net/services/http`);
- SHA-256 calculation and binding;
- Ed25519 signature verification (via the slot `capypkg_set_signature_verifier`
  that CapyAgent registers);
- filesystem staging under `/var/capypkg/<name>/`;
- activation (placement of marker
  `/var/capypkg/<name>/installed` for the `kernel/module_gate`);
- rollback / removal;
- audit trail via klog;
- user prompts (CLI and first-boot wizard).

The CapyOS in-tree adapter `services/capypkg` is the active
receiving boundary today. CapyAgent must publish, alongside its
high-level JSON index, a line-oriented `key=value` manifest in the
exact format documented in
[`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md).

The CapyAgent verifier **must** be registered with
`capypkg_set_signature_verifier` before the first `pkg-install`
against a `signed` repo; until then the adapter fails closed with
`CAPYPKG_ERR_SIGNATURE`. The kernel binder in
`src/arch/x86_64/kernel_services.c::kernel_capypkg_bind_runtime_adapters`
intentionally leaves the verifier slot NULL until CapyAgent
publishes the Ed25519 signer.

## Required descriptor fields (high-level JSON index)

Every installable component descriptor in the high-level JSON
index must include:

- `id` (component identifier; maps to manifest `name`);
- `display_name`;
- `kind` (`agent`, `browser-core`, `codec`, `ui`, `lang-runtime`,
  `benchmark`, `app`);
- `channel` (`stable`, `testing`, `experimental`, `custom`);
- `release_tag` (maps to manifest `version`; without the `v` prefix);
- `artifact` (path under the release base URL; concatenated to form
  the manifest `payload_url`);
- `sha256` (lowercase 64 hex; maps to manifest `payload_sha256`);
- `activation_class`;
- `required_abis` (array of `{name, minimum_version}` entries);
- `dependencies` (maps to manifest `depends`, comma-separated);
- `permissions` (descriptive only; adapter ignores in alpha);
- `rollback`/`staging` compatibility class.

The mapping from the high-level JSON to the line-oriented manifest
is documented in
`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md §10`.

## Validation before CapyOS integration

Before CapyOS consumes a CapyAgent release, externally validate:

- valid and invalid descriptors;
- invalid tags and sha256 strings;
- incompatible ABI rejection;
- missing dependency rejection;
- dependency ordering;
- cycle detection;
- unknown/invalid activation class rejection;
- round-trip of high-level JSON descriptor to the line-oriented
  manifest format consumed by the in-tree adapter;
- Ed25519 signature over the canonical descriptor recognised by
  the adapter (`name=N|version=V|payload_sha256=H|payload_url=U\n`);
- HTTPS publishing of both index and payload with valid certificates
  against the CapyOS trust anchors
  (`CapyOS/src/security/tls_trust_anchors.c`).

CapyOS runtime integration is gated by Etapas 8-9. The CapyOS
`services/capypkg` adapter is already in place as an Etapa 9 alpha
receiver; integration is unblocked only when CapyAgent ships the
Ed25519 signer and registers its verifier through
`capypkg_set_signature_verifier`.

Until that point, repositories declared with `require_signature=1`
(default `stable`) will refuse every CapyAgent install with
`CAPYPKG_ERR_SIGNATURE`. Lab installs with `--unsigned` repositories
are possible but must never be promoted to user-facing release.

## Publishing a Capy package

When CapyAgent (or any other publisher) emits a remote module for
the CapyOS adapter, the publisher must follow
[`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md).
Workflow (CapyAgent perspective):

1. Build the artifact (`.bin`, opaque bytes).
2. Compute `sha256sum payload.bin | awk '{print $1}'`.
3. Compute `wc -c < payload.bin` for `payload_size`.
4. Compose canonical descriptor for signing:
   `name=N|version=V|payload_sha256=H|payload_url=U\n`.
5. Ed25519-sign with the publisher private key; convert signature
   to 128 hex lowercase.
6. Emit the line-oriented manifest with all required fields +
   `signature_ed25519` + applicable optional fields.
7. Publish `index.txt` (concatenated manifests separated by
   `---\n`) and the payload `.bin` via HTTPS with valid certificates.
8. Update [`CapyOS/docs/reference/integration/compatibility-matrix.md`](../../CapyOS/docs/reference/integration/compatibility-matrix.md)
   with the new version, ABI and channel pinned for the CapyOS core.

## Continuous delivery

`make validate` (this repo) runs strict C warnings, contract tests,
release metadata checks and hardened compile flag verification.

`make package` (added in `alpha.240` per CapyOS aggregator) emits
the assets consumed by the CapyOS first-boot wizard under
`build/capypkg/` when this repo evolves into a producer of CapyAgent
runtime artefacts (currently only descriptor/model logic is
host-testable; the Ed25519 signer remains pending).
