# Rigid-body runtime contract

> **Status: proposed, revised 2026-10-18.** This document describes the
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

Phase 1 preserves the proven box shape. The local Phase 4 primitive slice adds
sphere and capsule; mesh and compound shapes still require explicit validation
and lifetime rules. `ShapeType` selects the active geometry fields: box uses
`halfExtents`, sphere uses `radius`, and capsule uses `radius` and `halfHeight`.
Radius must be finite and positive. Capsule half-height is finite and
non-negative, measures the cylinder excluding hemispheres, and follows local
Y. Zero half-height has sphere geometry. Inactive geometry fields are ignored.

A shape descriptor carries geometry, not backend cooking options.

`ShapeDescriptor::localTransform` places that geometry relative to the body
origin, defaulting to identity. Its translation is in meters; rotation uses
the core quaternion convention. Translation and rotation must be finite,
and rotation must be nonzero; the backend normalizes it. Scale belongs in
the geometry dimensions. A shape handle includes this local pose, so bodies
sharing the handle share it. Body creation and state reporting continue to
use the body origin, even when it differs from the shape's center of mass.

### 5.2 Bodies

A body descriptor identifies its shape, motion type, initial transform, mass,
and collision-filter semantics. `BodyDescriptor::contactMaterial` carries
dimensionless friction (finite, non-negative) and restitution (finite, in
`[0,1]`). `ContactMaterial` defaults to friction `0.2` and restitution `0`,
preserving the previous Jolt defaults. Validation applies to every motion type.
`validateContactMaterial()` also validates a material without a body handle.
`ContactMixing` selects an immutable world-wide pair policy: friction and
restitution each use `ContactCombineRule::geometricMean`, `maximum`, or
`product`. The defaults are geometric-mean friction and maximum restitution,
preserving the original Jolt behavior. Optional `maxFriction` caps the combined
friction after mixing; absent means no ceiling, and zero disables friction.
A ceiling must be finite and non-negative. Unknown rules reject through
`validateContactMixing()`. No source-format profile is defined in the core.
`physicsJolt::createWorld(const ContactMixing&)` copies the validated policy;
`createWorld()` retains its existing default contract. Settings are independent
between simultaneously alive worlds and cannot change during their lifetime.
Jolt applies the policy to both added and persistent contacts. Selecting the
same scalar rule as a source solver does not establish trajectory equivalence.

Jolt rejects positive coefficients rounded to zero and friction whose rounded
scalar squared exceeds `FLT_MAX` before body creation, keeping any pair product
finite. A supplied ceiling must also fit Jolt's scalar range without positive
underflow. Combined coefficients retain backend scalar precision. These backend
limits do not narrow the core's double-valued contract.

`BodyDescriptor::damping` carries `BodyDamping` with independent `linear` and
`angular` decay rates in inverse seconds: `dv/dt = -linear * v` and
`dw/dt = -angular * w`. Both must be finite and non-negative for every motion
type; `validateBodyDamping()` also validates them independently. Zero disables
that damping channel; rates above one are valid. Defaults are `0.05` for both,
preserving previous Jolt behavior. Damping affects dynamic motion only.
The backend may approximate exponential decay at each substep: Jolt multiplies
each velocity by `max(0, 1-rate*dt)` after force/gravity integration. Finite-step
trajectories therefore depend on the backend and timestep. Jolt rejects rates
outside its scalar range or positive rates rounded to zero before creating a
native body. Source retention fractions must be converted by their adapter,
not copied into these rate fields. Angular-velocity commands and
continuous-collision behavior remain deferred.

The local Phase 4 increment adds `MotionType::kinematicBody`. It is driven by
explicit pose commands rather than gravity, forces or impulses from contacts.
Its mass must be finite and non-negative but is not used for simulation.
Force and linear-velocity commands remain restricted to dynamic bodies.
Backends supporting kinematic motion expose the optional `KinematicMotion`
interface below; `PhysicsWorld` gains no mandatory virtual operation.

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

### 5.4 Optional six-DOF constraints

`SixDofConstraints` in `usd_physics/core/six_dof.h` is an optional capability
discovered from `PhysicsWorld`. `createSixDofConstraint()` returns the existing
constraint handle type; destruction and body-dependent cleanup use the existing
world operations. The mandatory fixed-constraint API is unchanged.
Jolt exposes this experimental capability only when built with
`PHYSICSJOLT_ENABLE_SIX_DOF=ON` (default OFF); consumers test capability presence.

`SixDofConstraintDescriptor` names two distinct endpoints; one invalid body
handle means world, and two invalid handles reject. Frames are relative to body
origins, or world for the invalid endpoint. Finite nonzero quaternions are
normalized by the backend. At least one endpoint must be dynamic in Jolt;
static/kinematic-only pairs reject before mutation.

The six axes are translation X/Y/Z in meters followed by rotation X/Y/Z in
radians. Translation measures the second anchor minus the first in the first
frame's axes. Rotation uses X twist and Y/Z pyramid swing of the second frame
relative to the first, rather than three independently composed Euler angles.
`AxisMotion` is free, locked at zero, or limited by finite `low < high` bounds.
Angular bounds lie strictly inside `(-pi, pi)`. Inactive bounds are ignored.
General drives and nonzero equal-bound locks are outside this slice. Backend
scalar range or collapsed-precision bounds reject before resource creation.

`collisionEnabled=false` suppresses contacts between the two body endpoints
while the constraint exists. Any suppressing constraint wins over an enabling
one; removing the last suppressing constraint restores ordinary category/mask
filtering. World endpoints have no collider to suppress. Removal wakes dynamic
endpoints, so a released sleeping body resumes motion. No public wake or sleep
command is introduced. All operations retain the single-owner scheduling rule.

### 5.5 Translation springs (experimental)

`SixDofConstraintDescriptor::translationSprings` adds three independent
`TranslationSpring` values in the first joint frame. Stiffness is N/m,
damping is N*s/m and equilibrium is meters. The continuous force law is
`F = -k*(x-equilibrium) - c*v`, with relative anchor velocity. Every value
must be finite; stiffness and damping must be non-negative. Zero stiffness
disables an axis and requires zero damping; its finite equilibrium is ignored.
Positive stiffness on a locked
axis rejects; free and hard-limited axes may have springs, including an
equilibrium outside their limit range. No angular spring is implied.

Jolt uses position motors with `StiffnessAndDamping` settings and targets in
the first constraint frame. Active scalar overflow and nonzero values rounded to
zero reject before resource creation. Hard limits remain hard. The existing
six-DOF build option gates execution. These are discretized implicit springs:
analytic response tests at 60/240 Hz measure timestep-dependent numerical
damping, even with zero configured damping. Source solver equivalence requires
separate evidence; source-format conversion belongs outside this package.

### 5.6 Rotation springs (experimental)

`SixDofConstraintDescriptor::rotationSprings` contains three `RotationSpring`
stiffness/damping pairs in N*m/rad and N*m*s/rad. Equilibrium is identity:
the two joint frames align. The restoring response is defined for small
relative rotations about the second joint frame's axes, with viscous damping
of relative angular velocity. No independent Euler-angle spring law is promised
for finite coupled rotations. General orientation targets are outside this slice.

Every coefficient must be finite and non-negative. Zero stiffness disables an
axis and requires zero damping. Positive stiffness on a locked angular axis
rejects; free or limited angular axes may have springs. Backend overflow and
nonzero values rounded to zero reject before creating resources. Hard limits
remain hard even when identity equilibrium lies outside their range.

Jolt uses position motors with stiffness/damping settings, identity orientation
target and axes in the second constraint frame. Its quaternion error and
projection onto enabled axes approximate small angular displacements; at finite
angles or with coupled anisotropic axes, this differs from independent Euler
springs. The existing experimental six-DOF option gates execution. Analytic
60/240 Hz tests measure small-angle and timestep approximation, including
numerical damping. Source-format units and activation rules remain consumer-owned.

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

### 7.1 Kinematic motion capability

`KinematicMotion`, declared in `usd_physics/core/kinematic_motion.h`, is
discovered with `dynamic_cast` from `PhysicsWorld`, like optional queries.
`moveKinematic(body, target, duration)` sets linear and angular velocities to
reach a world-space body origin and orientation after the supplied positive
duration. It does not teleport. Collider-local poses and centers of mass do
not change the target frame. Finite, nonzero quaternions are normalized by
the backend. Backend precision/range failures reject before mutation.

The caller commands a target before each matching fixed step. Velocities
persist until replaced; to hold a pose, resubmit the current pose. No implicit
stop, target tracking, timestep accumulator or USD animation sampling is
introduced. The linear velocity in `BodyState` is the center-of-mass velocity,
which can differ from body-origin velocity for offset shapes during rotation.

Invalid values throw `std::invalid_argument` before handle lookup. Unknown,
stale, cross-world and non-kinematic targets return `false`. Commands do not
enqueue changed states: stepping collects kinematic changes with the same
unique handle ordering, drain and destruction semantics as dynamic bodies.
Dynamic bodies can respond to contact with a moving kinematic collider;
solver contact tolerances remain backend-specific.

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
