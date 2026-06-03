# Security Policy

CapyAgent 0.0.7 is an early service release. Report security issues privately to the repository owner before opening public issues.

## Release gate

- `make validate` must pass before release tags.
- Component descriptors must declare tags, checksums, ABI requirements and activation class.
- Release manifests must use fixed `sha256` metadata and the semantic version recorded in `VERSION`.
- The Ed25519 signer (`src/signer/`) must pass its RFC 8032 + FIPS 180-4 known-answer tests under `make validate` before any signed release; the publisher private key must never be committed.
