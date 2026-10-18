# Phase 4 joint frame and limit snapshots (2026-10-18)

Follow-up: [primitive mass inference](2026-10-18-phase4-mass-inference.md).

Dated local Windows evidence following the
[kinematic increment](2026-10-18-phase4-kinematic.md).

## Change under test

Local changes over `01b727c` add `readPhysicsScene()` to `physicsUsd`, returning
sorted primitive bodies and generic/fixed joint declarations. The existing
body-only readers retain their joint rejection. No core or backend constraint
interface changes. This is declaration conversion, not joint simulation.

Joint snapshots preserve exact body/world endpoints, normalized body-local
frames and enabled/collision flags. Six-axis limits convert angular degrees
to radians, retain linear meters, and preserve absent, infinite, equal and
inverted bounds. USD's inverted limit means locked; this is separate from any
source format's convention ([OpenUSD LimitAPI reference](https://openusd.org/release/api/class_usd_physics_limit_a_p_i.html)).
Scaled joint body frames, unsupported endpoint forms, specialized joints,
distance limits, drives, breaking properties and animated data reject with
typed code/path diagnostics. Format metadata is ignored by the generic reader.

## Verification

Windows x86_64, MSVC 19.51, Visual Studio 18 2026, OpenUSD 26.08 and Jolt 5.5.0.
Using the existing configured USD/Jolt build:

```powershell
cmake --build build/phase4-usd --config Release
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-joint-prefix
```

All 14 tests pass, including `physicsUsd.joint_scene` and the updated
clean-prefix installed consumer. The focused joint test checks body-relative
frames independently of ancestor and collider transforms, quaternion
normalization, six-axis units and default/free/locked/equal/infinite limits,
fixed joints, world anchors, sorting, read-only behavior, retained legacy
rejection and invalid endpoints, frames, flags, limits, schemas and units.

Installed `physicsUsd.lib` SHA-256:
`41df83f6125ba50eaf9a356f78004e46e058661fe20f3f1f6f4e46c70e4d56a0`.

The isolated MMD consumer links only this installed `physicsUsd` package.
Its eight previous controls and a new two-body joint frame/limit control pass.
Among the 28 unchanged opening PMX fixtures, six have no physics and yield
empty snapshots; 21 reject with `invalidValue` at
`/Asset/physics/rigidBodies/chest` because the dynamic body's mass is not
specified. `recoverable/physics-repairs.pmx` yields three bodies and two
standard joints, with a further format-only joint outside the snapshot.
This is not complete PMX physics or simulation acceptance. No importer or
stage-contract change and no published artifact update were needed.

## Remaining boundary

Explicit dynamic mass remains required. Mass inference needs its own USD
contract and fixtures; no default mass is guessed here. Generic six-DOF
runtime execution, coefficients, filtering, spring fidelity, source-only
joint interpretation, synchronization, hosted validation and compatible
public package publication remain open.
