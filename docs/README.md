# usd-physics-plugins documentation

Documentation is organized by responsibility. The taxonomy follows
`usd-mmd-plugins`: design says what is intended, architecture fixes structure,
reference reports what exists, and the roadmap contains incomplete work.

**Current tree (2026-09-22): Phase 1 core extraction.** The root build,
packages, boundary checks, and installed-consumer test exist. Neutral rigid
transform values, typed handles, validated descriptors, and the single-owner
world contract are implemented; optional queries, a solver backend, an OpenUSD
bridge, and a release are not. The
[capability matrix](reference/CAPABILITY_MATRIX.md) is the only page that may
claim implementation status.

| Category | Answers | Start here |
| --- | --- | --- |
| [architecture/](architecture/) | How the workspace is structured: identities, dependency directions, external dependencies, and installed packages. | [WORKSPACE.md](architecture/WORKSPACE.md) · [DEPENDENCIES.md](architecture/DEPENDENCIES.md) · [PACKAGE_CONTRACT.md](architecture/PACKAGE_CONTRACT.md) |
| [design/](design/) | What the runtime contracts mean and why the boundaries exist. | [DESIGN_POLICY.md](design/DESIGN_POLICY.md) · [RIGID_BODY_CONTRACT.md](design/RIGID_BODY_CONTRACT.md) · [USD_BRIDGE_CONTRACT.md](design/USD_BRIDGE_CONTRACT.md) · [SECONDARY_MOTION_CONTRACT.md](design/SECONDARY_MOTION_CONTRACT.md) |
| [reference/](reference/) | Facts about the current tree, extraction input, and implemented capabilities. | [CAPABILITY_MATRIX.md](reference/CAPABILITY_MATRIX.md) · [EXTRACTION_BASELINE.md](reference/EXTRACTION_BASELINE.md) |
| [roadmap/](roadmap/) | What is planned next, what is incomplete, and which phase owns it. | [README.md](roadmap/README.md) · [current.md](roadmap/current.md) |
| [reports/](reports/) | Dated evidence from real verification runs. | [README.md](reports/README.md) |
| [contributing/](contributing/) | How to maintain these documents. | [documentation.md](contributing/documentation.md) |

`guides/` and `releases/` remain absent until verified user workflows or an
actual release exist. `reports/` begins with the Phase 0 scaffold verification.

## Canonical documents

- [design/DESIGN_POLICY.md](design/DESIGN_POLICY.md) is the canonical long-form
  design policy. It owns the repository purpose, central rule, boundaries,
  schema policy, testing policy, and phase definitions.
- Focused contracts own the detail of one area and win over the general design
  policy in that area:
  - [design/RIGID_BODY_CONTRACT.md](design/RIGID_BODY_CONTRACT.md) owns the
    backend-neutral handles, descriptors, lifecycle, stepping, state, and
    optional query capabilities;
  - [design/USD_BRIDGE_CONTRACT.md](design/USD_BRIDGE_CONTRACT.md) owns
    `UsdPhysics` interpretation, transform conversion, resource identity, and
    writeback boundaries;
  - [design/SECONDARY_MOTION_CONTRACT.md](design/SECONDARY_MOTION_CONTRACT.md)
    owns the separation between rigid-body simulation and generic spring-chain
    dynamics.
- [architecture/WORKSPACE.md](architecture/WORKSPACE.md) is the proposed
  binding workspace contract. Once accepted, a structural change updates that
  document first.

## Source-of-truth rules

- Code is authoritative for implemented behavior; `architecture/` and
  `reference/` record it and change with it.
- `design/` defines intended contracts and labels unimplemented behavior.
- `roadmap/` contains incomplete work only. Completed work is removed from the
  active roadmap and reflected in `reference/`.
- Exact dependency versions belong only in
  [architecture/DEPENDENCIES.md](architecture/DEPENDENCIES.md).
- Package names and consumer guarantees belong only in
  [architecture/PACKAGE_CONTRACT.md](architecture/PACKAGE_CONTRACT.md).
- The full maintenance rules are in
  [contributing/documentation.md](contributing/documentation.md).
