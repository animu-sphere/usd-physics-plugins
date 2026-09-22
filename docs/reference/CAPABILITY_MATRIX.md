# Capability matrix

This page is the only source of truth for what the current repository tree
implements. Design documents describe intended behavior; they do not upgrade a
capability on this page.

> **Tree status (2026-09-22): Phase 1 core extraction.** Buildable and
> installable `physicsCore` and `physicsJolt` package boundaries exist.
> `physicsCore` provides neutral rigid-transform values, typed handles, and
> validated box/body/fixed-constraint descriptors with semantic collision
> filters plus a single-owner world contract. Optional queries, a solver
> backend, and an OpenUSD plugin do not exist yet.

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
| Installed-consumer test | supported | Root clean-prefix test and OpenStrata-generated consumers pass for both packages |
| Versioned release | not present | `VERSION` exists for package coherence; no tag or release record exists |

## 3. Rigid-body core

| Capability | Status | Evidence / note |
| --- | --- | --- |
| `physicsCore` package | partial | Installable `physicsCore::physicsCore` provides version, value, handle, descriptor, error, and world surfaces; optional queries remain Phase 1 |
| Neutral vector, quaternion, and transform values | supported | `physicsCore.values_handles` and the installed-consumer test cover the public headers and identity defaults |
| Opaque shape/body/constraint handles | supported | [`physics_core_values_handles_test.cpp`](../../libs/physicsCore/tests/physics_core_values_handles_test.cpp) covers invalid zero, typed identity, comparison, hashing, and representation size |
| Shape descriptors | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers the validated box descriptor; sphere/capsule remain Phase 4 |
| Static and dynamic body descriptors | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers shape identity, motion type, rigid transform, mass, and collision filter validation; lifecycle is covered by the partial world contract below, while no solver backend exists yet |
| Fixed-constraint descriptors | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers two distinct valid body handles; lifecycle is covered by the partial world contract below, while no solver backend exists yet |
| Force and velocity operations | partial | [`physics_core_world_test.cpp`](../../libs/physicsCore/tests/physics_core_world_test.cpp) covers the public command contract, validation, and dynamic/static/stale/cross-world outcomes through a deterministic contract world; no solver backend exists |
| Fixed-step world | partial | [`physics_core_world_test.cpp`](../../libs/physicsCore/tests/physics_core_world_test.cpp) covers the single-owner lifecycle interface and finite positive step validation; no solver backend exists |
| Direct body state | partial | [`physics_core_world_test.cpp`](../../libs/physicsCore/tests/physics_core_world_test.cpp) covers neutral state and unknown/stale/cross-world errors through the contract world; no solver backend exists |
| Changed-body extraction | partial | [`physics_core_world_test.cpp`](../../libs/physicsCore/tests/physics_core_world_test.cpp) covers unique handle ordering, drain behavior, creation exclusion, and destruction cleanup through the contract world; sleep, wake, and teleport are outside the Phase 1 API |
| Segment query capability | planned | No implementation |
| Ground query capability | planned | No implementation |
| Ray or shape-cast capabilities | planned | Later consumer-driven addition |
| Neutral collision filtering | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers 64-bit category/mask semantics and pair matching without backend layer identifiers; `RB-O3` is resolved |

## 4. Backends

| Capability | Status | Evidence / note |
| --- | --- | --- |
| `physicsJolt` package | partial | Installable `physicsJolt::physicsJolt` scaffold explicitly reports the backend unavailable; solver is Phase 2 |
| Jolt initialization and lifetime | planned | No implementation |
| Jolt body/shape/constraint conversion | planned | No implementation |
| Jolt collision and support queries | planned | No implementation |
| Alternative rigid-body backend | unsupported | No implementation phase committed |

## 5. OpenUSD bridge

| Capability | Status | Evidence / note |
| --- | --- | --- |
| `physicsUsd` package | not present | Planned Phase 4 |
| `UsdPhysicsScene` gravity | planned | No implementation |
| Box/sphere/capsule collision | planned | No implementation |
| `UsdPhysicsRigidBodyAPI` | planned | No implementation |
| `UsdPhysicsMassAPI` | planned | No implementation |
| Fixed joints | planned | No implementation |
| Other standard joints, limits, and drives | planned | Phase 5 expands from consumer fixtures |
| Prim/resource mappings | planned | No implementation |
| Stage reset/rebuild | planned | No implementation |
| Incremental USD change processing | unsupported | Rebuild-first policy; no implementation |
| USD state synchronization records | planned | `USD-O3` unresolved |

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
| Stage Runner falling body | planned | Phase 3 |
| Stage Runner character grounding and camera collision | planned | Phase 3 |
| MMD rigid bodies and joints through standard USD | planned | Phase 5 |
| VRM-derived secondary motion | planned | Phase 7 |
| Vehicle primitives | planned | Phase 8 |
