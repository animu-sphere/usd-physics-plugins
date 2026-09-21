# Workspace contract

This document is the proposed binding contract for the planned
`usd-physics-plugins` workspace: component identities, directories, dependency
directions, build modes, and structural invariants. Once accepted, a change
that contradicts it updates this document first, in a focused change.

> **Status (2026-09-21): proposed; implementation not started.**
> Every identity below is reserved, not present. Creation status is recorded in
> the [capability matrix](../reference/CAPABILITY_MATRIX.md).

## 1. Identities

### 1.1 Initial rigid-body path

| Identity | Kind | Directory | Planned manifest | Role | Created in | Status |
| --- | --- | --- | --- | --- | --- | --- |
| `physicsCore` | plain static CMake library | `libs/physicsCore/` | `openstrata.library.yaml` | Solver-neutral value types, handles, descriptors, world lifecycle, state, validation, and optional query contracts. No OpenUSD or backend SDK. | Phase 1 | reserved |
| `physicsJolt` | plain static CMake library | `backends/physicsJolt/` | `openstrata.library.yaml` | Jolt implementation of `physicsCore`; owns Jolt initialization, filters, jobs, resources, stepping, and queries. | Phase 2 | reserved |
| `physicsUsd` | plain static CMake library | `libs/physicsUsd/` | `openstrata.library.yaml` | Standard `UsdPhysics` to core descriptors, transform conversion, transient scene/resource mapping, and synchronization records. | Phase 4 | reserved |

Phase 0 may scaffold `physicsCore` and `physicsJolt` targets so the workspace
builds, but a scaffold is reported as `not present` or `partial`, never as a
supported runtime capability.

### 1.2 Later, only when the responsibility is real

| Identity | Kind | Directory | Role | Created when |
| --- | --- | --- | --- | --- |
| `secondaryMotion` | plain static CMake library | `libs/secondaryMotion/` | Format-neutral spring-chain, collider, solver-state, and step contracts. | Phase 6 begins with a tested generic descriptor slice. |
| `secondaryMotionVerlet` | plain static CMake library | `backends/secondaryMotionVerlet/` | First deterministic CPU implementation of `secondaryMotion`. | Phase 6 has known-answer solver tests. |
| `physicsSchema` | OpenUSD schema plugin bundle | `plugins/physicsSchema/` | Narrow semantic extensions to standard USD physics. | A proposal passes [the schema admission rule](../design/DESIGN_POLICY.md#9-schema-admission-rule). |

Empty architectural placeholders are not created before their admission
condition. Libraries and bundles use lower-camel identities matching their
directory and exported package identity.

## 2. Dependency directions

### 2.1 Allowed edges

```text
physicsCore ------------------> no repository component; no OpenUSD; no SDK

physicsJolt ------------------> physicsCore, Jolt

physicsUsd -------------------> physicsCore, OpenUSD
                                 physicsSchema only if admitted

secondaryMotion -------------> no format repository; neutral shared math only
secondaryMotionVerlet --------> secondaryMotion

physicsSchema ----------------> OpenUSD only
```

Consumers point inward:

```text
usd-stage-runner -----> installed physicsCore and physicsJolt packages
usd-mmd-plugins ------> standard UsdPhysics authored data
usd-vrm-plugins ------> a generic adapter contract at composition time
usd-avatar-runtime ---> composes format adapters, runtimes, and this repository
```

`physicsCore` may use repository-owned math value types. If Phase 1 proves that
those types deserve their own identity, `WORKSPACE.md` changes before the new
target is created; `runtimeCore` is not reused because it would invert the
repository boundary.

### 2.2 Forbidden edges

| Edge | Why |
| --- | --- |
| `physicsCore -> OpenUSD` | the solver contract must work without a stage |
| `physicsCore -> Jolt` or another backend SDK | consumers must not compile against a backend to use neutral types |
| `physicsCore -> usd-stage-runner` | the extracted library cannot depend on its former host |
| any component -> `usd-mmd-plugins` or `usd-vrm-plugins` | source-format semantics stay in their owning repositories |
| `physicsJolt -> OpenUSD` | the backend consumes core descriptors, not scene objects |
| `physicsUsd -> Jolt` | the USD bridge and solver backend are independently replaceable |
| `physicsUsd -> Stage Runner runtime types` | scene mapping is reusable and must not target one host's world |
| `secondaryMotion -> VRM schemas` | VRM interpretation belongs in a VRM-aware adapter |
| a public header -> backend-native identifier or settings type | backend replacement must not rewrite consumers |
| a component -> a sibling source checkout | repositories are consumed as installed packages or OpenStrata members, never private source trees |

### 2.3 Enforcement

Each component declares edges in CMake and its OpenStrata manifest. Phase 0
adds boundary checks with each scaffold; Phase 1 proves `physicsCore` has no
OpenUSD, Stage Runner, MMD, VRM, or backend includes and no corresponding link
dependency. Phase 2 proves Jolt remains private to `physicsJolt`.

The checks must inspect source includes and resolved link interfaces. A prose
rule alone is not sufficient.

## 3. Planned repository layout

```text
usd-physics-plugins/
|- CMakeLists.txt
|- CMakePresets.json
|- VERSION
|- openstrata.toml
|- openstrata.ci.yaml
|- docs/
|  |- architecture/
|  |- contributing/
|  |- design/
|  |- reference/
|  `- roadmap/
|- libs/
|  |- physicsCore/
|  `- physicsUsd/                 # Phase 4
|- backends/
|  |- physicsJolt/
|  `- secondaryMotionVerlet/      # Phase 6, if admitted
|- plugins/
|  `- physicsSchema/              # only after schema admission
|- tests/
|  |- integration/
|  `- fixtures/
`- examples/                      # only when a maintained example exists
```

Unit tests live beside the component they test. Cross-component and installed
consumer tests live under root `tests/`.

## 4. Namespace and include policy

The public namespace and include root are `ARCH-O1`. They are fixed during
Phase 0 before public headers land. They must be repository-owned and must not
retain `usd_stage_runner` in the name.

Public headers contain only types from their declared dependencies. Private
backend headers live below the backend target and are not installed as part of
the neutral package.

## 5. Build modes

The same component graph must work in two modes:

1. a root plain-CMake build for development and packaging;
2. OpenStrata workspace composition for ecosystem builds.

Installed-consumer tests configure against a clean prefix and never rely on a
source checkout, build-tree include, undeclared sibling, or workspace-only
target alias.

Exact supported tools and platforms are owned by
[DEPENDENCIES.md](DEPENDENCIES.md) and remain unclaimed until Phase 0 runs.

## 6. Package boundaries

Each public library that exists installs its own CMake package. Package names,
targets, headers, transitive dependencies, and version compatibility are owned
by [PACKAGE_CONTRACT.md](PACKAGE_CONTRACT.md).

No umbrella package is created until a consumer demonstrates that it improves
composition without hiding optional dependencies.

## 7. Extraction rule

The Stage Runner sources are migration input, not a subtree to preserve.
History and behavior should be retained where practical, while includes,
namespaces, install paths, CMake package files, and manifests are rewritten to
this repository's ownership.

In particular, the current `physicsCore -> runtimeCore` dependency is removed:
math values move behind a neutral contract, and `PhysicsRuntime`'s
`PrimId <-> BodyHandle` mapping becomes a bridge/application responsibility
instead of dragging Stage Runner runtime identity into the core.

## 8. Open questions

- **ARCH-O1:** final public namespace and include root.
- **ARCH-O2:** whether neutral math values live inside `physicsCore` or a
  separate package proven useful by another repository.
- **ARCH-O3:** whether integration fixtures stay at root or under
  `libs/physicsUsd/tests/` until a second cross-component suite exists.
