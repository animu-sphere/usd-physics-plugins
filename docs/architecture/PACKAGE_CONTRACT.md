# Installed package contract

> **Status: accepted, revised 2026-09-23.** `physicsCore` and `physicsJolt`
> install and pass a clean-prefix consumer test. `physicsCore` now includes
> neutral values, typed handles, validated descriptors, typed errors, and the
> single-owner world plus optional segment and ground query interfaces and
> validation surfaces. A Jolt-enabled `physicsJolt` install also resolves its
> private SDK dependency and constructs a world through `createWorld()`.

## 1. Purpose

Each reusable component is consumable from a clean install prefix through
plain CMake and through an equivalent OpenStrata library identity. Consumers do
not need this repository's source tree.

## 2. Planned packages

| Package | Imported target | Provides | Planned phase |
| --- | --- | --- | --- |
| `physicsCore` | `physicsCore::physicsCore` | Version surface plus neutral vector, quaternion, transform, typed handles, validated box/body/fixed-constraint descriptors, semantic collision filters, typed errors, world lifecycle/state, and optional segment and ground query interfaces | Phase 1 surface present |
| `physicsJolt` | `physicsJolt::physicsJolt` | `physicsJolt_BACKEND_AVAILABLE` package metadata, `backendAvailable()`, and `createWorld()`; a Jolt-enabled build provides shapes, bodies, fixed constraints, commands, stepping, changed state, segment queries, and ground queries, while a no-Jolt build reports typed unavailability | Phase 2 complete |
| `physicsUsd` | `physicsUsd::physicsUsd` | Validated primitive body and generic/fixed joint snapshots; mappings and synchronization records remain planned | Phase 4 subset implemented |
| `secondaryMotion` | `secondaryMotion::secondaryMotion` | generic secondary-motion contracts | 6, if admitted |
| `secondaryMotionVerlet` | `secondaryMotionVerlet::secondaryMotionVerlet` | first CPU solver | 6, if admitted |
| `physicsSchema` | bundle contract to be defined at admission | generated schema library and resources | only if admitted |

The public include root and namespace are fixed by
[WORKSPACE.md §4](WORKSPACE.md#4-namespace-and-include-policy).

## 3. Install-interface rules

The optional `physicsUsd` target builds with `USDPHYSICS_BUILD_USD=ON` in the
root or as a standalone `libs/physicsUsd` project. It exports public headers
under `usd_physics/usd/`, links only `physicsCore` and OpenUSD `usd`, `usdGeom`,
and `usdPhysics` targets (also accepting `pxr::` names), and resolves those
dependencies from its installed config. The clean-prefix consumer includes a
standard Cube and parent-body/child-Cube imports when this option is enabled.
The bounded transform and geometry contract is defined by the
[USD bridge contract](../design/USD_BRIDGE_CONTRACT.md). Its OpenStrata member is
declared but has not entered the published release/artifact set.

The local collider-pose increment appends `localTransform` to
`ShapeDescriptor`, defaulting to identity so existing source initializers
retain their meaning. Rebuild core, backend, bridge and consumers together:
this descriptor layout addition is not binary-compatible with older static
packages. It is a local package increment, not a new published artifact pin.

The local primitive increment also appends `radius` and `halfHeight` and adds
`ShapeType::sphere` and `ShapeType::capsule`. Existing box source initializers
retain their meaning; rebuild all packages and consumers together for this
layout as well. `physicsUsd` adds `readPrimitiveScene()` in `box_scene.h`,
retaining the Cube-only `readBoxScene()` surface. No artifact pin is updated.

The local kinematic increment adds `MotionType::kinematicBody` and the optional
`KinematicMotion` interface in `usd_physics/core/kinematic_motion.h`.
`physicsJolt` exposes it through capability discovery; `PhysicsWorld`'s virtual
surface and existing enum values retain their meaning. Rebuild packages and
consumers together to use the new header and implementation. The installed
consumer exercises capability discovery and a target movement. No published
version or artifact pin changes.

The local joint increment adds `PhysicsScene`, `Joint`, `JointType`,
`JointLimit` and `readPhysicsScene()` in `box_scene.h`. It retains both body-only
APIs and their joint rejection. Generic and fixed joint declarations include
endpoint paths, body-local frames and six-axis limits in meters/radians. These
snapshot types do not extend the core constraint or backend solver interface.
Rebuild the bridge and consumers together; no published artifact pin changes.

The local six-DOF increment adds `usd_physics/core/six_dof.h` with validated
axis limits, descriptors and the optional `SixDofConstraints` capability,
implemented by `physicsJolt` when explicitly built with
`PHYSICSJOLT_ENABLE_SIX_DOF=ON` (default OFF). Its installed config reports
`physicsJolt_SIX_DOF_AVAILABLE`; capability discovery agrees with that metadata.
`physicsUsd` installs `joint_conversion.h` with
the stateless `toSixDofConstraint()` conversion. The mandatory world interface,
legacy fixed descriptors and dependency graph are unchanged. Rebuild packages
and consumers together; public versions and artifact pins remain unchanged.

The local translation-spring increment appends three neutral
`TranslationSpring` values to `SixDofConstraintDescriptor` (N/m, N*s/m, meters).
Its layout changes; rebuild core, backend, bridge and consumers together into
a separate prefix. Source initializers retain disabled springs. The existing
six-DOF option gates execution; versions, public pins and dependencies stay
unchanged. The installed consumer exercises a nonzero equilibrium spring.

The local contact-material increment appends `ContactMaterial contactMaterial`
to `BodyDescriptor` in `descriptors.h`, plus `validateContactMaterial()`.
Old source initializers retain their default contact behavior. This changes
the descriptor's binary layout: rebuild core, backend, bridge and every
consumer together. Pair mixing and numeric domains are defined by
[the rigid-body contract](../design/RIGID_BODY_CONTRACT.md#52-bodies).
There is no new published version or artifact pin. The USD reader gains no
format-specific metadata interpretation or standard material binding support.

The local damping increment appends `BodyDamping damping` to `BodyDescriptor`
and adds `validateBodyDamping()`. It preserves old source initializers and
Jolt defaults, but changes binary layout again: rebuild all three packages
and consumers together in a separate prefix. Rates use inverse seconds, not
source-format retention fractions. Versions and published pins are unchanged;
the generic USD reader gains no format metadata interpretation.

The contact-mixing increment adds `ContactCombineRule`, `ContactMixing` and
`validateContactMixing()` to the existing core descriptor header, plus
`physicsJolt::createWorld(const ContactMixing&)`. The no-argument factory and
body descriptor layout are unchanged. The no-Jolt package validates the policy
then reports typed backend unavailability. The installed consumer exercises
the overload in both builds. Use rebuilt packages in a separate prefix;
public versions and artifact pins remain unchanged. Format-specific policy
selection stays in the caller, outside `physicsUsd`.

Every library package must:

- install public headers under one repository-owned include root;
- export a namespaced CMake target;
- publish accurate transitive dependencies through its target interface;
- install a config and compatible-version file;
- avoid absolute source/build paths in exported metadata;
- work from a clean prefix in an installed-consumer test;
- match the dependency edges in `openstrata.library.yaml`.

Backend SDKs are private unless their symbols are unavoidably part of the
backend package link interface. They never become dependencies of
`physicsCore`.

## 4. Consumer examples

The intended neutral consumer shape is:

```cmake
find_package(physicsCore CONFIG REQUIRED)
target_link_libraries(my_runtime PRIVATE physicsCore::physicsCore)
```

A consumer selecting Jolt adds it explicitly:

```cmake
find_package(physicsJolt CONFIG REQUIRED)
target_link_libraries(my_runtime PRIVATE physicsJolt::physicsJolt)
```

The installed-consumer test verifies both package imports. A Jolt-enabled
consumer must also make the selected Jolt installation discoverable to CMake;
the installed `physicsJolt` config resolves that declared dependency.

## 5. Versioning

Packages begin pre-1.0. Minor versions may evolve source APIs, but changes are
documented and kept coherent across packages released together. Breaking
descriptor semantics or handle/lifecycle guarantees require an explicit
version change and migration note.

Compatibility mode for Stage Runner's old include paths or namespaces, if
needed, belongs in Stage Runner or a time-limited adapter package. It does not
become the permanent public surface here.

## 6. Bundle and runtime discovery

An admitted OpenUSD plugin installs its resources so `PlugRegistry` can
discover them from the installed bundle without a source-tree path. The exact
bundle contract is added only when `physicsSchema` passes admission.

`physicsCore` and solver libraries are ordinary CMake packages; loading them
must not require OpenUSD plugin discovery.

## 7. Acceptance

A package changes from `not present` to `supported` only when a clean consumer:

1. configures using only the install prefix and declared external dependencies;
2. includes its public headers;
3. links its imported target;
4. runs a minimal behavior test;
5. proves no source-tree or build-tree path leaked into the export.
