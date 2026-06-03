# CapyAgent publisher guide for the CapyOS capypkg adapter

**Status:** authoritative since 2026-05-19.
**CapyOS reference version:** `0.8.0-alpha.262+20260602`.
**Adapter contract source of truth:** `CapyOS/include/services/capypkg.h`.
**Manifest format source of truth:** `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`.

CapyAgent publishes two distinct artifacts:

1. **High-level component index (JSON).** Discovery and UX surface;
   matches `docs/component-index-example.md`. Free to evolve while
   CapyAgent's ABI is additive.
2. **Line-oriented manifest (`key=value`) for the in-tree adapter.**
   Strict, fail-closed; consumed by `services/capypkg` in the CapyOS
   core. This is the artifact the runtime actually reads.

This document defines how to derive the second from the first.

## 1. Two-format model

| Format | Owner | Consumer | Schema |
|---|---|---|---|
| High-level JSON component index | CapyAgent | UX/discovery layer (future Software Center, CLI listings) | `docs/component-index-example.md` |
| Line-oriented `key=value` manifest | CapyAgent publisher | CapyOS `services/capypkg` adapter | `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md` |

Both must be published from the same release. The line-oriented
manifest is the authoritative install record; the JSON index is the
catalogue for humans and UIs.

## 2. Field mapping from JSON to manifest

| JSON field | Manifest key | Notes |
|---|---|---|
| `id` | `name` | use org-style ids; restricted alphabet `[a-zA-Z0-9._-]`, no dot-only names |
| `tag` (`vX.Y.Z`) | `version` (`X.Y.Z`) | strip the leading `v` for the manifest; keep `v...` on the GitHub release tag |
| (base URL) + `artifact` | `payload_url` | absolute HTTPS URL only |
| `sha256` | `payload_sha256` | lowercase 64 hex |
| n/a | `payload_size` | byte length of the published payload |
| (computed) | `signature_ed25519` | Ed25519 hex 128 over the canonical descriptor (see §4) |
| (computed) | `install_root` | absolute path; default `/var/capypkg/<name>`; otherwise must live under `/var/capypkg` or `/opt/`; no `..` segments |
| `dependencies` | `depends` | comma-separated, ≤ 8 entries, same alphabet as `name` |
| `permissions` | (none) | not consumed by the alpha adapter; keep in JSON for UX |
| `required_abis` | (none) | not consumed by the alpha adapter; keep in JSON for UX |
| (channel) | n/a | adapter does not gate on channel; channel lives in the JSON index |
| (kind, activation_class) | n/a | adapter ignores; keep in JSON for UX |

## 3. Workflow

Each repository ships a `make package` target that produces a
deterministic payload tarball and the line-oriented manifest entry
in `build/capypkg/`:

```text
make package
# Produces in build/capypkg/:
#   <name>-<version>.bin       (tar of src + docs + VERSION)
#   <name>.manifest            (line-oriented key=value entry)
```

For repositories using cargo (CapyLang), the output directory is
`target/capypkg/` instead of `build/capypkg/`. The CapyOS aggregator
detects both paths.

Full publisher workflow:

```text
1. make package                  # build .bin + .manifest
2. (sha256 + size already in     # the manifest target computes them
    the generated manifest)
3. compose canonical descriptor  # see §4
4. ed25519_sign(secret, desc)    # capture 128 hex (lowercase)
5. inject signature_ed25519=     # add the line to the manifest
6. (optional) mirror in JSON     # component-index.json for UX
7. publish manifest + payload    # HTTPS only
8. CapyOS:  make modules-index   # aggregate into modules-index.txt
9. update compatibility-matrix   # CapyOS-side
```

Override the `payload_url` baked into the manifest by setting
`PUBLISH_URL_BASE` when invoking `make package`:

```bash
make package PUBLISH_URL_BASE=https://my.host.tld/capypkg
```

The default points to the suggested GitHub Release URL
(`https://github.com/<owner>/<repo>/releases/download/v<version>/`).

## 4. Canonical descriptor signed by Ed25519

```text
name=<N>|version=<V>|payload_sha256=<H>|payload_url=<U>\n
```

- literal `|` separators;
- fixed order: `name`, `version`, `payload_sha256`, `payload_url`;
- single `\n` (LF) terminator;
- values copied verbatim from the manifest.

CapyAgent must register the verifier exactly once before the first
`pkg-install` runs:

```c
capypkg_set_signature_verifier(capyagent_ed25519_verifier);
```

The verifier signature is:

```c
int (*capypkg_verify_signature_fn)(const char *signed_text,
                                   size_t signed_len,
                                   const char *signature_hex);
```

Return `0` for valid, non-zero for invalid. Until this is registered,
all installs from `signed` repositories (default `stable`) fail with
`CAPYPKG_ERR_SIGNATURE`.

## 5. Manifest entry example

```text
name=org.capyos.codecs.image-basic
version=0.0.1
summary=CapyCodecs Image Basic
payload_url=https://updates.capyos.org/capycodecs-image-basic-v0.0.1.bin
payload_sha256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
payload_size=524288
signature_ed25519=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
install_root=/var/capypkg/org.capyos.codecs.image-basic
depends=
---
```

Multiple entries are concatenated with `---\n` separators.

## 6. Publisher constraints

- only printable ASCII 0x20-0x7E in values;
- HTTPS only on `payload_url` and `index_url`;
- payload size ≤ 1 MiB until streaming writer ships (`CAPYPKG_PAYLOAD_MAX` is 8 MiB but the alpha buffer is 1 MiB);
- `name` alphabet `[a-zA-Z0-9._-]`, ≤ 63 chars, no dot-only;
- `install_root` under `/var/capypkg` or `/opt/`, no `..`;
- `depends` ≤ 8, same alphabet as `name`;
- chain of dependencies must be fully present in the index.

## 7. Validation before publishing

- run the future `capyagent-manifest-emit` golden test (planned);
- diff the JSON index against the line-oriented manifest;
- recompute `payload_sha256` from the bytes you intend to publish;
- sign the canonical descriptor and verify with your test verifier;
- confirm HTTPS endpoints with valid certificates;
- update `docs/compatibility.md` and the CapyOS `compatibility-matrix.md`
  with the new version and ABI.

## 8. CapyOS-side test gates

The CapyOS team runs (on their machine, not this one):

```bash
make test-capypkg                       # parser, signature gate, audit log
make smoke-x64-vmware-pkg-install       # when Etapa 9 opens
```

## 9. References

- `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
- `CapyOS/docs/reference/integration/compatibility-matrix.md`
- `CapyOS/docs/reference/integration/package-format-integration-contract.md`
- `CapyOS/docs/architecture/capypkg-adapter.md`
- `CapyOS/docs/operations/manual-module-deploy-runbook.md`
- `CapyOS/include/services/capypkg.h`
- `docs/component-index-example.md`
- `docs/compatibility.md`
- `docs/tag-release-index.md`
