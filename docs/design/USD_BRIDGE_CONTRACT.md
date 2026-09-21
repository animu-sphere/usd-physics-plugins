# OpenUSD physics bridge contract

> **Status: proposed, 2026-09-21.** `physicsUsd` is planned for Phase 4 and is
> not present. See the [capability matrix](../reference/CAPABILITY_MATRIX.md).

## 1. Purpose

`physicsUsd` translates standard OpenUSD physics declarations into
`physicsCore` descriptors and maintains transient mappings between scene
identities and runtime resources.

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

Phase 4 starts with:

- one selected `UsdPhysicsScene` and gravity;
- box, sphere, and capsule collision geometry;
- static and dynamic rigid bodies;
- authored mass;
- fixed joints;
- deterministic resource creation and destruction;
- body-state synchronization through an application-provided destination.

Additional geometry and joint types enter through fixtures that define exact
conversion and failure behavior.

## 4. Translation pipeline

```text
USD prims
  -> validated bridge model
  -> physicsCore descriptors
  -> runtime handles
  -> solver state
```

Parsing and validation complete before the bridge mutates a live world when
practical. A failed prim must not leave an untracked backend resource.

## 5. Identity and mapping

The bridge maintains bidirectional transient mappings:

```text
scene identity <-> BodyHandle / ShapeHandle / ConstraintHandle
```

The scene identity may use `SdfPath` with an explicit stage-generation token,
or another type that prevents a path from one stage instance being mistaken
for the same path after reload. Backend IDs never appear in USD.

Mappings support lookup, deletion, subtree change, stage reload, complete
reset, and deterministic rebuild. Deleting a prim removes dependent resources
in a valid lifecycle order.

## 6. Transform conversion

The bridge owns USD-to-runtime transform conversion. The long-term contract
covers composed translation, orientation, scale, parent transforms,
`resetXformStack`, collider-local transforms, and world/local conversion.

The first implementation may reject unsupported scale or composition cases,
but every restriction is listed in the capability matrix and tested. It does
not silently flatten a transform incorrectly.

Stage units and up axis are converted once at the boundary. Values inside
`physicsCore` use the core's declared units and basis.

## 7. Instancing and prototypes

Native instancing, point instancing, instance proxies, and prototype sharing
are outside the first slice. Their identity and ownership semantics must be
decided before support is claimed.

This is tracked as `USD-O1`.

## 8. Change processing

The first implementation may use full rebuild on relevant stage changes.
Incremental change processing is an optimization only after rebuild behavior
is correct and deterministic.

Incremental processing must define dependency invalidation for geometry,
materials, body APIs, joint endpoints, inherited transforms, and scene
settings. It may not depend on notification order for correctness.

## 9. Synchronization and writeback

`physicsUsd` may produce neutral synchronization records containing scene
identity and changed body state. The owning application decides when and where
to author or apply them.

The bridge must avoid a feedback loop in which solver writeback is immediately
re-imported as an authored edit. Session-layer policy, edit targets, and
scheduling belong to the application integration unless a reusable contract
is proven.

## 10. Diagnostics

Unsupported schema combinations, invalid dimensions, missing joint targets,
non-finite data, unsupported transforms, and lossy conversions produce stable
diagnostic identifiers. Tests assert identifiers rather than prose.

The diagnostic prefix and transport are `USD-O2` and must be fixed before
Phase 4 public API is accepted.

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
- **USD-O2:** stable diagnostic namespace and result transport.
- **USD-O3:** application-facing synchronization record and feedback-loop
  boundary.
- **USD-O4:** selection rules when a stage contains multiple physics scenes.
- **USD-O5:** material binding and precedence for friction and restitution.
