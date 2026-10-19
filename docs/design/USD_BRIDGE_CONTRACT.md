# OpenUSD physics bridge contract

> **Decision: proposed, revised 2026-10-19.** This document owns USD reading,
> conversion and rejection rules, not implementation status. See the
> [capability matrix](../reference/CAPABILITY_MATRIX.md) for current capabilities
> and the [roadmap](../roadmap/current.md) for ordered unfinished work.

## Bounded snapshot contract

The bounded API contract names `usd_physics::usd::readPrimitiveScene()` and
`PrimitiveBody` in `usd_physics/usd/box_scene.h`. `readBoxScene()` is Cube-only,
and `PrimitiveBody` aliases `BoxBody`. Both return validated neutral shape,
motion, transform and mass values sorted by absolute prim path. The reader owns no
world, handles, persistent mapping, or USD edits. Callers must keep the result
associated with its source Stage and discard it at reload; the consumer owns
stage-generation identity and synchronization.

The subset requires Y-up meters/kg. Dynamic bodies use positive explicit mass
or infer mass from their scaled collision volume and body-local density.
Zero or absent mass requests inference; zero or absent density uses USD's
fallback of 1000 kg/m³. A positive explicit mass overrides density. Cube volume
is the product of its full extents, Sphere volume is `4πr³/3`, and Capsule
volume is `πr²h + 4πr³/3`, where `h` excludes the caps. Collider offsets,
rotations and capsule axes do not affect volume. Mass/density must be readable,
time-independent, finite and non-negative; the inferred result must be positive
and finite. These rules follow the
[OpenUSD mass specification](https://openusd.org/release/api/usd_physics_page_front.html).
Collision may be on the body geometry or on one descendant geometry of
a body Xform. In the latter case the body's composed world frame must be rigid.
The collider's composed relative translation and rotation become
`ShapeDescriptor::localTransform`; reset stacks below the body are rejected.
The returned path and transform always identify the body frame. Shape dimensions absorb
Cube size and scale, including reflections. A direct collision Cube may have
composed scale and rotation if the resulting axes are orthogonal.

`readPrimitiveScene()` admits Cube, Sphere and Capsule. Sphere and Capsule
require uniform composed collider scale, within `1e-9` relative to the largest
axis scale. Nonuniform scale rejects rather than approximating ellipsoids or
distorted caps. Uniform reflections preserve geometry. Radius must be positive
and finite; capsule height is finite and non-negative and excludes the caps.
The X/Y/Z capsule axis is converted to the core's local Y through the shape
rotation without changing the body frame. Zero height yields sphere geometry.
Time-sampled radius, height and axis reject, as do invalid axis tokens.

Time-independent translate, rotate, orient, scale and matrix ops compose
through ancestors until `resetXformStack`; an empty stack is identity.
Unreadable, non-finite, singular, sheared, perspective and inverse-op
transforms are rejected. Rigid-frame scale and normalized-axis orthogonality
checks use a tolerance of `1e-9`.
Animated ancestors beyond a reset do not participate. Collision-only geometry
and disabled rigid bodies are static; disabled collision-only geometry is
skipped. A rigid body with disabled collision is rejected by this bounded
contract. Enabled bodies with `physics:kinematicEnabled = true` map to
`MotionType::kinematicBody` without requiring mass; their snapshot mass
is the neutral default and is ignored. Disabled rigid bodies remain static
even with the kinematic flag. The flag must be time-independent. The reader
does not sample animation or issue runtime movement commands.
Animated declarations, scenes in the body-only readers,
instances, nested or compound bodies, collider mass overrides,
inherited mass/density, physics materials and their bindings, and all other
authored `physics:*` properties are rejected. Collider MassAPI overrides,
inertia and center-of-mass overrides reject in this contract. Static and
kinematic snapshots do not consume mass/density. This is a bounded volume-based inference
slice, not the full hierarchical USD mass-properties algorithm.

`readPhysicsScene()` adds a `PhysicsScene` snapshot with path-sorted `bodies`
and `joints`, plus optional `SceneSettings`; the body-only APIs continue to
reject scenes and joints. The settings contain the absolute scene path and a
plain world-space gravity acceleration in meters/second squared, without USD
objects or runtime resources. Scene-prim and ancestor transforms do not rotate
or scale gravity, which USD declares in simulation world space.

Scene selection uses default stage traversal: active, loaded, defined,
non-abstract `UsdPhysicsScene` prims in the composed stage. No scene produces
absent settings; the consumer chooses its
world defaults. Exactly one scene is admitted. Multiple scenes reject with
`unsupportedDeclaration` at the second scene in lexical path order; there is
no implicit first/default scene selection or simulation-owner routing. Instances,
derived scene schemas, physics APIs applied to a scene and authored physics
properties other than `physics:gravityDirection` and `physics:gravityMagnitude`
reject at the scene path. Gravity also requires Y-up meters/kg, including in a
stage with no bodies; incompatible units reject with `unsupportedUnits` at
the scene path. Unit/axis conversion beyond this subset is not approximated.

Gravity attributes must be readable and time-independent. A finite nonzero
direction is normalized; a zero direction requests negative Y. A negative
magnitude, including USD's default negative infinity, requests earth gravity
of 9.81 m/s². Zero magnitude disables gravity. Non-negative finite magnitude
is used directly in meters/second squared; positive infinity, NaN and
non-finite direction components reject with `invalidValue` at the scene path.
These rules implement the defaults documented by `UsdPhysicsScene`; reading
does not author defaults. Scene settings and the remaining snapshot must all
validate before a result is returned.

`Joint` represents
generic `PhysicsJoint` or `PhysicsFixedJoint` declarations, not executable
`physicsCore` constraints. Endpoints are exact rigid-body paths with supported
body snapshots, or an empty world anchor. Two world anchors, identical bodies,
multiple or missing targets, descendant geometry targets and scaled/reflected
body frames reject. Forwarded relationships are resolved. Joint frames remain
relative to body origins (world for an empty endpoint); finite nonzero
quaternions are normalized. Joint-prim ancestor transforms do not alter these
body-relative attributes.

The snapshot preserves `jointEnabled` and `collisionEnabled`. Generic joints
accept `PhysicsLimitAPI` on `transX/Y/Z` and `rotX/Y/Z`; translation is meters,
rotation is converted from USD degrees to radians. Absent API means free;
infinite bounds are preserved, NaN rejects, equal bounds constrain to that
value, and inverted bounds retain USD's locked-axis meaning. Fixed joints
lock every axis and leave the limit array unused. Additional LimitAPI on a
fixed joint, distance limits, drives, breaking thresholds, other joint types,
unsupported joint physics properties/APIs and animated values reject, even
for disabled joints. Non-physics metadata is not interpreted. In particular,
format-only joints with no standard schema are outside this snapshot.

This reader creates no runtime constraints. The separate stateless
`toSixDofConstraint()` conversion in `usd_physics/usd/joint_conversion.h` maps
a snapshot and consumer-resolved endpoint handles to an optional neutral
descriptor. Disabled joints return no descriptor; endpoint path/world-handle
mismatches reject. Fixed joints lock all six axes. Absent limits or fully
unbounded `[-infinity, infinity]` limits are free; inverted bounds and zero
equal bounds are locked. Nonzero equal bounds, one-sided infinite bounds,
and angular ranges outside the core's open `(-pi, pi)` interval reject with
`unsupportedDeclaration` at the joint path. Snapshot readers keep their wider
declaration subset. Conversion edits no USD and creates no resources.

The core's twist/pyramid-swing rotation convention is explicit in the
[rigid-body contract](RIGID_BODY_CONTRACT.md#54-optional-six-dof-constraints).
Executing those limits does not establish PMX Euler-limit equivalence.
Source-format filtering, coefficient mapping, springs and bone synchronization
belong to consumer adapters. Consumer code owns creation, rollback, handles and
lifetimes; no retained stage mapping is added.

For this surface, `USD-O2` uses typed `ImportErrorCode` plus the offending path
on `ImportError`: `invalidStage`, `unsupportedUnits`, `unsupportedDeclaration`,
`unsupportedTransform`, and `invalidValue`. Consumers assert code/path, not
diagnostic prose. Validation completes before a snapshot is returned.

## 1. Purpose

`physicsUsd` translates standard OpenUSD physics declarations into neutral
snapshots containing `physicsCore` values and descriptors. It does not retain runtime worlds or
resource mappings; consumers own orchestration and cleanup.

It is an adapter, not a solver and not an application scheduler.

## 2. Input authority

Standard schemas are authoritative wherever they express the concept:

- `UsdPhysicsScene` for world settings;
- `UsdPhysicsRigidBodyAPI` and `UsdPhysicsMassAPI` for bodies and mass;
- `UsdPhysicsCollisionAPI` plus `UsdGeom` geometry for collision;
- standard fixed, revolute, prismatic, spherical, and distance joints;
- `UsdPhysicsLimitAPI` and `UsdPhysicsDriveAPI` for limits and drives.

The bridge does not interpret PMX structures, MMD body modes,
`VrmSpringBoneAPI`, or backend configuration attributes.

## 3. Initial planned slice

The minimal end-to-end acceptance scenario is a static floor, dynamic Box,
scene gravity and the minimum necessary joint. The bounded snapshot contract
above defines reader admission/rejection independently of this target scenario.

The bounded snapshot contract above defines scene selection, finite
acceleration, unit and axis handling, and typed rejection.
Additional geometry and joints require a real consumer and a minimal fixture,
rather than breadth for its own sake.

## 4. Translation pipeline

```text
USD prims -> physicsUsd validation -> neutral snapshot
                                          |
                                          v
                         consumer creates world/resources
                                          |
                                          v
                            step -> state -> presentation
```

Snapshot validation completes before return. The consumer constructs resources
in a temporary world and exposes it only after successful construction. On
failure it destroys partial resources and reports a diagnostic code and prim
path; no untracked or partially usable world escapes.

## 5. Identity and mapping

The consumer owns temporary mappings from absolute prim paths to shape, body
and constraint handles, associated with the source stage instance/generation.
A path from a reloaded stage must not resolve to the prior stage's resources.
Backend IDs never appear in USD or neutral snapshots.

On reload, explicitly destroy the old resources and fully rebuild. Cleanup
must respect dependent constraints and bodies; stale handles must reject.
Persistent mapping machinery and subtree updates are not prerequisites for
this initial execution path. A separate thin orchestration component may be
considered only when actual consumers demonstrate reusable behavior.

## 6. Transform conversion

The bridge owns USD-to-runtime transform conversion. The long-term contract
covers composed translation, orientation, scale, parent transforms,
`resetXformStack`, collider-local transforms, and world/local conversion.

The first implementation may reject unsupported scale or composition cases,
but every rejection rule is specified here and tested. The capability matrix
links to this contract rather than duplicating its full rules. It does
not silently flatten a transform incorrectly.

Stage units and up axis are converted once at the boundary. Values inside
`physicsCore` use the core's declared units and basis.

## 7. Instancing and prototypes

Native instancing, point instancing, instance proxies, and prototype sharing
are outside the first slice. Their identity and ownership semantics must be
decided before support is claimed.

This is tracked as `USD-O1`.

## 8. Change processing

The initial consumer path uses explicit destruction and full rebuild on stage
reload. Continuous stage-delta monitoring and incremental synchronization are
not required. They enter only after a consumer demonstrates a need and defines
invalidation for geometry, materials, bodies, joints, transforms and settings.
Notification ordering must never determine correctness.

## 9. Synchronization and writeback

Stage evaluation time and solver timestep are separate inputs. The consumer
owns fixed stepping, animation order and presentation of runtime body state.
Validate unit, scale and coordinate conversion once at the boundary.

Reading must not mutate authored USD. Display transforms may consume solver
state without automatic USD writeback. Any later authoring integration belongs
to the consumer, including edit targets, session-layer policy and prevention of
feedback loops. A synchronization-record API requires demonstrated shared need.

## 10. Diagnostics

Unsupported schema combinations, invalid dimensions, missing joint targets,
non-finite data, unsupported transforms, and lossy conversions produce stable
diagnostic identifiers. Tests assert identifiers rather than prose.

The bounded snapshot surface uses `ImportErrorCode` and a prim path on
`ImportError`, as specified above. Future surfaces must retain typed code/path
diagnostics; callers must not parse prose. This resolves `USD-O2` for snapshots.

## 11. Custom schema boundary

The bridge may consume an admitted generic physics extension from
`physicsSchema`. It must not grow direct dependencies on MMD, VRM, Stage
Runner, or a backend-specific schema.

Source-format-specific adapters remain in their owning repository or in an
application composition layer and translate into standard USD or neutral
descriptors.

## 12. Open questions

- **USD-O1:** identity and resource sharing for native instances, instance
  proxies, and point instancers.
- **USD-O2 (resolved for snapshots):** typed import code and prim path; retain
  that convention when extending the reader.
- **USD-O3:** application-facing synchronization record and feedback-loop
  boundary.
- **USD-O4 (resolved for bounded snapshots):** zero scenes means absent
  settings, one scene is selected, and multiple scenes reject. Explicit
  multi-scene selection and simulation-owner routing require a consumer need.
- **USD-O5:** material binding and precedence for friction and restitution.
