# Rigid-body runtime contract

> **Status: proposed, revised 2026-09-22.** This document describes the
> intended extraction target. Neutral values and handles are implemented; see
> the [capability matrix](../reference/CAPABILITY_MATRIX.md) for the exact
> current boundary.

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

The initial public values are `Vector3`, `Quaternion`, and `Transform`.
`Quaternion` uses `(w, x, y, z)` storage and defaults to identity;
`Transform` contains translation and rotation only because scale is not part
of a rigid-body pose.

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

`RB-O1` has the following proposed mixed policy for Phase 1:

- value and descriptor validation throws `std::invalid_argument` before a
  backend call;
- creation rejects unknown, stale, or cross-world dependency handles with
  `std::invalid_argument`;
- state retrieval for an unknown, stale, or cross-world handle throws
  `std::out_of_range`;
- destruction and command operations return `false` when the target is
  unknown, stale, cross-world, already destroyed, or incompatible with the
  operation; repeated destruction is therefore a safe reported no-op, not a
  successful idempotent operation;
- unsupported optional behavior is represented by absence of the capability,
  rather than by calling a mandatory method that fails; and
- backend unavailability, resource exhaustion, and solver failures throw a
  typed `PhysicsError` carrying a solver-neutral `PhysicsErrorCode`.

This retains the proven Stage Runner call patterns while making backend
failures distinguishable. Phase 1 descriptor and world slices must test the
relevant branch before this proposal becomes accepted.

## 10. Threading and ownership

The Phase 1 world is single-owner and is called from one scheduling context.
No thread-safety guarantee is implied. A backend may use worker threads
internally, but its allocator and job-system objects do not enter public API.

## 11. ABI and evolution

The first extraction preserves source-level semantics, not a binary ABI.
Installed CMake targets and public include roots are versioned in the package
contract. Broad additions require a consumer scenario and tests.

## 12. Open questions

- **RB-O1 (proposed resolution):** use the mixed policy in §9; accept it only
  with the descriptor and world contract tests.
- **RB-O2 (resolved for Phase 1):** minimal vector, rotation, and transform
  values live in `physicsCore`; a separate package requires an independent
  consumer and an architecture revision.
- **RB-O3:** collision category/mask vocabulary that serves Stage Runner and
  MMD without exposing backend layers.
- **RB-O4:** semantics and ordering of changed-body extraction across create,
  sleep, wake, teleport, and destroy.
