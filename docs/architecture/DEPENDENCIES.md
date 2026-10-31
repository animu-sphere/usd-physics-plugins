# External dependencies

This page is the single source of truth for dependency versions and SDK
configuration. Tested capabilities and verification evidence belong only in
the [capability matrix](../reference/CAPABILITY_MATRIX.md).

## 1. Baseline

| Dependency | Selected version | Used by |
| --- | --- | --- |
| CMake | 3.25 minimum | Workspace and every component |
| C++ | C++17 | All C++ targets |
| OpenStrata | 0.23.2 | Workspace composition and CI generation |
| OpenStrata platform | `cy2026`, `usd` profile | Workspace composition |
| OpenUSD | 26.08 exact for first ecosystem integration | `physicsUsd` and USD tests; admitted `physicsSchema` |
| Jolt Physics | 5.5.0, tag `v5.5.0`, commit `23dadd0e603f1b321142d4c74df07fce85064989` | Private `physicsJolt` implementation |
| CTest | Version shipped with CMake | Tests |

## 2. OpenUSD

Only `physicsUsd`, an admitted `physicsSchema`, and USD integration tests link
OpenUSD. `physicsCore`, `physicsJolt`, and `secondaryMotion` do not.

All composed sibling packages must use the same ABI-compatible OpenUSD build.
Mixing independently built OpenUSD distributions in one process is unsupported.

The bridge uses OpenUSD CMake package targets; accepted target spellings and
transitive package dependencies belong in [PACKAGE_CONTRACT.md](PACKAGE_CONTRACT.md).

## 3. Jolt Physics

Jolt is private to `physicsJolt`. Its headers and compile definitions do not
appear in the `physicsCore` install interface.

The selected source is
[`jrouwe/JoltPhysics`](https://github.com/jrouwe/JoltPhysics) tag `v5.5.0` at
commit `23dadd0e603f1b321142d4c74df07fce85064989`. It is MIT licensed and exports
the installed CMake target `Jolt::Jolt`. `physicsJolt` also accepts the
unnamespaced `Jolt` target for source distributions that expose upstream's
alternate target spelling; installed-package verification uses `Jolt::Jolt`.

The verified Windows artifact is a Release, single-precision build with
`DOUBLE_PRECISION=OFF`, `CROSS_PLATFORM_DETERMINISTIC=OFF`, and
`INTERPROCEDURAL_OPTIMIZATION=OFF`. Its imported target exports no optional x86
instruction-set definitions; compatible consumer builds disable SSE4.1, SSE4.2,
AVX, AVX2, AVX512, LZCNT, TZCNT, F16C, and FMADD. A Jolt artifact with different
ABI-affecting definitions must not be mixed into the same process.

Released `physicsJolt` packages use this non-deterministic set, with the debug
renderer and profiler on for Windows and off for Linux, matching the Jolt build
in Stage Runner's source CI. Linux keeps Jolt's default x86 instruction sets.
A consumer builds Jolt with the same options; the
[release workflow](../../.github/workflows/release.yml) names them.

The installed `physicsJolt` package rechecks Jolt 5.5.0 and the exact exported
ABI compile-definition set before importing its static target. A downstream
configure fails rather than binding the package to a different Jolt build.

`physicsJolt` uses `RegisterDefaultAllocator`, one reference-counted
process-global Jolt factory/type-registration lifetime, and a 10 MiB temporary
allocator per world. Each world owns a `JobSystemThreadPool` with Jolt's
maximum job/barrier counts and `hardware_concurrency - 1` workers, falling
back to zero worker threads when only one hardware thread is reported. The
initial world capacity is 1,024 bodies, 1,024 body pairs, and 1,024 contact
constraints. Exhausted creation capacity is reported as
`PhysicsErrorCode::resourceExhausted`; update failures are reported as
`PhysicsErrorCode::solverFailure`.

An optional no-Jolt stub may help a workspace configure, but it does not count
as backend support and cannot make backend tests pass vacuously.

Experimental six-DOF execution requires `CROSS_PLATFORM_DETERMINISTIC=ON`.
MSVC non-deterministic Release Jolt uses `/fp:fast`, which is unsuitable for
this execution gate regardless of x86 SIMD options. The deterministic SDK's
exported definitions add `JPH_CROSS_PLATFORM_DETERMINISTIC` and drop
`JPH_USE_FMADD`; it is a different ABI set from the non-deterministic artifact.
It must not silently replace that artifact or its consumer pins. Keep
`PHYSICSJOLT_ENABLE_SIX_DOF` OFF unless the selected SDK passes the enabled
six-axis tests. The `usd-jolt-six-dof` cells select this SDK, test it on both
OSes and do not package experimental binaries.

## 4. OpenStrata

OpenStrata composes libraries and runtime dependencies; it is not required by
plain-CMake consumers of installed packages. Manifests and CMake package
metadata must describe the same dependency graph.

The first profile target is `cy2026` with the `usd` profile because that is the
current Stage Runner extraction environment. OpenStrata 0.23.2 and the runtime
artifact pins are shared with the validated sibling workspaces.

## 5. Platform policy

Windows and Linux are target platforms. Support claims require CI or dated
verification of the relevant components and belong in the capability matrix.
Record compiler/environment details in the evidence, separately from minimum
dependency requirements here.

macOS, mobile, and WebAssembly are not initial targets.

## 6. Dependency update policy

A dependency change updates this page, lock or composition metadata, CI, and
package tests in one change. A major solver or OpenUSD update requires the
rigid-body and USD integration suites, not only a successful compile.

Public core behavior must not vary solely because a newer backend exposes an
extra feature. New features enter through an explicit capability and tests.
