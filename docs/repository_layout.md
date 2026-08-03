# Repository layout

The repository contains one canonical release source, documentation, tooling,
and generated output.

## Tracked areas

- `firmware/NexStar5-8-Bridge/` — canonical v1.0.0 release source.
- `docs/` — architecture, protocol, build, testing, and maintenance guidance.
- `tools/` — source-generation and static-analysis utilities.
- `scripts/` — reproducible test, build, and verification entry points.
- `simulator/` — host-side protocol simulation and development dependencies.
- `hardware/` — wiring and board documentation.
- `dist/` and `.build/` — generated output; never commit their contents.

## Local-only material

Local agent state, Python caches, historical working copies, CAD exports, and
build artifacts are ignored. They are not part of the firmware source of record.

Credentials and private settings must never be added to the release source.
The existing HTTPS certificate behavior is preserved; certificate changes are
out of scope for routine cleanup.

## Canonical source

The validated release source is committed under
`firmware/NexStar5-8-Bridge` and is used by the standard build and browser
checks. HTTPS certificate and private key material are supplied locally and
are not committed to the public repository.

## Release policy

1. Develop on a feature or refactor branch.
2. Compile and test the candidate source.
3. Record hardware validation results in the release notes or changelog.
4. Commit the complete release source and matching release documentation.
5. Merge stable releases to `main` only after review and validation.
