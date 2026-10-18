# Phase 4 experimental six-DOF execution (2026-10-18)

Local Windows evidence after [mass inference](2026-10-18-phase4-mass-inference.md).

## Implemented boundary

`physicsCore` adds `SixDofConstraintDescriptor`, `AxisMotion`, `AxisLimit` and
optional `SixDofConstraints` in `six_dof.h`. Jolt implements that capability
only with `PHYSICSJOLT_ENABLE_SIX_DOF=ON` (default OFF). Existing mandatory
world methods and fixed-constraint descriptors retain their meaning. The
installed config exposes `physicsJolt_SIX_DOF_AVAILABLE`.

Body-origin frames, one world endpoint, per-axis free/zero lock/finite limits,
X twist and pyramid Y/Z swing, collision exclusion and dependent cleanup are
covered. Bounds collapse or scalar overflow reject before creation. Dynamic
endpoints wake on removal; duplicate exclusions persist until the last
suppressing constraint disappears. Static/kinematic-only pairs reject.

`physicsUsd::toSixDofConstraint()` converts generic/fixed snapshots and
consumer-resolved handles without mutating a world. Disabled joints skip;
unsupported execution ranges report typed path diagnostics while snapshot
readers retain the wider declaration subset. Handles, rollback and scheduling
remain consumer-owned. There are no source-format includes or dependency
changes. Springs, drives and retained stage mappings remain outside this slice.

## SDK configuration gate

The previous local scalar SDK, built with non-deterministic `/fp:fast` settings,
fails the new independent Z translation test: under repeated 20 N forces its
displacement peaks near 0.0624 m rather than reaching the 0.4 m upper bound,
while reported velocity continues growing. X/Y controls pass. The declaration
settings read back the correct bounds. This is a failed SDK/configuration gate,
not accepted six-axis support. The exact cause has not been isolated.

A separate installation of the same pinned Jolt source was built with
`CROSS_PLATFORM_DETERMINISTIC=ON`, `INTERPROCEDURAL_OPTIMIZATION=OFF`,
`GENERATE_DEBUG_SYMBOLS=OFF`, `USE_STATIC_MSVC_RUNTIME_LIBRARY=OFF` and upstream
default x86 SIMD options. Its exported flags include deterministic mode,
AVX/AVX2, SSE4.1/SSE4.2, LZCNT/TZCNT and F16C. That candidate passes all axis
controls. Because several settings changed together, no single flag is claimed
as the fix. The original published/default package configuration is unchanged;
six-DOF execution stays explicitly opt-in pending SDK and hosted acceptance.

The backend maps frames/limits using the pinned
[Jolt six-DOF implementation](https://github.com/jrouwe/JoltPhysics/blob/v5.5.0/Jolt/Physics/Constraints/SixDOFConstraint.cpp).
Twist/pyramid swing is explicit; this is not evidence of source-format Euler
joint equivalence.

## Verification

Windows x86_64, MSVC 19.51, OpenUSD 26.08, Python 3.13.14:

- All 18 enabled root tests pass, including clean-prefix core/backend/USD
  consumers and ABI-mismatch rejection.
- With the feature left OFF and the previous SDK, all 17 root tests pass;
  the installed consumer confirms that the optional capability is absent.
- `physicsCore.six_dof` checks endpoint, frame, axis mode and limit validation.
- `physicsJolt.six_dof` checks world lock/release with offset COM, rotated
  asymmetric translation, each translation and angular axis under load, free
  motion, stale/cross-world handles, collision restoration, duplicate
  exclusions, body deletion and backend slot reuse. Transient loaded translation
  overshoot uses a 3 cm tolerance; the resting rotated slider uses 8 mm.
- `physicsUsd.joint_conversion` checks all axis mappings, world/body handles,
  disabled/fixed declarations, frame copying and typed rejection.
- A separate installed MMD consumer accepts 12 reader controls and all 28 open
  generated PMX fixtures. Each of 22 physics-bearing fixtures creates three
  bodies/two constraints and steps 120 times at 1/120 s with finite state.
  Six empty scenes stay empty. One format-only joint remains unexecuted.

The MMD run uses backend defaults for coefficients and category filtering;
kinematic bodies hold their rest poses. No trajectory, PMX mass equivalence,
spring or bone-feedback claim follows from finite stepping. Hosted checks
and public compatible package publication remain open.

## Reproduction

With `JOLT_SOURCE` pointing to the pinned source and `USD_ROOT` to the SDK:

```powershell
cmake -S "$env:JOLT_SOURCE/Build" -B build/jolt-six-dof-sdk `
  -G 'Visual Studio 18 2026' -A x64 `
  -DCROSS_PLATFORM_DETERMINISTIC=ON -DINTERPROCEDURAL_OPTIMIZATION=OFF `
  -DGENERATE_DEBUG_SYMBOLS=OFF -DUSE_STATIC_MSVC_RUNTIME_LIBRARY=OFF `
  -DTARGET_SAMPLES=OFF -DTARGET_UNIT_TESTS=OFF -DTARGET_HELLO_WORLD=OFF `
  -DTARGET_PERFORMANCE_TEST=OFF -DTARGET_VIEWER=OFF `
  "-DCMAKE_INSTALL_PREFIX=$PWD/build/jolt-six-dof-sdk-install"
cmake --build build/jolt-six-dof-sdk --config Release --target Jolt --parallel 4
cmake --install build/jolt-six-dof-sdk --config Release
cmake -S . -B build/phase4-usd -G 'Visual Studio 18 2026' -A x64 `
  -DUSDPHYSICS_BUILD_USD=ON -DUSDPHYSICS_BUILD_TESTS=ON `
  -DPHYSICSJOLT_REQUIRE_JOLT=ON -DPHYSICSJOLT_ENABLE_SIX_DOF=ON `
  "-DCMAKE_PREFIX_PATH=$env:USD_ROOT" "-Dpxr_DIR=$env:USD_ROOT" `
  "-DJolt_DIR=$PWD/build/jolt-six-dof-sdk-install/lib/cmake/Jolt"
cmake --build build/phase4-usd --config Release
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-six-dof-prefix
```

Candidate installed `Jolt.lib` SHA-256:
`f5ccf54211285d288a0912d01cf0f1c2dd68c22e2d8f10cc63861f55b35e8601`.
Source and public package pins are unchanged. Next gates are SDK cause/hosted
acceptance, coefficients, filtering, springs and runtime synchronization.
