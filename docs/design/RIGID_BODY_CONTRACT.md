# Rigid-body runtime contract

> **Status: proposed, revised 2026-09-22.** This document describes the
> intended runtime contract. The Phase 1 neutral values, handles, descriptors,
> typed errors, single-owner world interface, and optional segment and ground
> query interfaces are implemented; see the
> [capability matrix](../reference/CAPABILITY_MATRIX.md) for the exact current
> boundary.

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
- optional `SegmentQuery::segmentHit` and `GroundQuery::groundContact`.

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

Phase 1 represents filtering with two 64-bit sets: `categories` states the
semantic categories to which a body belongs, and `collidesWith` states the
categories it accepts. A pair is admitted only when each body's categories
intersect the other body's `collidesWith` set. `categories` must be non-zero;
`collidesWith` may be zero for an intentionally non-colliding body. The
default is category bit zero colliding with every category. This represents
Stage Runner's static/moving policy, MMD's 16 groups and per-body masks, and
future bridge-assigned categories without exposing backend layer numbers.

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
Destruction returns `false` for an unknown, stale, cross-world, or already
destroyed handle. Destroying a body also destroys its dependent constraints;
destroying an in-use shape fails.

Destroying a world destroys every transient resource and invalidates every
handle from that world. A separate in-place reset operation is not part of the
Phase 1 contract.

## 7. Stepping and state

The caller supplies an explicit positive fixed step. `physicsCore` validates
it and does not own wall-clock accumulation or an application update loop.

State retrieval has two paths:

- direct state for one body;
- changed-body extraction for synchronization after a step.

Changed-body extraction returns neutral body handles and physical state. It
does not write a USD stage or a Stage Runner runtime world.

Phase 1 extraction is deterministic: each body appears at most once, results
are ordered by ascending opaque handle value, and taking the results drains the
queue. Creation is represented by the descriptor and direct initial state, not
by the changed queue. A successful command becomes observable through changed
state after stepping; destruction removes pending state for that body. Sleep,
wake, and teleport operations are not in the Phase 1 API and must define their
queue behavior when admitted.

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
distance. Phase 1 names those independent interfaces `SegmentQuery` and
`GroundQuery`. A backend world advertises support by inheriting the applicable
interface; a consumer discovers it with `dynamic_cast`, and a null result means
the capability is unsupported.

Segment endpoints must be finite and distinct. A segment hit contains a valid
body and a finite fraction in `[0, 1]`. A ground probe distance must be finite
and non-negative. A ground contact contains a valid support body, a finite
non-zero normal, and a finite non-negative distance. Unknown, stale, or
cross-world target and ignored-body handles are backend lookup failures and
throw `std::out_of_range`.

## 9. Errors and validation

Programmer errors such as non-finite vectors, non-positive steps, invalid
dimensions, or impossible descriptors are rejected consistently before
backend calls. Resource exhaustion and unsupported backend capabilities are
distinguishable from invalid input.

`RB-O1` uses the following mixed policy for Phase 1:

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
failures distinguishable. Descriptor and world contract tests cover the
Phase 1 branches; a concrete backend must retain the same outcomes.

## 10. Threading and ownership

The Phase 1 world is single-owner and is called from one scheduling context.
No thread-safety guarantee is implied. A backend may use worker threads
internally, but its allocator and job-system objects do not enter public API.

## 11. ABI and evolution

The first extraction preserves source-level semantics, not a binary ABI.
Installed CMake targets and public include roots are versioned in the package
contract. Broad additions require a consumer scenario and tests.

## 12. Open questions

- **RB-O1 (resolved for Phase 1):** use the tested mixed policy in §9.
- **RB-O2 (resolved for Phase 1):** minimal vector, rotation, and transform
  values live in `physicsCore`; a separate package requires an independent
  consumer and an architecture revision.
- **RB-O3 (resolved for Phase 1):** use the 64-bit `categories` and
  `collidesWith` sets defined in §5.2. Backend layer assignment is private and
  may cache distinct filter combinations without changing this contract.
- **RB-O4 (resolved for Phase 1):** use the ordering and drain semantics in
  §7. Sleep, wake, and teleport remain outside the current API and must extend
  this rule explicitly when admitted.
