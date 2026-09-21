# External dependencies

This page is the single source of truth for dependency versions and support
claims.

> **Status (2026-09-22): the dependency-free core is validated with plain
> CMake, OpenStrata, and the hosted Windows/Linux matrix.** The hosted evidence
> is recorded in the [Phase 0 CI report](../reports/2026-09-22-phase0-hosted-ci.md).

## 1. Baseline

| Dependency | Selected version | Used by | Current status |
| --- | --- | --- | --- |
| CMake | 3.25 minimum | workspace and every component | verified with CMake 4.2.3 on Linux and 4.4.3 on Windows |
| C++ | C++17 | all C++ targets | verified with GCC 15.2.0 and MSVC 19.51 |
| OpenStrata | 0.23.2 | workspace composition and CI generation | Windows root build/test, isolated library tests, consumer verification, and packaging pass |
| OpenStrata platform | `cy2026`, `usd` profile | workspace composition | digest-pinned Windows runtime materialized and validated; scaffold code does not link OpenUSD |
| OpenUSD | 26.08 exact for the first ecosystem integration | future `physicsUsd`, `physicsSchema`, USD tests | aligned and CI runtime pinned; no target currently links it |
| Jolt Physics | 5.5.0, commit `23dadd0e603f1b321142d4c74df07fce85064989` | future `physicsJolt` implementation only | selected from the extraction source; not linked by the Phase 0 scaffold |
| CTest | version shipped with CMake | tests | five root tests pass on Linux and through OpenStrata on Windows |

## 2. OpenUSD

Only `physicsUsd`, an admitted `physicsSchema`, and USD integration tests link
OpenUSD. `physicsCore`, `physicsJolt`, and `secondaryMotion` do not.

All composed sibling packages must use the same ABI-compatible OpenUSD build.
Mixing independently built OpenUSD distributions in one process is unsupported.

The bridge prefers modern CMake package targets exposed by the chosen OpenUSD
distribution. Exact target names are recorded after Phase 0 because packaging
varies between distributions.

## 3. Jolt Physics

Jolt is private to `physicsJolt`. Its headers and compile definitions do not
appear in the `physicsCore` install interface.

Before the backend changes to supported, Phase 2 records:

- source or binary provenance and license;
- exact version or commit;
- CMake target name;
- compile options that affect ABI or determinism;
- allocator and job-system integration policy;
- Windows and Linux evidence where claimed.

An optional no-Jolt stub may help a workspace configure, but it does not count
as backend support and cannot make backend tests pass vacuously.

## 4. OpenStrata

OpenStrata composes libraries and runtime dependencies; it is not required by
plain-CMake consumers of installed packages. Manifests and CMake package
metadata must describe the same dependency graph.

The first profile target is `cy2026` with the `usd` profile because that is the
current Stage Runner extraction environment. OpenStrata 0.23.2 and the runtime
artifact pins are shared with the validated sibling workspaces.

## 5. Platform policy

Windows and Linux are intended. Neither is claimed supported until CI or a
dated report builds and tests the relevant components. Compiler families and
minimum versions will be added to the candidate table when verified.

macOS, mobile, and WebAssembly are not initial targets.

## 6. Dependency update policy

A dependency change updates this page, lock or composition metadata, CI, and
package tests in one change. A major solver or OpenUSD update requires the
rigid-body and USD integration suites, not only a successful compile.

Public core behavior must not vary solely because a newer backend exposes an
extra feature. New features enter through an explicit capability and tests.
