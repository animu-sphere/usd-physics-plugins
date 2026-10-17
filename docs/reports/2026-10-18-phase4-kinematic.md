# Phase 4 kinematic motion and snapshots (2026-10-18)

Dated local Windows evidence following the
[primitive reader increment](2026-10-18-phase4-primitives.md).

## Change under test

Local changes over `81905cf` add `MotionType::kinematicBody` and optional
`KinematicMotion::moveKinematic(body, target, duration)` in
`usd_physics/core/kinematic_motion.h`. The world interface keeps its existing
mandatory operations. Targets state world-space body origins and rotations;
the Jolt implementation uses velocities, preserving collider-local geometry
and the distinct center of mass. Targets require finite translation, finite
nonzero rotation and positive finite duration. Backend scalar-range failures
reject before mutation. Non-kinematic, unknown, stale and cross-world targets
return `false`.

Call before each matching step; velocities persist until replaced. Reissuing
the current pose holds the body. Commands do not teleport or enqueue changes;
stepping collects kinematic states with existing queue semantics. Force and
linear-velocity commands remain dynamic-only. No scheduler or synchronization
mapping is added.

Both reader entry points admit time-independent standard kinematic flags.
Enabled kinematic bodies need no explicit mass; snapshot mass is ignored.
Disabled rigid bodies remain static. The reader still owns no live world.
Packages and consumers were rebuilt together; no published artifact pin changes.

## Verification

Windows x86_64, MSVC 19.51, Visual Studio 18 2026, OpenUSD 26.08,
Python 3.13.14 and Jolt 5.5.0. Using the existing configured USD/Jolt build:

```powershell
cmake -S . -B build/phase4-usd -DUSDPHYSICS_BUILD_USD=ON
cmake --build build/phase4-usd --config Release
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-kinematic-prefix
```

All 13 tests pass, including the new `physicsJolt.kinematic` test. It checks
idle gravity resistance, target translation/rotation with displaced geometry,
normalized rotation, updated queries, holding, dynamic contact on a moving
platform, validation without mutation, handle/type rejection and queue drain
and destruction cleanup. Core tests validate kinematic mass and target values.
USD fixtures check mass-free Cube/Sphere/Capsule snapshots, disabled-body
precedence, read-only behavior and rejection of an animated flag.
Clean-prefix consumers exercise the installed capability, actual movement,
and standard kinematic snapshot declarations.

The OpenUSD/Jolt-free configuration also passes all 10 tests, including its
clean-prefix unavailable-backend consumer:

```powershell
cmake -S . -B build/phase2-no-jolt -DCMAKE_DISABLE_FIND_PACKAGE_Jolt=ON -DUSDPHYSICS_BUILD_USD=OFF
cmake --build build/phase2-no-jolt --config Release
ctest --test-dir build/phase2-no-jolt -C Release --output-on-failure
```

Installed `physicsUsd.lib` SHA-256:
`dcf264db958377b89eb42c07ce9170843eef174cc71fb2305f4090620a75142d`.

The MMD installed-reader probe accepts five previous controls and three new
kinematic child controls. Complete PMX inputs remain unchanged: 22
physics-bearing fixtures reject during joint declaration validation, while
six physics-free fixtures return empty snapshots. No full-PMX body or
simulation acceptance is established.

## Remaining boundary

Joint frames/limits, physical coefficients, compound colliders, scaled separate
body frames, scene gravity and mass inference remain open. MMD bone/body
synchronization remains deferred. Hosted Windows/Linux verification and
compatible artifact publication remain pending.
