# Stage Runner extraction baseline

This page records the source boundary inspected for the initial extraction.
The source repository is `usd-stage-runner` revision
`9b49e154f4e8bd0123c1aae5b19db9b0b65b59a3` (2026-09-21). Its physics paths
are unchanged from the source repository's frozen inventory revision
`12324992c7ddd0b016ace780acadc5f07903390c`.

## 1. Neutral behavior to preserve

| Source | Extraction treatment |
| --- | --- |
| `handles.h` | Preserve typed opaque 64-bit handles, invalid zero, equality, and hashing. |
| `physics_shape.h` | Preserve the box descriptor and validation; replace `runtime::Vec3d`. |
| `physics_body.h` | Preserve motion type, mass, collision value, initial state, and body state; replace Stage Runner transforms. |
| `physics_constraint.h` | Preserve the fixed-constraint descriptor and validation. |
| `physics_world.h` | Preserve explicit lifecycle, force/velocity commands, fixed stepping, direct state, and changed-state draining. |
| `collision_query.h` | Preserve the optional first-hit segment capability and ignored-body behavior. |
| `ground_query.h` | Preserve the optional support contact capability. |

Neutral vector, rotation, and transform values move into `physicsCore` under
the `usd_physics::core` namespace. This removes the current
`physicsCore -> runtimeCore` dependency.

## 2. Coupling that does not move into core

`physics_runtime.h/.cpp` owns the one-to-one `PrimId <-> BodyHandle` mapping,
delegates stepping, writes changed transforms into `RuntimeWorld`, and marks
dirty prims. That mapping becomes an application or future `physicsUsd` bridge
responsibility; `PrimId`, `RuntimeWorld`, and the dirty queue do not enter
`physicsCore`.

Stage traversal, fixed-step accumulation, host scheduling, character/camera
policy, and USD writeback stay in Stage Runner.

## 3. Backend implementation

`backends/physicsJolt/jolt_physics_world.cpp` remains the behavioral seed for
Jolt registration, allocators, jobs, filters, native resource maps, stepping,
changed-state collection, ground shape casts, and segment ray casts. Every
`JPH::*` type remains private to `physicsJolt`.

The public factory and availability seam may be adapted to the new namespace.
The numeric moving/non-moving layer constants are not copied into the neutral
API; collision filtering remains `RB-O3` until consumer semantics are fixed.

## 4. Test migration

Core contract tests move with neutral behavior after Stage Runner math and
mapping cases are separated. Backend tests move with Phase 2 and retain the
falling body, closest hit, ignored body, ground contact, changed-state drain,
constraint, and cleanup cases. Stage/session fixtures remain consumer tests in
Stage Runner.

