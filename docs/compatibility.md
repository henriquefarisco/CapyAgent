# CapyAgent compatibility and integration contract

CapyAgent owns the **package format, component-index, publish-time resolver and
Ed25519 signer** that produce the artefacts consumed by the
CapyOS in-tree adapter `services/capypkg`. CapyAgent modules must
remain compatible with the CapyOS modular installation boundary.

## CapyOS reference version

- CapyOS core pinned for this contract: `0.10.0-alpha.1+20260903`
- Authoritative cross-repo matrix: [`CapyOS/docs/reference/integration/compatibility-matrix.md`](../../CapyOS/docs/reference/integration/compatibility-matrix.md)
- Canonical manifest format consumed by the in-tree `services/capypkg` adapter: [`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md)
- Manual deploy runbook: [`CapyOS/docs/operations/manual-module-deploy-runbook.md`](../../CapyOS/docs/operations/manual-module-deploy-runbook.md)
- Current cross-repo audit: [`CapyOS/docs/reference/integration/compatibility-audit-2026-06-02.md`](../../CapyOS/docs/reference/integration/compatibility-audit-2026-06-02.md)

## Authoritative CapyOS references

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/tag-release-component-index.md`
- `CapyOS/docs/reference/integration/package-format-integration-contract.md`
- `CapyOS/docs/reference/integration/external-core-repositories.md`
- `CapyOS/docs/architecture/capypkg-adapter.md`

## Owned ABI

CapyAgent owns the `capy-agent-component-index` ABI (v2). Version 2 is an
additive tail extension over v1 and adds `provides_abi`, `abi_version`,
`core_abi_min`, `core_abi_max` and `known_good`.

This ABI covers:

- component descriptor fields (high-level JSON form);
- tag and sha256 validation rules;
- ABI requirement matching;
- dependency-ordered dry-run planning;
- activation class representation;
- mapping between the high-level JSON index and the line-oriented
  `key=value` manifest consumed by the in-tree adapter (documented in
  `capypkg-publisher-manifest-format.md §10`).
- deterministic publish-time selection of the newest SemVer candidate that is
  known-good and compatible with the requested CapyOS core ABI token.

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
| Payload size | `CAPYPKG_PAYLOAD_MAX = 8 MiB`; runtime allocation is sized from the authenticated manifest | CapyOS adapter |
| `name` length | 1-63 chars | CapyAgent + CapyOS |
| `name` alphabet | `[a-zA-Z0-9._-]`, no dot-only names | CapyAgent + CapyOS |
| Dependencies per package | ≤ 8, each a valid `name`, **no duplicate names, no self-dependency** (rejected fail-closed by both `capy_component_descriptor_valid` and `capy_manifest_emit`) | CapyAgent + CapyOS adapter |
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

The CapyOS kernel binder registers the production Ed25519 verifier before the
first official fetch. The official `capyos-modules-index-v2` authenticates the
resolved payload URL, digest and ABI metadata as one signed envelope; custom
signed repositories retain per-package canonical-descriptor signatures.

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

CapyOS runtime integration is active in Etapa 9. Official installs remain
fail-closed on a bad index signature, token, epoch, body hash, ABI range or
known-good policy. Lab-only unsigned repositories must never be promoted to a
user-facing release.

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
`build/capypkg/`.

## Ed25519 signer status

The signer is now implemented host-side and decoupled:

- `src/component_index/component_manifest.{h,c}` — line-oriented manifest
  serializer and the canonical descriptor builder
  (`name=N|version=V|payload_sha256=H|payload_url=U\n`).
- `src/signer/sha512.{h,c}` — FIPS 180-4 SHA-512.
- `src/signer/ed25519.{h,c}` — RFC 8032 Ed25519 sign/verify (faithful
  reproduction of the public-domain TweetNaCl reference).
- `src/signer/capyagent_signer.{h,c}` — hex codec, descriptor signing/
  verification and `capyagent_ed25519_verifier`, whose prototype matches the
  CapyOS `capypkg_verify_signature_fn` callback.

Correctness is gated by RFC 8032 + FIPS 180-4 known-answer tests in
`tests/test_signer.c`; these **must** pass under `make validate` on an
external machine/CI before the signer is trusted (this workspace is
review/edit only). Two gates remain before signed installs work end-to-end:
(1) external KAT validation, and (2) CapyOS registering the verifier through
`capypkg_set_signature_verifier` when Etapa 9 opens. Until both are done,
`signed` repositories still fail closed with `CAPYPKG_ERR_SIGNATURE`, and the
compatibility-matrix row stays "signer pending registration".

## Canonical descriptor known-answer vector

This frozen vector pins the canonical descriptor byte layout and a genuine
Ed25519 signature over it, anchored to the curve constants. It is enforced by
`tests/test_signer.c::test_canonical_descriptor_kat` and was produced by an
independent RFC 8032 oracle (OpenSSL via Python `cryptography`), not by the
in-repo signer, so it catches drift in keypair derivation, in the canonical
field order / separators / `\n` terminator, and in the signing path — including
a symmetric change a sign↔verify roundtrip cannot see.

The CapyOS-side registration of `capyagent_ed25519_verifier` through
`capypkg_set_signature_verifier` (workspace P0) can reuse this exact triple
(public key + canonical bytes + signature) to confirm the verifier slot is
wired correctly before promoting any `signed` repository.

| Field | Value |
|---|---|
| Seed (32 bytes) | `000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f` |
| Public key (32 bytes) | `03a107bff3ce10be1d70dd18e74bc09967e4d6309ba50d5f1ddc8664125531b8` |
| `name` | `org.capyos.agent.core` |
| `version` | `1.2.3` |
| `payload_sha256` | `9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08` (= `sha256("test")`) |
| `payload_url` | `https://github.com/henriquefarisco/CapyAgent/releases/download/v1.2.3/org.capyos.agent.core-1.2.3.bin` |

Canonical descriptor signed (exactly 235 bytes, single trailing `\n`):

```
name=org.capyos.agent.core|version=1.2.3|payload_sha256=9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08|payload_url=https://github.com/henriquefarisco/CapyAgent/releases/download/v1.2.3/org.capyos.agent.core-1.2.3.bin
```

Ed25519 signature (64 bytes, 128 lowercase hex):

```
9788539478ef8b7d0a64339047a98f9a5833f7b069ef29d9a17cd25f8a6642806a80afe64708c4eece3d6d80eb3ebb415bedde868f5de01d9f8ae30a199c3d0a
```

The seed, version and URL above are a fixed reference vector and are
intentionally independent of the live `VERSION`; do not "refresh" them, or the
signature will no longer match the bytes.
