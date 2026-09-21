# External dependencies

This page is the single source of truth for dependency versions and support
claims. A candidate is not a supported dependency until Phase 0 has configured,
built, and tested it in both build modes.

> **Status (2026-09-21): no dependency is validated in this repository.** The
> versions below are alignment candidates derived from the extraction source
> and sibling workspaces, not implementation claims.

## 1. Candidate baseline

| Dependency | Candidate | Used by | Current status |
| --- | --- | --- | --- |
| CMake | 3.25 or newer | workspace and every component | candidate from `usd-stage-runner`; not validated here |
| C++ | C++17 | all C++ targets | candidate from extracted targets; not validated here |
| OpenStrata platform | `cy2026`, `usd` profile | workspace composition | candidate from `usd-stage-runner`; not validated here |
| OpenUSD | 26.08 exact for the first ecosystem integration | `physicsUsd`, `physicsSchema`, USD tests | candidate aligned with `usd-mmd-plugins`; not validated here |
| Jolt Physics | exact version supplied by the selected OpenStrata/runtime composition | `physicsJolt` only | unresolved until Phase 0 records the package provenance and version |
| CTest | version shipped with CMake | tests | not validated here |

Phase 0 replaces each candidate with a verified version or range and links the
CI or dated report that proves it.

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

Before the version is accepted, Phase 0 records:

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
current Stage Runner extraction environment. The accepted version is recorded
after Phase 0 verifies it alongside the selected OpenUSD and Jolt artifacts.

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
