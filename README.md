# CapyAgent

Version: 0.0.2

CapyAgent owns the external package, component-index and release-manifest logic used by CapyOS services.

## Validation

```sh
make validate
```

The release gate compiles with strict C warnings, runs contract tests, checks release metadata and verifies hardened compile flags.
