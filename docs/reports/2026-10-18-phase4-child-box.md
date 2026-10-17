# Phase 4 child Box and composed transforms (2026-10-18)

Follow-up: [collider-local pose evidence](2026-10-18-phase4-collider-local-pose.md).

Dated local Windows evidence; append-only. No simulation acceptance is
claimed for an MMD stage.

## Scope

An increment over source baseline `3daad33` admits one centered, axis-aligned
collision Cube below a rigid body Xform, with explicit positive dynamic mass
on the body. Body path, mass and rigid world frame remain distinct from the
child's geometry. Direct collision Cubes also support composed transforms.
No public descriptor layout, dependency edge or backend is changed.

The reader validates declarations before building the returned snapshot.
The first reported rejection can therefore change when multiple unsupported
features coexist; it does not establish that earlier features were accepted.

## Verification

Windows x86_64, Visual Studio 18 2026, MSVC 19.51, OpenUSD 26.08 and
Python 3.13.14. From the repository root, using the existing Jolt-enabled
USD build configured against installed dependencies:

```powershell
cmake --build build/phase4-usd --config Release
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-box-prefix
```

All 12 tests pass, including the Jolt backend, USD reader, boundary/version
checks, documentation links and clean-prefix installed consumers. The USD
installed consumer verifies both the original direct Cube and a dynamic body
Xform with a child Cube. The installed `physicsUsd.lib` used by the MMD probe
has SHA-256
`f1c0b7005024cdd70c149d9c86b80a5e8859a3d7d1b4e94df6478cf8de6171c9`.
This is a local artifact, not a published package pin.

The focused reader suite compares all eight reconstructed runtime box corners
against USD geometry, including reflected/nonuniform Cube scale, parent
rotation/translation, a float orient op, an authored affine matrix and an
identity stack. It checks body identity, explicit mass, composed translation,
disabled rigid bodies and a reset that excludes an animated ancestor.
Negative fixtures cover child offsets, rotations, reset stacks, scaled body
frames, animated ancestors/colliders, zero scale, shear, perspective,
non-finite or unreadable ops, inverse ops, nested/compound bodies, collider
mass overrides and point-instancing of a child collider, alongside the
existing unsupported-declaration fixtures.

## Consumer result and remaining gaps

The `usd-mmd-plugins` isolated probe, built against this install, accepts
both its original direct Cube control and its new parent-body/child-Cube
control with a rotated ancestor. All 22 physics-bearing PMX fixtures still
reject, now at `/Asset/physics/joints/joint_0000` during declaration
validation; six physics-free fixtures return empty snapshots.

Sphere/capsule shapes, kinematic bodies, joints/limits and coefficients
remain unsupported. Child collider position/orientation offsets require a
neutral representation and backend support before the reader can preserve
them. The [capability matrix](../reference/CAPABILITY_MATRIX.md) owns current
status; this bounded increment does not complete generic PMX consumption.
