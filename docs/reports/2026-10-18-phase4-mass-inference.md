# Phase 4 primitive mass inference (2026-10-18)

Dated local Windows evidence following the
[joint snapshot increment](2026-10-18-phase4-joint-snapshots.md).

## Change under test

Local changes over `c420c35` add bounded mass inference to all three
`physicsUsd` snapshot readers. Positive explicit body mass wins; otherwise
scaled single-Cube/Sphere/Capsule volume is multiplied by body-local density.
Zero/absent mass and density are ignored, using the standard USD 1000 kg/m³
density fallback when needed. This follows the
[OpenUSD mass specification](https://openusd.org/release/api/usd_physics_page_front.html).
Capsule volume includes both caps; its height excludes them. Offset, rotation
and axis do not affect volume. Mass/density must be time-independent, finite
and non-negative; inferred mass must be positive and finite. Static and
kinematic snapshots continue to ignore mass.

Inherited/collider MassAPI overrides, physics materials and their bindings,
inertia and center-of-mass overrides remain rejected. No core/backend API,
descriptor layout, dependency direction, published pin or importer stage
contract changes. The reader remains read-only and creates no resources.

## Verification

Windows x86_64, MSVC 19.51, Visual Studio 18 2026, OpenUSD 26.08 and Jolt 5.5.0.
Using the existing configured USD/Jolt build:

```powershell
cmake --build build/phase4-usd --config Release
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-mass-prefix
```

All 15 tests pass, including `physicsUsd.mass_scene` and clean-prefix installed
consumers. The mass suite checks direct and child geometry for each primitive,
scaled/reflected volume, collider offsets/rotation, Capsule caps and zero
height, no MassAPI, schema defaults, authored zeros, density, explicit-mass
precedence, read-only behavior, and typed code/path rejection for negative,
non-finite, animated, overflowed/underflowed mass and physics bindings.
Existing hierarchy tests retain rejection of inherited and child MassAPI.
The installed consumer checks default-density and authored-density mass on a
child Cube. Its first run exposed an exact floating-point comparison in the
new assertion; it passes with a numerical tolerance for rotated geometry.

Installed `physicsUsd.lib` SHA-256:
`1e5e92629d45cafb81257b1042e55014ff8cce786dd9fea084f08f42c309aab7`.

The isolated MMD consumer links only this installed `physicsUsd` package. All
12 controls pass, including three new primitive inference controls. All 28
unchanged opening PMX fixtures are accepted: 22 yield three bodies and two
standard joints each, and six yield empty snapshots. Reconstructed world
joint frames coincide within `1e-6` per matrix element. One fixture retains a
format-only joint outside the standard snapshot. The previous 21 missing-mass
rejections are resolved as USD declarations, without asserting equivalence to
PMX's zero-mass behavior or complete physics/simulation fidelity.

## Remaining boundary

Generic six-DOF runtime execution, full hierarchical mass properties, physics
materials, coefficients, filtering, springs, source-only semantics,
synchronization, hosted validation and compatible public package publication
remain open. Format-specific interpretation belongs to consumers.
