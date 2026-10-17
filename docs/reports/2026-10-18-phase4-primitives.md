# Phase 4 Sphere and Capsule snapshots (2026-10-18)

Dated local Windows evidence following the
[collider-local Box increment](2026-10-18-phase4-collider-local-pose.md).

## Change under test

Local changes over `a2121b4` add neutral sphere/capsule shape types, radius and
cylinder half-height. Capsule geometry follows local Y; zero half-height uses
a sphere. Jolt creates native rounded shapes and applies the existing local
pose wrapper. The descriptor layout changes, so core, backend, bridge and all
consumers are rebuilt together. No published artifact pin changes.

`readPrimitiveScene()` in `box_scene.h` returns `PrimitiveBody` (an alias of
`BoxBody`) for Cube, Sphere and Capsule. The existing `readBoxScene()` remains
Cube-only. Both retain read-only snapshots, typed errors, path ordering,
body identity, explicit dynamic mass and composed body/collider poses.
Rounded shapes require uniform scale; the reader rejects nonuniform scale.
Capsule axes X/Y/Z map into the shape-local rotation. Height excludes the caps.

## Verification

Windows x86_64, MSVC 19.51, Visual Studio 18 2026, OpenUSD 26.08,
Python 3.13.14 and Jolt 5.5.0. The existing configured USD/Jolt build was used:

```powershell
cmake -S . -B build/phase4-usd -DUSDPHYSICS_BUILD_USD=ON
cmake --build build/phase4-usd --config Release
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-primitives-prefix
```

All 12 tests pass. Core validation checks invalid radii and cylinder heights.
USD tests compare composed geometry centers and both capsule tips with USD
transforms for direct and child colliders, every axis, rotation, translation
and reflected uniform scale. Invalid/animated radius, height and axis,
nonuniform scale and zero-height behavior have fixtures. The original Box
tests remain, including their rejection of non-Cube geometry.

Jolt tests check axial/radial ray hits, local capsule rotation, radius scalar
range rejection, falling/settling and body-origin state for rounded shapes,
including zero-height capsules. Clean-prefix consumers link both reader APIs
and read a static Z-axis capsule. All inputs are generated synthetic data.

The installed `physicsUsd.lib` has SHA-256
`2e06c0d6a3ecd0657da3077ff4847af92b35ca9407a11013f1e41ee107c61885`.

The MMD consumer accepts its three Box controls and new Sphere/Capsule child
controls. Its complete PMX inputs remain unmodified: 22 physics-bearing
fixtures reject at joint declaration validation, and six physics-free fixtures
return empty snapshots. This establishes no full PMX body or simulation
acceptance.

## Remaining boundary

Kinematic operations, joints/limits, coefficients, compound colliders, scaled
separate body frames, scene gravity and mass inference remain open. Hosted
Windows/Linux verification and compatible artifact publication remain pending.
