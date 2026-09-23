# Capability matrix

This page is the only source of truth for what the current repository tree
implements. Design documents describe intended behavior; they do not upgrade a
capability on this page.

> **Tree status (2026-09-23): Phase 2 complete; Phase 3 migration in progress.**
> Buildable and installable `physicsCore` and `physicsJolt` package boundaries
> exist.
> `physicsCore` provides neutral rigid-transform values, typed handles, and
> validated box/body/fixed-constraint descriptors with semantic collision
> filters, a single-owner world contract, and optional segment and ground
> query interfaces. `physicsJolt` implements those contracts when a compatible
> Jolt package is present and retains a typed unavailable fallback otherwise.
> The Jolt-required OpenStrata intent passes locally and in hosted Windows and
> Linux CI. Stage Runner has not migrated to the installed packages, and an
> OpenUSD plugin does not exist yet.

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
| Shape descriptors | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers the validated box descriptor; sphere/capsule remain Phase 4 |
| Static and dynamic body descriptors | supported | [`physics_core_descriptors_test.cpp`](../../libs/physicsCore/tests/physics_core_descriptors_test.cpp) covers shape identity, motion type, rigid transform, mass, and collision filter validation; [`physics_jolt_backend_test.cpp`](../../backends/physicsJolt/tests/physics_jolt_backend_test.cpp) covers backend lifecycle and filtering |
| Fixed-constraint descriptors | supported | Core tests cover two distinct valid body handles; the Jolt backend test covers creation and dependent cleanup |
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
| Jolt body/shape/constraint conversion | supported | Box shapes, static/dynamic bodies, normalized rotations, mass, semantic filters, fixed constraints, force/velocity commands, stepping, state, and cleanup pass focused Windows and Linux tests |
| Jolt collision and support queries | supported | Closest segment hit, ignored body, support identity, upward normal, distance, validation, and cross-world rejection pass focused Windows and Linux tests |
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
