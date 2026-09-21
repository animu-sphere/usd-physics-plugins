# 2026-09-22 Phase 0 hosted CI verification

## Environment

- Workflow: [`ost source ci`](https://github.com/animu-sphere/usd-physics-plugins/actions/runs/35589514659)
- Event: pull request for `codex/phase-0-workspace`
- Result: success

## Executed evidence

The generated hosted workflow completed all three Phase 0 cells:

- `workspace-graph-pr` on Ubuntu;
- `workspace-pr-linux` on Ubuntu 24.04; and
- `workspace-pr-windows` on Windows Server 2022.

The graph cell completed in 7 seconds, the Linux test cell in 43 seconds, and
the Windows test cell in 58 seconds. Together with the local plain-CMake and
OpenStrata reports, this closes the Phase 0 hosted-delivery evidence gate.

## Limits

This run verifies the package scaffolds, workspace graph, boundary checks, and
installed-consumer test. It does not claim rigid-body runtime behavior or a
Jolt solver implementation.
