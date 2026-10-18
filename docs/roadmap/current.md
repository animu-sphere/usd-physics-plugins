# Current roadmap — release follow-up and Phase 4

Status: in progress, 2026-10-18. The installed core/backend consumer migration
has passed hosted Windows/Linux verification. An optional standard primitive reader
is implemented; the [capability matrix](../reference/CAPABILITY_MATRIX.md)
records the supported subset and tests.

The `usd-jolt` intent and source-CI cells now build the Box reader together
with Jolt and package all three libraries on both OSes. The
[local artifact report](../reports/2026-10-04-phase4-usd-artifacts.md) records
Windows validation; hosted results and public `physicsUsd` pins remain pending.

## 1. Release follow-up

Implement and dry-run the versioned release workflow against the
[release schema](../architecture/RELEASE_SCHEMA.md) before creating a tag.
Existing Windows/Linux core/backend artifacts remain migration inputs rather
than a full release.

## 2. Next Box delivery slice

- Validate and publish Windows/Linux `physicsUsd` artifacts and pin them in
  Stage Runner's OpenStrata requirements before enabling standard import by default.
- Run hosted standard/compatibility parity including standalone and usdview.
- Extend through working fixtures for scene gravity, additional geometry beyond
  the local Cube/Sphere/Capsule slice,
  hierarchical mass properties/material density beyond single-primitive body
  density, scaled
  separate body frames and compound colliders.
- Experimental six-DOF execution passes hosted Windows/Linux acceptance on
  the selected deterministic SDK through the `usd-jolt-six-dof` cells, which
  test but never package. The option stays OFF by default and no six-DOF
  package is published. Body friction/restitution now pass local backend
  response controls ([contact evidence](../reports/2026-10-18-phase4-contact-material.md));
  neutral damping rates also have local 60/240 Hz linear/angular response
  controls ([damping evidence](../reports/2026-10-18-phase4-body-damping.md)).
  Pair-rule selection now has local 60/240 Hz evidence
  ([mixing evidence](../reports/2026-10-18-phase4-contact-mixing.md)).
  Standard USD material bindings, instantaneous source damping, timestep and source-solver trajectory fidelity,
  springs, source-only
  semantics and full MMD fidelity remain open
  ([cause](../reports/2026-10-18-phase4-six-dof-sdk-cause.md),
  [execution evidence](../reports/2026-10-18-phase4-six-dof-execution.md),
  [hosted acceptance](../reports/2026-10-18-phase4-six-dof-hosted.md)).
- Resolve stage-generation identity and synchronization contracts before
  introducing retained resource mappings in the package.

## 3. Completion criteria

Standard declarations become the primary authored representation in Stage
Runner while core/backend packages remain reusable and contain no gameplay
policy. The same package graph must work in plain CMake and OpenStrata, on
Windows and Linux. Keep Runner compatibility fixtures until all host and
runtime-layer parity gates pass. The wider intended bridge is defined in
[USD_BRIDGE_CONTRACT.md](../design/USD_BRIDGE_CONTRACT.md).
