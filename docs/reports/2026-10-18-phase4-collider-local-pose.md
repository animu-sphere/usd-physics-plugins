# Phase 4 collider-local Box pose (2026-10-18)

Follow-up: [Sphere and Capsule snapshots](2026-10-18-phase4-primitives.md).

Dated local Windows evidence, following the
[centered child Box increment](2026-10-18-phase4-child-box.md).

## Change under test

Local changes over `1a41b77` append an identity-defaulting `localTransform`
to the neutral `ShapeDescriptor`. The USD reader decomposes a single child
Cube's composed relative transform into scale (geometry dimensions) and a
local pose. Intermediate Xforms, rotations, offsets and reflected box scales
are accepted when the resulting axes are orthogonal. Body path, mass and
world frame remain those of the rigid body, independent of its collider.

Jolt wraps displaced geometry in a rotated/translated shape. Ray queries
check the composed body and shape rotations and the displaced collision
surface. A dynamic offset box settles on a floor with its body origin below
the contact surface; direct and changed-state reporting agree on that origin.
Non-unit local rotations normalize; zero and non-finite rotations fail.

This adds a public descriptor field. Existing source initializers retain
identity behavior; all libraries and consumers must be rebuilt together.
No published package pin is changed.

## Verification

Windows x86_64, MSVC 19.51, Visual Studio 18 2026, OpenUSD 26.08,
Python 3.13.14, Jolt 5.5.0. Using the existing configured USD/Jolt build:

```powershell
cmake --build build/phase4-usd --config Release
$env:PATH = "$env:USD_ROOT/bin;$env:USD_ROOT/lib;$env:PATH"
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-local-pose-prefix
```

All 12 tests pass, including core validation, Jolt collision/settling/state,
USD eight-corner geometry comparisons, rejection fixtures, boundaries,
documentation, package versions and clean-prefix installed consumers. The
installed USD consumer also reads a translated/rotated child shape while
preserving the parent body frame. Tests and input are synthetic.

The MMD repository's isolated installed consumer accepts its original two
controls plus a new offset/rotated child Cube control. Its unmodified PMX
fixture probe still reports 22 physics-bearing stages rejected at the first
joint declaration and six physics-free stages accepted with empty snapshots.
That prevalidation failure establishes no full PMX body or simulation support.

## Remaining boundary

Separate body frames must still be rigid. Shear, singular and non-finite
geometry, child reset stacks, inverse ops and animated declarations reject.
Sphere/capsule shapes, kinematic operations, joints, physical coefficients,
compound colliders and scaled separate body frames remain outside this slice.
Hosted verification and publication of compatible artifacts remain pending.
