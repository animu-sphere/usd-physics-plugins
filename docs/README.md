# usd-physics-plugins documentation

Each kind of information has one owner. Link to that owner instead of copying
status, tables, measurements or unfinished work into another document.

| Information | Canonical document |
| --- | --- |
| Purpose, first use and entry points | [Repository README](../README.md) |
| Design principles and responsibility boundaries | [DESIGN_POLICY.md](design/DESIGN_POLICY.md) |
| Neutral rigid-body semantics | [RIGID_BODY_CONTRACT.md](design/RIGID_BODY_CONTRACT.md) |
| USD reading, conversion and rejection rules | [USD_BRIDGE_CONTRACT.md](design/USD_BRIDGE_CONTRACT.md) |
| Secondary-motion admission and boundary | [SECONDARY_MOTION_CONTRACT.md](design/SECONDARY_MOTION_CONTRACT.md) |
| Component identities, dependency edges and build modes | [WORKSPACE.md](architecture/WORKSPACE.md) |
| External dependency pins and SDK configuration | [DEPENDENCIES.md](architecture/DEPENDENCIES.md) |
| Installation, public API and compatibility | [PACKAGE_CONTRACT.md](architecture/PACKAGE_CONTRACT.md) |
| Release format and promotion rules | [RELEASE_SCHEMA.md](architecture/RELEASE_SCHEMA.md) |
| Current capabilities and limits | [CAPABILITY_MATRIX.md](reference/CAPABILITY_MATRIX.md) |
| Extraction input | [EXTRACTION_BASELINE.md](reference/EXTRACTION_BASELINE.md) |
| Ordered unfinished work and acceptance criteria | [current.md](roadmap/current.md) |
| Dated verification facts, environments and logs | [reports/](reports/README.md) and linked CI runs |
| Document maintenance | [documentation.md](contributing/documentation.md) |

Code determines actual behavior. The capability matrix records that behavior
with evidence; design and architecture define contracts without claiming
implementation progress. Focused contracts own the details of their domain.

GitHub Issues and PRs own execution discussion. The roadmap keeps outcomes,
acceptance and tracking links, rather than duplicating task logs. Releases own
published artifact history. Dated reports remain append-only historical evidence.

There are no exceptions permitting a second current-status table or copied
progress paragraph. The [contribution guide](../CONTRIBUTING.md) and
[documentation guidelines](contributing/documentation.md) explain the checks.
