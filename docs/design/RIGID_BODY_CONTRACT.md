# Rigid-body runtime contract

> **Decision: proposed, revised 2026-10-20.** This document defines the neutral
> runtime contract. Implementation status and evidence belong only in the
> [capability matrix](../reference/CAPABILITY_MATRIX.md).

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

The shape contract includes box, sphere and capsule. Mesh and compound shapes require explicit validation
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

`MotionType::kinematicBody` is driven by explicit pose commands rather than
gravity, forces or impulses from contacts.
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

Category/mask meaning is neutral; backend filter/layer mapping is private to
each world. A format adapter translates MMD's 16 groups into these sets.
The host decides category-bit allocation and cross-avatar collision policy
when avatars share a world. Neither the reader nor the backend assigns bits
from MMD identities or silently aliases categories when capacity is exhausted.

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

### 6.1 World gravity at construction

World gravity is immutable world-space acceleration in meters/second squared.
It acts on dynamic bodies independently of mass; static and kinematic bodies
do not accelerate under gravity. Zero disables gravity and every finite
direction is allowed. It is already an acceleration vector: factories must
not normalize it or reinterpret zero as an earth-gravity default.

`createWorld(mixing, gravity)` (in namespace
`usd_physics::jolt`) accepts the neutral `ContactMixing` policy and `Vector3`
acceleration. The no-argument and mixing-only overloads retain `{0, -9.81, 0}`.
Settings apply only to the returned world; constructing another world must
not change them. No mandatory `PhysicsWorld` method or mutable setting is added.

Nonfinite acceleration rejects with `std::invalid_argument` before backend
availability is checked. An available backend additionally rejects numeric
overflow and nonzero components rounded to zero before world construction.
Valid input without a solver throws `PhysicsErrorCode::backendUnavailable`.
Scene selection, absent-scene defaults, factory invocation and full rebuild
remain consumer responsibilities. A consumer may pass
`SceneSettings::gravity` from the read-only USD snapshot without retaining USD
objects in a world.

### 6.2 Host ownership and discontinuities

The host owns the world and may inject a borrowed world into multiple consumers.
Consumers must not destroy a borrowed world. A world owns its backend resources;
descriptors and input snapshots are value data, not retained USD or renderer
objects. Host-owned path/handle tables and avatar resource sets must be discarded
before their world is destroyed. Per-avatar removal first removes dependent
cross-avatar constraints under host policy, then destroys that avatar's
constraints, bodies and unused shapes in dependency order.

Reset, seek and animation-loop discontinuities must invalidate the affected
runtime mappings and presentation history. Full destruction/rebuild is the
baseline reset mechanism; an in-place reset is a separate API proposal. The
host must coordinate all users before rebuilding a shared world, recreate
resources in a stable order and resume only after successful construction.
Rebuilt bodies use their construction state and explicit initialization commands;
old simulation velocity must not survive accidentally. A pose teleport must
explicitly choose velocity clearing or preservation and refresh presentation
history; it must not be
substituted with `moveKinematic()`, which describes motion over time.

### 6.3 Avatar integration vocabulary (proposal)

These names describe integration operations, not new installed signatures or a
frozen ABI. Extend the existing interfaces only where the consumer needs more:

| Conceptual operation | Contract surface / composition |
| --- | --- |
| `CreateWorld(config)` | Backend `createWorld()` overloads return an owning `std::unique_ptr<PhysicsWorld>`; host injects the world. |
| `AddBodiesAndConstraints(snapshot)` | Consumer composes `createShape()`, `createBody()` and constraint capabilities after snapshot validation, with rollback on failure. |
| `SetKinematicTargets(ids, transforms)` | Consumer resolves handles and calls optional `KinematicMotion::moveKinematic()` with the matching fixed duration. |
| `StepFixed(dt)` | One scheduler calls `PhysicsWorld::step(Duration)` for each world and tick. |
| `ReadBodyTransforms(ids)` | `bodyState()` or scheduler-distributed `takeChangedBodyStates()` values; no renderer dependency. |
| `Reset/Rebuild` | Host coordinates teardown/recreation under §6.2; dedicated reset/teleport APIs require explicit semantics and version review. |

The [package contract](../architecture/PACKAGE_CONTRACT.md#5-versioning) governs
API/ABI evolution; conceptual batch operations do not introduce mandatory
virtual methods or move orchestration into `physicsCore`.

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

### 7.2 Reproducible fixed stepping

Repeatability is scoped to the same backend, SDK/build, environment, world
configuration, creation order and ordered command sequence. Record the fixed
duration, host catch-up limit, backend substep policy and any configurable seed
(or absence of one). Backend configuration must document its substep count or
upper bound separately from the host's maximum ticks per frame. The host owns
accumulation and the explicit policy for excess elapsed time.

Rebuilding and replaying identical inputs under that configuration must be
tested for repeatable finite state within declared tolerances. This does not
require bitwise agreement across platforms or numerical agreement with Bullet.
Opaque handle values need not repeat after rebuild; compare state by stable
consumer identity. Changes to update order or substep policy must be explicit.

### 7.3 Shared-world scheduling

For each fixed simulation tick, the host/avatar scheduler must enforce:

1. Complete pre-physics evaluation for every avatar using the world and submit
   all kinematic targets for the same duration.
2. Call `step()` exactly once for that world, regardless of avatar count.
3. Distribute post-step state to every avatar, then let its format adapter apply
   bone feedback.

Calls remain serialized under §10. An avatar must not step a borrowed world
inside its own update. Because `takeChangedBodyStates()` drains a world-wide
queue, the scheduler must drain it once and distribute copies, or consumers
must use direct `bodyState()` reads. One avatar must not consume another's
changes. A scheduler snapshot is associated with its world generation and tick.

Copy completed simulation state into previous/current presentation snapshots.
The renderer may interpolate those copies; interpolation must not write back
into canonical physics state. Reset/rebuild invalidates both copies before
the next frame. The world owns neither avatar clocks nor presentation objects.

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

### 9.1 Integration diagnostics (proposal)

Integration must report unsupported joint axes, spring approximations, invalid
data and body/constraint creation failures as structured diagnostics. Records
must identify a stable code, severity, operation and neutral object identity;
the consumer attaches source prim paths or avatar identities. Approximation
records must identify the affected axes and approximation semantics. A rejected
operation must be distinguishable from an explicitly accepted approximation.

Preserve the existing exception/return-value policy above; adapters may wrap
those outcomes rather than replacing them or parsing message text. This proposal
does not assign new `PhysicsErrorCode` enumerators or claim a diagnostic-record
API. Unsupported values must not silently fall back, and failed construction
must clean up partial resources before publication.

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
