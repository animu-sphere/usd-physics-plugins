# Capability matrix

This page is the only source of truth for what the current repository tree
implements. Design documents describe intended behavior; they do not upgrade a
capability on this page.

> **Tree status (2026-10-18): Phase 2 complete; Phase 3 release follow-up and Phase 4 Box foundation in progress.**
> Buildable and installable `physicsCore` and `physicsJolt` package boundaries
> exist.
> `physicsCore` provides neutral rigid-transform values, typed handles, and
> validated box/sphere/capsule/body/fixed-constraint descriptors with semantic collision
> filters, a single-owner world contract, and optional segment and ground
> query interfaces. `physicsJolt` implements those contracts when a compatible
> Jolt package is present and retains a typed unavailable fallback otherwise.
> The Jolt-required OpenStrata intent passes locally and in hosted Windows and
> Linux CI. Stage Runner has migrated to installed core/backend packages. An
> optional installable `physicsUsd` primitive reader now exists; an OpenUSD plugin
> does not. Bridge artifact publication and hosted parity remain open.

## 1. Status vocabulary

| Status | Meaning |
| --- | --- |
| supported | Implemented. Executable behavior has an automated test or linked dated report; documentation can cite its owning page and link check. |
| partial | A documented subset is implemented and tested; the row states the limit. |
| experimental | Implemented for evaluation but not a supported contract. |
| unsupported | The component exists but intentionally does not provide the capability. |
| planned | Owned by an accepted/proposed phase, but not implemented. |
| not present | No implementing component exists in this tree. |

## 2. Workspace and delivery

| Capability | Status | Evidence / note |
| --- | --- | --- |
| Documentation taxonomy and source-of-truth rules | supported | [`docs/README.md`](../README.md) and [documentation guidelines](../contributing/documentation.md) |
| Plain CMake root build | supported | Linux configure/build/test evidence in the [Phase 0 report](../reports/2026-09-21-phase0-bootstrap.md) |
| OpenStrata workspace | supported | Root build/test plus isolated package verification in the [Windows OpenStrata report](../reports/2026-09-21-openstrata-windows.md) |
| Windows build | supported | MSVC 19.51 and OpenStrata evidence in the [Windows OpenStrata report](../reports/2026-09-21-openstrata-windows.md) |
| Linux build | supported | GCC 15.2.0 evidence in the [Phase 0 report](../reports/2026-09-21-phase0-bootstrap.md) |
| Hosted Windows and Linux CI | supported | Generated graph and test cells passed in the [hosted CI report](../reports/2026-09-22-phase0-hosted-ci.md) |
| Installed-consumer test | supported | Root clean-prefix test passes for both no-Jolt and Jolt-enabled package installs on Windows and Linux; see the [Windows](../reports/2026-09-22-phase2-windows-backend.md) and [Linux](../reports/2026-09-22-phase2-linux-backend.md) Phase 2 reports |
| Versioned release | not present | `VERSION` exists for package coherence; no tag or release record exists |

## 3. Rigid-body core

| Capability | Status | Evidence / note |
| --- | --- | --- |
| `physicsCore` package | supported | Installable `physicsCore::physicsCore` provides the complete Phase 1 version, value, handle, descriptor, error, world, segment-query, and ground-query surfaces; the installed-consumer test exercises every public header |
| Neutral vector, quaternion, and transform values | supported | `physicsCore.values_handles` and the installed-consumer test cover the public headers and identity defaults |
| Opaque shape/body/constraint handles | supported | [`physics_core_values_handles_test.cpp`](../../libs/physicsCore/tests/physics_core_values_handles_test.cpp) covers invalid zero, typed identity, comparison, hashing, and representation size |
| Shape descriptors | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers box dimensions, positive sphere/capsule radius, non-negative capsule cylinder half-height, and identity/finite/nonzero local pose validation |
| Collider-local pose in Jolt | supported | [`physics_jolt_backend_test.cpp`](../../backends/physicsJolt/tests/physics_jolt_backend_test.cpp) checks translated/rotated geometry by ray queries, contact settling, and body-origin state/changed-state reporting |
| Static, dynamic and kinematic body descriptors | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers shape identity, motion type, rigid transform, mass, and collision filter validation; [`physics_jolt_backend_test.cpp`](../../backends/physicsJolt/tests/physics_jolt_backend_test.cpp) covers backend lifecycle and filtering |
| Body contact coefficients | supported | `ContactMaterial` validates non-negative finite friction and restitution in `[0,1]`; `physicsJolt.material` checks friction on either body, unequal-pair mixing, bounce on either body, scalar rejection and cleanup. Local Windows evidence in the [contact report](../reports/2026-10-18-phase4-contact-material.md); standard USD material binding remains outside this slice |
| Contact pair mixing | supported | Neutral `ContactMixing` selects independent geometric-mean, maximum or product rules and an optional friction ceiling per world. `physicsJolt.mixing` checks asymmetric/equal/zero pairs, ceilings including 10, all rules, concurrently alive worlds and 60/240 Hz responses. Existing no-argument defaults and typed no-Jolt behavior pass installed-consumer tests. [Local evidence](../reports/2026-10-18-phase4-contact-mixing.md); hosted acceptance and source trajectory equivalence remain open |
| Body damping | partial | Neutral inverse-second `BodyDamping` rates, independently validated and passed to Jolt. `physicsJolt.damping` measures independent linear and torque-free angular decay at 60/240 Hz, scalar rejection and cleanup. Jolt uses finite-step exponential approximation; source conversion stays outside the shared reader. [Local evidence](../reports/2026-10-18-phase4-body-damping.md) |
| Kinematic motion capability | supported | Optional `KinematicMotion::moveKinematic`; [`physics_jolt_kinematic_test.cpp`](../../backends/physicsJolt/tests/physics_jolt_kinematic_test.cpp) checks body-origin targets with offset geometry, rotation, holding, dynamic contact, validation, handle rejection and changed-state cleanup; local Windows evidence only |
| Fixed-constraint descriptors | supported | Core tests cover two distinct valid body handles; the Jolt backend test covers creation and dependent cleanup |
| Neutral six-DOF descriptors | supported | `physicsCore.six_dof` checks body/world endpoints, local frames, free/locked/limited axes, finite ordered limits and angular range validation |
| Neutral translation springs | experimental | Three SI stiffness/damping/equilibrium axes; `physicsCore.six_dof` validates domains and locked-axis rejection. `physicsJolt.translation_spring` compares 60/240 Hz responses to analytic solutions across mass, stiffness, damping, rotated frames, world/static/two-dynamic endpoints, and hard limits. Numerical damping remains measurable; [local evidence](../reports/2026-10-18-phase4-translation-springs.md) |
| Neutral rotation springs | experimental | Three SI stiffness/damping pairs, identity equilibrium and second-frame axes. Core validation, 60/240 Hz small-angle analytic response, inertia/reduced-inertia scaling, rotated frames, swapped world/static endpoints, anisotropic settings, hard limits and installed-consumer tests pass locally. Finite-angle Euler/source equivalence remains unverified; [local evidence](../reports/2026-10-18-phase4-rotation-springs.md) |
| Jolt six-DOF capability | experimental | Explicit `PHYSICSJOLT_ENABLE_SIX_DOF=ON`; per-axis execution, offset COM, rotated frames, world endpoints, pair exclusion/restoration and cleanup pass with the deterministic SDK. An MSVC Jolt built with `/fp:fast` (non-deterministic) fails the Z-axis gate; GCC passes either way; [local evidence](../reports/2026-10-18-phase4-six-dof-execution.md), [SDK cause](../reports/2026-10-18-phase4-six-dof-sdk-cause.md); the `usd-jolt-six-dof` cells pass on hosted Windows and Linux ([hosted acceptance](../reports/2026-10-18-phase4-six-dof-hosted.md)) |
| Force and velocity operations | supported | Core tests cover validation and dynamic/static/stale/cross-world outcomes; the focused Jolt test covers admitted dynamic commands, static and cross-world rejection, and finite-vector validation |
| Fixed-step world | supported | Core validation plus the Jolt falling/settling test pass on Windows and Linux; see the Phase 2 backend reports |
| Direct body state | supported | Core contract tests and the Jolt falling, filtering, stale-handle, and cross-world tests pass on Windows and Linux |
| Changed-body extraction | supported | Core and Jolt tests cover unique handle ordering, drain behavior, creation exclusion, and destruction cleanup on Windows and Linux; sleep, wake, and teleport remain outside the current API |
| Segment query capability | supported | [`physics_core_queries_test.cpp`](../../libs/physicsCore/tests/physics_core_queries_test.cpp) covers optional capability discovery, finite distinct endpoints, an ignored body, and first-hit body/fraction validation |
| Ground query capability | supported | [`physics_core_queries_test.cpp`](../../libs/physicsCore/tests/physics_core_queries_test.cpp) covers optional capability discovery, body and probe-distance forwarding, and support body/normal/distance validation |
| Ray or shape-cast capabilities | planned | Later consumer-driven addition |
| Neutral collision filtering | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers 64-bit category/mask semantics and pair matching without backend layer identifiers; `RB-O3` is resolved |

## 4. Backends

| Capability | Status | Evidence / note |
| --- | --- | --- |
| `physicsJolt` package | supported | Installable `physicsJolt::physicsJolt` exposes `backendAvailable()` and `createWorld()`; Jolt-enabled and typed unavailable clean-prefix consumers pass on Windows and Linux, and the Jolt-required OpenStrata intent passes locally and in the [hosted backend report](../reports/2026-09-23-phase2-hosted-backend.md) |
| Jolt initialization and lifetime | supported | Reference-counted factory/type registration, per-world temporary allocator and job system, explicit resource cleanup, and typed errors are covered by Windows/Linux plain-CMake, local Windows OpenStrata, and [hosted Windows/Linux](../reports/2026-09-23-phase2-hosted-backend.md) evidence |
| Jolt body/shape/constraint conversion | supported | Box shapes, static/dynamic bodies, normalized rotations, mass, semantic filters, fixed constraints, force/velocity commands, stepping, state, and cleanup pass focused Windows and Linux tests. Sphere/capsule query and settling checks pass locally on Windows; [primitive report](../reports/2026-10-18-phase4-primitives.md) |
| Jolt collision and support queries | supported | Closest segment hit, ignored body, support identity, upward normal, distance, validation, and cross-world rejection pass focused Windows and Linux tests |
| Alternative rigid-body backend | unsupported | No implementation phase committed |

## 5. OpenUSD bridge

| Capability | Status | Evidence / note |
| --- | --- | --- |
| `physicsUsd` package | partial | Optional installed `physicsUsd::physicsUsd`; `physicsUsd.box_scene` and clean-prefix USD consumer test |
| Box artifact validation | partial | `usd-jolt` CI cells configure Windows/Linux builds and isolated packaging of core/backend/USD; [local Windows evidence](../reports/2026-10-04-phase4-usd-artifacts.md). Hosted execution and public USD artifacts pending |
| `UsdPhysicsScene` gravity | planned | No implementation |
| Box collision | partial | Cube size and composed scales, on the body Cube or one translated/rotated descendant Cube of a rigid body Xform; `physicsUsd.box_scene` checks all eight transformed corners, disabled static colliders, and invalid dimensions |
| Composed transforms | partial | Time-independent translate/rotate/orient/scale/matrix ops, ancestors, identity/body reset stacks and collider-local poses; shear, perspective, inverse ops and child reset stacks rejected. `physicsUsd.box_scene` checks intermediate Xforms and reflected scales |
| Sphere/capsule collision | partial | `readPrimitiveScene()` accepts direct or single-child Sphere/Capsule with uniform composed scale and local poses; X/Y/Z capsule axes and zero height are covered by `physicsUsd.box_scene`. Nonuniform scale, invalid/animated dimensions and axes reject. Jolt query, local rotation, scalar-range rejection and settling checks run in `physicsJolt.backend`; [local Windows evidence](../reports/2026-10-18-phase4-primitives.md) |
| `UsdPhysicsRigidBodyAPI` | partial | Enabled dynamic/kinematic or disabled static body on supported collision geometry or parent Xform; kinematic snapshots need no mass, animated flags, collider-free, nested, and compound bodies rejected |
| `UsdPhysicsMassAPI` | partial | Explicit positive dynamic mass in kg, or scaled single-primitive volume × body density (fallback 1000 kg/m³); zero mass/density ignored and positive mass wins. `physicsUsd.mass_scene` covers defaults, direct/child geometry, scale/reflection, Capsule caps, precedence, read-only behavior and typed invalid/animated/overflow rejection. Inherited/collider mass overrides, physics materials, inertia and center-of-mass overrides rejected |
| Fixed joints | partial | `readPhysicsScene()` snapshots exact body/world endpoints and local frames; no bridge resource creation. `physicsUsd.joint_scene` covers world anchors, defaults, validation and sorting |
| Generic joints and limits | partial | Standard `PhysicsJoint` plus six-axis `LimitAPI` snapshots; meters/radians, free/locked/equal/infinite bounds, normalized local rotations and flags. `physicsUsd.joint_scene` and clean-prefix USD consumer cover snapshots. Stateless `toSixDofConstraint()` adds a bounded executable conversion; `physicsUsd.joint_conversion` covers finite ranges, zero locks, fixed/disabled joints, handles and typed rejection. Wider equal/infinite declarations remain snapshot-only |
| Other standard joints and drives | planned | Distance limits, specialized joint types, drives and breaking thresholds reject |
| Prim/resource mappings | planned | No implementation |
| Stage reset/rebuild | planned | No implementation |
| Incremental USD change processing | unsupported | Rebuild-first policy; no implementation |
| USD state synchronization records | planned | `USD-O3` unresolved |

The [bridge contract](../design/USD_BRIDGE_CONTRACT.md) documents the bounded
subset. It requires Y-up meters/kg and explicit or inferred positive dynamic mass;
separate body Xforms must compose to a rigid world frame. Animated values,
instances, other standard
physics APIs/properties and scenes fail with typed code/path diagnostics.
Body-only readers still reject joints. Tests live in
[box_scene_test.cpp](../../libs/physicsUsd/tests/box_scene_test.cpp) and
[joint_scene_test.cpp](../../libs/physicsUsd/tests/joint_scene_test.cpp), plus
[mass_scene_test.cpp](../../libs/physicsUsd/tests/mass_scene_test.cpp).
Latest local Windows validation is recorded in the
[six-DOF execution report](../reports/2026-10-18-phase4-six-dof-execution.md).
The parser never edits USD or creates resources; returned paths are snapshot
identities belonging to the caller's Stage, not persistent runtime mappings.

## 6. Secondary motion

| Capability | Status | Evidence / note |
| --- | --- | --- |
| `secondaryMotion` package | not present | Deferred to Phase 6 |
| Generic spring-chain descriptors | planned | Contract requires validation against a real VRM slice |
| Deterministic CPU solver | planned | Algorithm not yet accepted |
| Generic collider interaction | planned | `SM-O1` unresolved |
| VRM adapter | unsupported in this repository | VRM interpretation remains outside the generic core |

## 7. Schema extensions

| Capability | Status | Evidence / note |
| --- | --- | --- |
| `physicsSchema` bundle | not present | No proposal has passed the schema admission rule |
| Backend-specific authored properties | unsupported | Forbidden by design policy |

## 8. Consumer validation

| Scenario | Status | Evidence / note |
| --- | --- | --- |
| Stage Runner falling body | supported | Installed core/backend migration verified in the [Phase 3 artifact report](../reports/2026-09-23-phase3-linux-artifacts.md); standard Box consumer remains opt-in |
| Stage Runner character grounding and camera collision | supported | Same installed-package consumer evidence; standard/compatibility parity is covered in Stage Runner's optional `stage_runtime.standard_physics_parity` test |
| MMD rigid bodies and joints through standard USD | experimental | Installed consumer creates three bodies/two constraints for all 22 generated physics-bearing fixtures and steps 120 times with finite state using the deterministic SDK. The source adapter supplies collision filters, contact coefficients and converted damping rates outside the generic reader. Unit source damping (instant stopping), finite-step/source-solver equivalence, format-only joints, pair-mixing fidelity, springs and bone coupling remain open. [Damping evidence](../reports/2026-10-18-phase4-body-damping.md); full Phase 5 acceptance remains open |
| VRM-derived secondary motion | planned | Phase 7 |
| Vehicle primitives | planned | Phase 8 |
