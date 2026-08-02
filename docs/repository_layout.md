# Repository layout

The repository separates development source, immutable release snapshots,
documentation, tooling, and generated output.

## Tracked areas

- `firmware/Nexstar_Protocol_Converter/` — development sketch used by the
  standard build script.
- `firmware/Nexstar_Protocol_Converter_vX.YY/` — versioned release snapshots.
  Do not edit an already-published snapshot in place.
- `docs/` — architecture, protocol, build, testing, and maintenance guidance.
- `tools/` — source-generation and static-analysis utilities.
- `scripts/` — reproducible test, build, and verification entry points.
- `simulator/` — host-side protocol simulation and development dependencies.
- `hardware/` — wiring and board documentation.
- `dist/` and `.build/` — generated output; never commit their contents.

## Local-only material

Local agent state, Python caches, historical working copies, CAD exports, and
build artifacts are ignored. They may remain on a developer workstation, but
they are not part of the firmware source of record.

Credentials and private settings must never be added to a release snapshot.
The existing HTTPS certificate behavior is preserved; certificate changes are
out of scope for routine cleanup.

## Release policy

1. Develop on a feature or refactor branch.
2. Compile and test the development sketch and the candidate release snapshot.
3. Record hardware validation results in the release notes or changelog.
4. Commit the complete release snapshot with its matching versioned sketch name.
5. Merge stable releases to `main` only after review and validation.
