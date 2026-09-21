# Rigid-body runtime contract

> **Status: proposed, 2026-09-21.** This document describes the intended
> extraction target. Nothing here is implemented in this repository yet; see
> the [capability matrix](../reference/CAPABILITY_MATRIX.md).

## 1. Purpose

`physicsCore` defines the smallest solver-neutral contract needed to create,
step, query, and destroy a rigid-body world. It has no OpenUSD, Jolt, Stage
Runner, MMD, or VRM dependency.

## 2. Proven extraction baseline

The initial behavior comes from `usd-stage-runner`'s current `physicsCore` and
`physicsJolt`:

- typed 64-bit `ShapeHandle`, `BodyHandle`, and `ConstraintHandle` values;
- box `ShapeDescriptor`;
- static and dynamic `BodyDescriptor` values;
- fixed `ConstraintDescriptor` values;
- create/destroy lifecycle;
- force and linear-velocity operations;
- `bodyState`, fixed-step simulation, and changed-body collection;
- optional `CollisionQuery::segmentHit` and `GroundQuery::groundContact`.

This is a behavioral seed, not a commitment to retain Stage Runner's namespace,
headers, or `runtimeCore` types.

## 3. Core value types

The core owns plain value types for vectors, rotations, and transforms, or a
separately packaged math contract with no application dependency. It must not
depend on `usd-stage-runner::runtime::RuntimeTransform` or
`usd-stage-runner::runtime::PrimId`.

All numeric inputs must be finite. Units and basis are explicit at the bridge:
core rigid-body distances are meters, time is seconds, and world-space
directions are expressed in the world's declared basis.

## 4. Handles

Handles are opaque, typed, comparable values. The invalid value is stable and
detectable. A handle never reveals a backend body ID, pointer, array slot, or
generation encoding.

The implementation must reject stale and cross-world handles safely. Whether
generation counters are encoded in the value is private.

## 5. Descriptors

### 5.1 Shapes

Phase 1 preserves the proven box shape. Sphere and capsule enter with the USD
vertical slice; mesh and compound shapes enter only with explicit validation
and lifetime rules.

A shape descriptor carries geometry, not backend cooking options.

### 5.2 Bodies

A body descriptor identifies its shape, motion type, initial transform, mass,
and collision-filter semantics. Damping, friction, restitution, angular
velocity, kinematic motion, and continuous-collision behavior are added only
when a phase requires them.

Collision filtering must be expressed semantically. A Jolt object-layer or
broad-phase-layer index is not a public collision category.

### 5.3 Constraints

Phase 1 preserves fixed constraints. Later descriptors may cover revolute,
prismatic, spherical, distance, and six-degree-of-freedom behavior through
semantics shared with the USD bridge.

Limits, drives, springs, and damping are explicit data. A descriptor never
contains a native Jolt constraint settings object.

## 6. World lifecycle

The world supports deterministic ownership operations:

```text
create shape -> create body -> create constraint
destroy constraint -> destroy body -> destroy unused shape
```

Destroying an invalid or stale handle must not access freed backend memory.
The contract must define whether an operation reports failure or is idempotent
before Phase 1 freezes the API.

A world reset destroys every transient resource and invalidates every handle
from that world.

## 7. Stepping and state

The caller supplies an explicit positive fixed step. `physicsCore` validates
it and does not own wall-clock accumulation or an application update loop.

State retrieval has two paths:

- direct state for one body;
- changed-body extraction for synchronization after a step.

Changed-body extraction returns neutral body handles and physical state. It
does not write a USD stage or a Stage Runner runtime world.

## 8. Optional query capabilities

Queries are capability interfaces rather than mandatory methods on a
monolithic world:

```text
PhysicsWorld
  +-- SegmentQuery
  +-- RayQuery
  +-- GroundQuery
  +-- ShapeCastQuery
  +-- ContactQuery
```

A backend reports capability support explicitly. Query filters use neutral
handles, categories, masks, and predicates whose lifetime is defined by the
core contract.

The proven baseline includes first-hit world-space segment queries with an
ignored body, and ground contacts containing support body, normal, and
distance.

## 9. Errors and validation

Programmer errors such as non-finite vectors, non-positive steps, invalid
dimensions, or impossible descriptors are rejected consistently before
backend calls. Resource exhaustion and unsupported backend capabilities are
distinguishable from invalid input.

The exact error transport—exceptions, result values, or a mixed policy—is
`RB-O1` and must be settled before Phase 1 public headers are accepted.

## 10. Threading and ownership

The Phase 1 world is single-owner and is called from one scheduling context.
No thread-safety guarantee is implied. A backend may use worker threads
internally, but its allocator and job-system objects do not enter public API.

## 11. ABI and evolution

The first extraction preserves source-level semantics, not a binary ABI.
Installed CMake targets and public include roots are versioned in the package
contract. Broad additions require a consumer scenario and tests.

## 12. Open questions

- **RB-O1:** error transport for validation, unsupported capabilities, and
  backend failures.
- **RB-O2:** the minimal repository-owned math types and whether they form a
  separate installed package.
- **RB-O3:** collision category/mask vocabulary that serves Stage Runner and
  MMD without exposing backend layers.
- **RB-O4:** semantics and ordering of changed-body extraction across create,
  sleep, wake, teleport, and destroy.
