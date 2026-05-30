# Security Policy

CapyAgent 0.0.3 is an early service release. Report security issues privately to the repository owner before opening public issues.

## Release gate

- `make validate` must pass before release tags.
- Component descriptors must declare tags, checksums, ABI requirements and activation class.
- Release manifests must use fixed `sha256` metadata and semantic version `0.0.3`.
